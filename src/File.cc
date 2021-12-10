#include "File.h"

File::File(std::string pathname, int flags) {
  fd_ = open(pathname.c_str(), flags);
}

ssize_t File::fread(void* buf, size_t count) const {
  return read(fd_, buf, count);
}

ssize_t File::fwrite(void* buf, size_t count) const {
  return write(fd_, buf, count);
}

File::~File() { close(fd_); }
