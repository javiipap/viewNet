#include <array>
#include <iostream>

#include "File.h"
#include "Message.h"
#include "Socket.h"
#include "functions.h"

int protected_main(int argc, char* argv[]) {
  std::string filename = "public/1024.txt";
  std::string ip = "127.0.0.1";

  if (argc == 2) {
    filename = "public/";
    filename += argv[1];
  } else if (argc > 2) {
    ip = argv[2];
  }

  Socket socket(make_ip_address(25565, ip));
  std::cout << "Waiting for connections..." << std::endl;

  struct sockaddr_in addr {};
  Message buff;
  socket.recieve_from(buff, addr);
  std::cout << "Recieved connection from: " << inet_ntoa(addr.sin_addr)
            << std::endl;

  File file(filename, O_RDONLY);
  Message file_buffer;
  char* file_buffer_beggin = file_buffer.text.begin();
  int bytes_read = 0;

  do {
    if (file_buffer_beggin == file_buffer.text.begin()) {
      bytes_read = 0;
    }

    file_buffer_beggin = file_buffer.text.begin();

    bytes_read = file.fread(file_buffer_beggin, sizeof(file_buffer.text));
    if (bytes_read > 0) {
      file_buffer.chunk_size = bytes_read;
      socket.send_to(file_buffer, addr);
    } else {
      std::cout << "Sent " << filename << " to: " << inet_ntoa(addr.sin_addr)
                << std::endl;
      Message end_of_file;
      end_of_file.text[0] = '\0';
      end_of_file.chunk_size = 1;
      socket.send_to(end_of_file, addr);
    }

    if (file_buffer_beggin == file_buffer.text.end()) {
      file_buffer_beggin = file_buffer.text.begin();
    }
  } while (bytes_read > 0);

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
