#include "Server.h"

Server::Server() {}

Server::~Server() {
  if (main_thread_.args != nullptr) {
    stop();
    pthread_join(main_thread_.fd, nullptr);
  };
}

void* Server::get_file(void* args) {
  const auto [filename, client_addr, self, instance] = *static_cast<get_file_args*>(args);
  int position = 0;
  int chunk = 1;
  Message msg;
  File file("public/" + filename);
  Socket socket(make_ip_address(0));
  int size = 0;

  EncodeAction(msg, server_action::retrieve_uuid, self.uuid);
  msg.end_of_mesage = true;
  socket.send_to(msg, client_addr);
  msg.end_of_mesage = false;

  while (((chunk - 1) * MESSAGESIZE) < file.size()) {
    position = file.read(&msg.text, MESSAGESIZE);
    size += position;

    if ((MESSAGESIZE * (chunk - 1) + position) == file.size() && position % MESSAGESIZE != 0) {
      msg.text[position % MESSAGESIZE] = 0;
      msg.end_of_mesage = true;
    }
    msg.chunk_size = position + 1;
    // pthread_mutex_lock(&aes_mutex);
    // aes.Encrypt(msg.text.data(), MESSAGESIZE, msg.text.data());
    // pthread_mutex_unlock(&aes_mutex);
    socket.send_to(msg, client_addr);

    if ((MESSAGESIZE * (chunk - 1) + position) == file.size() && position % MESSAGESIZE == 0) {
      msg.text[0] = 0;
      msg.chunk_size = 1;
      msg.end_of_mesage = true;
      socket.send_to(msg, client_addr);
    }
    chunk++;
  }
  std::cout << "[SERVER] (" << self.uuid.substr(0, 4) << "): Sent " << filename << " successfully ("
            << size << " bytes)" << std::endl;
  instance->delete_self(self.uuid);
  return nullptr;
}

void* Server::list(void* args) {
  const auto [client_addr, self, instance] = *static_cast<list_args*>(args);
  auto dir = opendir("public");
  dirent* dir_file;
  bool terminated = false;
  Socket socket(make_ip_address(0));
  Message msg;

  EncodeAction(msg, server_action::retrieve_uuid, self.uuid);
  msg.end_of_mesage = true;
  socket.send_to(msg, client_addr);
  msg.end_of_mesage = false;

  while ((dir_file = readdir(dir)) != nullptr) {
    int i = 0;

    std::string line;
    line += dir_file->d_name;
    line += '\n';

    for (const auto ch : line) {
      if (msg.chunk_size == MESSAGESIZE) {
        // pthread_mutex_lock(&aes_mutex);
        // aes.Encrypt(msg.text.data(), MESSAGESIZE, msg.text.data());
        // pthread_mutex_unlock(&aes_mutex);

        socket.send_to(msg, client_addr);

        msg.chunk_size = 0;
      }
      msg.text[msg.chunk_size++] = ch;
    }
  };

  if (msg.chunk_size + 1 < MESSAGESIZE) {
    msg.text[msg.chunk_size++] = 0;
    msg.end_of_mesage = true;
    // pthread_mutex_lock(&aes_mutex);
    // aes.Encrypt(msg.text.data(), MESSAGESIZE, msg.text.data());
    // pthread_mutex_unlock(&aes_mutex);

    socket.send_to(msg, client_addr);

    terminated = true;
  }

  if (!terminated) {
    msg.text[0] = 0;
    msg.chunk_size = 1;
    msg.end_of_mesage = true;
    // pthread_mutex_lock(&aes_mutex);
    // aes.Encrypt(msg.text.data(), MESSAGESIZE, msg.text.data());
    // pthread_mutex_unlock(&aes_mutex);

    socket.send_to(msg, client_addr);
  }

  std::cout << "[SERVER] (" << self.uuid.substr(0, 4) << "): Sent directory list." << std::endl;
  closedir(dir);
  instance->delete_self(self.uuid);
  return nullptr;
}

void Server::listen(int port) {
  port_ = port;
  std::cout << "[SERVER]: Setting up... " << std::endl;
  main_thread_.fd = pthread_t();
  main_thread_.args = this;
  pthread_create(&main_thread_.fd, nullptr, main_thread, this);
}

