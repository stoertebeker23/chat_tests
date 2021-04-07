/**
 * @file haw_client_udp_socket_dgram.c
 * @author Lukas, s20acu642
 * @date 23.04.2020
 * @brief Chat client with udp
 */

/* 
 * Compile: siehe Makefile 
 */

/* UDPChat Client by Lukas Becker
UDP Datagram Socket chat 
Usage: ./client [username] 
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/ioctl.h> 
#include <sys/stat.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdbool.h>

#define STDIN 0
#define SERVER_PORT  8421
#define SERVER_IP "127.0.0.1"
#define BUFFER_LEN 512
#define REGISTER_CHAR "#"
#define DISC_CHAR "%"

char* username;
int sock_cli;
char ip[INET_ADDRSTRLEN];

/**
 * @brief Return current timestamp as format
 * @param void
 * @return time string
 */
char* calctime() {
	time_t t;
   	struct tm *tmp;
   	t = time(NULL);
   	tmp = localtime(&t);

	static char t_str[16];
	strftime(t_str, 16, "%Y%m%d_%H%M%S", tmp);
	return t_str;
}

/**
 * @brief Cleanup sockets after closing
 * @param void
 * @return void
 */
void cleanup() {
	/* Delete client socket file 
	 * TODO: two clients with the same name will delete the file when on client disconnects */
	close(sock_cli);
	exit(EXIT_SUCCESS);
}

/**
 * @brief Send disconnect message to server
 * @param void
 * @return void
 */
void disconnect() {
	size_t bye_len = strlen(DISC_CHAR) + strlen(username) + 1;
	char *bye = malloc(bye_len);

	snprintf(bye, bye_len, "%s%s", DISC_CHAR, username);

	/* Static initializers also zero all non-specified fields.
	 * The previous code had possible garbage in the address. */
	struct sockaddr_in address_ser = {
		.sin_family = AF_INET,
		.sin_port = htons(SERVER_PORT),
		.sin_addr.s_addr = inet_addr(ip)
	};
	memset(address_ser.sin_zero, '\0', sizeof(address_ser.sin_zero));
	socklen_t addrlen_ser = sizeof(address_ser);

	/* Send disconnect message to server */
	sendto (sock_cli, bye, strlen(bye), 0, (struct sockaddr *) &address_ser, 
addrlen_ser);
	
	printf("\n%s:UCHAT: Disconnected properly\n", calctime());
	cleanup();
	
}

/**
 * @brief Handler for Str+C to close sockets
 * @param signal
 * @return void
 */
void exit_handler(int s){
	/* Disconnect on str+c */
	disconnect();
}

/**
 * @brief Chat formatting
 * @param messagebuffer
 * @param current line
 * @return void
 */
void output_handler(const char buffer[], int line) {//, struct winsize * size) {
	
	/* A lot of shiny improvement can be done here 
	 * \033[line;rowH specifies cursor position */
	printf("\033[%d;0H%s %s\n", line-2, calctime(), buffer);
	printf("\033[%d;0H--------------------------------------------",line-1);
}

/**
 * @brief Main function, handles all communication
 * @param number of arguments
 * @param list of arguments
 * @return success state
 */
