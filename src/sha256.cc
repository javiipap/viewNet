#include "sha256.h"

sha256::sha256() = default;

std::string sha256::digest(uint8_t* input, size_t length) {
  padding(input, length);
  int j = 0;
  for (int i = 0; i < extendend_message.size(); i += 16) {
    memcpy(chunk.data(), extendend_message.data() + i, 64);
    msg_schedule();
    compress();
    j++;
  }

  std::array<uint8_t, 32> out;

  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      out[i + (j * 4)] = (state[j] >> (24 - i * 8)) & 0x000000ff;
    }
  }

  std::stringstream s;
  s << std::setfill('0') << std::hex;

  for (uint8_t i = 0; i < 32; i++) {
    s << std::setw(2) << (unsigned int)out[i];
  }
  std::cout << s.str() << std::endl;
  return s.str();
}

uint32_t sha256::rotate_right(uint32_t word, uint32_t offset) {
  return (word >> offset) | (word << 32 - offset);
}

uint32_t sha256::choose(uint32_t e, uint32_t f, uint32_t g) { return (e & f) ^ (~e & g); }

uint32_t sha256::majority(uint32_t a, uint32_t b, uint32_t c) { return (a & (b | c)) | (b & c); }

uint32_t sha256::sig0(uint32_t x) { return rotate_right(x, 7) ^ rotate_right(x, 18) ^ (x >> 3); }

uint32_t sha256::sig1(uint32_t x) { return rotate_right(x, 17) ^ rotate_right(x, 19) ^ (x >> 10); }

uint32_t sha256::sum(uint32_t a, uint32_t b) { return (a + b) % 0x100000000; }

void sha256::msg_schedule() {
  uint32_t s0, s1;
  for (int i = 16; i < 64; i++) {
    chunk[i] = chunk[i - 16] + sig0(chunk[i - 15]) + chunk[i - 7] + sig1(chunk[i - 2]);
  }
}

void sha256::compress() {
  uint32_t s0, s1;
  uint32_t temp1, temp2;
  std::array<uint32_t, 8> temp_state;
  memcpy(temp_state.data(), state.data(), sizeof(uint32_t) * 8);

  for (int i = 0; i < 64; i++) {
    s0 = rotate_right(temp_state[0], 2) ^ rotate_right(temp_state[0], 13) ^
         rotate_right(temp_state[0], 22);
    s1 = rotate_right(temp_state[4], 6) ^ rotate_right(temp_state[4], 11) ^
         rotate_right(temp_state[4], 25);

    temp1 = sum(temp_state[7], choose(temp_state[4], temp_state[5], temp_state[6]));
    temp1 += sum(sum(round_constants[i], chunk[i]), s1);

    temp2 = sum(majority(temp_state[0], temp_state[1], temp_state[2]), s0);

    temp_state[7] = temp_state[6];
    temp_state[6] = temp_state[5];
    temp_state[5] = temp_state[4];
    temp_state[4] = sum(temp_state[3], temp1);
    temp_state[3] = temp_state[2];
    temp_state[2] = temp_state[1];
    temp_state[1] = temp_state[0];
    temp_state[0] = sum(temp1, temp2);
  }

  state[7] = sum(state[7], temp_state[7]);
  state[6] = sum(state[6], temp_state[6]);
  state[5] = sum(state[5], temp_state[5]);
  state[4] = sum(state[4], temp_state[4]);
  state[3] = sum(state[3], temp_state[3]);
  state[2] = sum(state[2], temp_state[2]);
  state[1] = sum(state[1], temp_state[1]);
  state[0] = sum(state[0], temp_state[0]);
}

void sha256::padding(uint8_t* input, uint64_t length) {
  uint64_t size = length * 8;

  int i = 0;
  for (; i + 4 <= length; i += 4) {
    extendend_message.push_back(input[i] << 24 | input[i + 1] << 16 | input[i + 2] << 8 |
                                input[i + 3]);
  }

  if (i < length) {
    uint32_t word = 0;
    int offset = 24;
    for (; i < length; i++, offset -= 8) {
      word |= input[i] << offset;
    }
    word |= 0x80 << offset;
    extendend_message.push_back(word);
  } else {
    extendend_message.push_back(0x80000000);
  }

  extendend_message.resize((size / 512) * 16 + (((size + 32) % 512) < 447) * 14, 0);

  extendend_message.push_back(size >> 32);
  extendend_message.push_back(size);
}
