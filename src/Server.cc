#include "Server.h"

Server::Server() {}

Server::~Server() {
  if (main_thread_.args != nullptr) {
    stop();
    pthread_join(main_thread_.fd, nullptr);
  };
}

void* Server::get_file(void* args) {
  const auto [filename, client_addr, uuid, instance] = *static_cast<thread_args*>(args);

  try {
    int position = 0;
    int chunk = 1;
    Message msg;
    File file("public/" + filename);
    Socket socket(make_ip_address(0));
    int size = 0;

    EncodeAction(msg, server_action::retrieve_uuid, uuid);

    send_encrypted(msg, socket, client_addr);

    while (((chunk - 1) * MESSAGESIZE) < file.size() &&
           !instance->internal_threads_.at(uuid).stop) {
      position = file.read(&msg.text, MESSAGESIZE);
      size += position;

      msg.chunk_size = position;
      if (size == file.size() && position % MESSAGESIZE != 0) {
        msg.text[position % MESSAGESIZE] = 0;
        msg.chunk_size++;
      }

      int sent = send_encrypted(msg, socket, client_addr);
      if (!sent && instance->internal_threads_[uuid].pause) {
        instance->internal_threads_[uuid].pause = false;

        send_encrypted(msg, socket, client_addr);
      }

      if (size == file.size() && position % MESSAGESIZE == 0) {
        msg.text[0] = 0;
        msg.chunk_size = 1;

        send_encrypted(msg, socket, client_addr);
      }
      chunk++;
      usleep(50000);  // Para que de tiempo a imprimir por pantalla antes de enviar el siguiente
      // paquete.
    }
    if (instance->internal_threads_.at(uuid).stop) {
      std::cout << "[SERVER]: Terminated task " << uuid << std::endl;
    };

    std::string file_info = "Filename: " + filename + "\nSize: " + std::to_string(size) +
                            "\nChunks: " + std::to_string(chunk - 1) +
                            "\nsha256: " + instance->files_sha256_[filename];

    memcpy(msg.text.data(), file_info.c_str(), file_info.size() + 1);
    msg.chunk_size = file_info.size() + 1;
    send_encrypted(msg, socket, client_addr);

    std::cout << "[SERVER] (" << uuid.substr(0, 4) << "): Sent " << filename << " successfully ("
              << size << " bytes) = " << chunk - 1 << " chunks." << std::endl;
  } catch (socket_error& err) {
    std::cerr << "viewNet [SERVER]: " << err.what() << std::endl;
  } catch (std::exception& err) {
    abort_client(instance, client_addr, err.what());
    std::cerr << "viewNet [SERVER]: " << err.what() << std::endl;
  }

  instance->delete_self(uuid);
  return nullptr;
}

void* Server::list(void* args) {
  const auto [_, client_addr, uuid, instance] = *static_cast<thread_args*>(args);
  DIR* dir = opendir("public");
  dirent* dir_file;

  if (dir == nullptr) {
    instance->delete_self(uuid);
    std::cerr << "viewNet [SERVER]: Error opening public directory." << std::endl;
    return nullptr;
  }

  try {
    bool terminated = false;
    Socket socket(make_ip_address(0));
    Message msg;

    EncodeAction(msg, server_action::retrieve_uuid, uuid);
    send_encrypted(msg, socket, client_addr);

    while ((dir_file = readdir(dir)) != nullptr && !instance->internal_threads_.at(uuid).stop) {
      int i = 0;

      std::string line;
      line += dir_file->d_name;
      line += '\n';

      for (const auto ch : line) {
        if (msg.chunk_size == MESSAGESIZE) {
          send_encrypted(msg, socket, client_addr);

          msg.chunk_size = 0;
        }
        msg.text[msg.chunk_size++] = ch;
      }
    };

    if (msg.chunk_size + 1 < MESSAGESIZE) {
      msg.text[msg.chunk_size++] = 0;

      send_encrypted(msg, socket, client_addr);

      terminated = true;
    }

    if (!terminated) {
      msg.text[0] = 0;
      msg.chunk_size = 1;

      send_encrypted(msg, socket, client_addr);
    }
    std::cout << "[SERVER] (" << uuid.substr(0, 4) << "): Sent directory list." << std::endl;
  } catch (socket_error& err) {
    std::cerr << "viewNet [SERVER]: " << err.what() << std::endl;
  } catch (std::exception& err) {
    abort_client(instance, client_addr, err.what());

    std::cerr << "viewNet [SERVER]: " << err.what() << std::endl;
  }

  delete dir_file;
  closedir(dir);
  instance->delete_self(uuid);
  return nullptr;
}