int main (int argc, char* argv[]) {
	
	struct timeval timeout;
	timeout.tv_sec = 1; 
    	timeout.tv_usec = 0; 
    	bool waiting = 1;
    	//char ip[INET_ADDRSTRLEN];
    	int nbytes, line = 4, maxfd;
    	fd_set read_fds;
    	
    	signal (SIGINT, exit_handler);
    	
	// Check if username was supplied
	if (argc < 2) {
		printf("%s:UCHAT: Please enter your username /uchat [name]\n", calctime());
		exit (EXIT_FAILURE);
	}
	if (argc == 3) {
		if (strlen(ip) > INET_ADDRSTRLEN) {
			printf("%s:UCHAT: Server IP too long\n", calctime());
			exit(EXIT_FAILURE);
		}
		strcpy(ip, argv[2]);
	} else {
		strcpy(ip, SERVER_IP);
	}
	if (strlen(argv[1]) > 50) {
		printf("%s:UCHAT: Your username can only be 50 characters long\n", calctime());
	}
	username = strdup(argv[1]);
	
	// Initialize the server socket address.
	struct sockaddr_in address_ser = {
		.sin_family = AF_INET,
		.sin_port = htons(SERVER_PORT),
		.sin_addr.s_addr = inet_addr(ip)
	};
	memset(address_ser.sin_zero, '\0', sizeof(address_ser.sin_zero));
	socklen_t addrlen_ser = sizeof(address_ser);
	
	FD_ZERO(&read_fds);
	
	// Construct message header containing name and parathenses
	
	char message_header = '+';
	printf("%s:UCHAT: Message prefix is %c\n", calctime(), message_header);
	
	// Create login message
	size_t welcome_len = strlen(REGISTER_CHAR) + strlen(argv[1]) + 1;
	char *welcome = malloc(welcome_len);
	snprintf(welcome, welcome_len, "%s%s", REGISTER_CHAR, argv[1]);

	// Create client socket.
	if((sock_cli=socket (AF_INET, SOCK_DGRAM, 0)) > 0) {
		printf("%s:UCHAT: Client socket created\n", calctime());
	}
	
	printf("\e[1;1H\e[2J");
	maxfd = (sock_cli > STDIN) ? sock_cli:STDIN;
	while(1) {
		if(waiting) {
			nbytes = sendto (sock_cli, welcome, strlen (welcome), 0, (struct sockaddr *) &address_ser, addrlen_ser);
			if (nbytes < 0) {
				printf("%s:ERROR: Server not available\n", calctime());
				cleanup();
			}
			// Only problem here, this spams the commandline if a server is not available, if one
			// is started it will end up in a mess. I can be fixed by acknowledging the connection on the client
			printf("%s:UCHAT: Reconnecting...Press Ctrl+c to exit\n",calctime());
			waiting = 1;
			
		} 
		FD_SET(sock_cli,&read_fds);
		FD_SET(0,&read_fds);
		size_t bufsize = 100;
		char *message = malloc(bufsize);

		select(maxfd+1, &read_fds, NULL, NULL, &timeout);
		
		if (FD_ISSET(0, &read_fds)) { // STDIN has information
			
			int len = getline(&message,&bufsize,stdin);
		
			/* Disconnect if a the user writes either exit or quit */
			if(len > 1 && (strncmp(message,"exit", len-1)  == 0 || strncmp(message,"quit",len-1) == 0)) disconnect();

			char *linebreak = strchr(message, '\n');
			if (linebreak) linebreak[0] = '\0';
			if (strlen(message) != 0) {
				size_t blen = 1 + strlen(message) + 1;
				char *buf = malloc(blen);
				snprintf(buf, blen, "%c%s", message_header, message);
				nbytes = sendto (sock_cli,buf,strlen(buf), 0, (struct sockaddr *) &address_ser, addrlen_ser);
				if (nbytes < 0) {
					printf("%s:ERROR: Communication to the server has failed.\n", calctime());
					cleanup();
				}
				free(buf);
			}
		}
		if (FD_ISSET(sock_cli, &read_fds)) { // Server has new information
			
			char *buffer = malloc(BUFFER_LEN);
			ssize_t nbytes = recv(sock_cli, buffer, BUFFER_LEN, 0);
			if (nbytes <= 0)
				break;
			buffer[nbytes] = '\0';
			waiting = 0;
			// React on special characters by the server
			if (strncmp(buffer,"##", strlen("##")) == 0) {
				waiting = 1;
				printf("\e[1;1H\e[2J");
				sleep(2);
				printf("%s:UCHAT: Server is full, you are waiting to be registered\n",calctime());
				continue;
			}
			if (strncmp(buffer,"--", strlen("--")) == 0) {
				printf("\n\n%s:ERROR: Server is closing, you are being disconnected!\n", calctime());
				cleanup();
			}
			output_handler(buffer, line);
			line += 1;
		}
		free(message);
  	}
	/* close socket */
	close (sock_cli); 
}
