CC = gcc
REM = rm
CFLAGS = -std=c99 -Wall -Werror -D _POSIX_C_SOURCE=200809L


all: uchat.bin uchat_server.bin

uchat.bin: uchat.o
	$(CC) -g -o uchat.bin uchat.o -lpthread

uchat_server.bin: uchat_ser.o
	$(CC) -g -o uchat_server.bin uchat_ser.o

uchat.o: haw_client_unix_socket_dgram.c
	$(CC) $(CFLAGS) -c -g -o uchat.o haw_client_unix_socket_dgram.c

uchat_ser.o: haw_server_unix_socket_dgram.c
	$(CC) $(CFLAGS) -c -g -o uchat_ser.o haw_server_unix_socket_dgram.c

clean:
	$(REM) -f *.o *.bin
