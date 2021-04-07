# UDP Socket Chat server

## Build

Compile with make. Resulting binaries are client.bin and server.bin.
Attention, every warning is treated as an error, to change this, remove Werror flag.

## Makefile

Alle files are first compiled and linked in a second step, if in the future more code files
are supplied for one binary.

Following compiler flags are used:
- Wall: Enable all warnings
- Werror: Treat every warning as an error
- std=c99: Compile with c99 standard
- D _POSIX_C_SOURCE: Define on the 2008 september version
- g: mandatory for use with gdb
- o: Outputfile
- c: Compile
- lpthread: Compile against pthread.h

## Run Server

Server need a number of accepted clients which is specified in the argument. It doesnt run without
the argument. Run with ./server.bin [NUMBER]. You can also choose to run the server in debug mode which gives you much more text output.
This can help you to understand the protocol. Add "-d" to the program call such like ./server.bin 2 -d.

## Run Client

Start client by supplying a user name. The client connects to the server socket by sending a login 
message. If there is free space on the server, the client gets a suceed message. After 
successfully connecting, you can start sending messages. Logoff by typing exit, quit or hitting 
ctrl+c. Run with ./client.bin [NAME]. You can also specify a server IP address such as ./client.bin [NAME] [IP]. If no ip is specified, localhoste is used.
