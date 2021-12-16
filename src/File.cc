#include "File.h"

File::File(std::string pathname, int flags) {
  fd_ = open(pathname.c_str(), flags);
}

ssize_t File::fread(void* buf, size_t count) const {
  int res = read(fd_, buf, count);
  if (res < 0) {
    throw std::system_error(errno, std::system_category(),
                            "Fallo al leer del archivo.");
  }
  return res;
}

ssize_t File::fwrite(void* buf, size_t count) const {
  int res = write(fd_, buf, count);
  if (res < 0) {
    throw std::system_error(errno, std::system_category(),
                            "Fallo al escribir al archivo.");
  }
  return res;
}

File::~File() { close(fd_); }
