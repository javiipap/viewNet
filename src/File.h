#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#ifndef FILE_H_
#define FILE_H_
class File {
 public:
  File(std::string pathname, int flags = O_RDONLY);

  ~File();

  ssize_t fread(void* buf, size_t count) const;

  template <class T>
  ssize_t fread(T* buf) const;

  ssize_t fwrite(void* buf, size_t count) const;

  template <class T>
  ssize_t fwrite(T* buf) const;

 private:
  int fd_;
};

#endif