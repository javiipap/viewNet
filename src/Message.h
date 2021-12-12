#include <array>

#ifndef MESSAGE_H_
#define MESSAGE_H_

#define MESSAGESIZE 1024

class Message {
 public:
  std::array<char, MESSAGESIZE> text;
  int chunk_size = 0;
};

#endif