void Server::listen(int port) {
  store_hashes();
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
      read_bytes = socket.recieve_from(msg, client_addr);

      if (!read_bytes) {
        instance->delete_internal_threads();
        return nullptr;
      }

      std::string param;
      auto action = DecodeAction(msg, &param);
      pthread_mutex_lock(&instance->threads_vector_mutex_);

      if (action == server_action::abortar) {
        std::cout << "[SERVER]: Aborting task " << param << std::endl;

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
              new thread_args{param, client_addr, uuid, instance};
          pthread_create(&instance->internal_threads_[uuid].fd, nullptr, get_file,
                         instance->internal_threads_[uuid].args);
          break;
        }
        case server_action::list_files: {
          instance->internal_threads_[uuid].args = new thread_args{"", client_addr, uuid, instance};
          pthread_create(&instance->internal_threads_[uuid].fd, nullptr, list,
                         instance->internal_threads_[uuid].args);
          break;
        }
        case server_action::pausar: {
          instance->internal_threads_[uuid].args =
              new thread_args{param, client_addr, uuid, instance};
          pthread_create(&instance->internal_threads_[uuid].fd, nullptr, pause,
                         instance->internal_threads_[uuid].args);
          break;
        }
        case server_action::resumir: {
          instance->internal_threads_[uuid].args =
              new thread_args{param, client_addr, uuid, instance};
          pthread_create(&instance->internal_threads_[uuid].fd, nullptr, resume,
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

void Server::stop() { pthread_kill(main_thread_.fd, SIGUSR1); };

void* Server::pause(void* args) {
  auto [target_uuid, client_addr, uuid, instance] = *static_cast<thread_args*>(args);
  std::cout << "[SERVER]: Pausing task " << target_uuid << std::endl;

  if (instance->internal_threads_.find(target_uuid) == instance->internal_threads_.end()) {
    std::cerr << "[SERVER]: No task found identified by" << target_uuid << std::endl;
    return nullptr;
  }

  instance->internal_threads_[target_uuid].pause = true;
  pthread_kill(instance->internal_threads_[target_uuid].fd, SIGUSR2);
  instance->delete_self(uuid);
  return nullptr;
}

void* Server::resume(void* args) {
  auto [target_uuid, client_addr, uuid, instance] = *static_cast<thread_args*>(args);
  std::cout << "[SERVER]: Resuming task " << target_uuid << std::endl;

  if (instance->internal_threads_.find(target_uuid) == instance->internal_threads_.end()) {
    std::cerr << "[SERVER]: No task found identified by" << target_uuid << std::endl;
    return nullptr;
  }

  instance->internal_threads_[target_uuid].pause = false;
  pthread_kill(instance->internal_threads_[target_uuid].fd, SIGUSR1);
  instance->delete_self(uuid);
  return nullptr;
}

void Server::delete_internal_threads(bool force) {
  std::cout << "[SERVER]: Closing threads..." << std::endl;

  pthread_mutex_lock(&threads_vector_mutex_);
  for (auto it = internal_threads_.begin(); it != internal_threads_.end(); it++) {
    if (force) it->second.stop = true;
    pthread_join(it->second.fd, nullptr);

    delete static_cast<thread_args*>(it->second.args);
    internal_threads_.erase(it->first);
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

  delete static_cast<thread_args*>(internal_threads_[uuid].args);
  internal_threads_.erase(uuid);
  pthread_mutex_unlock(&threads_vector_mutex_);
  std::cout << "[SERVER]: Deleted task " << uuid << std::endl;
}

void Server::info() const {
  if (internal_threads_.size()) {
    std::cout << "[SERVER]: Active tasks:" << std::endl;
    for (auto it = internal_threads_.begin(); it != internal_threads_.end(); it++) {
      std::cout << it->first << (it->second.pause ? ": Paused." : ": Running.") << std::endl;
    }
  } else {
    std::cout << "[SERVER]: No tasks running." << std::endl;
  }
}

void Server::abort_client(Server* instance, sockaddr_in client_address, std::string error) {
  Socket socket(make_ip_address(0));
  Message msg;
  EncodeAction(msg, server_action::abortar, error);

  send_encrypted(msg, socket, client_address);
}

bool Server::has_pending_tasks() const { return internal_threads_.size(); }

ssize_t Server::send_encrypted(const Message& msg, Socket& socket, const sockaddr_in client_addr) {
  Message buffer;
  AES aes = {AES::AES_256};

  aes.Encrypt(msg.text.data(), MESSAGESIZE, buffer.text.data());
  buffer.chunk_size = msg.chunk_size;
  return socket.send_to(buffer, client_addr);
}

void Server::store_hashes() {
  DIR* dir = opendir("public");
  dirent* dir_file;
  if (dir == nullptr) {
    std::cerr << "viewNet [SERVER]: Error opening public directory." << std::endl;
    return;
  }

  while ((dir_file = readdir(dir)) != nullptr) {
    sha256 sha;
    std::string filename = "public/";
    filename += dir_file->d_name;
    if (filename == "public/." || filename == "public/..") continue;
    File file(filename);
    std::vector<uint8_t> file_content(file.size());
    file.read(file_content.data(), file.size());
    files_sha256_[dir_file->d_name] = sha.digest(file_content.data(), file_content.size());
  }

  closedir(dir);
  delete dir_file;
}
