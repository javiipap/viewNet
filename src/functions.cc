#include "functions.h"

sockaddr_in make_ip_address(int port, const std::string &ip_address) {
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

std::string DecodeAction() { return ""; }

std::string GenerateUid() {
  char uid[61];
  srand(time(NULL));
  snprintf(uid, sizeof(uid), "%x%x-%x-%x-%x-%x%x%x", rand(), rand(), rand(),
           ((rand() & 0x0fff) | 0x4000), rand() % 0x3fff + 0x8000, rand(), rand(), rand());

  return uid;
}

void sigusr_1_handler(int signo, siginfo_t *info, void *context) { info->si_errno = EINTR; }

void set_signals() {
  sigset_t set;
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGHUP);
  pthread_sigmask(SIG_BLOCK, &set, nullptr);

  struct sigaction sigusr_1_action = {0};

  sigusr_1_action.sa_flags = SA_SIGINFO;
  sigusr_1_action.sa_sigaction = &sigusr_1_handler;

  sigaction(SIGUSR1, &sigusr_1_action, nullptr);
}
