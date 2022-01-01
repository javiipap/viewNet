#include "Client.h"

Client::Client(){};

Client::~Client() {
  if (threads_.size() > 0) delete_internal_threads();
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

void Client::pause_resume(std::string uuid) {
  if (threads_.find(uuid) == threads_.end()) {
    std::cerr << "[CLIENT]: No thread found identified by " << uuid << std::endl;
    return;
  }
  std::cout << "[CLIENT]: Pausing/resuming task" << std::endl;
  Socket socket(make_ip_address(0));
  Message msg;
  EncodeAction(msg, server_action::pause_resume, threads_[uuid].server_task_uuid);
  socket.send_to(msg, server_address_);
  pthread_kill(threads_[uuid].fd, SIGUSR2);
};

void* Client::internal_handler(void* args) {
  const auto [action, param, uuid, instance] = *static_cast<thread_args*>(args);

  try {
    std::cout << "[CLIENT]: Starting task " << uuid << std::endl;
    Socket socket(make_ip_address(0));
    Message buffer;
    EncodeAction(buffer, action, param);
    socket.send_to(buffer, instance->server_address_);

    sockaddr_in worker_addr;
    socket.recieve_from(buffer, worker_addr);
    DecodeAction(buffer, &instance->threads_[uuid].server_task_uuid);

    bool end_found = false;
    while (!end_found && !instance->threads_[uuid].stop) {
      socket.recieve_from(buffer, worker_addr);

      for (int i = 0; i < buffer.chunk_size; i++) {
        std::cout << buffer.text[i];

        if (buffer.text[i] == 0) {
          end_found = true;
          std::cout << std::endl;
          break;
        }
      }
    }
  } catch (std::exception& err) {
    std::cerr << "viewNet [CLIENT]: " << err.what() << std::endl;
  }

  instance->delete_self(uuid);
  return nullptr;
}

void Client::delete_internal_threads() {
  std::cout << "[CLIENT]: Closing threads..." << std::endl;

  pthread_mutex_lock(&threads_mutex_);
  for (auto it = threads_.begin(); it != threads_.end(); it++) {
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

void Client::info() {
  std::cout << "Active tasks:" << std::endl;
  for (auto it = threads_.begin(); it != threads_.end(); it++) {
    std::cout << it->first << ": Running." << std::endl;
  }
}
