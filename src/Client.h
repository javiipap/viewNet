#include "AES.h"
#include "Message.h"
#include "Socket.h"
#include "functions.h"

#ifndef CLIENT_H_
#define CLIENT_H_
class Client {
 public:
  Client();
  void set_up(sockaddr_in server_address);
  void request(server_action action, std::string param = "");
  void abort();
  void pause();
  void resume();

 private:
  Socket socket_ = Socket(make_ip_address(0, "127.0.0.1"));
  sockaddr_in server_address_;
  AES aes_ = AES(AES::AES_256);
};
#endif
