#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

void crush(char* error_message) {
    printf("%s\n", error_message);
    printf("errno: %d\n", errno);
    exit(EXIT_FAILURE);
}

int main(int argc, char const* argv[]) {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = { 0 };
    char* hello_text = "HTTP/1.1 200 OK \n\nhello from server\n";

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        crush("socket failed");
    }

    int err = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (err) {
        crush("failed setsockopt");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        crush("bind 8080 port failed");
    }

    if (listen(server_fd, 3) < 0) {
        crush("bind 8080 port failed");
    }

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            crush("accept");
        }

        valread = read(new_socket, buffer, 1024);
        printf("*****************\nNEW MESSAGE:\n%s\n", buffer);
        send(new_socket, hello_text, strlen(hello_text), 0);
        printf("MESSAGE SENT:\n%s\n\n", hello_text);

        close(new_socket);
    }

    shutdown(server_fd, SHUT_RDWR);
    return 0;
}
