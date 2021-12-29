#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "Message.h"

#ifndef SOCKET_H_
#define SOCKET_H_
class Socket {
 public:
  Socket();
  Socket(sockaddr_in local_address);

  void set_up(sockaddr_in local_address);

  ssize_t send_to(const Message& message, const sockaddr_in& address) const;

  ssize_t recieve_from(Message& message, sockaddr_in& address) const;

  static sockaddr_in make_ip_address(int port, const std::string& ip_address = std::string());

  ~Socket();

 private:
  int fd_ = -2;
};
#endif
