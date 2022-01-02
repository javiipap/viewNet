
#include "Client.h"
#include "Server.h"
#include "functions.h"

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
    } else if (starts_with(user_input, "pause")) {
      auto args = split(user_input);
      if (args.size() < 2) {
        std::cerr << "Falta un argumento." << std::endl;
      }
      client.pause(args[1]);
    } else if (starts_with(user_input, "resume")) {
      auto args = split(user_input);
      if (args.size() < 2) {
        std::cerr << "Falta un argumento." << std::endl;
      }
      client.resume(args[1]);
    } else if (user_input == "server info") {
      server.info();
    } else if (user_input == "client info") {
      client.info();
    } else if (user_input == "exit") {
      if (client.has_pending_tasks()) {
        std::cout << "Hay tareas pendientes, ¿Forzar el cierre? [y/n]: ";
        std::cin >> user_input;

        if (user_input == "y") {
          client.stop();
        } else {
          std::cout << "Se esperará a su finalización antes del cierre." << std::endl;
        }
      }
      break;
    } else if (user_input != "") {
      std::cout << "Comando no reconocido." << std::endl;
    }
  }

  return 0;
}
