#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define MAX_MESSAGE_SIZE 1024
#define ADDRESS "127.0.0.1"

int main(int argc, char *argv[]) {
    int port;

    if (argc < 2) {
        perror("usage: client $port\n");
        exit(EXIT_FAILURE);
    }

    if (sscanf(argv[1], "%i", &port) != 1) {
        perror("port should be a number\n");
        exit(EXIT_FAILURE);
    }

    int listen_socket = 0;

    struct sockaddr_in server_address;
    socklen_t server_address_size = sizeof(server_address);
    memset(&server_address, 0, server_address_size);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(ADDRESS);

    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (bind(listen_socket, (struct sockaddr *) &server_address, server_address_size)) {
        perror("bind() failed.\n");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_socket, 10) < 0) {
        perror("listen() failed.\n");
        exit(EXIT_FAILURE);
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(listen_socket, &fds);
    int max_socket = listen_socket;

    while(1) {
        fd_set listen_fds;
        listen_fds = fds;

        if (select(max_socket + 1, &listen_fds, 0, 0, 0) < 0) {
            perror("select() failed.\n");
            exit(EXIT_FAILURE);
        }

        for(int i = 1; i <= max_socket; ++i) {
            if (FD_ISSET(i, &listen_fds)) {
                if (i == listen_socket) {
                    int client_socket = 0;
                    struct sockaddr_in client_address;
                    socklen_t client_address_size = sizeof(client_address);
                    memset(&client_address, 0, client_address_size);

                    client_socket = accept(listen_socket, (struct sockaddr*) &client_address, &client_address_size);

                    if (client_socket < 0) {
                        perror("accept() failed.\n");
                        exit(EXIT_FAILURE);
                    }

                    FD_SET(client_socket, &fds);

                    if (client_socket > max_socket) {
                        max_socket = client_socket;
                    }
                } else {
                    char read_buffer[MAX_MESSAGE_SIZE];
                    int bytes_received = recv(i, read_buffer, MAX_MESSAGE_SIZE, 0);

                    if (bytes_received < 1) {
                        FD_CLR(i, &fds);
                        close(i);
                        continue;
                    }

                    for (int j = 1; j <= max_socket; ++j) {
                        if (FD_ISSET(j, &fds) && j != listen_socket && j != i) {
                            if(!send(j, read_buffer, bytes_received, 0)) {
                                perror("send() failed.\n");
                            };
                        }
                    }
                }
            }
        }
    }

    return 0;
}
