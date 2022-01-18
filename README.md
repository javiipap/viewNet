# viewNet 2.0
Este repositorio contiene todo el código relacionado con el proyecto viewNet para la asignatura de sistemas operativos de la ULL.

## Requisitos previos
Se requiere de c++20 para ejecutar este programa.

## Cómo compilar
Situado en el directorio base del repositorio ejecutar:

```$ make viewNet```

Los archivos ejecutables serán guardados en el directorio bin.

## Ejecución

```$ ./bin/viewNet```

Por el momento el programa no acepta parámetros, por lo que todo debe ser especificado a través de la CLI.
Los comandos disponibles son los siguientes:
 - server on `port`: Enciende el servidor en el puerto especificado. El cliente escucha por default en el puerto 5555, que tambien es el predefinido del servidor, por lo que se recomienda no especificar un puerto al llamar a este comando.
 - server off: Apaga el servidor en caso de estar encendido.
 - list [CLIENT]: Pide al servidor el contenido del directorio public.
 - get `filename` [CLIENT]: Pide el archivo `filename` al servidor.
 - abort `uuid` [CLIENT]: Aborta el hilo identificado por el `uuid` en el cliente y el que está enviando en el servidor.
 - pause `uuid` [CLIENT]: Pausa el hilo identificado por el `uuid` en el cliente y el que está enviando en el servidor.
 - resume `uuid` [CLIENT]: Reanuda el hilo identificado por el `uuid` en el cliente y el que está enviando en el servidor.
 - client info: Devuelve algo de información acerca de los hilos en ejecución del cliente.
 - server info: Devuelve algo de información acerca de los hilos en ejecución del servidor.
 - exit: Salir del programa.

En caso de no especificar un `uuid` en los comandos abort, pause y resume; se intentará actuar sobre el último hilo creado.

## Extras y consideraciones
Todos los mensajes enviados del servidor al cliente van cifrados con AES256 y el servidor envía algo de información adicional al pedir un archivo incluyendo el sha256 del archivo. En futuras versiones se podrá configurar la clave de encriptación.

En el servidor tanto el hilo de get_file como el de list no bloquean el mutex del unordered_map para acceder a la información del hilo. Esto es así para que en caso de cancelar o parar el hilo no haya problemas en que el mutex estuviera bloqueado y el otro hilo (pausar, abortar) no pudiera actuar. En todos los tests que he ejecutado no ha habido problemas, ya que aunque acceden de manera descontrolada a esta estructura, su comportamiento no es destructivo, cada hilo accede solo a su información dentro del unordered_map.

Se ha añadido una espera inicial en la ejecución de los hilos get por parte del servidor, esto permite copiar el uuid antes de que comience a ejecutarse.
