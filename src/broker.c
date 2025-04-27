#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// UNIX lib for sockets and connections logic
#include <unistd.h>
#include <pthread.h>
// TCP logic
#include <netinet/in.h>
#include <arpa/inet.h>

// Port where the broker is going to listen
#define PORT 12345         
// Maximum number of pending connections        
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// struct to store consumers
typedef struct Consumer {
    int socket;                   // socket of the consumer
    char group[BUFFER_SIZE];       // group assigned to the consumer
    struct Consumer* next;         // next consumer
} Consumer;

// global pointer to the list of consumers
Consumer* consumers_head = NULL;

// muxet to block concurrent access to the queue
pthread_mutex_t consumers_mutex = PTHREAD_MUTEX_INITIALIZER;

// struct to manage last sent consumer per group (for round-robin)
typedef struct GroupPointer {
    char group_name[BUFFER_SIZE];  // name of the group
    Consumer* last_sent;           // last consumer used in this group
    struct GroupPointer* next;     // next group pointer
} GroupPointer;

// global pointer to the list of groups
GroupPointer* groups_head = NULL;

// mutex to block concurrent access to group pointers
pthread_mutex_t groups_mutex = PTHREAD_MUTEX_INITIALIZER;

// groups for automatic consumer assignment
const char* groups[] = {"GroupA", "GroupB", "GroupC"};
int group_index = 0;

// struct to store the message
typedef struct Message {
    char content[BUFFER_SIZE];     // info
    struct Message* next;          // node to the next node
} Message;

// global pointers to tail and head of the queue
Message* head = NULL;
Message* tail = NULL;

// muxet to block concurrent access to the queue
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// adds a message to the end of the queue
void enqueue(const char* message) {
    pthread_mutex_lock(&queue_mutex);           
    Message* node = malloc(sizeof(Message));    // allocate memory for the new node
    strcpy(node->content, message);             // copy the message to the node
    node->next = NULL;
    if (tail == NULL) {                        // empty queue
        head = tail = node;                    
    } else {
        tail->next = node;                      // link the new node to the end of the queue
        tail = node;                     
    }
    pthread_mutex_unlock(&queue_mutex);    
}

// returns first message from the queue 
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

// find or create group pointer for round-robin tracking
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
    // if not found, create new
    GroupPointer* new_group = malloc(sizeof(GroupPointer));
    strcpy(new_group->group_name, group_name);
    new_group->last_sent = NULL;
    new_group->next = groups_head;
    groups_head = new_group;
    pthread_mutex_unlock(&groups_mutex);
    return new_group;
}

// selects next consumer in round-robin fashion for a group
Consumer* select_consumer_for_group(const char* group_name) {
    pthread_mutex_lock(&consumers_mutex);
    GroupPointer* group_ptr = get_or_create_group(group_name);
    Consumer* start = group_ptr->last_sent ? group_ptr->last_sent->next : consumers_head;
    Consumer* current = start;

    while (current != NULL) {
        if (strcmp(current->group, group_name) == 0) {
            group_ptr->last_sent = current;
            pthread_mutex_unlock(&consumers_mutex);
            return current;
        }
        current = current->next;
    }

    // if we reach end, start from head
    current = consumers_head;
    while (current != start) {
        if (strcmp(current->group, group_name) == 0) {
            group_ptr->last_sent = current;
            pthread_mutex_unlock(&consumers_mutex);
            return current;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&consumers_mutex);
    return NULL; // no available consumer in group
}

// adds a new consumer to the list
void register_consumer(int socket) {
    pthread_mutex_lock(&consumers_mutex); 
    Consumer* new_consumer = malloc(sizeof(Consumer));
    new_consumer->socket = socket;
    strcpy(new_consumer->group, groups[group_index]); // asigna grupo automáticamente
    new_consumer->next = consumers_head;
    consumers_head = new_consumer;

    // update group_index to cycle between GroupA, GroupB, GroupC
    group_index = (group_index + 1) % 3;

    pthread_mutex_unlock(&consumers_mutex);

    printf("Registered consumer in group: %s\n", new_consumer->group);
}

// function that processes the queue every 2 seconds
void* process_queue(void* arg) {
    while (1) {
        Message* msg = dequeue(); // try to dequeue a message
        if (msg != NULL) {
            printf("[QUEUE] Dequeued message: %s", msg->content); // print the message

            // send the message to one consumer per group
            for (int i = 0; i < 3; i++) {
                const char* group_name = groups[i];
                Consumer* consumer = select_consumer_for_group(group_name);
                if (consumer != NULL) {
                    send(consumer->socket, msg->content, strlen(msg->content), 0);
                }
            }
            free(msg); // free memory of the dequeued message
        }
        sleep(2); // wait 2 seconds before trying again
    }
}


// executed per each client's thread that connects to the broker
// socket_desc -> represents the client's socket
void* handle_client(void* socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[BUFFER_SIZE]; // local buffer to store the data received from the client

    // receives the handshake to determine if it is a producer or consumer
    memset(buffer, 0, sizeof(buffer));
    int len = recv(sock, buffer, sizeof(buffer), 0);
    if (len <= 0) {
        close(sock);
        free(socket_desc);
        return NULL;
    }

    if (strncmp(buffer, "[PRODUCER]", 10) == 0) {
        printf("Producer connected.\n");

        // while the producer is connected
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

        // while the consumer is connected (waiting for messages)
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            int len = recv(sock, buffer, sizeof(buffer), 0);
            if (len <= 0) break;
            // Consumers shouldn't send anything, but if they do, just ignore for now
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
    // both socket, server and client
    int server_sock, client_socket;
    // stores Server's Port and IP 
    struct sockaddr_in server_addr;
    // size of the struct
    int addrlen = sizeof(server_addr);

    // creates socket TCP in IPV4 (AF_INET)
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    // IPv4
    server_addr.sin_family = AF_INET;
    // Accepts conns from every IP
    server_addr.sin_addr.s_addr = INADDR_ANY;
    // Converts port to Net Format (big-endian)
    server_addr.sin_port = htons(PORT);

    // binds the socket with a specific port and IP
    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    // puts the socket in listen mode
    listen(server_sock, MAX_CLIENTS);
    
    pthread_t processor_thread;
    pthread_create(&processor_thread, NULL, process_queue, NULL);
    pthread_detach(processor_thread);

    printf("Broker listening in port: %d...\n", PORT);

    while (1) {
        // client's socket, returns a descriptor
        client_socket = accept(server_sock, (struct sockaddr*)&server_addr, (socklen_t*)&addrlen);
        // creates a descriptor and passes it to a thread
        int* pclient = malloc(sizeof(int));
        *pclient = client_socket;

        // creates thread with the handle_client function as an argument
        pthread_t t;
        // TODO
        // Need a handsake to identify the consumer or producer
        pthread_create(&t, NULL, handle_client, pclient);

        // releases resources use by the thread
        pthread_detach(t);
    }

    return 0;
}
