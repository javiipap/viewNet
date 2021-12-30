
#include "Client.h"
#include "Server.h"
#include "functions.h"

int main(int argc, char *argv[]) {
  set_signals();

  std::string user_input;

  Server server = Server();
  Client client = Client();

  std::cout << "Bienvenido a viewNet\n";
  std::cout << std::flush;
  while (true) {
    std::cout << "viewNet: ";
    std::cin >> user_input;
    if (user_input == "server_on") {
      std::cout << "Puerto de escucha: ";
      std::cin >> user_input;
      server.listen(stoi(user_input));
      client.set_up(make_ip_address(stoi(user_input)));
    } else if (user_input == "server_off") {
      server.stop();
    } else if (user_input == "get") {
      std::cout << "Nombre del fichero: ";
      std::cin >> user_input;
      client.request(server_action::get_file, user_input);
    } else if (user_input == "list") {
      client.request(server_action::list_files);
    } else if (user_input == "abort") {
      server.abort();
      client.abort();
    } else if (user_input == "pause") {
      server.pause();
    } else if (user_input == "resume") {
      server.resume();
    } else if (user_input == "exit") {
      break;
    } else {
    }
  }
  return 0;
}
