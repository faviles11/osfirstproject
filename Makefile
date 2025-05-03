CC = gcc
CFLAGS = -Wall -pthread

all: broker producer consumer

broker: src/broker.c
	$(CC) $(CFLAGS) src/broker.c -o broker

producer: src/producer.c
	$(CC) $(CFLAGS) src/producer.c -o producer

consumer: src/consumer.c
	$(CC) $(CFLAGS) src/consumer.c -o consumer

clean:
	rm -f broker producer consumer
