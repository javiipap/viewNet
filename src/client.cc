#include <iostream>

#include "Socket.h"
#include "functions.h"

int protected_main(int argc, char* argv[]) {
  const std::string kExit = "exit";
  auto local_sock = Socket(make_ip_address(0, "127.0.0.1"));

  std::string ip = "127.0.0.1";

  if (argc > 1) {
    ip = argv[1];
  }

  std::string filename;

  auto serv_addr = make_ip_address(25565, ip);

  Message buff;
  local_sock.send_to(buff, serv_addr);
  std::cout << "[RECIEVED]: ";

  Message file_chunk;
  bool end_of_file = false;
  do {
    local_sock.recieve_from(file_chunk, serv_addr);
    for (int i = 0; i < file_chunk.chunk_size && !end_of_file; i++) {
      std::cout << file_chunk.text[i];
      end_of_file = file_chunk.text[i] == '\0';
    }
  } while (!end_of_file);

  std::cout << std::endl;

  return 0;
}

int main(int argc, char* argv[]) {
  try {
    return protected_main(argc, argv);
  } catch (std::system_error& e) {
    std::cerr << "viewNet: " << e.what() << std::endl;
    return 1;
  }
}
