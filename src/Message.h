/**
 * @author Javier Padilla Pío
 * @date 22/12/2021
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en ingeniería informática
 * Curso: 2º
 * Practice de programación: viewNet
 * Email: alu0101410463@ull.edu.es
 * Message.h: Contenedor para enviar información entre sockets.
 * Revision history: 22/12/2021 -
 *                   Creación (primera versión) del código
 */

#include <array>

#ifndef MESSAGE_H_
#define MESSAGE_H_

#define MESSAGESIZE 1024

class Message {
 public:
  std::array<uint8_t, MESSAGESIZE> text;
  int chunk_size = 0;
};

#endif