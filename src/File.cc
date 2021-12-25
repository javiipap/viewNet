#include "File.h"

File::File(std::string pathname, int flags) {
  fd_ = open(pathname.c_str(), flags);
  if (fd_ < 0) {
    throw std::system_error(errno, std::system_category(),
                            "Error al abrir el archivo.");
  }
  struct stat st {};
  fstat(fd_, &st);
  size_ = st.st_size;
  mem_start_ = static_cast<uint8_t*>(
      mmap(nullptr, size_ * sizeof(uint8_t), PROT_READ, MAP_SHARED, fd_, 0));

  lockf(fd_, F_LOCK, 0);
}

ssize_t File::read(void* buf, size_t size) {
  ssize_t stored = 0;
  for (int i = 0; i < size && position_ + i < size_; i++, stored++) {
    reinterpret_cast<uint8_t*>(buf)[i] = mem_start_[i + position_];
  }
  position_ += stored;
  return stored;
}

ssize_t File::write(void* buf, size_t size) {
  ssize_t saved = 0;
  for (int i = 0; i < size && position_ < size_; i++, position_++, saved++) {
    mem_start_[i] = static_cast<uint8_t*>(buf)[position_];
  }
  return saved;
}

int File::size() const { return size_; }

File::~File() {
  lockf(fd_, F_ULOCK, 0);
  munmap(mem_start_, size_);
  close(fd_);
}