void* Server::main_thread(void* args) {
  auto instance = static_cast<Server*>(args);
  try {
    auto server_addr = make_ip_address(instance->port_);
    Socket socket(server_addr);
    std::cout << "[SERVER]: Listening on port " << instance->port_ << std::endl;
    Message msg;
    struct sockaddr_in client_addr {};
    ssize_t read_bytes = 0;

    while (true) {
      pthread_mutex_unlock(&instance->stop_server_mutex_);
      read_bytes = socket.recieve_from(msg, client_addr);

      if (!read_bytes) {
        std::cout << "[SERVER]: Closing threads..." << std::endl;
        instance->delete_internal_threads();
        std::cout << "[SERVER]: Exit." << std::endl;
        return nullptr;
      }

      pthread_mutex_lock(&instance->stop_server_mutex_);
      std::string param;
      auto action = DecodeAction(msg, &param);
      pthread_mutex_lock(&instance->threads_vector_mutex_);
      instance->internal_threads_.push_back({pthread_t(), GenerateUid(), action, nullptr});
      std::cout << "[SERVER]: Starting task (" << instance->internal_threads_.back().uuid << ")"
                << std::endl;
      switch (action) {
        case server_action::get_file: {
          instance->internal_threads_.back().args =
              new get_file_args{param, client_addr, instance->internal_threads_.back(), instance};
          pthread_create(&instance->internal_threads_.back().fd, nullptr, get_file,
                         instance->internal_threads_.back().args);
          break;
        }
        case server_action::list_files: {
          instance->internal_threads_.back().args =
              new list_args{client_addr, instance->internal_threads_.back(), instance};
          pthread_create(&instance->internal_threads_.back().fd, nullptr, list,
                         instance->internal_threads_.back().args);
          break;
        }
        default:
          std::cout << "Te mamaste" << std::endl;
      }
      pthread_mutex_unlock(&instance->threads_vector_mutex_);
    }
  } catch (std::exception& err) {
    std::cerr << "[SERVER]: Critical error: " << err.what() << std::endl;
    instance->delete_internal_threads();
    return nullptr;
  }
}

void Server::stop() {
  pthread_mutex_unlock(&stop_server_mutex_);
  pthread_kill(main_thread_.fd, SIGUSR1);
};

void Server::pause() {}

void Server::resume() {}

void Server::abort() {}

void Server::delete_internal_threads() {
  pthread_mutex_lock(&threads_vector_mutex_);
  for (int i = internal_threads_.size() - 1; i >= 0; i--) {
    pthread_join(internal_threads_[i].fd, nullptr);

    switch (internal_threads_[i].type) {
      case server_action::get_file:
        delete static_cast<get_file_args*>(internal_threads_[i].args);
        break;
      case server_action::list_files:
        delete static_cast<list_args*>(internal_threads_[i].args);
        break;
      default:
        std::cerr << "[SERVER]: Memory leak detected in thread (" << internal_threads_[i].uuid
                  << ")" << std::endl;
        break;
    }
  }
  pthread_mutex_unlock(&threads_vector_mutex_);
}

void Server::delete_self(std::string uuid) {
  std::cout << "[SERVER]: Closing task (" << uuid << ")" << std::endl;
  pthread_mutex_lock(&threads_vector_mutex_);
  for (int i = 0; i < internal_threads_.size(); i++) {
    if (internal_threads_[i].uuid == uuid) {
      switch (internal_threads_[i].type) {
        case server_action::get_file:
          delete static_cast<get_file_args*>(internal_threads_[i].args);
          break;
        case server_action::list_files:
          delete static_cast<list_args*>(internal_threads_[i].args);
          break;
        default:
          std::cerr << "[SERVER]: Memory leak detected in thread (" << internal_threads_[i].uuid
                    << ")" << std::endl;
          break;
      }
    }
    internal_threads_.erase(internal_threads_.begin() + i);
    break;
  }
  pthread_mutex_unlock(&threads_vector_mutex_);
}

void Server::info() { std::cout << "Threads: " << internal_threads_.size() << std::endl; }
