#include "Client.h"

Client::Client(){};

void Client::set_up(sockaddr_in server_address) { server_address_ = server_address; }

void Client::request(server_action action, std::string param) {
  Message buffer;
  EncodeAction(buffer, action, param);
  socket_.send_to(buffer, server_address_);

  bool end_found = false;
  sockaddr_in worker_addr;
  while (!end_found) {
    socket_.recieve_from(buffer, worker_addr);
    for (int i = 0; i < buffer.chunk_size; i++) {
      std::cout << buffer.text[i];

      if (buffer.text[i] == 0) {
        end_found = true;
        std::cout << std::endl;
        break;
      }
    }
  }
}

void Client::abort(){};

void Client::pause(){};

void Client::resume(){};
