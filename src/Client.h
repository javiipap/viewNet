/**
 * @author Javier Padilla Pío
 * @date 22/12/2021
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en ingeniería informática
 * Curso: 2º
 * Practice de programación: viewNet
 * Email: alu0101410463@ull.edu.es
 * Client.h: Interfaz de la clase Client. Se encarga de hacer peticiones al servidor.
 * Revision history: 22/12/2021 -
 *                   Creación (primera versión) del código
 */

#include <atomic>
#include <unordered_map>

#include "AES.h"
#include "Message.h"
#include "Socket.h"
#include "functions.h"

#ifndef CLIENT_H_
#define CLIENT_H_
class Client {
 public:
  Client();
  ~Client();

  /**
   * @brief Inicializa la dirección del servidor.
   * @param[in] server_address Dirección del servidor.
   */
  void set_up(sockaddr_in server_address);

  /**
   * @brief Inicializa un hilo para atender a una orden del usuario.
   * @param[in] action Tipo del hilo a crear.
   * @param[in] param Posible parámetro enviado por el usuario.
   * @return uuid del hilo inicializado.
   */
  std::string request(server_action action, std::string param = "");

  /**
   * @brief Envía una señal al servidor para cancelar el envío de paquetes y luego cancela el hilo
   *        que los estaba recibiendo.
   * @param[in] uuid Identificador del hilo de recepción.
   */
  void abort(std::string uuid);

  /**
   * @brief Envía una señal al servidor para pausar el envío de paquetes y luego para el hilo que
   *        los estaba recibiendo.
   * @param[in] uuid Identificador del hilo de recepción.
   */
  void pause(std::string uuid);

  /**
   * @brief Resume el hilo parado y luego envía una señal al servidor para reanudar el envío de
   *        paquetes.
   * @param[in] uui Identificador del hilo de recepción.
   */
  void resume(std::string uuid);

  /**
   * @brief Imprime por pantalla el estado de ejecución de los hilos existentes.
   */
  void info() const;

  /**
   * @brief Retorna verdadero si hay hilos ejecutando o en espera.
   */
  bool has_pending_tasks() const;

  /**
   * @brief Cancela todos los hilos de recepción.
   */
  void stop();

 private:
  struct thread_args {
    server_action action;
    std::string param;
    std::string uuid;
    Client* instance;
  };

  struct thread_info {
    pthread_t fd;
    std::string server_task_uuid;
    std::atomic<bool> stop = false;
    std::atomic<bool> pause = false;
    thread_args* args = nullptr;
  };

  sockaddr_in server_address_;
  std::unordered_map<std::string, thread_info> threads_;
  pthread_mutex_t threads_mutex_ = pthread_mutex_t();

  /**
   * @brief Hilo encargado de hacer peticiones al servidor y mostrar la respuesta por pantalla.
   * @param[in] args Puntero a una estructura de tipo thread_args.
   * @return nullptr
   */
  static void* internal_handler(void* args);

  /**
   * @brief Desencripta un mensaje enviado por el servidor.
   * @param[out] message Estructura donde guardar el mensaje desencriptado.
   * @param[in] socket Socket que recibirá el mensaje.
   * @param[out] server_address Estructura en la que guardar la dirección del remitente.
   * @return Bytes recibidos.
   */
  static ssize_t recieve_encrypted(Message& message, Socket& socket, sockaddr_in& server_address);

  /**
   * @brief Elimina todos los registros de un hilo liberando los recursos ocupados. Se llama justo
   *        antes del retorno de un hilo.
   * @param[in] uuid Identificador del hilo a limpiar.
   */
  void delete_self(std::string uuid);

  /**
   * @brief Elimina todos los hilos.
   * @param[in] force En caso de ser true aborta todos los hilos incluso los que estén en espera.
   */
  void delete_internal_threads(bool force = false);
};
#endif
