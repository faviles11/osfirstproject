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

#define GROUP_SIZE 40 // Maximum number of consumers in a group
#define MAX_GROUPS 10 

#define MAX_MESSAGE_QUEUE_SIZE 10 // Maximum number of messages in the queue

// Group structure to manage the consumers ------------------------------------------------
typedef struct Group {
    char name[255];
    int socketConsumerList[GROUP_SIZE]; //store the socket's identifier of the consumers
    int consumerCount; //counts the number of actual consumers, must be initialized to 0
    int actualConsumerIndex; //index of the actual consumer that is going to receive the message
} Group;

Group* createGroup(char* name) { 
    Group* group = (Group*)malloc(sizeof(Group));
    strcpy(group->name, name);
    group->consumerCount = 0;
    group->actualConsumerIndex = 0;
    return group;
}

void group_addConsumer(Group* group, int socket) {
    group->socketConsumerList[group->consumerCount] = socket;
    group->consumerCount++;
}

void group_sendMessage(Group* group, char* message) {
    // Send the message to the actual consumer
    int consumerSocket = group->socketConsumerList[group->actualConsumerIndex];
    send(consumerSocket, message, strlen(message), 0);
    group->actualConsumerIndex = (group->actualConsumerIndex + 1) % group->consumerCount;
}
//----------------------------------------------------------------------------------------
// Array to store groups
Group* groupList[MAX_GROUPS]; 

// Adds a consumer to a group, creates the group if it doesn't exist
void addConsumer(char* groupName, int socket) { 
    for (int i = 0; i < MAX_GROUPS; i++) {
        // if the group is empty, create it and add the consumer
        if (groupList[i] == NULL) {
            groupList[i] = createGroup(groupName);
            group_addConsumer(groupList[i], socket);
            return;
        }
        if (strcmp(groupList[i]->name, groupName) == 0) {
            group_addConsumer(groupList[i], socket);
            return;
        }
    }
    printf("No more groups available\n");
}

void sendMessageToGroup(char* message) {
    // Send the message to all groups
    for (int i = 0; i < MAX_GROUPS; i++) {
        if (groupList[i] != NULL) {
            group_sendMessage(groupList[i], message);
        }
    }
}
//----------------------------------------------------------------------------------------
// A queue to store messages, will follow de FIFO logic. At the same time, must use an offset, just in case there are more messagese coming than the queue dispatching time

typedef struct MessageQueue {
    char messages[MAX_MESSAGE_QUEUE_SIZE][BUFFER_SIZE];
    int actualMessageIndex; // index of the actual message that is going to be sent
    int addMessageIndex; // counts the number of messages in the queue
} MessageQueue;

MessageQueue* createMessageQueue() {
    MessageQueue* queue = (MessageQueue*)malloc(sizeof(MessageQueue));
    queue->actualMessageIndex = 0;
    queue->addMessageIndex = 0;
    return queue;
}

void enqueueMessage(MessageQueue* queue, char* message) {
    if ((queue->addMessageIndex + 1) % MAX_MESSAGE_QUEUE_SIZE != queue->actualMessageIndex) {
        strcpy(queue->messages[queue->addMessageIndex], message);
        queue->addMessageIndex = (queue->addMessageIndex + 1) % MAX_MESSAGE_QUEUE_SIZE;
    } else {
        // Queue is full, handle the error (e.g., discard the message or wait)
        printf("Message queue is full\n");
    }
}


// -------------------------------------------------------------------------------
// THREAD UNSAFE
void sendMessageToGroups(MessageQueue* queue) {
    if (queue->actualMessageIndex != queue->addMessageIndex) {
        // Send the message to the group
        sendMessageToGroup(queue->messages[queue->actualMessageIndex]);
        queue->actualMessageIndex = (queue->actualMessageIndex + 1) % MAX_MESSAGE_QUEUE_SIZE;
    } else {
        printf("Message queue is empty\n");
    }
}
// global variable to store the queue
MessageQueue* messageQueue; 

#define GROUP_SIZE 40 // Maximum number of consumers in a group
#define MAX_GROUPS 10 

#define MAX_MESSAGE_QUEUE_SIZE 10 // Maximum number of messages in the queue

// Group structure to manage the consumers ------------------------------------------------
typedef struct Group {
    char name[255];
    int socketConsumerList[GROUP_SIZE]; //store the socket's identifier of the consumers
    int consumerCount; //counts the number of actual consumers, must be initialized to 0
    int actualConsumerIndex; //index of the actual consumer that is going to receive the message
} Group;

