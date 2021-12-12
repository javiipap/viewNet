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

  /**
   * @brief Read count bytes of the file and store the output in a buffer.
   * @param[out] buf Buffer in which the readed chunk will be stored.
   * @param[in] count Amount of bytes to read.
   * @return Amount of bytes succesfully read.
   */
  ssize_t fread(void* buf, size_t count) const;

  /**
   * @brief writes count bytes of the buffer and stores it in the file.
   * @param[in] buf Buffer with the data to write.
   * @param[in] count Amount of bytes to write.
   * @return Amount of bytes succesfully writen.
   */
  ssize_t fwrite(void* buf, size_t count) const;

 private:
  int fd_;
};

#endif