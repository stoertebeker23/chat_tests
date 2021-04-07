/**
 * @file haw_client_unix_socket_dgram.c
 * @author Lukas, s20acu642
 * @date 23.04.2020
 * @brief Chat client with unix dgram socket
 */

/* 
 * Compile: siehe Makefile 
 */

/* UChat Client by Lukas Becker
UNIX Datagram Socket chat 
Usage: ./uchat [username] 
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/ioctl.h> 
#include <sys/stat.h>
#include <time.h>

#define SERVER_SOCKET_FILE_PATH  "/tmp/uchat_ser"
#define CLIENT_SOCKET_FILE_BASEPATH  "/tmp/uchat_cli"
#define BUFFER_LEN 512
#define REGISTER_CHAR "#"
#define DISC_CHAR "%"

char* username;
int sock_cli;

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
	char *cli = (char*)malloc (strlen(CLIENT_SOCKET_FILE_BASEPATH) -1 + strlen(username));
	strcpy(cli,CLIENT_SOCKET_FILE_BASEPATH);
	strcat(cli,username);
	printf("%s:UCHAT: Clearing up returned %d\n", calctime(), remove(cli));
	free(cli);
	exit(EXIT_SUCCESS);
}

/**
 * @brief Disconnect propely from server and fomr sockets
 * @param void
 * @return void
 */
