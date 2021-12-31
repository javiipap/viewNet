#include "Client.h"

Client::Client(){};

Client::~Client() { delete_internal_threads(); };

void Client::set_up(sockaddr_in server_address) { server_address_ = server_address; }

void Client::request(server_action action, std::string param) {
  threads_.push_back({pthread_t(), GenerateUid(), "", nullptr});
  threads_.back().args = new thread_args{action, param, threads_.back(), this};
  pthread_create(&threads_.back().fd, nullptr, internal_handler, threads_.back().args);
}

void Client::abort(std::string uuid) { delete_self(uuid); };

void Client::pause(){};

void Client::resume(){};

void* Client::internal_handler(void* args) {
  const auto [action, param, self, instance] = *static_cast<thread_args*>(args);
  Socket socket(make_ip_address(0));
  Message buffer;
  EncodeAction(buffer, action, param);
  socket.send_to(buffer, instance->server_address_);

  sockaddr_in worker_addr;
  socket.recieve_from(buffer, worker_addr);
  DecodeAction(buffer, &self.server_task_uuid);

  bool end_found = false;
  while (!end_found) {
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

  return nullptr;
}

void Client::delete_internal_threads() {
  pthread_mutex_lock(&threads_mutex_);
  for (int i = threads_.size() - 1; i >= 0; i--) {
    pthread_join(threads_[i].fd, nullptr);

    delete static_cast<thread_args*>(threads_[i].args);
  }
  pthread_mutex_unlock(&threads_mutex_);
}

void Client::delete_self(std::string uuid) {
  pthread_mutex_lock(&threads_mutex_);
  for (int i = 0; i < threads_.size(); i++) {
    if (threads_[i].uuid == uuid) {
      delete static_cast<thread_args*>(threads_[i].args);
    }
    threads_.erase(threads_.begin() + i);
    break;
  }
  pthread_mutex_unlock(&threads_mutex_);
}
