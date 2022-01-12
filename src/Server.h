/**
 * @author Javier Padilla Pío
 * @date 22/12/2021
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en ingeniería informática
 * Curso: 2º
 * Practice de programación: viewNet
 * Email: alu0101410463@ull.edu.es
 * Server.h: Interfaz de la clase Server. Se encarga de servir archivos a un cliente.
 * Revision history: 22/12/2021 -
 *                   Creación (primera versión) del código
 */

#ifndef SERVER_H_
#define SERVER_H_
#include <pthread.h>
#include <signal.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <utility>

#include "AES.h"
#include "File.h"
#include "Socket.h"
#include "dirent.h"
#include "functions.h"
#include "sha256.h"

class Server {
 public:
  Server();

  ~Server();

  /**
   * @brief Incia el servidor.
   * @param[in] port Puerto en el que escuchar.
   */
  void listen(int port);

  /**
   * @brief Para el servidor enviando un SIGUSR1 al hilo principal. Esto hará que se llame a
   *        delete_internal_threads esperando a cada hilo.
   */
  void stop();

  /**
   * @brief Imprime por pantalla el estado de ejecución de los hilos existentes.
   */
  void info() const;

  /**
   * @brief Retorna verdadero si hay hilos ejecutando o en espera.
   */
  bool has_pending_tasks() const;

 private:
  struct thread_args {
    std::string param;
    sockaddr_in client_addr;
    std::string uuid;
    Server* instance;
  };

  struct thread_info {
    pthread_t fd;
    server_action type;
    std::atomic<bool> stop = false;
    std::atomic<bool> pause = false;
    void* args = nullptr;
  };

  std::unordered_map<std::string, thread_info> threads_;
  pthread_mutex_t threads_mutex_ = pthread_mutex_t();
  int port_ = -1;
  thread_info main_thread_;
  std::unordered_map<std::string, std::string> files_sha256_;

  /**
   * @brief Computa los hashes de todos los archivos en el directorio public/ y los guarda en
   *        files_sha_256_.
   */
  void store_hashes();

  /**
   * @brief Hilo principal del servidor encargado de eschuchar a través de port_ las peticiones de
   *        los clientes y crear hilos dinámicamente para atender a las mismas.
   * @param[in] args Puntero a una instancia inicializada de la clase.
   * @return nullptr
   */
  static void* main_thread(void* args);

  /**
   * @brief Hilo encargado de leer un archivo del directorio public y enviarlo al cliente. El
   *        archivo a enviar viene especificado en thread_args (param).
   * @param[in] args Puntero a una estructura de tipo thread_args.
   * @return nullptr
   */
  static void* get_file(void* args);

  /**
   * @brief Hilo encargado de enviar al cliente un listado con todos los archivos en el directorio
   *        public.
   * @param[in] args Puntero a una estructura de tipo thread_args.
   * @return nullptr
   */
  static void* list(void* args);

  /**
   * @brief Hilo encargado de pausar un hilo de tipo get_file o list. Envía una señal de tipo
   *        SIGUSR2 al hilo, lo que inicia una espera a otra señal. Además se actualiza la
   *        información de estado del hilo almacenada en threads_ para que así otros hilos puedan
   *        saber que este está detenido (threads_[uuid].pause = true).
   * @param[in] args Puntero a una estructura de tipo thread_args.
   * @return nullptr
   */
  static void* pause(void* args);

  /**
   * @brief Hilo encargado de resumir un hilo de tipo get_file o list. Envía una señal de tipo
   *        SIGUSR1 para cancelar la espera a una señal iniciada en pause. Además se reinicia el
   *        valor de threads_[uuid].pause a false.
   * @param[in] args Puntero a una estructura de tipo thread_args.
   * @return nullptr
   */
  static void* resume(void* args);

  /**
   * @brief Envía una señal a un cliente para abortar la recepción. Este método es llamado en caso
   *        de error en los hilos get_file y list.
   * @param[in] instance Instancia del servidor.
   * @param[in] client_address Dirección del cliente a notificar.
   * @param[in] error Información acerca del error cometido.
   */
  static void abort_client(Server* instance, sockaddr_in client_address, std::string error);

  /**
   * @brief Elimina todos los registros de un hilo liberando los recursos ocupados. Se llama justo
   *        antes del retorno de un hilo.
   * @param[in] uuid Identificador del hilo a limpiar.
   */
  void delete_self(std::string uuid);

  /**
   * @brief Elimina todos los hilos del servidor.
   * @param[in] force En caso de ser true aborta todos los hilos incluso los que estén en espera.
   */
  void delete_internal_threads(bool force = false);

  /**
   * @brief Encripta un mensaje usando AES y lo envía al cliente.
   * @param[out] msg Mensaje para enviar.
   * @param[in] socket Socket a través del cual enviar el mensaje.
   * @param[in] client_addr Dirección del cliente.
   */
  static ssize_t send_encrypted(Message& msg, Socket& socket, const sockaddr_in client_addr);
};
#endif
