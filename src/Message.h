#include <array>

#ifndef MESSAGE_H_
#define MESSAGE_H_

#define MESSAGESIZE 1024

class Message {
 public:
  std::array<uint8_t, MESSAGESIZE> text;
  int chunk_size = 0;
  bool end_of_mesage = false;
  int chunk_idx = 0;
};

#endif