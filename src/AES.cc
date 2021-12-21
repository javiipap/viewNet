#include "AES.h"

AES::AES(KeyLength key_length) : key_length_{key_length} {
  switch (key_length_) {
    case AES_128:
      rounds_ = 10;
      break;
    case AES_192:
      rounds_ = 12;
      break;
    case AES_256:
      rounds_ = 14;
      break;
  }
  expanded_key_.resize((rounds_ + 1) * 16);
  memcpy(expanded_key_.data(), key_.data(), key_length_);
  ExpandKey();
}

size_t AES::EncryptedLength(int length) {
  return (length / 16) * 16 + (length % 16 > 0) * 16;
}

/**
 * A todos los bloques se les añade un byte de padding 0x80
 */
void AES::Encrypt(const unsigned char* input, int length,
                  unsigned char* output) {
  int pos = 0;
  while (pos + 16 <= length) {
    EncryptBlock(input + pos, output + pos);
    pos += 16;
  }

  if (pos < length) {
    int size = (length - pos);
    memcpy(output + pos, input + pos, size);
    output[pos + size] = 0;
    EncryptBlock(output + pos, output + pos);
  }
}

void AES::Decrypt(const unsigned char* input, int length,
                  unsigned char* output) {
  if (length && length % 16 != 0) {
    throw std::invalid_argument("Eso no está cifrado");
  }

  int pos = 0;

  while (pos < length) {
    DecryptBlock(input + pos, output + pos);
    pos += 16;
  }
}

void AES::set_key(uchar* key, KeyLength key_length) {
  key_length_ = key_length;
  if (key_length != key_.size()) {
    key_.resize(key_length);
    expanded_key_.resize((rounds_ + 1) * 16);
  };
  memcpy(key_.data(), key, key_length);
  ExpandKey();
}

inline void AES::AddRoundKey(int iteration) {
  Xor(state_.data(), state_.data(), expanded_key_.data() + iteration * 16, 16);
}

void AES::EncryptBlock(const unsigned char input[16],
                       unsigned char output[16]) {
  memcpy(state_.data(), input, 16);
  AddRoundKey(0);

  for (int i = 1; i < rounds_ - 1; i++) {
    for (int j = 0; j < 16; j++) {
      state_[j] = GetSubByte(state_[j]);
    }

    ShiftRows();
    MixColumns();
    AddRoundKey(i);
  }

  for (int j = 0; j < 16; j++) {
    state_[j] = GetSubByte(state_[j]);
  }

  ShiftRows();
  AddRoundKey(rounds_ - 1);

  memcpy(output, state_.data(), 16);
}

void AES::DecryptBlock(const unsigned char input[16],
                       unsigned char output[16]) {
  memcpy(state_.data(), input, 16);

  AddRoundKey(rounds_ - 1);
  InverseShiftRows();
  for (int j = 0; j < 16; j++) {
    state_[j] = GetInverseSubByte(state_[j]);
  }

  for (int i = rounds_ - 2; i > 0; i--) {
    AddRoundKey(i);
    InverseMixColumns();
    InverseShiftRows();

    for (int j = 0; j < 16; j++) {
      state_[j] = GetInverseSubByte(state_[j]);
    }
  }

  AddRoundKey(0);

  memcpy(output, state_.data(), 16);
}

void AES::ExpandKey() {
  for (int i = key_length_; i < (rounds_ + 1) * 16; i += key_length_) {
    uchar* position = expanded_key_.data() + i;
    // Se copia la última columna.
    memcpy(position, position - 4, 4);

    // Realmente actua sobre una columna, pero el funcionamiento es el mismo.
    ShiftRow(position, 1);

    for (int i = 0; i < 4; i++) {
      position[i] = GetSubByte(position[i]);
    }

    Xor(position, position, position - key_length_);
    Xor(position, position, rcon_.data());

    rcon_[0] = dbl(rcon_[0]);

    for (int j = 4; j < key_length_ && i + j < (rounds_ + 1) * 16; j += 4) {
      memcpy(position + j, position + j - 4, 4);
      for (int k = 0; k < 4 && j == 16 && key_length_ == AES_256; k++) {
        position[j + k] = GetSubByte(position[j + k]);
      }
      Xor(position + j, position + j, position + j - key_length_);
    }
  }
}

