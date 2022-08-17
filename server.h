#ifndef __SERVER_H__
#define __SERVER_H__

void add_handler(char* path, char* (*func)());
void run_server(char* ip_address, int port);

#endif // __SERVER_H__
