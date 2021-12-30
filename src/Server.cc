#include "Server.h"

Server::Server() {}

Server::~Server() {
  if (main_thread_.args != nullptr) {
    stop();
    pthread_join(main_thread_.fd, nullptr);
  };
}

void* Server::get_file(void* args) {
  const auto [filename, server_socket, client_addr, aes, aes_mutex] =
      *static_cast<get_file_args*>(args);
  int position = 0;
  int chunk = 1;
  Message msg;
  File file("public/" + filename);

  while (((chunk - 1) * MESSAGESIZE) < file.size()) {
    position = file.read(&msg.text, MESSAGESIZE);
    if ((MESSAGESIZE * (chunk - 1) + position) == file.size() && position % MESSAGESIZE != 0) {
      msg.text[position % MESSAGESIZE] = '\0';
    }
    msg.chunk_size = position + 1;
    // pthread_mutex_lock(&aes_mutex);
    // aes.Encrypt(msg.text.data(), MESSAGESIZE, msg.text.data());
    // pthread_mutex_unlock(&aes_mutex);
    ssize_t sent_bytes = server_socket.send_to(msg, client_addr);

    if ((MESSAGESIZE * (chunk - 1) + position) == file.size() && position % MESSAGESIZE == 0) {
      msg.text[0] = '\0';
      msg.chunk_size = 1;
      server_socket.send_to(msg, client_addr);
    }
    chunk++;
  }

  // TODO: Eliminar thread del registro.
  return nullptr;
}

void* Server::list(void* args) {
  const auto [server_socket, client_addr, aes, aes_mutex] = *static_cast<list_args*>(args);
  auto dir = opendir("public");
  dirent* dir_file;
  closedir(dir);
  bool terminated = false;

  Message msg;
  while ((dir_file = readdir(dir)) != nullptr) {
    int i = 0;

    std::string line;
    line += dir_file->d_type;
    line += ' ';
    line += dir_file->d_name;
    line += '\n';

    for (const auto ch : line) {
      if (msg.chunk_size == MESSAGESIZE) {
        // pthread_mutex_lock(&aes_mutex);
        // aes.Encrypt(msg.text.data(), MESSAGESIZE, msg.text.data());
        // pthread_mutex_unlock(&aes_mutex);

        server_socket.send_to(msg, client_addr);

        msg.chunk_size = 0;
      }
      msg.text[msg.chunk_size++] = ch;
    }
  };

  if (msg.chunk_size + 1 < MESSAGESIZE) {
    msg.text[msg.chunk_size++] = 0;
    // pthread_mutex_lock(&aes_mutex);
    // aes.Encrypt(msg.text.data(), MESSAGESIZE, msg.text.data());
    // pthread_mutex_unlock(&aes_mutex);

    server_socket.send_to(msg, client_addr);

    terminated = true;
  }

  if (!terminated) {
    msg.text[0] = 0;
    msg.chunk_size = 1;
    // pthread_mutex_lock(&aes_mutex);
    // aes.Encrypt(msg.text.data(), MESSAGESIZE, msg.text.data());
    // pthread_mutex_unlock(&aes_mutex);

    server_socket.send_to(msg, client_addr);
  }
  // Eliminar thread del registro.
  return nullptr;
}

void Server::listen(int port) {
  port_ = port;
  socket_.set_up(make_ip_address(port, "127.0.0.1"));
  std::cout << "[SERVER]: Setting up... " << std::endl;
  main_thread_.fd = pthread_t();
  main_thread_.args = new main_thread_args{std::ref(socket_),
                                           port_,
                                           std::ref(internal_threads_),
                                           std::ref(stop_server_mutex_),
                                           std::ref(aes_),
                                           std::ref(aes_mutex_)};
  pthread_create(&main_thread_.fd, nullptr, main_thread, main_thread_.args);
  std::cout << "[SERVER]: Listening on port " << port << std::endl;
}

void* Server::main_thread(void* args) {
  auto [socket, port, threads, stop_server_mutex, aes, aes_mutex] =
      *static_cast<main_thread_args*>(args);

  try {
    auto server_addr = make_ip_address(port);
    Message msg;
    struct sockaddr_in client_addr {};
    ssize_t read_bytes = 0;

    while (true) {
      pthread_mutex_unlock(&stop_server_mutex);
      read_bytes = socket.recieve_from(msg, client_addr);

      if (!read_bytes) {
        std::cout << "[SERVER]: Closing threads..." << std::endl;
        delete static_cast<main_thread_args*>(args);
        delete_internal_threads(threads);
        std::cout << "[SERVER]: Exit." << std::endl;
        return nullptr;
      }

      pthread_mutex_lock(&stop_server_mutex);
      std::string param;
      auto action = DecodeAction(msg, &param);
      threads.push_back({pthread_t(), GenerateUid(), "", nullptr});
      if (action == server_action::get_file) {
        threads.back().args = new get_file_args{param, std::ref(socket), client_addr, std::ref(aes),
                                                std::ref(aes_mutex)};
        threads.back().type = "get";
        pthread_create(&threads.back().fd, nullptr, get_file, threads.back().args);
      } else if (action == server_action::list_files) {
        threads.back().args =
            new list_args{std::ref(socket), client_addr, std::ref(aes), std::ref(aes_mutex)};
        threads.back().type = "list";
        pthread_create(&threads.back().fd, nullptr, list, threads.back().args);
      } else {
        std::cout << "Te mamaste" << std::endl;
      }
    }
  } catch (std::invalid_argument& err) {
    std::cout << err.what() << std::endl;
    delete_internal_threads(threads);
    return nullptr;
  } catch (std::exception& err) {
    std::cerr << "[SERVER]: Critical error: " << err.what() << std::endl;
    delete_internal_threads(threads);
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

void Server::delete_internal_threads(std::vector<thread_info>& threads) {
  for (int i = threads.size() - 1; i >= 0; i--) {
    pthread_join(threads[i].fd, nullptr);

    if (threads[i].type == "list") {
      delete static_cast<list_args*>(threads[i].args);
    } else if (threads[i].type == "get") {
      delete static_cast<get_file_args*>(threads[i].args);
    } else {
      std::cout << "[SERVER]: Memory leak detected in thread (" << threads[i].uuid << ")"
                << std::endl;
    }
  }
}
