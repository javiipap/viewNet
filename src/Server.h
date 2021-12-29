#ifndef SERVER_H_
#define SERVER_H_
#include <pthread.h>
#include <signal.h>

#include <memory>
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

  void abort();
  void pause();
  void resume();

 private:
  struct thread_info {
    pthread_t fd;
    std::string uuid;
    std::string type;
    void* args = nullptr;
  };

  struct get_file_args {
    const std::string& filename;
    const Socket& server_socket;
    sockaddr_in& client_addr;
    AES& aes;
  };

  struct list_args {
    const Socket& server_socket;
    sockaddr_in& client_addr;
    AES& aes;
  };

  struct main_thread_args {
    Socket& socket;
    int port;
    std::vector<thread_info>& threads;
    pthread_mutex_t& stop_server_mutex_;
    AES& aes;
  };

  struct prueba {
    int port;
    int algo;
  };

  AES aes_ = {AES::AES_256};
  Socket socket_;
  std::vector<thread_info> internal_threads_;
  pthread_mutex_t stop_server_mutex_ = pthread_mutex_t();
  int port_;
  thread_info main_thread_;

  static void* main_thread(void* args);

  static void* get_file(void* args);
  static void* list(void* args);

  static void delete_internal_threads(std::vector<thread_info>& threads);
};
#endif
