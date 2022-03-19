#include "common.h"
#include <netdb.h>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <thread>
#include <unistd.h>

char recv_buffer[MAX_BUFFER] = {};

int main(int argc, char * argv[]) {
  std::string hostname = "localhost";
  int port = SERVER_PORT;

  if (argc > 2) {
    port = atoi(argv[2]);
  } else {
    std::cout << "port not provided, using: " << port << std::endl;
  }
  if (argc > 1) {
    hostname = argv[1];
  } else {
    std::cout << "host not provided using: " << hostname << std::endl;
  }

  struct addrinfo hints{}, *res;
  int sockfd = -1;

  if (getaddrinfo(hostname.c_str(), std::to_string(port).c_str(), &hints, &res) < 0) {
    std::cout << "Error looking up host: " << hostname << ":" << port << ". Error: " << strerror(errno) << std::endl;
  }

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    std::cout << "Couldn't create the state socket: " << strerror(errno) << std::endl;
    return errno;
  }

  if(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK) < 0) {
    std::cout <<  "Couldn't set the socket in nonblocking mode: " << strerror(errno) << std::endl;
    return errno;
  }

  fd_set wrfds;
  FD_ZERO(&wrfds);
  FD_SET(sockfd, &wrfds);

  // don't bother to check for an error because it will give EINPROGRESS
  connect(sockfd, res->ai_addr, res->ai_addrlen);

  struct timeval tout{0, 500000};
  int error = select(FD_SETSIZE, nullptr, &wrfds, nullptr, &tout);
  if (error <= 0) {
    if (error == 0) {
      std::cout << "Timeout on connect" << std::endl;
    } else {
      std::cout << "Error on connect: " << strerror(errno) << std::endl;
    }
    return errno;
  }
  std::cout << "connected." << std::endl;

  std::string message = "Some message";
  send_buffer(sockfd, message.c_str(), message.length());

  // wait until we're ready to recv again
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(sockfd, &rfds);
  error = select(FD_SETSIZE, &rfds, nullptr, nullptr, &tout);
  if (error <= 0) {
    if (error == 0) {
      std::cout << "Timeout on recv" << std::endl;
    } else {
      std::cout << "Error on recv: " << strerror(errno) << std::endl;
    }
    return errno;
  }
  std::cout << "ready to recv." << std::endl;

  std::cout << "Waiting to recv message" << std::endl;
  int message_size = recv_uint(sockfd);
  if (message_size > 0) {
    std::cout << "Expecting to receive " << message_size << " bytes for next message." << std::endl;
    int received_bytes = recv_bytes(sockfd, recv_buffer, message_size);
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
    close(sockfd);
    sockfd = -1;
    return -1;
  }

  return 0;
}