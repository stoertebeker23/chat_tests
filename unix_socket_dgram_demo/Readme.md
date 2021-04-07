# UDP Unixsocket Chat server

## Build

Compile with make. Resulting binaries are uchat.bin and uchat_ser.bin.
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
the argument. Run with ./uchat_ser.bin <NUMBER>

## Run Client

Start client by supplying a user name. The client connects to the server socket by sending a login 
message. If there is free space on the server, the client gets a suceed message. After 
successfully connecting, you can start sending messages. Logoff by typing exit, quit or hitting 
ctrl+c. Run with ./uchat.bin <NAME>.
