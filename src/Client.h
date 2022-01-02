#include <atomic>
#include <unordered_map>

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
  void pause(std::string uuid);
  void resume(std::string uuid);
  void info() const;
  bool has_pending_tasks() const;
  void stop();

 private:
  struct thread_args {
    server_action action;
    std::string param;
    std::string uuid;
    Client* instance;
  };

  struct thread_info {
    pthread_t fd;
    std::string server_task_uuid;
    std::atomic<bool> stop = false;
    std::atomic<bool> pause = false;
    thread_args* args = nullptr;
  };

  sockaddr_in server_address_;
  AES aes_ = AES(AES::AES_256);
  std::unordered_map<std::string, thread_info> threads_;
  pthread_mutex_t threads_mutex_ = pthread_mutex_t();

  static void* internal_handler(void* args);

  void delete_self(std::string uuid);
  void delete_internal_threads(bool force = false);
};
#endif
