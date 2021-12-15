#include <dirent.h>
#include <sys/types.h>

#include <functional>

#include "File.h"
#include "Socket.h"
#include "functions.h"

void Server(int port) {
  auto server_addr = Socket::make_ip_address(port);
  Socket socket(server_addr);
  Message msg;
  struct sockaddr_in client_addr {};
  socket.recieve_from(msg, client_addr);

  if (msg.text[0] == 'a') {
    GetFile(extract_string(msg), std::cref(socket), client_addr);
  } else {
    List(std::cref(socket), client_addr);
  }
}

void GetFile(std::string filename, const Socket& server_sock,
             const sockaddr_in& client_addr) {
  File file(filename);
  Message msg;
  ssize_t read_bytes = 0;
  do {
    read_bytes = file.read(msg.text.begin(), sizeof(msg.text));
    server_sock.send_to(msg, client_addr);
  } while (read_bytes);
}

void InsertToBuffer(Message& msg, int position, char character) {
  if (position == MESSAGESIZE) {
    throw std::runtime_error("Buffer overflow");
  }

  msg.text[position] = character;
}

void List(const Socket& server_sock, const sockaddr_in& client_addr) {
  auto dir = opendir("public");
  dirent* dir_file;
  closedir(dir);

  Message msg;
  while ((dir_file = readdir(dir)) != NULL) {
    int i = 0;

  try_start:
    try {
      for (int ch = dir_file->d_name[i]; ch != '\0'; i++) {
        InsertToBuffer(msg, msg.chunk_size++, ch);
      }
      InsertToBuffer(msg, msg.chunk_size++, ' ');
      InsertToBuffer(msg, msg.chunk_size++, dir_file->d_type);
    } catch (const std::runtime_error& err) {
      server_sock.send_to(msg, client_addr);
      msg.chunk_size = 0;
      goto try_start;
    }
  };

  if (msg.chunk_size < MESSAGESIZE) {
    if (msg.chunk_size < MESSAGESIZE - 1) {
      msg.text[msg.chunk_size++] = '\0';
    }
    server_sock.send_to(msg, client_addr);
    if (msg.text[msg.chunk_size - 1] != '\0') {
      msg.text[0] = '\0';
      msg.chunk_size = 1;
      server_sock.send_to(msg, client_addr);
    }
  }
}

void Abort() {}

void Pause() {}

void Resume() {}

void Cli() {
  while (true) {
  }
}

int main(int argc, char* argv[]) {
  Cli();
  return 0;
}
