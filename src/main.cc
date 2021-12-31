
#include "Client.h"
#include "Server.h"
#include "functions.h"

int main(int argc, char *argv[]) {
  set_signals();

  std::string user_input;

  Server server = Server();
  Client client = Client();
  client.set_up(make_ip_address(5555));
  std::cout << "Bienvenido a viewNet\n";
  std::cout << std::flush;

  while (true) {
    std::cout << "viewNet $> ";

    std::cin.clear();  // Clear flags

    getline(std::cin, user_input);
    if (errno == EINTR) {
      return 0;
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
    } else if (user_input == "abort") {
      // client.abort();
    } else if (user_input == "pause") {
      server.pause();
    } else if (user_input == "resume") {
      server.resume();
    } else if (user_input == "info") {
      server.info();
    } else if (user_input == "exit") {
      break;
    } else {
    }
  }
  return 0;
}
