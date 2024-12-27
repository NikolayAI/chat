#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/select.h>

#define MAX_MESSAGE_SIZE 1024
#define MAX_EVENTS 100

int set_nonblocking(int sockfd) {
	if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
		return -1;
	}
	return 0;
}

void epoll_add_events(int epfd, int fd, uint32_t events) {
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		perror("epoll_ctl failed.\n");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[]) {
    int port;

    if (argc < 3) {
        perror("usage: client $address $port\n");
        exit(EXIT_FAILURE);
    }

    if (sscanf(argv[2], "%i", &port) != 1) {
        perror("port should be a number\n");
        exit(EXIT_FAILURE);
    }

    char *address = argv[1];
    int listen_socket = 0;
    struct epoll_event events[MAX_EVENTS];

    struct sockaddr_in server_address;
    socklen_t server_address_size = sizeof(server_address);
    memset(&server_address, 0, server_address_size);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if(inet_pton (AF_INET, address, &server_address.sin_addr) != 1) {
        perror("address parsing failed");
        exit(EXIT_FAILURE);
    }

    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    int val = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    if (bind(listen_socket, (struct sockaddr *) &server_address, server_address_size)) {
        perror("bind() failed.\n");
        exit(EXIT_FAILURE);
    }

    if (set_nonblocking(listen_socket) == -1) {
        perror("set_nonblocking() failed.\n");
        exit(EXIT_FAILURE);
    };

    if (listen(listen_socket, 10) < 0) {
        perror("listen() failed.\n");
        exit(EXIT_FAILURE);
    }

    int epoll_fd = epoll_create(1);

    epoll_add_events(epoll_fd, listen_socket, EPOLLIN | EPOLLOUT | EPOLLET);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(listen_socket, &fds);
    int max_fd = epoll_fd;

    while(1) {
        int events_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for(int i = 0; i < events_fds; i++) {
            if (events[i].data.fd == listen_socket) {
                struct sockaddr_in client_address;
                socklen_t client_address_size = sizeof(client_address);
                memset(&client_address, 0, client_address_size);

                int client_socket = accept(listen_socket, (struct sockaddr*) &client_address, &client_address_size);
                if (client_socket < 0) {
                    perror("accept() failed.\n");
                    exit(EXIT_FAILURE);
                }

                set_nonblocking(client_socket);
                epoll_add_events(epoll_fd, client_socket, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP);

                FD_SET(client_socket, &fds);
                if (client_socket > max_fd) {
                    max_fd = client_socket;
                }
            } else if (events[i].events & EPOLLIN) {
                char read_buffer[MAX_MESSAGE_SIZE];
                memset(&read_buffer, 0, MAX_MESSAGE_SIZE);
                int current_fd = events[i].data.fd;

                int bytes_received = recv(current_fd, read_buffer, MAX_MESSAGE_SIZE, 0);
                if (bytes_received > 0) {
                    for (int j = 0; j <= sizeof(&fds); ++j) {
                        if (FD_ISSET(j, &fds) && j != listen_socket && j != current_fd) {
                            if(!send(j, read_buffer, bytes_received, 0)) {
                                perror("send() failed.\n");
                            };
                        }
                    }
                }
            }

            if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                FD_CLR(events[i].data.fd, &fds);
                close(events[i].data.fd);
            }
        }
    }

    close(epoll_fd);
    close(listen_socket);
    return 0;
}
