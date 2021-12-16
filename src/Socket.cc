#include "Socket.h"

Socket::Socket(sockaddr_in local_address) {
  fd_ = socket(AF_INET, SOCK_DGRAM, 0);

  if (fd_ < 0) {
    throw std::system_error(errno, std::system_category(),
                            "Fallo al crear el socket.");
  }

  int res = bind(fd_, reinterpret_cast<const sockaddr*>(&local_address),
                 sizeof(local_address));
  if (res < 0) {
    throw std::system_error(errno, std::system_category(), "Fallo el bind.");
  }
}

void Socket::send_to(const Message& message, const sockaddr_in& address) const {
  int res =
      sendto(fd_, &message, sizeof(message), 0,
             reinterpret_cast<const sockaddr*>(&address), sizeof(address));
  if (res < 0) {
    throw std::system_error(errno, std::system_category(),
                            "Fallo al enviar el mensaje.");
  }
}

void Socket::recieve_from(Message& message, sockaddr_in& address) const {
  socklen_t length = sizeof(address);
  int res = recvfrom(fd_, &message, sizeof(message), 0,
                     reinterpret_cast<sockaddr*>(&address), &length);

  if (res < 0) {
    throw std::system_error(errno, std::system_category(),
                            "Fallo al recibir el mensaje.");
  }
}

Socket::~Socket() { close(fd_); }
