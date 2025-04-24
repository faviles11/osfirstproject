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
        // TODO
        // CONSUMER GROUP LOGIC
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
        pthread_create(&t, NULL, handle_client, pclient);

        // releases resources use by the thread
        pthread_detach(t);
    }

    return 0;
}
