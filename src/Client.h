#include "AES.h"
#include "Message.h"
#include "Socket.h"
#include "functions.h"

#ifndef CLIENT_H_
#define CLIENT_H_
class Client {
 public:
  Client();
  ~Client();
  void set_up(sockaddr_in server_address);
  void request(server_action action, std::string param = "");
  void abort(std::string uuid);
  void pause();
  void resume();

 private:
  struct thread_info {
    pthread_t fd;
    std::string uuid;
    std::string server_task_uuid;
    void* args = nullptr;
  };

  struct thread_args {
    server_action action;
    std::string param;
    thread_info& self;
    Client* instance;
  };

  sockaddr_in server_address_;
  AES aes_ = AES(AES::AES_256);
  std::vector<thread_info> threads_;
  pthread_mutex_t threads_mutex_ = pthread_mutex_t();

  static void* internal_handler(void* args);

  void delete_self(std::string uuid);
  void delete_internal_threads();
};
#endif