inline void AES::InverseMixColumns() {
  InverseMixColumn(state_.data());
  InverseMixColumn(state_.data() + 4);
  InverseMixColumn(state_.data() + 8);
  InverseMixColumn(state_.data() + 12);
}

inline void AES::MixColumns() {
  MixColumn(state_.data());
  MixColumn(state_.data() + 4);
  MixColumn(state_.data() + 8);
  MixColumn(state_.data() + 12);
}

void AES::InverseShiftRows() {
  std::array<uchar, 16> temp;
  RotateMatrix(state_.data(), temp.data());

  ShiftRow(temp.data() + 4, 3);
  ShiftRow(temp.data() + 8, 2);
  ShiftRow(temp.data() + 12, 1);

  RotateMatrix(temp.data(), state_.data());
}

void AES::ShiftRows() {
  std::array<uchar, 16> temp;
  RotateMatrix(state_.data(), temp.data());

  ShiftRow(temp.data() + 4, 1);
  ShiftRow(temp.data() + 8, 2);
  ShiftRow(temp.data() + 12, 3);

  RotateMatrix(temp.data(), state_.data());
}

void AES::Xor(uchar* a, const uchar* b, const uchar* c, int size) const {
  for (int i = 0; i < size; i++) {
    a[i] = b[i] ^ c[i];
  }
}

inline unsigned char AES::GetSubByte(uchar index) const {
  return s_box_[index];
}

inline unsigned char AES::GetInverseSubByte(uchar index) const {
  return inverse_s_box_[index];
}

void AES::ShiftRow(uchar* row, int offset) const {
  std::array<uchar, 4> temp;
  memcpy(temp.data(), row, 4);

  for (int i = 0; i < 4; i++) {
    row[i] = temp[(i + offset) % 4];
  }
}

void AES::MixColumn(uchar* column) const {
  auto c_0 = column[0], c_1 = column[1], c_2 = column[2], c_3 = column[3];

  column[0] = dbl(c_0 ^ c_1) ^ c_1 ^ c_2 ^ c_3;
  column[1] = dbl(c_1 ^ c_2) ^ c_2 ^ c_3 ^ c_0;
  column[2] = dbl(c_2 ^ c_3) ^ c_3 ^ c_0 ^ c_1;
  column[3] = dbl(c_3 ^ c_0) ^ c_0 ^ c_1 ^ c_2;
}

void AES::InverseMixColumn(uchar* column) const {
  auto c_0 = column[0], c_1 = column[1], c_2 = column[2], c_3 = column[3];
  auto x = dbl(c_0 ^ c_1 ^ c_2 ^ c_3);
  auto y = dbl(x ^ c_0 ^ c_2);
  auto z = dbl(x ^ c_1 ^ c_3);

  column[0] = dbl(y ^ c_0 ^ c_1) ^ c_1 ^ c_2 ^ c_3;
  column[1] = dbl(z ^ c_1 ^ c_2) ^ c_2 ^ c_3 ^ c_0;
  column[2] = dbl(y ^ c_2 ^ c_3) ^ c_3 ^ c_0 ^ c_1;
  column[3] = dbl(z ^ c_3 ^ c_0) ^ c_0 ^ c_1 ^ c_2;
}

/**
 * Duplicación dentro de GF(2^8)
 */
inline unsigned char AES::dbl(uchar a) const {
  return (a << 1) ^ (0x11b & -(a >> 7));
}

void AES::RotateMatrix(const uchar* input, uchar* output) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      output[(i * 4) + j] = input[(j * 4) + i];
    }
  };
}
