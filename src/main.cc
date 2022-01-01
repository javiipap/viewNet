
#include "Client.h"
#include "Server.h"
#include "functions.h"

void* cli(void* args) {
  std::string user_input;

  Server server = Server();
  Client client = Client();
  client.set_up(make_ip_address(5555));
  std::cout << "Bienvenido a viewNet\n";
  std::cout << std::flush;

  while (true) {
    std::cout << "viewNet $> ";

    std::cin.clear();

    getline(std::cin, user_input);
    if (errno == EINTR) {
      return nullptr;
    }

    if (starts_with(user_input, "server on")) {
      auto args = split(user_input);
      int port = 5000;
      if (args.size() > 2) {
        port = stoi(args[2]);
      }
      server.listen(port);
    } else if (user_input == "server off") {
      server.stop();
    } else if (starts_with(user_input, "get")) {
      client.request(server_action::get_file, split(user_input)[1]);
    } else if (user_input == "list") {
      client.request(server_action::list_files);
    } else if (starts_with(user_input, "abort")) {
      auto args = split(user_input);
      if (args.size() < 2) {
        std::cerr << "Falta un argumento." << std::endl;
      }
      client.abort(args[1]);
    } else if (starts_with(user_input, "pause") || starts_with(user_input, "resume")) {
      auto args = split(user_input);
      if (args.size() < 2) {
        std::cerr << "Falta un argumento." << std::endl;
      }
      client.pause_resume(args[1]);
    } else if (user_input == "info") {
      server.info();
    } else if (user_input == "exit") {
      break;
    } else {
    }
  }
  return nullptr;
}

int main_2(int argc, char* argv[]) {
  // auto cli_thread = pthread_t();
  // set_signals();

  // pthread_create(&cli_thread, nullptr, cli, nullptr);

  // int cli_output = 0;

  // auto error_handler = [](void* args) -> void* {
  //   sigset_t set;
  //   sigaddset(&set, SIGINT);
  //   sigaddset(&set, SIGTERM);
  //   sigaddset(&set, SIGHUP);
  //   int sig;
  //   bool interruption = false;
  //   sigprocmask(SIG_BLOCK, &set, nullptr);

  //   while (true) {
  //     sigwait(&set, &sig);
  //     if (sig == SIGINT && !interruption) {
  //       if (interruption) return nullptr;
  //       std::cout << "Presiona Ctrl + c de nuevo para salir." << std::endl;
  //       interruption = true;
  //     } else {
  //       break;
  //     }
  //   }

  //   pthread_kill(cli_thread, SIGUSR1);
  //   return nullptr;
  // };

  // auto error_handler_thread = pthread_t();
  // pthread_create(&error_handler_thread, nullptr, error_handler, nullptr);
  // pthread_detach(error_handler_thread);

  // pthread_join(cli_thread, nullptr);
  return 0;
}

int main() {
  sigset_t set;
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGHUP);
  pthread_sigmask(SIG_BLOCK, &set, nullptr);

  struct sigaction sigterm_action = {0};

  sigterm_action.sa_flags = SA_SIGINFO;
  sigterm_action.sa_sigaction = &sigusr_handler;

  sigaction(SIGUSR1, &sigterm_action, nullptr);
  sigaction(SIGUSR2, &sigterm_action, nullptr);

  std::string user_input;

  Server server = Server();
  Client client = Client();
  client.set_up(make_ip_address(5555));
  std::cout << "Bienvenido a viewNet" << std::endl;

  while (true) {
    std::cout << "viewNet $> ";

    std::cin.clear();

    getline(std::cin, user_input);
    if (errno == EINTR) {
      return 1;
    }

    if (starts_with(user_input, "server on")) {
      auto args = split(user_input);
      int port = 5555;
      if (args.size() > 2) {
        port = stoi(args[2]);
      }
      server.listen(port);
    } else if (user_input == "server off") {
      server.stop();
    } else if (starts_with(user_input, "get")) {
      client.request(server_action::get_file, split(user_input)[1]);
    } else if (user_input == "list") {
      client.request(server_action::list_files);
    } else if (starts_with(user_input, "abort")) {
      auto args = split(user_input);
      if (args.size() < 2) {
        std::cerr << "Falta un argumento." << std::endl;
      }
      client.abort(args[1]);
    } else if (starts_with(user_input, "pause") || starts_with(user_input, "resume")) {
      auto args = split(user_input);
      if (args.size() < 2) {
        std::cerr << "Falta un argumento." << std::endl;
      }
      client.pause_resume(args[1]);
    } else if (user_input == "info") {
      server.info();
    } else if (user_input == "exit") {
      break;
    } else if (user_input == "client info") {
      client.info();
    } else if (user_input != "") {
      std::cout << "Comando no reconocido." << std::endl;
    }
  }

  return 0;
}
