#include "server.h"

#define SERVER_PORT 8080

char* message = "hello from server!\n";
char* get_message() {
    return message;
}

char* message_with_route = "hello from server! route /test\n";
char* get_message_with_route() {
    return message_with_route;
}

int main() {
    add_handler("/", get_message);
    add_handler("/test", get_message_with_route);
    run_server("0.0.0.0", 8080); 

    return 0;
}
