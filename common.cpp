#include "common.h"
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

// https://stackoverflow.com/questions/166132/maximum-length-of-the-textual-representation-of-an-ipv6-address
// max is 39 with :: but we need room for the null-terminator /0
constexpr int MAX_ADDR_CHARS = 40;

/**
 * Convert a socket address to a std::string for displaying
 * based on: https://gist.github.com/jkomyno/45bee6e79451453c7bbdc22d033a282e
 */
std::string sockaddr_to_string(const struct sockaddr *sa)
{
  char ip_string[MAX_ADDR_CHARS] = {};
  switch(sa->sa_family) {
    case AF_INET: {
      auto sa_ip4 = reinterpret_cast<const sockaddr_in *>(sa);
      inet_ntop(AF_INET, &(sa_ip4->sin_addr), ip_string, MAX_ADDR_CHARS);
      return std::string(ip_string) + ":" + std::to_string(sa_ip4->sin_port);
    }
    case AF_INET6: {
      auto sa_ip6 = reinterpret_cast<const sockaddr_in6 *>(sa);
      inet_ntop(AF_INET, &(sa_ip6->sin6_addr), ip_string, MAX_ADDR_CHARS);
      return std::string(ip_string) + ":" + std::to_string(sa_ip6->sin6_port);
    }
  }
  return "Unknown SA_FAMILY";
}

/**
 * Calls recv until expected_bytes are received. There should be enough
 * room in the buffer provided to receive all of the expected bytes.
 *
 * Todo: should probably add a timeout mechanism here so that if the other side
 * starts sending and doesn't send the whole amount, it times out so we don't
 * get stuck.
 *
 * @param socket the socket to receive from
 * @param buffer the buffer to receive the bytes into
 * @param expected_bytes the amount of bytes to receive
 * @return -1 if an error occurs, received bytes otherwise
 */
long recv_bytes(int socket, char * buffer, long expected_bytes) {
  long total_bytes = 0;
  while (total_bytes < expected_bytes) {
    int r_bytes = recv(socket, &buffer[total_bytes], expected_bytes - total_bytes, 0);
    if (r_bytes == 0) {
      std::cout << "Connection has been closed!" << std::endl;
      return total_bytes;
    }
    total_bytes += r_bytes;
  }
  return total_bytes;
}

/**
 * Receive an unsigned int from the socket. Converts from network byte ordering
 * to host byte ordering and returns the int which was received.
 *
 * @param socket the socket to recv from
 * @return the unsigned integer, or -1 if failure
 */
int recv_uint(int socket) {
  char buffer[sizeof(int)] = {};
  long received = recv_bytes(socket, buffer, sizeof(int));
  if (received != sizeof(int)) {
    return -1;
  }
  int network_int;
  memcpy(&network_int, buffer, sizeof(int));
  // todo: need to handle the uint32_t to int more gracefully
  return ntohl(network_int);
}

/**
 * Internal function to call send until the entire buffer has been sent
 * @param socket the socket to send onto
 * @param buffer the buffer to send
 * @param length the length of the buffer
 * @return the number of sent bytes
 */
long send_bytes(int socket, const char * buffer, int length) {
  int total_sent = 0;
  while (total_sent < length) {
    int sent = send(socket, &buffer[total_sent], length - total_sent, 0);
    if (sent == -1) {
      std::cout << "Error sending data: " << strerror(errno) << std::endl;
      return total_sent;
    }
    total_sent += sent;
  }
  return total_sent;
}

/**
 * Converts the int to network byte ordering and sends it.
 * @param socket the socket to send on
 * @param value the int to send
 * @return the number of bytes sent (should be size of int)
 */
int send_uint(int socket, uint32_t value) {
  char buffer[sizeof(int)] = {};
  int net_value = htonl(value);
  memcpy(buffer, &net_value, sizeof net_value);
  return send_bytes(socket, buffer, sizeof(net_value));
}

/**
 * Sends a buffer by first sending an int with the length of the buffer
 * and then the buffer itself.
 * @param buffer the buffer to send
 * @param length the length of the buffer to send
 * @return the number of bytes sent (excluding the size int itself)
 */
int send_buffer(int socket, const char * buffer, int length) {
  if (send_uint(socket, length) < sizeof(length)) {
    return -1;
  }
  return send_bytes(socket, buffer, length);
}