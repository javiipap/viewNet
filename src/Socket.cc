#include "Socket.h"

Socket::Socket() = default;

Socket::Socket(sockaddr_in local_address) { set_up(local_address); }

void Socket::set_up(sockaddr_in local_address) {
  if (fd_ != -2) {
    return;
  }

  fd_ = socket(AF_INET, SOCK_DGRAM, 0);

  if (fd_ < 0) {
    throw std::runtime_error("Fallo al crear el socket");
  }

  int res = bind(fd_, reinterpret_cast<const sockaddr*>(&local_address), sizeof(local_address));
  if (res < 0) {
    throw std::runtime_error("Fallo el bind");
  }
}

ssize_t Socket::send_to(const Message& message, const sockaddr_in& address) const {
  int res = sendto(fd_, &message, sizeof(message), 0, reinterpret_cast<const sockaddr*>(&address),
                   sizeof(address));
  if (res < 0) {
    throw std::runtime_error("Fallo al enviar el mensaje.");
  }
  return res;
}

ssize_t Socket::recieve_from(Message& message, sockaddr_in& address) const {
  socklen_t length = sizeof(address);
  int res =
      recvfrom(fd_, &message, sizeof(message), 0, reinterpret_cast<sockaddr*>(&address), &length);

  if (res < 0) {
    // errno == 4 => La espera ha sido cancelada de manera externa.
    if (errno == 4) {
      return 0;
    }

    throw std::runtime_error("Fallo al recibir el mensaje");
  }
  return res;
}

sockaddr_in Socket::make_ip_address(int port, const std::string& ip_address) {
  in_addr address;
  if (ip_address.length())
    inet_aton(ip_address.c_str(), &address);
  else
    address.s_addr = htonl(INADDR_ANY);

  return {AF_INET, htons(port), address};
}

Socket::~Socket() { close(fd_); }
