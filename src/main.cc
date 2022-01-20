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
  const auto [argc, argv] = *static_cast<cli_args*>(args);
  cli_output* output = new cli_output{0};

  CLI_arguments_parser parsed_args(argc, argv);

  if (!parsed_args.valid) {
    output->err = 1;
    std::cerr << "[PANIC]: Error parsing arguments." << std::endl;
    return output;
  }

  if (parsed_args.show_help) {
    std::cout
        << "Bienvenido a viewNet, un programa para intercambiar archivos entre dos ordenadores.\n"
           "Los comandos disponibles son los siguientes:\n"
           "\t- server on `port`: Enciende el servidor en el puerto especificado. El cliente "
           "escucha por default en el puerto 5555, que tambien es el predefinido del servidor, por "
           "lo que se recomienda no especificar un puerto al llamar a este comando.\n"
           "\t- server off: Apaga el servidor en caso de estar encendido.\n"
           "\t- list [CLIENT]: Pide al servidor el contenido del directorio public.\n"
           "\t- get `filename` [CLIENT]: Pide el archivo `filename` al servidor.\n"
           "\t- abort `uuid` [CLIENT]: Aborta el hilo identificado por el `uuid` en el cliente y "
           "el que está enviando en el servidor.\n"
           "\t- pause `uuid` [CLIENT]: Pausa el hilo identificado por el `uuid` en el cliente y el "
           "que está enviando en el servidor.\n"
           "\t- resume `uuid` [CLIENT]: Reanuda el hilo identificado por el `uuid` en el cliente y "
           "el que está enviando en el servidor.\n"
           "\t- client info: Devuelve algo de información acerca de los hilos en ejecución del "
           "cliente.\n"
           "\t- server info: Devuelve algo de información acerca de los hilos en ejecución del "
           "servidor.\n"
           "\t- stats: Imprime la información de transferencia cliente-servidor.\n"
           "\t- /run `command arg1 arg2...`: Ejecuta el comando dado en el servidor.\n"
           "\t- exit: Salir del programa.\n"
           "\n"
           "En caso de no especificar un `uuid` en los comandos abort, pause y resume, se "
           "intentará actuar sobre el último hilo creado."
        << std::endl;
    return output;
  }

  std::string last_thread = "";

  Server server = Server();
  Client client = Client();
  client.set_up(make_ip_address(parsed_args.server_port, parsed_args.server_ip));
  if (parsed_args.server_mode) server.listen(parsed_args.listen_port);

  std::cout << "Welcome to viewNet" << std::endl;

  std::string user_input;
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
      int port = args.size() > 2 ? stoi(args[2]) : parsed_args.listen_port;

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
    } else if (user_input == "stats") {
      client.stats();
      server.stats();
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

int main(int argc, char** argv) {
  // Bloquear señales.
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGHUP);
  pthread_sigmask(SIG_BLOCK, &set, nullptr);

  // Añadir manejador para señales de usuario.
  struct sigaction sigterm_action = {0};
  sigterm_action.sa_flags = SA_SIGINFO;
  sigterm_action.sa_sigaction = &sigusr_handler;
  sigaction(SIGUSR1, &sigterm_action, nullptr);
  sigaction(SIGUSR2, &sigterm_action, nullptr);

  // Hilo principal
  cli_args args = {argc, argv};
  pthread_create(&main_thread, nullptr, &cli, &args);

  // Hilo para manejar errores
  pthread_t exit_handler_thread = pthread_t();
  pthread_create(&exit_handler_thread, nullptr, &exit_handler, nullptr);
  pthread_detach(exit_handler_thread);

  pthread_join(main_thread, nullptr);
}
