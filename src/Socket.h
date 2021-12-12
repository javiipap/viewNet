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
  Socket(sockaddr_in local_address);

  void send_to(const Message& message, const sockaddr_in& address) const;

  void recieve_from(Message& message, sockaddr_in& address) const;

  static sockaddr_in make_ip_address(
      int port, const std::string& ip_address = std::string());

  ~Socket();

 private:
  int fd_;
  sockaddr_in local_address_;
};
#endif