Group* createGroup(char* name) { 
    Group* group = (Group*)malloc(sizeof(Group));
    strcpy(group->name, name);
    group->consumerCount = 0;
    group->actualConsumerIndex = 0;
    return group;
}

void group_addConsumer(Group* group, int socket) {
    group->socketConsumerList[group->consumerCount] = socket;
    group->consumerCount++;
}

void group_sendMessage(Group* group, char* message) {
    // Send the message to the actual consumer
    int consumerSocket = group->socketConsumerList[group->actualConsumerIndex];
    send(consumerSocket, message, strlen(message), 0);
    group->actualConsumerIndex = (group->actualConsumerIndex + 1) % group->consumerCount;
}
//----------------------------------------------------------------------------------------
// Array to store groups
Group* groupList[MAX_GROUPS]; 

// Adds a consumer to a group, creates the group if it doesn't exist
void addConsumer(char* groupName, int socket) { 
    for (int i = 0; i < MAX_GROUPS; i++) {
        // if the group is empty, create it and add the consumer
        if (groupList[i] == NULL) {
            groupList[i] = createGroup(groupName);
            group_addConsumer(groupList[i], socket);
            return;
        }
        if (strcmp(groupList[i]->name, groupName) == 0) {
            group_addConsumer(groupList[i], socket);
            return;
        }
    }
    printf("No more groups available\n");
}

void sendMessageToGroup(char* message) {
    // Send the message to all groups
    for (int i = 0; i < MAX_GROUPS; i++) {
        if (groupList[i] != NULL) {
            group_sendMessage(groupList[i], message);
        }
    }
}
//----------------------------------------------------------------------------------------
// A queue to store messages, will follow de FIFO logic. At the same time, must use an offset, just in case there are more messagese coming than the queue dispatching time

typedef struct MessageQueue {
    char messages[MAX_MESSAGE_QUEUE_SIZE][BUFFER_SIZE];
    int actualMessageIndex; // index of the actual message that is going to be sent
    int addMessageIndex; // counts the number of messages in the queue
} MessageQueue;

MessageQueue* createMessageQueue() {
    MessageQueue* queue = (MessageQueue*)malloc(sizeof(MessageQueue));
    queue->actualMessageIndex = 0;
    queue->addMessageIndex = 0;
    return queue;
}

void enqueueMessage(MessageQueue* queue, char* message) {
    if ((queue->addMessageIndex + 1) % MAX_MESSAGE_QUEUE_SIZE != queue->actualMessageIndex) {
        strcpy(queue->messages[queue->addMessageIndex], message);
        queue->addMessageIndex = (queue->addMessageIndex + 1) % MAX_MESSAGE_QUEUE_SIZE;
    } else {
        // Queue is full, handle the error (e.g., discard the message or wait)
        printf("Message queue is full\n");
    }
}


// -------------------------------------------------------------------------------
// THREAD UNSAFE
void sendMessageToGroups(MessageQueue* queue) {
    if (queue->actualMessageIndex != queue->addMessageIndex) {
        // Send the message to the group
        sendMessageToGroup(queue->messages[queue->actualMessageIndex]);
        queue->actualMessageIndex = (queue->actualMessageIndex + 1) % MAX_MESSAGE_QUEUE_SIZE;
    } else {
        printf("Message queue is empty\n");
    }
}
// global variable to store the queue
MessageQueue* messageQueue; 

#define GROUP_SIZE 40 // Maximum number of consumers in a group
#define MAX_GROUPS 10 

#define MAX_MESSAGE_QUEUE_SIZE 10 // Maximum number of messages in the queue

// Group structure to manage the consumers ------------------------------------------------
typedef struct Group {
    char name[255];
    int socketConsumerList[GROUP_SIZE]; //store the socket's identifier of the consumers
    int consumerCount; //counts the number of actual consumers, must be initialized to 0
    int actualConsumerIndex; //index of the actual consumer that is going to receive the message
} Group;

Group* createGroup(char* name) { 
    Group* group = (Group*)malloc(sizeof(Group));
    strcpy(group->name, name);
    group->consumerCount = 0;
    group->actualConsumerIndex = 0;
    return group;
}

void group_addConsumer(Group* group, int socket) {
    group->socketConsumerList[group->consumerCount] = socket;
    group->consumerCount++;
}

