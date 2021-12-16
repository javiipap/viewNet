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

  /**
   * @brief Envía un mensaje a otro socket.
   * @param[in] message Mensaje a enviar.
   * @param[in] address Dirección del socket remoto.
   * @return Bytes enviados con éxito.
   */
  void send_to(const Message& message, const sockaddr_in& address) const;

  /**
   * @brief Recibe un mensaje de otro socket.
   * @param[out] message Estructura de datos donde guardar el mensaje.
   * @param[in] address Dirección del socket remoto.
   * @return Bytes recibidos con éxito.
   */
  void recieve_from(Message& message, sockaddr_in& address) const;

  ~Socket();

 private:
  int fd_;
  sockaddr_in local_address_;
};
#endif
