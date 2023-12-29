CC = gcc
CFLAGS = -Wall -I./includes

all: ./bin/server ./bin/client

bin/server: bin/server.o bin/utils.o bin/main.o
	gcc $^ -o $@

./bin/client: ./bin/client.o ./bin/utils.o
	$(CC) $^ -o $@

./bin/utils.o: ./src/utils.c ./includes/utils.h
	$(CC) -c $(CFLAGS) $< -o $@

./bin/server.o: ./src/server.c ./includes/server.h
	$(CC) -c $(CFLAGS) $< -o $@

./bin/client.o: ./src/client.c
	$(CC) -c $(CFLAGS) $< -o $@

./bin/main.o: ./src/main.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f ./bin/*