void disconnect() {
	size_t bye_len = strlen(DISC_CHAR) + strlen(username) + 1;
	char *bye = malloc(bye_len);

	snprintf(bye, bye_len, "%s%s", DISC_CHAR, username);

	/* Static initializers also zero all non-specified fields.
	 * The previous code had possible garbage in the address. */
	struct sockaddr_un address_ser = {
		.sun_family = AF_LOCAL,
		.sun_path = SERVER_SOCKET_FILE_PATH
	};
	socklen_t addrlen_ser = sizeof(address_ser);

	/* Send disconnect message to server */
	sendto (sock_cli, bye, strlen(bye), 0, (struct sockaddr *) &address_ser, addrlen_ser);
	printf("%s:UCHAT: Disconnected properly\n", calctime());
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
 * @param linenumber
 * @return void
 */
void output_handler(const char buffer[], int line) {//, struct winsize * size) {
	
	/* A lot of shiny improvement can be done here 
	 * \033[line;rowH specifies cursor position */
	printf("\033[%d;0H|%s| -  %s\n", line, calctime(), buffer);
	printf("\033[%d;0H--------------------------------------------",line+2);//, size->ws_row, size->ws_col);
	printf("\033[%d;0H| Write:",line+3);
}

/**
 * @brief Thread to receive messages nonblocking
 * @param threadargs
 * @return void*
 */
void *receiver_thread(void* threadargs) {

	char *buffer = malloc(BUFFER_LEN);
	int line = 2;

	//struct winsize size;
	
	
	while(1) {
		/* Get currenwindow size */
		// ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		ssize_t nbytes = recv(sock_cli, buffer, BUFFER_LEN, 0);
		if (nbytes <= 0)
			break;
		buffer[nbytes] = '\0';
		
		if (strncmp(buffer,"##", strlen("##")) == 0) {
			printf("%s:ERROR: Server is full, try again later!\n", calctime());
			cleanup();
		}
		output_handler(buffer, line);//,&size);
		line += 1;
	}
	/* TODO: Disconnect on server full */
	return NULL;
}

/**
 * @brief Main function, handles all communication
 * @param number of arguments
 * @param list of arguments
 * @return success state
 */
int main (int argc, char* argv[]) {

	// Check if username was supplied
	if (argc != 2) {
		printf("%s:UCHAT: Please enter your username /uchat [name]", calctime());
		exit (EXIT_FAILURE);
	}
	username = strdup(argv[1]);
	signal (SIGINT, exit_handler);
	
	int nbytes;
	struct sockaddr_un address_cli;
	socklen_t addrlen_cli = sizeof(address_cli);

	// Create socket path
	char *cli = (char*)malloc (strlen(CLIENT_SOCKET_FILE_BASEPATH) -1 + strlen(argv[1]));
	strcat(cli,CLIENT_SOCKET_FILE_BASEPATH);
	strcat(cli,argv[1]);
	printf("%s:UCHAT: Your socketfile is %s\n", calctime(), cli);

	// Construct message header containing name and parathenses
	size_t message_header_len = strlen(argv[1]) + 4;
	char *message_header = malloc(message_header_len);
	snprintf(message_header, message_header_len, "[%s] ", argv[1]);
	printf("%s:UCHAT: Message prefix is %s\n", calctime(), message_header);
	
	// Create login message
	size_t welcome_len = strlen(REGISTER_CHAR) + strlen(argv[1]) + 1;
	char *welcome = malloc(welcome_len);
	snprintf(welcome, welcome_len, "%s%s", REGISTER_CHAR, argv[1]);

	// Create client socket.
	if((sock_cli=socket (AF_LOCAL, SOCK_DGRAM, 0)) > 0) {
		printf("%s:UCHAT: Client socket created\n", calctime());
	}
	// Unline existing previous files;
	unlink(cli);
	
  	// Bind client socket to socket file
        address_cli.sun_family = AF_LOCAL;
	strcpy(address_cli.sun_path, cli);
	if ( bind(sock_cli, (struct sockaddr *) &address_cli, addrlen_cli) != 0) {
		printf("%s:ERROR: Socket port in use, cant bind\n", calctime());
		exit(EXIT_FAILURE);
	}
	printf("%s:UCHAT: Binding to socket file succeeded.\n", calctime());
	
	/* Set socket file to 666 which means rw for group/user/everyone */
	char mode[] ="0777";
	int mod;
	mod = strtol(mode,0,8);
	int retval;
	retval = chmod(cli,mod);
	if(retval < 0) {
    		printf("%s:ERROR: A problem occured setting the socket permissions correctly: %d\n", calctime(), retval);
    		exit(EXIT_FAILURE);
	}
	printf("%s:UCHAT: Setting permissions for socket file to %s\n", calctime(), mode);
	
  	// Initialize the server socket address.
	struct sockaddr_un address_ser = {
		.sun_family = AF_LOCAL,
		.sun_path = SERVER_SOCKET_FILE_PATH
	};
	socklen_t addrlen_ser = sizeof(address_ser);

  	// Send register message
	nbytes = sendto (sock_cli, welcome, strlen (welcome), 0, (struct sockaddr *) (struct sockaddr*)&address_ser, addrlen_ser);
	if (nbytes < 0) {
		printf("%s:ERROR: Server not available\n", calctime());
		cleanup();
	}
	// Init asynchronous receive thread
	pthread_t thread_id;

	// Spawn thread
	pthread_create(&thread_id, NULL, receiver_thread, NULL);

	printf("\e[1;1H\e[2J");

	while(1) {
		size_t bufsize = 100;
		char *message = malloc(bufsize);

		/* Getline from the user */
		int len = getline(&message,&bufsize,stdin);
		
		/* Disconnect if a the user writes either exit or quit */
		if(len > 1 && (strncmp(message,"exit", len-1)  == 0 || strncmp(message,"quit",len-1) == 0)) disconnect();

		char *linebreak = strchr(message, '\n');
		if (linebreak)
			linebreak[0] = '\0';

		// Send to server
		if (strlen(message) != 0) {
			size_t blen = strlen(message_header) + strlen(message) + 1;
			char *buf = malloc(blen);
			snprintf(buf, blen, "%s%s", message_header, message);
			nbytes = sendto (sock_cli,buf,strlen(buf), 0, (struct sockaddr *) &address_ser, addrlen_ser);
			if (nbytes < 0) {
				printf("%s:ERROR: Communication to the server has failed.\n", calctime());
				cleanup();
			}
			free(buf);
		}
		
		// free buffers
		free(message);
		
  	}
	/* close socket */
	close (sock_cli); 
	/* remove socket file, the program should actually never get here */
	unlink(cli);
		
}
