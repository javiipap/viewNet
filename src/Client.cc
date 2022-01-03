#include "Client.h"

Client::Client(){};

Client::~Client() {
  if (threads_.size()) delete_internal_threads();
};

void Client::set_up(sockaddr_in server_address) { server_address_ = server_address; }

void Client::request(server_action action, std::string param) {
  std::string uuid = generate_uuid();
  threads_[uuid].fd = pthread_t();
  threads_[uuid].args = new thread_args{action, param, uuid, this};
  pthread_create(&threads_[uuid].fd, nullptr, internal_handler, threads_[uuid].args);
}

void Client::abort(std::string uuid) {
  if (threads_.find(uuid) == threads_.end()) {
    std::cerr << "[CLIENT]: No thread found identified by " << uuid << std::endl;
    return;
  }

  Socket socket(make_ip_address(0));
  Message msg;
  EncodeAction(msg, server_action::abortar, threads_[uuid].server_task_uuid);
  socket.send_to(msg, server_address_);

  std::cout << "[CLIENT]: Stoping task" << uuid << std::endl;

  pthread_mutex_lock(&threads_mutex_);
  threads_[uuid].stop = true;
  pthread_kill(threads_[uuid].fd, SIGUSR1);
  pthread_mutex_unlock(&threads_mutex_);
}

void Client::pause(std::string uuid) {
  std::cout << "[CLIENT]: Pausing task " << uuid << std::endl;
  if (threads_.find(uuid) == threads_.end()) {
    std::cerr << "[CLIENT]: No thread found identified by " << uuid << std::endl;
    return;
  }

  Socket socket(make_ip_address(0));
  Message msg;
  std::cout << threads_[uuid].server_task_uuid << std::endl;
  EncodeAction(msg, server_action::pausar, threads_[uuid].server_task_uuid);
  socket.send_to(msg, server_address_);
  threads_[uuid].pause = false;
  pthread_kill(threads_[uuid].fd, SIGUSR2);
}

void Client::resume(std::string uuid) {
  std::cout << "[CLIENT]: Resuming task " << uuid << std::endl;
  if (threads_.find(uuid) == threads_.end()) {
    std::cerr << "[CLIENT]: No thread found identified by " << uuid << std::endl;
    return;
  }
  Socket socket(make_ip_address(0));
  Message msg;
  EncodeAction(msg, server_action::resumir, threads_[uuid].server_task_uuid);
  socket.send_to(msg, server_address_);
  threads_[uuid].pause = true;
  pthread_kill(threads_[uuid].fd, SIGUSR1);
}

void* Client::internal_handler(void* args) {
  const auto [action, param, uuid, instance] = *static_cast<thread_args*>(args);

  try {
    std::cout << "[CLIENT]: Starting task " << uuid << std::endl;
    Socket socket(make_ip_address(0));
    Message buffer;
    EncodeAction(buffer, action, param);
    socket.send_to(buffer, instance->server_address_);

    sockaddr_in worker_addr;
    recieve_encrypted(buffer, socket, worker_addr);
    auto retrieved_action = DecodeAction(buffer, &instance->threads_[uuid].server_task_uuid);
    if (retrieved_action == server_action::abortar) {
      throw std::runtime_error("El servidor ha cerrado la conexión. " +
                               instance->threads_[uuid].server_task_uuid);
    }

    bool end_found = false;
    while (!end_found && !instance->threads_[uuid].stop) {
      ssize_t read = recieve_encrypted(buffer, socket, worker_addr);
      if (!read && errno == 4) {
        continue;
      }

      try {
        std::string param;
        auto action = DecodeAction(buffer, &param);

        if (action == server_action::close_connection) {
          throw std::runtime_error("El servidor ha cerrado la conexión. " + param);
        }
      } catch (std::invalid_argument) {
      }

      for (int i = 0; i < buffer.chunk_size; i++) {
        std::cout << buffer.text[i];

        if (buffer.text[i] == 0) {
          end_found = true;
          std::cout << std::endl;
          break;
        }
      }
    }

    if (action == server_action::get_file) {
      recieve_encrypted(buffer, socket, worker_addr);
      std::cout << "\n" << buffer.text.data() << "\n" << std::endl;
    }
  } catch (std::exception& err) {
    std::cerr << "viewNet [CLIENT]: " << err.what() << std::endl;
  }

  instance->delete_self(uuid);
  return nullptr;
}

void Client::stop() { delete_internal_threads(true); }

void Client::delete_internal_threads(bool force) {
  std::cout << "[CLIENT]: Closing threads..." << std::endl;

  pthread_mutex_lock(&threads_mutex_);
  for (auto it = threads_.begin(); it != threads_.end(); it++) {
    if (force) pthread_kill(it->second.fd, SIGUSR1);
    pthread_join(it->second.fd, nullptr);

    it->second.args;
  }
  pthread_mutex_unlock(&threads_mutex_);
  std::cout << "[CLIENT]: Exit." << std::endl;
}

void Client::delete_self(std::string uuid) {
  std::cout << "[CLIENT]: Closing task " << uuid << std::endl;
  pthread_mutex_lock(&threads_mutex_);
  if (threads_.find(uuid) == threads_.end()) {
    std::cerr << "[CLIENT]: No task found identified by " << uuid << std::endl;
    return;
  }
  pthread_join(threads_[uuid].fd, nullptr);
  delete threads_[uuid].args;
  threads_.erase(uuid);
  pthread_mutex_unlock(&threads_mutex_);
  std::cout << "[CLIENT]: Deleted task " << uuid << std::endl;
}

void Client::info() const {
  if (threads_.size()) {
    std::cout << "[CLIENT]: Active tasks:" << std::endl;
    for (auto it = threads_.begin(); it != threads_.end(); it++) {
      std::cout << it->first << (it->second.pause ? ": Paused." : ": Running.") << std::endl;
    }
  } else {
    std::cout << "[CLIENT]: No tasks running." << std::endl;
  }
}

bool Client::has_pending_tasks() const { return threads_.size(); }

ssize_t Client::recieve_encrypted(Message& message, Socket& socket, sockaddr_in& server_address) {
  AES aes = {AES::AES_256};
  Message buffer;
  ssize_t read = socket.recieve_from(buffer, server_address);
  aes.Decrypt(buffer.text.data(), MESSAGESIZE, message.text.data());
  message.chunk_size = buffer.chunk_size;
  return read;
}
