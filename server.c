#include "server.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#define GET "GET"
#define HEAD "HEAD"
#define POST "POST"
#define PUT "PUT"
#define DELETE "DELETE"
#define CONNECT "CONNECT"
#define OPTIONS "OPTIONS"
#define TRACE "TRACE"
#define PATCH "PATCH"

#define MESSAGE_BUFF_SIZE 1024
#define MAX_CLIENTS 30
#define ROUTES_COUNT 20
#define URL_PATH_BUFF_SIZE 256


struct route_handler {
    char* path;
    char* (*get_message)();
};

// TODO: use hashmap
struct route_handler* handlers[ROUTES_COUNT];
int route_handlers_count = 0;


static void crush(char* error_message) {
    printf("%s\n", error_message);
    printf("errno: %d\n", errno);
    exit(EXIT_FAILURE);
}

static void get_url_path(char* message, char* result) {
    int to_skip = 0;
    if (strncmp(GET, message, strlen(GET)) == 0) to_skip = strlen(GET) + 1;
    if (strncmp(HEAD, message, strlen(HEAD)) == 0) to_skip = strlen(HEAD) + 1;
    if (strncmp(POST, message, strlen(POST)) == 0) to_skip = strlen(POST) + 1;
    if (strncmp(PUT, message, strlen(PUT)) == 0) to_skip = strlen(PUT) + 1;
    if (strncmp(DELETE, message, strlen(DELETE)) == 0) to_skip = strlen(DELETE) + 1;
    if (strncmp(CONNECT, message, strlen(CONNECT)) == 0) to_skip = strlen(CONNECT) + 1;
    if (strncmp(OPTIONS, message, strlen(OPTIONS)) == 0) to_skip = strlen(OPTIONS) + 1;
    if (strncmp(TRACE, message, strlen(TRACE)) == 0) to_skip = strlen(TRACE) + 1;
    if (strncmp(PATCH, message, strlen(PATCH)) == 0) to_skip = strlen(PATCH) + 1;

    int message_len = strlen(message);
    int i = 0;
    while (to_skip < message_len
        && message[to_skip] != ' '
        && i < URL_PATH_BUFF_SIZE - 1) {
        result[i] = message[to_skip];
        ++to_skip;
        ++i;
    }

    result[i] = '\0';
}

static void handle_message(int socket_fd, char* message) {
    char url_path_buff[URL_PATH_BUFF_SIZE] = {0};
    get_url_path(message, url_path_buff);

    for (int i = 0; i < route_handlers_count; ++i) {
        if (strcmp(url_path_buff, handlers[i]->path) == 0) {
            char response_text_buff[MESSAGE_BUFF_SIZE] = "HTTP/1.1 200 OK \n\n";
            char* text = handlers[i]->get_message();

            // TODO: handle out of bounds of BUFF when cat
            strcat(response_text_buff, text);
            send(socket_fd, response_text_buff, strlen(response_text_buff), 0);
        }
    }
}

void add_handler(char* path, char* (*func)()) {
    if (route_handlers_count >= ROUTES_COUNT) return;

    struct route_handler* item = malloc(sizeof(struct route_handler));
    if (item == NULL) return;

    item->path = path;
    item->get_message = func;
    handlers[route_handlers_count] = item;
    ++route_handlers_count;
}

void run_server(char* ip_address, int port) {
    char buffer[MESSAGE_BUFF_SIZE] = { 0 };
    char* hello_text = "HTTP/1.1 200 OK";

    int master_socket_fd = 0;
    if ((master_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        crush("socket failed");
    }
    
    int opt = 1;
    int err = setsockopt(master_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (err) {
        crush("failed setsockopt");
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip_address);
    address.sin_port = htons(port);
    int address_size = sizeof(address);

    if (bind(master_socket_fd, (struct sockaddr*)&address, address_size) < 0) {
        crush("bind port failed");
    }

    if (listen(master_socket_fd, 3) < 0) {
        crush("bind port failed");
    }

    printf("listen on port %d\n", port);

    fd_set readfds;

    int client_socket[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        client_socket[i] = 0;
    }

    printf("Waiting for connections...\n");

    while (1) {
        FD_ZERO(&readfds);

        FD_SET(master_socket_fd, &readfds);
        int max_fd = master_socket_fd;

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (client_socket[i] > 0)
                FD_SET(client_socket[i], &readfds);

            if (client_socket[i] > max_fd)
                max_fd = client_socket[i];
        }

        // wait for an activity
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            crush("select error");
        }

        // when something happened on the master socket, than it's incoming connection
        if (FD_ISSET(master_socket_fd, &readfds)) {
            int new_socket;
            if ((new_socket = accept(master_socket_fd, (struct sockaddr*)&address, (socklen_t*)&address_size)) < 0) {
                crush("accept");
            }

            printf("[INFO] New connection, socket fd: %d, IP: %s, PORT: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            if (send(new_socket, hello_text, strlen(hello_text), 0) != strlen(hello_text)) {
                printf("error sending \'%s\' message", hello_text);
            }

            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        // otherwise its IO operation on some other socket
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int socket_fd = client_socket[i];
            if (FD_ISSET(socket_fd, &readfds)) {
                int read_count;
                if ((read_count = read(socket_fd, buffer, MESSAGE_BUFF_SIZE)) == 0) {
                    // somebody disconnected
                    getpeername(socket_fd, (struct sockaddr*)&address, (socklen_t*)&address_size);
                    printf("[INFO] Client disconnected, IP: %s, PORT: %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                }
                else {
                    printf("[INFO] New message:\n%s\n", buffer);
                    handle_message(socket_fd, buffer);
                }

                // should I close it every time message sent back?
                close(socket_fd);
                client_socket[i] = 0;
            }
        }
    }
}



