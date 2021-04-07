CC = gcc
REM = rm
CFLAGS = -std=c99 -Wall -Werror -D _POSIX_C_SOURCE=200809L


all: client.bin server.bin

client.bin: udpchat.o
	$(CC) -g -o client.bin udpchat.o -lpthread

server.bin: udpchat_ser.o
	$(CC) -g -o server.bin udpchat_ser.o

udpchat.o: haw_client_udp_socket_dgram.c
	$(CC) $(CFLAGS) -c -g -o udpchat.o haw_client_udp_socket_dgram.c

udpchat_ser.o: haw_server_udp_socket_dgram.c
	$(CC) $(CFLAGS) -c -g -o udpchat_ser.o haw_server_udp_socket_dgram.c

clean:
	$(REM) -f *.o *.bin
