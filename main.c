#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#define MAX_CLIENTS 30
#define SERVER_PORT 8080
#define BUFF_SIZE 1024

void crush(char* error_message) {
    printf("%s\n", error_message);
    printf("errno: %d\n", errno);
    exit(EXIT_FAILURE);
}

int main(int argc, char const* argv[]) {
    char buffer[BUFF_SIZE + 1] = { 0 };
    char* hello_text = "HTTP/1.1 200 OK";
    char* response_text = "HTTP/1.1 200 OK \n\nresponse from server\n";

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
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);
    int address_size = sizeof(address);

    if (bind(master_socket_fd, (struct sockaddr*)&address, address_size) < 0) {
        crush("bind port failed");
    }

    if (listen(master_socket_fd, 3) < 0) {
        crush("bind port failed");
    }

    printf("listen on port %d\n", SERVER_PORT);

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
                if ((read_count = read(socket_fd, buffer, BUFF_SIZE)) == 0) {
                    // somebody disconnected
                    getpeername(socket_fd, (struct sockaddr*)&address, (socklen_t*)&address_size);
                    printf("[INFO] Client disconnected, IP: %s, PORT: %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                }
                else {
                    printf("[INFO] New message:\n%s\n", buffer);
                    send(socket_fd, response_text, strlen(response_text), 0);
                    printf("[INFO] Message sent:\n%s\n\n", response_text);
                }

                // should I close it every time message sent back?
                close(socket_fd);
                client_socket[i] = 0;
            }
        }
    }

    return 0;
}
