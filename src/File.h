#include <fcntl.h>
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

 private:
  int fd_;
  size_t size_;
  char* mem_start_;
  int position_;
};

#endif
