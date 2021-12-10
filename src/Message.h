#include <array>

#ifndef MESSAGE_H_
#define MESSAGE_H_

class Message {
 public:
  std::array<char, 1024> text;
  int chunk_size = 0;
};

#endif