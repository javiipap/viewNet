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
  ~Socket();

  /**
   * @brief Inicializa el socket.
   * @param[in] local_address Dirección del socket a crear.
   */
  void set_up(sockaddr_in local_address);

  /**
   * @brief Envía un mensaje a otro socket.
   * @param[in] message Buffer con información a enviar.
   * @param[in] address Dirección del socket receptor.
   */
  ssize_t send_to(const Message& message, const sockaddr_in& address);

  /**
   * @brief Recibe un mensaje de otro socket.
   * @param[in] message Buffer donde guardar el mensaje recibido.
   * @param[in] address Dirección del socket remitente.
   */
  ssize_t recieve_from(Message& message, sockaddr_in& address) const;

 private:
  int fd_ = -2;
  pthread_mutex_t mutex_;
};

class socket_error : public std::system_error {
  using system_error::system_error;
};
#endif
