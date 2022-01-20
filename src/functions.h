/**
 * @author Javier Padilla Pío
 * @date 22/12/2021
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en ingeniería informática
 * Curso: 2º
 * Practice de programación: viewNet
 * Email: alu0101410463@ull.edu.es
 * functions.h: Definiciones de funciones de ayuda.
 * Revision history: 22/12/2021 -
 *                   Creación (primera versión) del código
 */

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#define WITH_PARAM_ACTION 0x80

#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include "Message.h"

extern pthread_t main_thread;

enum server_action {
  unknown = 0,
  list_files,
  get_file = WITH_PARAM_ACTION | 0,
  abortar = WITH_PARAM_ACTION | 1,
  pausar = WITH_PARAM_ACTION | 2,
  resumir = WITH_PARAM_ACTION | 3,
  retrieve_uuid = WITH_PARAM_ACTION | 4,
  close_connection = WITH_PARAM_ACTION | 5,
  exec_cmd = WITH_PARAM_ACTION | 6
};

const struct option longopts[] = {
    {"help", no_argument, 0, 'h'},
    {"client", required_argument, 0, 'c'},
    {"server", required_argument, 0, 's'},
    {"port", required_argument, 0, 'p'},
    {0, 0, 0, 0},
};

struct CLI_arguments_parser {
  uint32_t server_port = 5555;
  uint32_t listen_port = 5555;
  std::string server_ip = "0.0.0.0";
  bool show_help = false;
  bool server_mode = false;
  bool valid = true;

  CLI_arguments_parser(int argc, char* argv[]);
};

struct cli_args {
  int argc;
  char** argv;
};

struct cli_output {
  int err;
};

/**
 * @brief Dado un puerto y una ip se genera una estructura de tipo sockaddr_in.
 * @param[in] port Puerto de la estructura.
 * @param[in] ip_address Dirección de la estructura.
 * @return Estructura generada.
 */
sockaddr_in make_ip_address(int port, const std::string& ip_address = std::string());

/**
 * @brief Revisa si una cadena de texto tiene un cierto prefijo.
 * @param[in] haystack Cadena de texto.
 * @param[in] neeldle Prefijo a comprobar.
 */
bool starts_with(const std::string haystack, const std::string needle);

/**
 * @brief Codifica una acción a un buffer de tipo mensaje.
 * @param[out] buffer Buffer en donde guardar la acción.
 * @param[in] action Acción a codificar.
 * @param[in] param Posible parámetro a codificar junto con la acción.
 */
void EncodeAction(Message& buffer, server_action action, const std::string param = "");

/**
 * @brief Decodifica una acción de un buffer de tipo mensaje.
 * @param[in] buffer Buffer de entrada.
 * @param[out] param Posible parámetro decodificado.
 * @return Acción decodificada.
 */
server_action DecodeAction(const Message& buffer, std::string* param = nullptr);

/**
 * @brief Genera un uuid.
 */
std::string generate_uuid();

/**
 * @brief Divide una cadena de entrada por espacios.
 * @param[in] string Cadena de texto a dividir.
 * @return Vector con los elementos divididos.
 */
std::vector<std::string> split(const std::string& s);

/**
 * @brief Manejador de las señales SIGUSR1 y SIGUSR2.
 * @param[in] signo Identificador de la señal.
 * @param[in] info Información del estado.
 * @param[out] context Contexto general de la aplicación.
 */
void sigusr_handler(int signo, siginfo_t* info, void* context);

/**
 * @brief Fuerza la salida del programa parando el hilo principal.
 * @param[in] args nullptr.
 */
void* exit_handler(void* args);

#endif
