/**
 * @author Javier Padilla Pío
 * @date 22/12/2021
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en ingeniería informática
 * Curso: 2º
 * Practice de programación: viewNet
 * Email: alu0101410463@ull.edu.es
 * Server.cc: Interfaz de la clase Server. Se encarga de servir archivos a un cliente.
 * Revision history: 22/12/2021 -
 *                   Creación (primera versión) del código
 */

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
  sleep(5);
  try {
    int position = 0;
    int chunk = 1;
    Message msg;
    File file("public/" + filename);
    Socket socket(make_ip_address(0));
    int size = 0;

    EncodeAction(msg, server_action::retrieve_uuid, uuid);

    send_encrypted(msg, socket, client_addr);

    while (((chunk - 1) * MESSAGESIZE) < file.size() && !instance->threads_.at(uuid).stop) {
      position = file.read(&msg.text, MESSAGESIZE);
      size += position;

      msg.chunk_size = position;
      if (size == file.size() && position % MESSAGESIZE != 0) {
        msg.text[position % MESSAGESIZE] = 0;
        msg.chunk_size++;
      }

      int sent = send_encrypted(msg, socket, client_addr);
      if (!sent && instance->threads_[uuid].pause) {
        instance->threads_[uuid].pause = false;

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

    if (instance->threads_.at(uuid).stop) {
      std::cout << "[SERVER] (" << uuid.substr(0, 4) << "): Recieved kill. " << std::endl;
      instance->delete_self(uuid);
      return nullptr;
    };

    std::string file_info = "Filename: " + filename + "\nSize: " + std::to_string(size) +
                            " bytes\nChunks: " + std::to_string(chunk - 1) +
                            "\nsha256: " + instance->files_sha256_[filename];

    memcpy(msg.text.data(), file_info.c_str(), file_info.size() + 1);
    msg.chunk_size = file_info.size() + 1;
    send_encrypted(msg, socket, client_addr);

    std::cout << "[SERVER] (" << uuid.substr(0, 4) << "): Sent " << filename << " successfully ("
              << size << " bytes) = " << chunk - 1 << " chunks." << std::endl;
  } catch (socket_error& err) {
    std::cerr << "[SERVER] (" << uuid.substr(0, 4) << ") Error: " << err.what() << std::endl;
  } catch (std::exception& err) {
    abort_client(instance, client_addr, err.what());
    std::cerr << "[SERVER] (" << uuid.substr(0, 4) << ") Error: " << err.what() << std::endl;
  }

  instance->delete_self(uuid);
  return nullptr;
}

void* Server::list(void* args) {
  const auto [_, client_addr, uuid, instance] = *static_cast<thread_args*>(args);
  DIR* dir = opendir("public");
  dirent* dir_file;

  if (dir == nullptr) {
    std::cerr << "[SERVER] (" << uuid.substr(0, 4) << ") Error: Opening public directory."
              << std::endl;
    instance->delete_self(uuid);
    return nullptr;
  }

  try {
    bool terminated = false;
    Socket socket(make_ip_address(0));
    Message msg;

    EncodeAction(msg, server_action::retrieve_uuid, uuid);
    send_encrypted(msg, socket, client_addr);

    while ((dir_file = readdir(dir)) != nullptr && !instance->threads_.at(uuid).stop) {
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
    std::cerr << "[SERVER] (" << uuid.substr(0, 4) << ") Error: " << err.what() << std::endl;
  } catch (std::exception& err) {
    abort_client(instance, client_addr, err.what());

    std::cerr << "[SERVER] (" << uuid.substr(0, 4) << ") Error: " << err.what() << std::endl;
  }

  delete dir_file;
  closedir(dir);
  instance->delete_self(uuid);
  return nullptr;
}

void Server::listen(int port) {
  if (port_ != -1) {
    std::cerr << "[SERVER]: El servidor ya se está ejecutando en el puerto " << port_ << std::endl;
    return;
  }

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
      pthread_mutex_lock(&instance->threads_mutex_);

      if (action == server_action::abortar) {
        std::cout << "[SERVER]: Aborting task " << param << std::endl;

        if (instance->threads_.find(param) == instance->threads_.end()) {
          std::cerr << "[SERVER]: No task found identified by" << param << std::endl;
          pthread_mutex_unlock(&instance->threads_mutex_);
          continue;
        }

        instance->threads_[param].stop = true;
        pthread_kill(instance->threads_[param].fd, SIGUSR1);
        pthread_mutex_unlock(&instance->threads_mutex_);

        continue;
      }

      uuid = generate_uuid();
      instance->threads_[uuid].fd = pthread_t();
      instance->threads_[uuid].type = action;
      std::cout << "[SERVER]: Starting task " << uuid << std::endl;

      switch (action) {
        case server_action::get_file: {
          instance->threads_[uuid].args = new thread_args{param, client_addr, uuid, instance};
          pthread_create(&instance->threads_[uuid].fd, nullptr, get_file,
                         instance->threads_[uuid].args);
          break;
        }
        case server_action::list_files: {
          instance->threads_[uuid].args = new thread_args{"", client_addr, uuid, instance};
          pthread_create(&instance->threads_[uuid].fd, nullptr, list,
                         instance->threads_[uuid].args);
          break;
        }
        case server_action::pausar: {
          instance->threads_[uuid].args = new thread_args{param, client_addr, uuid, instance};
          pthread_create(&instance->threads_[uuid].fd, nullptr, pause,
                         instance->threads_[uuid].args);
          break;
        }
        case server_action::resumir: {
          instance->threads_[uuid].args = new thread_args{param, client_addr, uuid, instance};
          pthread_create(&instance->threads_[uuid].fd, nullptr, resume,
                         instance->threads_[uuid].args);
          break;
        }
        default:
          std::cout << "Te mamaste" << std::endl;
      }

      pthread_mutex_unlock(&instance->threads_mutex_);
    }
  } catch (std::exception& err) {
    std::cerr << "[SERVER] (main) Critical error: " << err.what() << std::endl;
    instance->delete_internal_threads();
    return nullptr;
  }
}

