#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// UNIX lib for sockets and connections logic
#include <unistd.h>
// TCP logic
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    // verifies that both ip, port and consumer group are passed for execution
    if (argc < 3) {
        printf("Using: %s <ip_broker> <port> \n", argv[0]);
        return 1;
    }
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    // creates client's TCP socker
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // sets broker address
    serv_addr.sin_family = AF_INET;
    // port converted to NET format (big-endian)
    serv_addr.sin_port = htons(atoi(argv[2]));
    // converts IP to binary
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    // tries to connect to the broker
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed"); // manejo de error en conexión
        return 1;
    }

    // sends the handshake to identify as CONSUMER
    snprintf(buffer, BUFFER_SIZE, "[CONSUMER]\n");
    send(sock, buffer, strlen(buffer), 0);

    // recieves message
    while (1) {
        // cleans buffer
        memset(buffer, 0, BUFFER_SIZE);
        // waits for broker message
        int len = recv(sock, buffer, BUFFER_SIZE, 0);
        if (len <= 0) break;
        printf("Received: %s", buffer);
    }

    close(sock);
    return 0;
}
