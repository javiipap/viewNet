#include "Socket.h"

Socket::Socket() = default;

Socket::Socket(sockaddr_in local_address) { set_up(local_address); }

void Socket::set_up(sockaddr_in local_address) {
  if (fd_ != -2) {
    return;
  }

  fd_ = socket(AF_INET, SOCK_DGRAM, 0);

  if (fd_ < 0) {
    throw socket_error(errno, std::system_category(), "Failed socket creation.");
  }

  int res = bind(fd_, reinterpret_cast<const sockaddr*>(&local_address), sizeof(local_address));
  if (res < 0) {
    throw socket_error(errno, std::system_category(), "Faild bind attempt.");
  }
}

ssize_t Socket::send_to(const Message& message, const sockaddr_in& address) {
  int res = sendto(fd_, &message, sizeof(message), 0, reinterpret_cast<const sockaddr*>(&address),
                   sizeof(address));
  if (res < 0) {
    throw socket_error(errno, std::system_category(), "Failed sending message.");
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

    throw socket_error(errno, std::system_category(), "Failed recieving message.");
  }
  return res;
}

Socket::~Socket() { close(fd_); }