void Server::stop() { pthread_kill(main_thread_.fd, SIGUSR1); };

void* Server::pause(void* args) {
  auto [target_uuid, client_addr, uuid, instance] = *static_cast<thread_args*>(args);
  std::cout << "[SERVER]: Pausing task " << target_uuid << std::endl;
  pthread_mutex_lock(&instance->threads_mutex_);

  if (instance->threads_.find(target_uuid) == instance->threads_.end()) {
    std::cerr << "[SERVER]: No task found identified by" << target_uuid << std::endl;
    return nullptr;
  }

  instance->threads_[target_uuid].pause = true;
  pthread_kill(instance->threads_[target_uuid].fd, SIGUSR2);
  pthread_mutex_unlock(&instance->threads_mutex_);
  instance->delete_self(uuid);
  return nullptr;
}

void* Server::resume(void* args) {
  auto [target_uuid, client_addr, uuid, instance] = *static_cast<thread_args*>(args);
  std::cout << "[SERVER]: Resuming task " << target_uuid << std::endl;
  pthread_mutex_lock(&instance->threads_mutex_);

  if (instance->threads_.find(target_uuid) == instance->threads_.end()) {
    std::cerr << "[SERVER]: No task found identified by" << target_uuid << std::endl;
    return nullptr;
  }

  instance->threads_[target_uuid].pause = false;
  pthread_kill(instance->threads_[target_uuid].fd, SIGUSR1);
  pthread_mutex_unlock(&instance->threads_mutex_);
  instance->delete_self(uuid);
  return nullptr;
}

void Server::delete_internal_threads(bool force) {
  std::cout << "[SERVER]: Closing threads..." << std::endl;

  pthread_mutex_lock(&threads_mutex_);
  for (auto it = threads_.begin(); it != threads_.end(); it++) {
    if (force) {
      it->second.stop = true;
      pthread_kill(it->second.fd, SIGUSR1);
    };

    pthread_join(it->second.fd, nullptr);

    delete static_cast<thread_args*>(it->second.args);
    threads_.erase(it->first);
  }
  pthread_mutex_unlock(&threads_mutex_);
  port_ = -1;
  std::cout << "[SERVER]: Exit." << std::endl;
}

void Server::delete_self(std::string uuid) {
  // std::cout << "[SERVER]" << uuid << ": Closing task" << std::endl;
  pthread_mutex_lock(&threads_mutex_);
  pthread_join(threads_[uuid].fd, nullptr);

  if (threads_.find(uuid) == threads_.end()) {
    pthread_mutex_unlock(&threads_mutex_);
    return;
  }

  delete static_cast<thread_args*>(threads_[uuid].args);
  threads_.erase(uuid);
  pthread_mutex_unlock(&threads_mutex_);
  std::cout << "[SERVER] (" << uuid.substr(0, 4) << "): Task deleted" << std::endl;
}

void Server::info() const {
  if (threads_.size()) {
    std::cout << "[SERVER]: Active tasks:" << std::endl;
    for (auto it = threads_.begin(); it != threads_.end(); it++) {
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

bool Server::has_pending_tasks() const { return threads_.size(); }

ssize_t Server::send_encrypted(Message& msg, Socket& socket, const sockaddr_in client_addr) {
  AES aes = {AES::AES_256};

  aes.Encrypt(msg.text.data(), MESSAGESIZE, msg.text.data());
  return socket.send_to(msg, client_addr);
}

void Server::store_hashes() {
  DIR* dir = opendir("public");
  dirent* dir_file;

  if (dir == nullptr) {
    std::cerr << "[SERVER] Error: Opening public directory." << std::endl;
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
