# Makefile
SHELL := /bin/bash
CC = gcc

$(info make all: Genereazione dell'eseguibile del client e del server)
$(info make test1: esegue test1)
$(info make test2: esegue test2)
$(info make test3: esegue test3)
$(info make clean: Elimina elimina file .o ed eseguibili)
$(info make cleanScript: Elimina tutti i file prodotti dai test)
$(info make cleanAll: Esegue clean e cleanScript)
$(info )


# Flag comipilaotre
CFLAGS = -Wall -std=c99 -pthread -Wno-missing-braces -D_DEFAULT_SOURCE 


# directory dove memorizzare gli eseguibili
BUILD = build

# file .o del servere e del client
OBJSERVER = obj/server.o obj/serverAPI.o obj/sortedList.o obj/queue.o obj/cacheFile.o obj/queueFile.o obj/queueChar.o obj/readers_writers_lock.o obj/icl_hash.o obj/log.o obj/configParser.o obj/utilsPathname.o
OBJCLIENT = obj/client.o obj/clientApi.o obj/commandLine_parser.o obj/queueChar.o obj/utilsPathname.o

# percorso file oggetti
OBJ = obj
# percorso file .c
SRC = src

.PHONY: all test1 test2 test3 clean cleanScript cleanAll

all: server client

server: $(SRC)/$< $(OBJSERVER)
	$(CC) $(CFLAGS) -o $(BUILD)/$@ $(OBJSERVER)

client: $(SRC)/$< $(OBJCLIENT)
	$(CC) $(CFLAGS) -o $(BUILD)/$@ $(OBJCLIENT)

# Crea il file .o per ogni della directory SRC
$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS)-c $< -o $@

#Crea i file .o per le strutture dati
obj/icl_hash.o: strutture_dati/hash/icl_hash.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/sortedList.o: strutture_dati/sortedList/sortedList.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/queue.o: strutture_dati/queueInt/queue.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/queueFile.o: strutture_dati/queueFile/queueFile.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/queueChar.o: strutture_dati/queueChar/queueChar.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/readers_writers_lock.o: strutture_dati/readers_writers_lock/readers_writers_lock.c
	$(CC) $(CFLAGS) -c $< -o $@


test1: server client
	@chmod +x ./test/test1/test1.sh
	@chmod +x ./script/statistiche.sh
	
	./test/test1/test1.sh
	./script/statistiche.sh test/test1/log/log.txt

test2: server client
	@chmod +x ./test/test2/test2.sh
	@chmod +x ./script/statistiche.sh
	
	./test/test2/test2.sh
	./script/statistiche.sh test/test2/log/log.txt

test3: server client
	@chmod +x ./test/test3/test3.sh
	@chmod +x ./script/statistiche.sh

	./test/test3/test3.sh
	./script/statistiche.sh test/test3/log/log.txt


clean:
	rm -f *~ $(OBJ)/*.o $(BUILD)/*

cleanScript:
	rm -f test/test1/cacheDestination/* test/test1/readDestination/* test/test1/log/*
	rm -f test/test2/cacheDestination/* test/test2/readDestination/* test/test2/log/*
	rm -f test/test3/cacheDestination/* test/test3/readDestination/* test/test3/log/*

cleanAll: clean cleanScript