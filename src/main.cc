/**
 * @author Javier Padilla Pío
 * @date 22/12/2021
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en ingeniería informática
 * Curso: 2º
 * Practice de programación: viewNet
 * Email: alu0101410463@ull.edu.es
 * main.cc: Punto de entrada de la aplicación.
 * Revision history: 22/12/2021 -
 *                   Creación (primera versión) del código
 */

#include "Client.h"
#include "Server.h"
#include "functions.h"

pthread_t main_thread = pthread_t();

void* cli(void* args) {
  std::string last_thread = "";
  main_output* output = new main_output{0};

  struct sigaction sigterm_action = {0};

  sigterm_action.sa_flags = SA_SIGINFO;
  sigterm_action.sa_sigaction = &sigusr_handler;

  sigaction(SIGUSR1, &sigterm_action, nullptr);
  sigaction(SIGUSR2, &sigterm_action, nullptr);

  std::string user_input;

  Server server = Server();
  Client client = Client();
  client.set_up(make_ip_address(5555));
  std::cout << "Welcome to viewNet" << std::endl;

  while (true) {
    std::cout << "viewNet $> ";

    std::cin.clear();

    getline(std::cin, user_input);
    if (errno == EINTR) {
      client.stop();
      return output;
    }

    if (starts_with(user_input, "server on")) {
      auto args = split(user_input);
      int port = 5555;
      if (args.size() > 2) {
        port = stoi(args[2]);
      }
      if (port < 1024) {
        std::cerr << "[SERVER]: You require sudo privileges to use this port." << std::endl;
        continue;
      }
      server.listen(port);
    } else if (user_input == "server off") {
      server.stop();
    } else if (starts_with(user_input, "get")) {
      last_thread = client.request(server_action::get_file, split(user_input)[1]);
    } else if (user_input == "list") {
      last_thread = client.request(server_action::list_files);
    } else if (starts_with(user_input, "abort")) {
      auto args = split(user_input);

      if (args.size() < 2 && last_thread == "") {
        std::cerr << "Missing argument." << std::endl;
      } else if (args.size() < 2) {
        client.abort(last_thread);
        last_thread = "";
      } else {
        client.abort(args[1]);
      }
    } else if (starts_with(user_input, "pause")) {
      auto args = split(user_input);

      if (args.size() < 2 && last_thread == "") {
        std::cerr << "Missing argument." << std::endl;
      } else if (args.size() < 2) {
        client.pause(last_thread);
      } else {
        client.pause(args[1]);
      }
    } else if (starts_with(user_input, "resume")) {
      auto args = split(user_input);

      if (args.size() < 2 && last_thread == "") {
        std::cerr << "Missing argument." << std::endl;
      } else if (args.size() < 2) {
        client.resume(last_thread);
      } else {
        client.resume(args[1]);
      }
    } else if (starts_with(user_input, "set client port")) {
      auto args = split(user_input);
      if (args.size() < 4) {
        std::cerr << "" << std::endl;
      } else {
        if (stoi(args[3]) < 1024) {
          std::cerr << "[CLIENT]: You require sudo privileges to use this port." << std::endl;
          continue;
        }
        client.set_up(make_ip_address(stoi(args[3])));
        std::cout << "[CLIENT]: Port changed successfully." << std::endl;
      }
    } else if (user_input == "server info") {
      server.info();
    } else if (user_input == "client info") {
      client.info();
    } else if (starts_with(user_input, "/run")) {
      std::string uuid = client.request(server_action::exec_cmd, user_input.substr(5));
      client.wait(uuid);
    } else if (user_input == "exit") {
      if (client.has_pending_tasks()) {
        std::cout << "There are pending tasks, Force? [y/n]: ";
        std::cin >> user_input;

        if (user_input == "y") {
          client.stop();
        } else {
          std::cout << "Tasks will be waited." << std::endl;
        }
      }
      break;
    } else if (user_input != "") {
      std::cout << "Command not recognized. More information on available commands in README.md"
                << std::endl;
    }
  }

  return output;
}

int main() {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGHUP);
  pthread_sigmask(SIG_BLOCK, &set, nullptr);

  pthread_create(&main_thread, nullptr, &cli, nullptr);

  pthread_t exit_handler_thread = pthread_t();
  pthread_create(&exit_handler_thread, nullptr, &exit_handler, nullptr);
  pthread_detach(exit_handler_thread);

  pthread_join(main_thread, nullptr);
}
