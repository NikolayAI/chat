#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>

#define MAX_MESSAGE_SIZE 1024
#define ADDRESS "127.0.0.1"
#define HELLO_MESSAGE "Connected to the chat\n"

int main(int argc, char *argv[]) {
    int port;

    if (argc < 3) {
        perror("usage: ./client $port $name\n");
        exit(EXIT_FAILURE);
    }

    if (sscanf(argv[1], "%i", &port) != 1) {
        perror("port should be a number\n");
        exit(EXIT_FAILURE);
    }

    char *name = argv[2];
    int client_socket;

    struct sockaddr_in server_address;
    socklen_t server_address_size = sizeof(server_address);
    memset(&server_address, 0, server_address_size);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(ADDRESS);

    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    if(connect(client_socket, (struct sockaddr *) &server_address, server_address_size)) {
        perror("socket connection failed");
        exit(EXIT_FAILURE);
    };

    write(fileno(stdin), HELLO_MESSAGE, strlen(HELLO_MESSAGE));

    while(1) {
        fd_set fds;
        int std_in_fd = fileno(stdin);
        FD_ZERO(&fds);
        FD_SET(client_socket, &fds);
        FD_SET(std_in_fd, &fds);

        if (select(client_socket + 1, &fds, 0, 0, 0) < 0) {
            perror("select() failed.\n");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(client_socket, &fds)) {
            char read[MAX_MESSAGE_SIZE];
            int bytes_received = recv(client_socket, read, MAX_MESSAGE_SIZE, 0);
            if (bytes_received < 1) {
                printf("connection closed by server.\n");
                break;
            }
            printf("Received from %.*s", bytes_received, read);
        }

        if(FD_ISSET(std_in_fd, &fds)) {
            size_t read_size = MAX_MESSAGE_SIZE - sizeof(name);
            char read[read_size];
            char result[MAX_MESSAGE_SIZE];

            if (!fgets(read, read_size, stdin)) {
                break;
            }

            sprintf(result, "%s%s%s",name, ": ", read);

            if(!send(client_socket, result, strlen(result), 0)) {
                perror("send() failed.\n");
            };
        }
    }

    return 0;
}
