#ifndef SERVER_H_
#define SERVER_H_
#include <pthread.h>
#include <signal.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <utility>

#include "AES.h"
#include "File.h"
#include "Socket.h"
#include "dirent.h"
#include "functions.h"

class Server {
 public:
  Server();

  ~Server();

  void listen(int port);

  void stop();

  void info();

 private:
  struct thread_info {
    pthread_t fd;
    server_action type;
    std::atomic<bool> stop = false;
    void* args = nullptr;
  };

  struct get_file_args {
    const std::string& filename;
    sockaddr_in& client_addr;
    std::string uuid;
    Server* instance;
  };

  struct general_args {
    sockaddr_in& client_addr;
    std::string uuid;
    Server* instance;
  };

  struct main_thread_args {
    int port;
    std::vector<thread_info>& threads;
    pthread_mutex_t& stop_server_mutex_;
    AES& aes;
    pthread_mutex_t& aes_mutex;
    Server* instance;
  };

  AES aes_ = {AES::AES_256};
  std::unordered_map<std::string, thread_info> internal_threads_;
  pthread_mutex_t stop_server_mutex_ = pthread_mutex_t();
  pthread_mutex_t aes_mutex_ = pthread_mutex_t();
  pthread_mutex_t threads_vector_mutex_ = pthread_mutex_t();
  int port_;
  thread_info main_thread_;
  std::unordered_map<std::string, std::string> files_sha256_;

  static void* main_thread(void* args);
  static void* get_file(void* args);
  static void* list(void* args);

  static void* pause_resume(void* args);

  void delete_self(std::string uuid);

  void delete_internal_threads();
};
#endif
