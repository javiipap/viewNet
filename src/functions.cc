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

std::string generate_uuid() {
  static std::random_device dev;
  static std::mt19937 rng(dev());

  std::uniform_int_distribution<int> dist(0, 15);

  const char* v = "0123456789abcdef";
  const bool dash[] = {0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};

  std::string res;
  for (int i = 0; i < 16; i++) {
    if (dash[i]) res += "-";
    res += v[dist(rng)];
    res += v[dist(rng)];
  }
  return res;
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

void sigusr_handler(int signo, siginfo_t* info, void* context) {
  switch (signo) {
    case SIGUSR1:
      info->si_errno = EINTR;
      return;
    case SIGUSR2:
      pause();
      info->si_errno = EINTR;
      return;
  }
}

void* exit_handler(void* args) {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGHUP);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGINT);
  int signum;
  sigwait(&set, &signum);
  std::cout << "\nExit forced..." << std::endl;
  pthread_kill(main_thread, SIGUSR1);
  return nullptr;
}
