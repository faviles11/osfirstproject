# Mini-Kafka: Broker de Mensajes Distribuido en C

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

- Lenguaje obligatorio: **C estándar**
- Comunicación entre procesos: `IPC` (memoria compartida o pipes)
- Comunicación en red: uso de `Sockets`
- Persistencia: almacenamiento de mensajes en un `logfile`
- Control de concurrencia: implementación de sincronización para evitar `deadlocks`
- Cada consumidor escucha en un puerto específico (ej. `:8000`, `:8001`, etc.)

## 🕹️ Ejecución

Cada componente debe ser compilado y ejecutado por separado. Ejemplo de compilación:

```bash
gcc producer.c -o producer
gcc broker.c -o broker
gcc consumer.c -o consumer
```
