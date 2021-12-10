#include "Socket.h"

Socket::Socket(sockaddr_in local_address) {
  fd_ = socket(AF_INET, SOCK_DGRAM, 0);

  if (fd_ < 0) {
    throw std::runtime_error("Fallo al crear el socket");
  }

  int res = bind(fd_, reinterpret_cast<const sockaddr*>(&local_address),
                 sizeof(local_address));
  if (res < 0) {
    throw std::runtime_error("Fallo el bind");
  }
}

void Socket::send_to(const Message& message, const sockaddr_in& address) const {
  int res =
      sendto(fd_, &message, sizeof(message), 0,
             reinterpret_cast<const sockaddr*>(&address), sizeof(address));
  if (res < 0) {
    throw std::runtime_error("Fallo al enviar el mensaje.");
  }
}

void Socket::recieve_from(Message& message, sockaddr_in& address) const {
  socklen_t length = sizeof(address);
  int res = recvfrom(fd_, &message, sizeof(message), 0,
                     reinterpret_cast<sockaddr*>(&address), &length);

  if (res < 0) {
    throw std::runtime_error("Fallo al recibir el mensaje");
  }
}

sockaddr_in Socket::make_ip_address(int port, const std::string& ip_address) {
  in_addr address;
  if (ip_address.length())
    inet_aton(ip_address.c_str(), &address);
  else
    address.s_addr = htonl(INADDR_ANY);

  return {AF_INET, htons(port), address};
}

ssize_t Socket::ssend(const Message& message, int flags) const {
  return send(fd_, &message.text, sizeof(message.text), flags);
}

Socket::~Socket() { close(fd_); }
