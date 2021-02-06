CC=g++
CFLAGS=-Wall -o

all: server client

server: 
	$(CC) $(CFLAGS) Server Server.cpp

client:
	$(CC) $(CFLAGS) Client Client.cpp
