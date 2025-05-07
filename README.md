# Mini-Kafka: Broker de Mensajes Distribuido en C - Proyecto de Sistemas Operativos

## Descripción del Sistema

Este proyecto implementa una versión simplificada de un sistema de mensajería distribuido inspirado en **Apache Kafka**. El sistema está diseñado para ilustrar conceptos clave de sistemas operativos como:

- Comunicación entre procesos usando **sockets TCP**.
- **Concurrencia y sincronización** con hilos, mutexes y semáforos.
- Manejo de múltiples productores y consumidores de manera simultánea.

### Componentes del sistema:

- **Broker**: Servidor central que recibe, almacena y distribuye mensajes.
- **Producer**: Cliente que se conecta al broker para enviar mensajes.
- **Consumer**: Cliente que se conecta al broker para recibir mensajes según su grupo.


## Introducción y Descripción del Sistema
Este proyecto implementa una versión de un sistema de mensajería distribuido inspirado en la arquitectura de Apache Kafka. Su propósito es ilustrar conceptos clave de comunicación entre procesos, concurrencia y manejo de recursos.

Este proyecto implementa un sistema distribuido de mensajería inspirado en Apache Kafka, utilizando **C estándar** y conceptos como la **comunicación entre procesos (IPC)**, **sincronización**, y **sockets**.

## 📌 Descripción General

El sistema consiste en varios procesos:

- **Productores (Producers)**: generan y envían mensajes.
- **Broker**: recibe mensajes, los almacena y los distribuye.
- **Consumidores (Consumers)**: reciben mensajes desde el broker.

El objetivo es simular un flujo de mensajes de múltiples productores hacia múltiples consumidores, administrado por un broker central.

## 🧩 Componentes del Sistema

- `producer.c`: envía mensajes al broker.
- `broker.c`: recibe los mensajes, los escribe en un archivo de log y los reenvía a los consumidores.
- `consumer.c`: se conecta al broker para recibir los mensajes.

## 🔧 Requisitos Técnicos

- **Sistema operativo**: Linux 
- **Compilador**: GCC estándar
- **Herramientas de construcción**: `make`
- **Conexión TCP local o en red**
- **Persistencia**: Almacenamiento de mensajes en un archivo `messages.log`
- **Sincronización**: Uso de `mutexes` y `semáforos` para evitar condiciones de carrera.

## 🕹️ Ejecución del sistema

### 🏗 Compilación
Para compilar el proyecto, se utiliza el siguiente comando:
```bash
make
```
## 🔵 Iniciar el Broker
- Cada componente debe ser compilado y ejecutado por separado. 

```bash
./broker
```
- El broker iniciará escuchando conexiones entrantes en el puerto 12345 y mostrará un mensaje indicando que está en funcionamiento.

## 🟢 Iniciar los Productores
```bash
./producer 127.0.0.1 12345
```
Envía mensajes al broker, que luego serán distribuidos a los consumidores.

## 🔴 Iniciar los Consumidores
```bash
./consumer 127.0.0.1 12345
```
Los consumidores se conectan al broker y se registran automáticamente en uno de los grupos (GroupA, GroupB, GroupC). Luego recibirán mensajes a medida que el broker los distribuya.

# 🔄 Comunicación entre procesos (IPC)

El sistema utiliza sockets TCP/IP para comunicar productores y consumidores con el broker, lo que simula una red local.

# 🔁 Concurrencia y sincronización

El broker maneja múltiples conexiones usando hilos (pthread), donde cada conexión se atiende en un hilo separado.

**Para evitar condiciones de carrera:

-Se utilizan mutexes (pthread_mutex_t) para proteger estructuras compartidas como la cola de mensajes.

-Se usa un pool de hilos preinicializado para reutilizar recursos y evitar la sobrecarga de crear y destruir hilos frecuentemente.

### 🔒 Estrategia para evitar interbloqueos (Deadlocks)

**Para evitar interbloqueos, el sistema sigue las siguientes prácticas:

1️⃣ Uso de pthread_mutex_t para proteger secciones críticas

Se utilizan mutexes en la cola de mensajes (queue_mutex) y en el archivo de log (log_mutex), asegurando que cada recurso sea accedido de manera segura.

```bash

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

```

2️⃣  Uso de sem_t para sincronización de consumidores

Se emplea un semáforo (consumers_semaphore) para controlar el acceso a la lista de consumidores, evitando que múltiples hilos intenten modificar la lista al mismo tiempo.

```bash

sem_t consumers_semaphore;

```
3️⃣ Sincronización en la distribución de mensajes

Para evitar interbloqueos en la selección de consumidores, primero se bloquea el acceso a los grupos, y luego se busca un consumidor disponible.

```bash

pthread_mutex_lock(&groups_mutex); // 🔒 Bloqueo para asegurar acceso ordenado

GroupPointer* current = groups_head;
while (current != NULL) {
    if (strcmp(current->group_name, group_name) == 0) {
        pthread_mutex_unlock(&groups_mutex); // 🔓 Libera el lock del grupo
        return current;
    }
    current = current->next;
}

pthread_mutex_unlock(&groups_mutex); // 🔓 Asegura que el lock siempre se libera


```

🧪 Módulos principales



**broker.c: Gestiona conexiones, sincronización y almacenamiento.

**producer.c: Envía mensajes al broker.

**consumer.c: Recibe mensajes desde el broker.


Makefile: automatiza la compilación.

⚠️ Limitaciones

No hay gestión de múltiples particiones ni replicación.

Una interfaz para cambiar dinámicamente configuraciones del broker.

7️⃣ Estructura del Repositorio

```bash

osfirstproject
├── src/
│   ├── broker.c       # Lógica del broker
│   ├── producer.c     # Cliente productor
│   ├── consumer.c     # Cliente consumidor
├── README.md         # Documentación del proyecto
├── Makefile          # Para compilar en C
├── messages.log      # Archivo de persistencia de mensajes

```

## 📚 Referencias

- **Kafka: The Definitive Guide** – Gwen Shapira, Neha Narkhede, Todd Palino.  
 
- **Apache Kafka Architecture Video (YouTube)**.  
 


