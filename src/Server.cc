#include "Server.h"

Server::Server() {}

Server::~Server() {
  if (main_thread_.args != nullptr) {
    stop();
    pthread_join(main_thread_.fd, nullptr);
  };
}

void* Server::get_file(void* args) {
  const auto [filename, client_addr, uuid, instance] = *static_cast<get_file_args*>(args);

  try {
    int position = 0;
    int chunk = 1;
    Message msg;
    File file("public/" + filename);
    Socket socket(make_ip_address(0));
    int size = 0;

    EncodeAction(msg, server_action::retrieve_uuid, uuid);
    msg.end_of_mesage = true;
    socket.send_to(msg, client_addr);
    msg.end_of_mesage = false;

    while (((chunk - 1) * MESSAGESIZE) < file.size() && !instance->internal_threads_[uuid].stop) {
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
      msg.chunk_idx = chunk - 1;
      sleep(15);
      // usleep(50000);  // Para que de tiempo a imprimir por pantalla antes de enviar el siguiente
      // paquete.
    }
    std::cout << "[SERVER] (" << uuid.substr(0, 4) << "): Sent " << filename << " successfully ("
              << size << " bytes) = " << chunk << " chunks." << std::endl;
  } catch (std::exception& err) {
    std::cerr << "viewNet [SERVER]: " << err.what() << std::endl;
  }

  instance->delete_self(uuid);
  return nullptr;
}

void* Server::list(void* args) {
  const auto [client_addr, uuid, instance] = *static_cast<general_args*>(args);
  DIR* dir = opendir("public");

  if (dir == nullptr) {
    instance->delete_self(uuid);
    std::cerr << "viewNet [SERVER]: Error opening public directory." << std::endl;
    return nullptr;
  }

  try {
    dirent* dir_file;
    bool terminated = false;
    Socket socket(make_ip_address(0));
    Message msg;

    EncodeAction(msg, server_action::retrieve_uuid, uuid);
    msg.end_of_mesage = true;
    socket.send_to(msg, client_addr);
    msg.end_of_mesage = false;

    while ((dir_file = readdir(dir)) != nullptr && !instance->internal_threads_[uuid].stop) {
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

    std::cout << "[SERVER] (" << uuid.substr(0, 4) << "): Sent directory list." << std::endl;
  } catch (std::exception& err) {
    std::cerr << "viewNet [SERVER]: " << err.what() << std::endl;
  }

  closedir(dir);
  instance->delete_self(uuid);
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

    std::string uuid;

    while (true) {
      pthread_mutex_unlock(&instance->stop_server_mutex_);
      read_bytes = socket.recieve_from(msg, client_addr);

      if (!read_bytes) {
        instance->delete_internal_threads();
        return nullptr;
      }

      pthread_mutex_lock(&instance->stop_server_mutex_);
      std::string param;
      auto action = DecodeAction(msg, &param);
      pthread_mutex_lock(&instance->threads_vector_mutex_);

      if (action == server_action::abortar) {
        std::cout << "[SERVER]: Stoping task " << param << std::endl;
        if (instance->internal_threads_.find(param) == instance->internal_threads_.end()) {
          std::cerr << "[SERVER]: No task found identified by" << param << std::endl;
          pthread_mutex_unlock(&instance->threads_vector_mutex_);
          continue;
        }

        instance->internal_threads_[param].stop = true;
        pthread_kill(instance->internal_threads_[param].fd, SIGUSR1);
        pthread_mutex_unlock(&instance->threads_vector_mutex_);

        continue;
      }

      uuid = generate_uuid();
      instance->internal_threads_[uuid].fd = pthread_t();
      instance->internal_threads_[uuid].type = action;
      std::cout << "[SERVER]: Starting task " << uuid << std::endl;
      switch (action) {
        case server_action::get_file: {
          instance->internal_threads_[uuid].args =
              new get_file_args{param, client_addr, uuid, instance};
          pthread_create(&instance->internal_threads_[uuid].fd, nullptr, get_file,
                         instance->internal_threads_[uuid].args);
          break;
        }
        case server_action::list_files: {
          instance->internal_threads_[uuid].args = new general_args{client_addr, uuid, instance};
          pthread_create(&instance->internal_threads_[uuid].fd, nullptr, list,
                         instance->internal_threads_[uuid].args);
          break;
        }
        case server_action::pause_resume: {
          instance->internal_threads_[uuid].args = new general_args{client_addr, uuid, instance};
          pthread_create(&instance->internal_threads_[uuid].fd, nullptr, pause_resume,
                         instance->internal_threads_[uuid].args);
          break;
        }
        default:
          std::cout << "Te mamaste" << std::endl;
      }
      pthread_mutex_unlock(&instance->threads_vector_mutex_);
    }
  } catch (std::exception& err) {
    std::cerr << "viewNet [SERVER]: Critical error: " << err.what() << std::endl;
    instance->delete_internal_threads();
    return nullptr;
  }
}

void Server::stop() {
  pthread_mutex_unlock(&stop_server_mutex_);
  pthread_kill(main_thread_.fd, SIGUSR1);
};

void* Server::pause_resume(void* args) {
  auto [client_addr, uuid, instance] = *static_cast<general_args*>(args);

  try {
    std::cout << "[SERVER]: Pausing/resuming task " << uuid << std::endl;
    pthread_mutex_lock(&instance->threads_vector_mutex_);
    if (instance->internal_threads_.find(uuid) == instance->internal_threads_.end()) {
      std::cerr << "[SERVER]: No task found identified by" << uuid << std::endl;
      pthread_mutex_unlock(&instance->threads_vector_mutex_);
      return nullptr;
    }

    pthread_kill(instance->internal_threads_[uuid].fd, SIGUSR2);
    pthread_mutex_unlock(&instance->threads_vector_mutex_);
  } catch (std::exception& err) {
    std::cerr << "viewNet [SERVER]: " << err.what() << std::endl;
  }

  instance->delete_self(uuid);
  return nullptr;
}

void Server::delete_internal_threads() {
  std::cout << "[SERVER]: Closing threads..." << std::endl;

  pthread_mutex_lock(&threads_vector_mutex_);
  for (auto it = internal_threads_.begin(); it != internal_threads_.end(); it++) {
    pthread_join(it->second.fd, nullptr);

    switch (it->second.type) {
      case server_action::get_file:
        delete static_cast<get_file_args*>(it->second.args);
        break;
      case server_action::list_files:
      default:
        delete static_cast<general_args*>(it->second.args);
        break;
    }
  }
  pthread_mutex_unlock(&threads_vector_mutex_);
  std::cout << "[SERVER]: Exit." << std::endl;
}

void Server::delete_self(std::string uuid) {
  std::cout << "[SERVER]: Closing task " << uuid << std::endl;
  pthread_mutex_lock(&threads_vector_mutex_);
  pthread_join(internal_threads_[uuid].fd, nullptr);

  if (internal_threads_.find(uuid) == internal_threads_.end()) {
    pthread_mutex_unlock(&threads_vector_mutex_);
    return;
  }

  switch (internal_threads_[uuid].type) {
    case server_action::get_file:
      delete static_cast<get_file_args*>(internal_threads_[uuid].args);
      break;
    case server_action::list_files:
    default:
      delete static_cast<general_args*>(internal_threads_[uuid].args);
      break;
  }
  internal_threads_.erase(uuid);
  pthread_mutex_unlock(&threads_vector_mutex_);
  std::cout << "[SERVER]: Deleted task " << uuid << std::endl;
}

void Server::info() { std::cout << "Threads: " << internal_threads_.size() << std::endl; }
