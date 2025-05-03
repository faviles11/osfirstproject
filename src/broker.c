#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// UNIX lib for sockets and connections logic
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h> 
// TCP logic
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 12345         
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// struct to store consumers
typedef struct Consumer {
    int socket;
    char group[BUFFER_SIZE];
    struct Consumer* next;
} Consumer;

Consumer* consumers_head = NULL;

// semaphore for consumers list
sem_t consumers_semaphore; 

// struct to manage last sent consumer per group (for round-robin)
typedef struct GroupPointer {
    char group_name[BUFFER_SIZE];
    Consumer* last_sent;
    struct GroupPointer* next;
} GroupPointer;

GroupPointer* groups_head = NULL;
pthread_mutex_t groups_mutex = PTHREAD_MUTEX_INITIALIZER;

const char* groups[] = {"GroupA", "GroupB", "GroupC"};
int group_index = 0;

typedef struct Message {
    char content[BUFFER_SIZE];
    struct Message* next;
} Message;

Message* head = NULL;
Message* tail = NULL;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
int offset = 0;

void enqueue(const char* message) {
    pthread_mutex_lock(&queue_mutex);           
    Message* node = malloc(sizeof(Message));
    strcpy(node->content, message);
    node->next = NULL;
    if (tail == NULL) {
        head = tail = node;
    } else {
        tail->next = node;
        tail = node;
    }
    pthread_mutex_unlock(&queue_mutex);

    pthread_mutex_lock(&log_mutex); 
    FILE* log_file = fopen("messages.log", "a");
    if (log_file != NULL) {
        fprintf(log_file, "[%d] %s", offset++, message);
        fclose(log_file);
    } else {
        perror("Failed to open log file");
    }
    pthread_mutex_unlock(&log_mutex);
}

Message* dequeue() {
    pthread_mutex_lock(&queue_mutex);      
    Message* node = head;
    if (head != NULL) {
        head = head->next;       
        if (head == NULL) {
            tail = NULL;                 
        }
    }
    pthread_mutex_unlock(&queue_mutex);    
    return node;
}

GroupPointer* get_or_create_group(const char* group_name) {
    pthread_mutex_lock(&groups_mutex);
    GroupPointer* current = groups_head;
    while (current != NULL) {
        if (strcmp(current->group_name, group_name) == 0) {
            pthread_mutex_unlock(&groups_mutex);
            return current;
        }
        current = current->next;
    }
    GroupPointer* new_group = malloc(sizeof(GroupPointer));
    strcpy(new_group->group_name, group_name);
    new_group->last_sent = NULL;
    new_group->next = groups_head;
    groups_head = new_group;
    pthread_mutex_unlock(&groups_mutex);
    return new_group;
}

Consumer* select_consumer_for_group(const char* group_name) {
    sem_wait(&consumers_semaphore);
    GroupPointer* group_ptr = get_or_create_group(group_name);
    Consumer* start = group_ptr->last_sent ? group_ptr->last_sent->next : consumers_head;
    Consumer* current = start;

    while (current != NULL) {
        if (strcmp(current->group, group_name) == 0) {
            group_ptr->last_sent = current;
            sem_post(&consumers_semaphore);
            return current;
        }
        current = current->next;
    }

    current = consumers_head;
    while (current != start) {
        if (strcmp(current->group, group_name) == 0) {
            group_ptr->last_sent = current;
            sem_post(&consumers_semaphore);
            return current;
        }
        current = current->next;
    }

    sem_post(&consumers_semaphore);
    return NULL;
}

void register_consumer(int socket) {
    sem_wait(&consumers_semaphore);
    Consumer* new_consumer = malloc(sizeof(Consumer));
    new_consumer->socket = socket;
    strcpy(new_consumer->group, groups[group_index]);
    new_consumer->next = consumers_head;
    consumers_head = new_consumer;
    group_index = (group_index + 1) % 3;
    sem_post(&consumers_semaphore);
    printf("Registered consumer in group: %s\n", new_consumer->group);
}

void* process_queue(void* arg) {
    while (1) {
        Message* msg = dequeue();
        if (msg != NULL) {
            printf("[QUEUE] Dequeued message: %s", msg->content);
            for (int i = 0; i < 3; i++) {
                const char* group_name = groups[i];
                Consumer* consumer = select_consumer_for_group(group_name);
                if (consumer != NULL) {
                    send(consumer->socket, msg->content, strlen(msg->content), 0);
                }
            }
            free(msg);
        }
        sleep(2);
    }
}

void* handle_client(void* socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[BUFFER_SIZE];

    memset(buffer, 0, sizeof(buffer));
    int len = recv(sock, buffer, sizeof(buffer), 0);
    if (len <= 0) {
        close(sock);
        free(socket_desc);
        return NULL;
    }

    if (strncmp(buffer, "[PRODUCER]", 10) == 0) {
        printf("Producer connected.\n");
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            int len = recv(sock, buffer, sizeof(buffer), 0);
            if (len <= 0) break;
            printf("Broker received: %s", buffer);
            enqueue(buffer);
        }
    } 
    else if (strncmp(buffer, "[CONSUMER]", 10) == 0) {
        printf("Consumer connected.\n");
        register_consumer(sock);
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            int len = recv(sock, buffer, sizeof(buffer), 0);
            if (len <= 0) break;
        }
    } 
    else {
        printf("Unknown client type. Closing connection.\n");
        close(sock);
    }

    close(sock);
    free(socket_desc);
    return NULL;
}

int main() {
    int server_sock, client_socket;
    struct sockaddr_in server_addr;
    int addrlen = sizeof(server_addr);

    sem_init(&consumers_semaphore, 0, 1); // Initialize semaphore

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, MAX_CLIENTS);

    pthread_t processor_thread;
    pthread_create(&processor_thread, NULL, process_queue, NULL);
    pthread_detach(processor_thread);

    printf("Broker listening in port: %d...\n", PORT);

    while (1) {
        client_socket = accept(server_sock, (struct sockaddr*)&server_addr, (socklen_t*)&addrlen);
        int* pclient = malloc(sizeof(int));
        *pclient = client_socket;

        pthread_t t;
        pthread_create(&t, NULL, handle_client, pclient);
        pthread_detach(t);
    }

    sem_destroy(&consumers_semaphore); // Cleanup
    return 0;
}
