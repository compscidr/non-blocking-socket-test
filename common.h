#pragma once

#include <string>
constexpr int SERVER_PORT = 9090;
constexpr int MAX_OUTSTANDING_CONNECTIONS = 10;
constexpr int MAX_BUFFER = 1024;

std::string sockaddr_to_string(const struct sockaddr *sa);
long recv_bytes(int socket, char * buffer, long expected_bytes);
int recv_uint(int socket);

int send_buffer(int socket, const char * buffer, int length);