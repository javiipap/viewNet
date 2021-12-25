/**
 * @author Javier Padilla Pío
 * @date 22/12/2021
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en ingeniería informática
 * Curso: 2º
 * Practice 6 - Simulación de DFAs
 * Email: alu0101410463@ull.edu.es
 * File.h: Interfaz de la clase File, encargada de gestionar la lectura y
 *         escritura de archivos mapeados en memoria. References: Practice
 * Revision history:
 *                29/10/2021 - Creation (first version) of the code
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#ifndef MAPPED_FILE_H_
#define MAPPED_FILE_H_
class File {
 public:
  File(std::string pathname, int flags = O_RDONLY);

  ~File();

  ssize_t read(void* buf, size_t size);

  ssize_t write(void* buf, size_t size);

  int size() const;

 private:
  int fd_;
  size_t size_;
  uint8_t* mem_start_;
  int position_ = 0;
};

#endif
