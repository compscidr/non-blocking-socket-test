#include "common.h"
#include <netdb.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <cstdlib>

int server_socket = -1;
int client_socket = -1;
volatile bool listening = false;
char recv_buffer[MAX_BUFFER] = {};

void my_handler(int s){
  std::cout << "GOT SIGNAL" << std::endl;
  listening = false;
  if (server_socket != -1) {
    close(server_socket);
    server_socket = -1;
  }
  if (client_socket != -1) {
    close(client_socket);
    client_socket = -1;
  }
}

int main(int argc, char * argv[]) {
  int port = SERVER_PORT;
  if (argc > 1) {
    port = atoi(argv[1]);
  } else {
    std::cout << "port not provided using: " << port << std::endl;
  }
  // initial server accept socket is blocking, but the client socket is put
  // into non-blocking mode so that it doesn't get stuck on recv

  server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_socket < 0) {
    std::cout << "Error creating socket: " << strerror(errno) << std::endl;
    return errno;
  }

  // set socket to be re-usable in case we re-run before its released
  int enable = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
    std::cout << "Error re-using socket: " << strerror(errno) << std::endl;
    return errno;
  }

  // bind to ipv4 address
  struct sockaddr_in bind_addr {};
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_addr.s_addr = INADDR_ANY;
  bind_addr.sin_port = htons(port);
  if (bind(server_socket, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) {
    std::cout << "Error binding on port: " << SERVER_PORT << ". "
              << strerror(errno) << std::endl;
    return errno;
  }

  if (listen(server_socket, MAX_OUTSTANDING_CONNECTIONS) < 0) {
    std::cout << "Error listening for " << MAX_OUTSTANDING_CONNECTIONS
      << " outstanding connections: " << strerror(errno) << std::endl;
    return errno;
  }
  listening = true;

  // setup a signal handler so we can break out of the listen loop on ctrl-c
  // cleanly by closing the socket and stopping the loop
  struct sigaction sigIntHandler{};
  sigIntHandler.sa_handler = my_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, nullptr);

  struct sockaddr_storage their_addr {};
  socklen_t addr_size = sizeof(their_addr);
  std::cout << "Listening for incoming connections..." << std::endl;
  while (listening) {
    client_socket = accept(server_socket,(struct sockaddr *)&their_addr, &addr_size);
    if (client_socket < 0) {
      std::cout << "Error accepting a connection from the client: " << strerror(errno) << std::endl;
      continue;
    }
    std::cout << "Got connection from: " << sockaddr_to_string(
                     reinterpret_cast<const sockaddr *>(&their_addr)) << std::endl;

    // set client socket to non-blocking
//    if(fcntl(client_socket, F_SETFL, fcntl(server_socket, F_GETFL) | O_NONBLOCK) < 0) {
//      std::cout <<  "Couldn't set the socket in nonblocking mode: " << strerror(errno) << std::endl;
//      return errno;
//    }

    // no need to use select here, create a thread, or fork a process because
    // we're not supporting any concurrent clients

    // pattern is always send, recv
    std::cout << "Waiting to recv message" << std::endl;
    int message_size = recv_uint(client_socket);
    if (message_size > 0) {
      std::cout << "Expecting to receive " << message_size << " bytes for next message." << std::endl;
      int received_bytes = recv_bytes(client_socket, recv_buffer, message_size);
      if (received_bytes != message_size) {
        std::cout << "Execpting " << message_size << " but got: " << received_bytes << std::endl;
        return -1;
      }
      std::cout << "Received " << received_bytes << " bytes." << std::endl;
      char * temp = (char*)calloc(received_bytes + 1, sizeof(char)); // c string need to be null terminated
      memcpy(temp, recv_buffer, received_bytes);
      std::string recv_string(temp);
      free(temp);
      std::cout << "MESSAGE: " << recv_string << std::endl;
    } else {
      std::cout << "Got malformed expected message size: " << message_size << std::endl;
      close(client_socket);
      client_socket = -1;
      continue;
    }

    // send
    std::string message = "Got your message";
    send_buffer(client_socket, message.c_str(), message.length());

    close(client_socket);
  }

  std::cout << "Stopped listening" << std::endl;
  return 0;
}