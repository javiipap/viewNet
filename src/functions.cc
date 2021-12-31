#include "functions.h"

sockaddr_in make_ip_address(int port, const std::string& ip_address) {
  in_addr address;
  if (ip_address.length())
    inet_aton(ip_address.c_str(), &address);
  else
    address.s_addr = htonl(INADDR_ANY);

  return {AF_INET, htons(port), address};
}

std::string extract_string(Message message) {
  std::string result;
  for (const auto ch : message.text) {
    result += ch;
    if (ch == '\0') break;
  }

  if (result[result.length() - 1] != '\0') {
    result += '\0';
  }

  return result;
}

void EncodeAction(Message& buffer, server_action action, const std::string param) {
  for (int i = 0; i < 6; i++) {
    buffer.text[i] = 0;
  }
  buffer.text[6] = action;
  if (action ^ WITH_PARAM_ACTION < WITH_PARAM_ACTION) {
    memcpy(buffer.text.data() + 7, param.c_str(), param.size() + 1);
  }
}

server_action DecodeAction(const Message& buffer, std::string* param) {
  for (int i = 0; i < 6; i++) {
    if (buffer.text[i] == 0) {
      continue;
    }
    throw std::invalid_argument("El mensaje está mal codificado");
  }

  int action = buffer.text[6];  // Admite 2^8 acciones.
  server_action casted_action;

  try {
    casted_action = static_cast<server_action>(action);
    if (casted_action ^ WITH_PARAM_ACTION < WITH_PARAM_ACTION) {
      for (int i = 7; buffer.text[i] != 0; i++) {
        *param += buffer.text[i];
      }
    }
  } catch (...) {
    throw std::invalid_argument("Acción no soportada.");
  }

  return casted_action;
}

bool starts_with(std::string haystack, std::string needle) {
  if (needle.size() > haystack.size()) return false;

  for (int i = 0; i < needle.size(); i++) {
    if (haystack[i] != needle[i]) {
      return false;
    }
  }
  return true;
}

std::string GenerateUid() {
  std::string uuid;
  uuid.resize(UUID_LENGTH);
  srand(time(NULL));
  snprintf(uuid.data(), uuid.size(), "%x%x-%x-%x-%x-%x%x%x", rand(), rand(), rand(),
           ((rand() & 0x0fff) | 0x4000), rand() % 0x3fff + 0x8000, rand(), rand(), rand());

  return uuid;
}

std::vector<std::string> split(const std::string& s) {
  std::stringstream ss(s);
  std::vector<std::string> words;

  while (!ss.eof()) {
    std::string word;
    ss >> word;
    words.push_back(word);
  }

  return words;
}

void sigusr_1_handler(int signo, siginfo_t* info, void* context) { info->si_errno = EINTR; }

void sigusr_2_handler(int signo, siginfo_t* info, void* context) {
  if (info->si_errno == 128) {
    info->si_errno = 0;
    return;
  };
  info->si_errno = 128;
  pause();
}

void quit_handler(int signo, siginfo_t* info, void* context) {
  std::cout << "hola " << std::endl;
  info->si_errno = EINTR;
}

void* signal_handler(void* args) {
  struct sigaction quit_action = {0};

  quit_action.sa_sigaction = &quit_handler;
  quit_action.sa_flags = 0;

  sigaction(SIGINT, &quit_action, nullptr);
  sigaction(SIGTERM, &quit_action, nullptr);
  sigaction(SIGHUP, &quit_action, nullptr);

  return nullptr;
}

void set_signals() {
  pthread_t signal_handler_thread = pthread_t();
  pthread_create(&signal_handler_thread, nullptr, signal_handler, nullptr);
  pthread_detach(signal_handler_thread);

  sigset_t set;
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGHUP);
  // pthread_sigmask(SIG_BLOCK, &set, nullptr);

  struct sigaction sigterm_action = {0};

  sigterm_action.sa_flags = SA_SIGINFO;
  sigterm_action.sa_sigaction = &sigusr_1_handler;

  sigaction(SIGUSR1, &sigterm_action, nullptr);

  struct sigaction sigusr_action = {0};

  sigterm_action.sa_flags = 0;
  sigterm_action.sa_sigaction = &sigusr_2_handler;

  sigaction(SIGUSR2, &sigusr_action, nullptr);
}
