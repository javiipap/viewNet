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
 *         escritura de archivos mapeados en memoria.
 * Revision history:
 *                29/10/2021 - Creation (first version) of the code
 */

#include "File.h"

File::File(std::string pathname, int flags) {
  fd_ = open(pathname.c_str(), flags);
  if (fd_ < 0) {
    throw std::system_error(errno, std::system_category(), "Error al abrir el archivo.");
  }
  struct stat st {};
  fstat(fd_, &st);
  size_ = st.st_size;
  mem_start_ =
      static_cast<uint8_t*>(mmap(nullptr, size_ * sizeof(uint8_t), PROT_READ, MAP_SHARED, fd_, 0));

  lockf(fd_, F_LOCK, 0);
}

size_t File::read(void* buf, size_t size) {
  size_t stored = 0;
  for (int i = 0; i < size && position_ + i < size_; i++, stored++) {
    reinterpret_cast<uint8_t*>(buf)[i] = mem_start_[i + position_];
  }
  position_ += stored;
  return stored;
}

size_t File::write(void* buf, size_t size) {
  size_t saved = 0;
  for (int i = 0; i < size && position_ < size_; i++, position_++, saved++) {
    mem_start_[i] = static_cast<uint8_t*>(buf)[position_];
  }
  return saved;
}

size_t File::rewind(size_t size) {
  int prev_pos = position_;
  position_ = position_ > size ? position_ - size : 0;

  return position_ > size ? size : position_;
}

int File::size() const { return size_; }

File::~File() {
  lockf(fd_, F_ULOCK, 0);
  munmap(mem_start_, size_);
  close(fd_);
}