void group_sendMessage(Group* group, char* message) {
    // Send the message to the actual consumer
    int consumerSocket = group->socketConsumerList[group->actualConsumerIndex];
    send(consumerSocket, message, strlen(message), 0);
    group->actualConsumerIndex = (group->actualConsumerIndex + 1) % group->consumerCount;
}
//----------------------------------------------------------------------------------------
// Array to store groups
Group* groupList[MAX_GROUPS]; 

// Adds a consumer to a group, creates the group if it doesn't exist
void addConsumer(char* groupName, int socket) { 
    for (int i = 0; i < MAX_GROUPS; i++) {
        // if the group is empty, create it and add the consumer
        if (groupList[i] == NULL) {
            groupList[i] = createGroup(groupName);
            group_addConsumer(groupList[i], socket);
            return;
        }
        if (strcmp(groupList[i]->name, groupName) == 0) {
            group_addConsumer(groupList[i], socket);
            return;
        }
    }
    printf("No more groups available\n");
}

void sendMessageToGroup(char* message) {
    // Send the message to all groups
    for (int i = 0; i < MAX_GROUPS; i++) {
        if (groupList[i] != NULL) {
            group_sendMessage(groupList[i], message);
        }
    }
}
//----------------------------------------------------------------------------------------
// A queue to store messages, will follow de FIFO logic. At the same time, must use an offset, just in case there are more messagese coming than the queue dispatching time

typedef struct MessageQueue {
    char messages[MAX_MESSAGE_QUEUE_SIZE][BUFFER_SIZE];
    int actualMessageIndex; // index of the actual message that is going to be sent
    int addMessageIndex; // counts the number of messages in the queue
} MessageQueue;

MessageQueue* createMessageQueue() {
    MessageQueue* queue = (MessageQueue*)malloc(sizeof(MessageQueue));
    queue->actualMessageIndex = 0;
    queue->addMessageIndex = 0;
    return queue;
}

void enqueueMessage(MessageQueue* queue, char* message) {
    if ((queue->addMessageIndex + 1) % MAX_MESSAGE_QUEUE_SIZE != queue->actualMessageIndex) {
        strcpy(queue->messages[queue->addMessageIndex], message);
        queue->addMessageIndex = (queue->addMessageIndex + 1) % MAX_MESSAGE_QUEUE_SIZE;
    } else {
        // Queue is full, handle the error (e.g., discard the message or wait)
        printf("Message queue is full\n");
    }
}


// -------------------------------------------------------------------------------
// THREAD UNSAFE
void sendMessageToGroups(MessageQueue* queue) {
    if (queue->actualMessageIndex != queue->addMessageIndex) {
        // Send the message to the group
        sendMessageToGroup(queue->messages[queue->actualMessageIndex]);
        queue->actualMessageIndex = (queue->actualMessageIndex + 1) % MAX_MESSAGE_QUEUE_SIZE;
    } else {
        printf("Message queue is empty\n");
    }
}
// global variable to store the queue
MessageQueue* messageQueue; 

// executed per each client's thread that connects to the broke
// socket_desc -> represents the client's socket
void* handle_client(void* socket_desc) {
    int sock = *(int*)socket_desc;
    // local buffer to store the data received from the client
    char buffer[BUFFER_SIZE];

    // while the client is connected
    while (1) {
        // cleans the buffer
        memset(buffer, 0, sizeof(buffer));

        // recv() receives data from the socket, stores it in the local buffer
        int len = recv(sock, buffer, sizeof(buffer), 0);
        // if error then return 0
        if (len <= 0) break;
        printf("Broker recieved: %s", buffer);
        
        // CONSUMER GROUP LOGIC
        // will take the buffer as the group name
        // addConsumer(buffer, sock);
    }
    close(sock);

    // free memory that stored the socket info
    free(socket_desc);
    return NULL;
}

int main() {
    // both socker, server and client
    int server_fd, new_socket;
    // stores Server's Port and IP 
    struct sockaddr_in address;
    // size of the struct
    int addrlen = sizeof(address);

    // creates socket TCP in IPV4 (AF_INET)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // IPv4
    address.sin_family = AF_INET;
    // Accepts conns from every IP
    address.sin_addr.s_addr = INADDR_ANY;
    // Converts port to Net Format (big-endian)
    address.sin_port = htons(PORT);

    // binds the socket with a specific port and IP
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    // puts the socket in listen mode
    listen(server_fd, MAX_CLIENTS);
    printf("Broker listening in port: %d...\n", PORT);

    while (1) {
        // client's socket, returns a descriptor
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        // creates a descriptor and passes it to a thread
        int* pclient = malloc(sizeof(int));
        *pclient = new_socket;

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
