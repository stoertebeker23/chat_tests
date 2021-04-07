/**
 * @file haw_server_udp_socket_dgram.c
 * @author Lukas, s20acu642
 * @date 23.04.2020
 * @brief Chat Server with udp
 */

/* 
 * Compile: siehe Makefile 
 */

/* UDPChat Server by Lukas Becker
Udp Datagram Socket chat server
Usage: ./uchat_ser <num clients> (-d Debug)
*/

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>

#define SERVER_PORT  8421
#define SERVER_IP "127.0.0.1"
#define BUFFER_LEN 4096
#define REGISTER_CHAR '#' /* Character to identify a new client */
#define DISC_CHAR '%' /* Character to identify a disconnection */
#define CLOSING_MSG "--" /* Character send to clients on server termination */

struct Client {
	struct sockaddr_in data;
	char name[51];
};

bool debug = 0;
struct Client *clients;
socklen_t clientlen;
int sock, n_clients;

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
	printf("%s:SERVER: Sucessfully closed server\n", calctime());
	close(sock);
	exit(EXIT_SUCCESS);
}

/**
 * @brief Handler for Str+C to close sockets
 * @param signal
 * @return void
 */
void exit_handler(int s){
	char ip_str[INET_ADDRSTRLEN];
	for (int i = 0; i < n_clients; i++) {
		if (clients[i].data.sin_family != AF_INET)
			continue;
		sendto(
			sock, 
			CLOSING_MSG, 
			strlen(CLOSING_MSG), 
			0, 
			(struct sockaddr*)&clients[i].data, 
			clientlen
			);
		inet_ntop(AF_INET, &clients[i].data.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN);
		printf("%s:SERVER: Sending disconnect message to %d of %d possible clients. Target client: %s\n", 
						calctime(), i+1, n_clients, ip_str);
	}
	cleanup();
}

/**
 * @brief Get Index of client in client list
 * @param socket address of client which sent the message
 * @return index or -1 if not present
 */
int get_client_index(struct sockaddr_in *received_client) {
	for (int i = 0; i < n_clients; i++) {
		if (clients[i].data.sin_family != AF_INET) continue;
		if (debug) printf("%s:DEBUG: Comparing Ports: %d, %d\n", calctime(), received_client->sin_port, clients[i].data.sin_port);
		if (debug) printf("%s:DEBUG: Comparing IPs: %d, %d\n", calctime(), received_client->sin_addr.s_addr, clients[i].data.sin_addr.s_addr);
		if (received_client->sin_addr.s_addr == clients[i].data.sin_addr.s_addr 
			&& received_client->sin_port == clients[i].data.sin_port) {
				if (debug) printf("%s:DEBUG: Client at index %d\n",calctime(), i);
				return i;
		}
	}
	return -1;
}

/**
 * @brief Main function, handles all communication
 * @param number of arguments
 * @param list of arguments
 * @return success state
 */
int main (int argc, char* argv[]) {

	char ip_str[INET_ADDRSTRLEN];
	char *message;
	if (argc < 2) {
		printf("%s:ERROR: Please enter client number %s <NUMBER> (-d Debug)\n", calctime(), argv[0]);
		exit (EXIT_FAILURE);
	} else if (argc > 3) {
		printf("%s:ERROR: Too many arguments submitted\n", calctime());
	}
	if (argc == 3) {
		if (strcmp(argv[2],"-d") == 0) {
			debug = 1;
			printf("%s:DEBUG: Debug mode enabled\n", calctime());
		}
	}
	// Signal handler for str+c
	signal (SIGINT, exit_handler);
		
	n_clients = atoi(argv[1]);
	printf("%s:SERVER: %d-clients server started\n", calctime(), n_clients);
	ssize_t nbytes;

	// Temporary client address for saving receiver of incoming messages
	struct sockaddr_in cliaddress;
	socklen_t cliaddrlen = sizeof(cliaddress);
	// Overwrite address
	memset(&cliaddress,0,cliaddrlen);
	
	// Server IP
	struct sockaddr_in address = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(SERVER_PORT)
	};
	memset(address.sin_zero, '\0', sizeof(address.sin_zero));
	socklen_t addrlen = sizeof(address);\

	// allocate clients
	clients = calloc(sizeof(struct Client), n_clients);
	clientlen = sizeof(clients[0].data);

	sock = socket (AF_INET, SOCK_DGRAM, 0);
	
	if ( bind(sock, (struct sockaddr *) &address, addrlen) < 0) {
		printf("%s:ERROR: Socket port in use, cant bind\n",calctime());
		cleanup();
	} 
	inet_ntop(AF_INET, &address.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN);
	printf("%s:SERVER: Binding to socket succeeded %s\n", calctime(), ip_str);

	char *buffer = malloc(BUFFER_LEN);
	/* TODO: start receival and message ping in extra thread, so the console still works
	 * this is nice for kicking clients server side oder sending messages to all clients */
	while (1) {

		memset(&cliaddress,0,cliaddrlen);
		
		nbytes = recvfrom(sock, buffer, BUFFER_LEN, MSG_WAITALL, (struct sockaddr *) &cliaddress, &cliaddrlen);
		// Print sender information if debug is on
		inet_ntop(AF_INET, &cliaddress.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN);
		if(debug) printf("%s:DEBUG: Sender information %d, %d, %s, %d\n", calctime(), cliaddress.sin_family, cliaddress.sin_port, ip_str, cliaddrlen);
		
		if (nbytes < 0) {
		  exit (EXIT_FAILURE);
		}

		buffer[nbytes] = '\0';
		printf ("%s:SERVER: Got message: \"%s\", length = %zd\n", calctime(), buffer, nbytes);
		if (buffer[0] == '#') {
			char *cli = malloc (100);
			snprintf(cli, 100, "%s", buffer+1);

			printf("%s:SERVER: New client [%s] registering...\n", calctime(), cli);

			// client is already registred TOFIX: other chat windows dies
			if (get_client_index(&cliaddress) >= 0) continue;

			for (int i = 0; i <= n_clients; i++) {
				/* write reject message if server is full */
				if (i == n_clients) {
					const char *reject = "##";
					// Send reject message if server is full
					sendto(
						sock,
						reject,
						strlen(reject),
						0,
						(struct sockaddr*) &cliaddress,
						cliaddrlen
						);
					break;
				}
				// determine an empty slot by checking family
				if (clients[i].data.sin_family != AF_INET) {
					if(debug) printf("%s:DEBUG: Empty slot for client available at index %d\n", calctime(), i);
					
					// Copy temp client information to client list
					clients[i].data.sin_family = AF_INET;
					clients[i].data.sin_addr.s_addr = cliaddress.sin_addr.s_addr;
					clients[i].data.sin_port = cliaddress.sin_port;
					strcpy(clients[i].name, cli);
					const char *connected = "[SERVER] Successfully registered to the server";
					/* send connect message to connecting client */
					sendto(
						sock, 
						connected, 
						strlen(connected), 
						0, 
						(struct sockaddr *) &cliaddress,
						cliaddrlen
						);
					/* Sending connect message to all clients except the registring client */
					for(int j = 0; j < n_clients; j ++ ) {
						if (j == i) continue;
						
						char *joined = malloc(strlen("[SERVER] \"") + strlen(clients[i].name) + strlen("\" joined the server") + 1);
						strcpy(joined, "[SERVER] \"");
						strcat(joined, clients[i].name);
						strcat(joined, "\" joined the server");
						sendto(
							sock, 
							joined, 
							strlen(joined), 
							0, 
							(struct sockaddr*)&clients[j].data, 
							clientlen
							);
						free(joined);
						
					}
					printf("%s:SERVER: Client %s succesfully registered to the server\n",calctime(), clients[i].name);
					break;
				} else {
					inet_ntop(AF_INET, &cliaddress.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN);
					if(debug) printf("%s:DEBUG: Registered client nr. %d is at address %s | %d\n", calctime(), i, ip_str, AF_INET);
				} 
			}
			free(cli);
		} else if (buffer[0] == DISC_CHAR) {
			int pos = get_client_index(&cliaddress);
			if(pos < 0) {
				if(debug) printf("%s:DEBUG: Unregistred client tried to disconnect\n", calctime());
				continue;
			}
			inet_ntop(AF_INET, &clients[pos].data.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN);
			printf("%s:SERVER: Client %s with IP %s:%d successfully disconnected\n", calctime(), clients[pos].name, ip_str, ntohs(clients[pos].data.sin_port));
			/* Set family to unspecified and the path to to \0 if a client disconnects"  */
			clients[pos].data.sin_family = AF_UNSPEC;
			clients[pos].data.sin_addr.s_addr = 0;
			clients[pos].data.sin_port = 0;
			/* Send disconnect message to every user */
			for(int j = 0; j < n_clients; j++) {
				if (clients[j].data.sin_family != AF_INET)
					continue;
				inet_ntop(AF_INET, &clients[j].data.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN);
				ssize_t stlen = strlen("[SERVER] \"") + strlen(clients[pos].name) 
+ strlen("\" disconnected from the server") + 1;
				/* construct disconnect message */
				char *disc = malloc(stlen);
				snprintf(
					disc, 
					stlen, 
					"%s%s",
					"[SERVER] \"",
					clients[pos].name
					);
				/* send info message to all clients */
				strcat(disc,"\" disconnected from the server");
				sendto(
					sock, 
					disc, 
					strlen(disc), 
					0, 
					(struct sockaddr*)&clients[j].data, 
					clientlen
					);
				free(disc);
			}
		} else if (buffer[0] == '+') {
			
			
			// if no special character is detected, send the message to every client if the sender is registred
			int pos = get_client_index(&cliaddress);
			if (pos < 0) continue;
           		message = calloc(sizeof(char),BUFFER_LEN + 50 + 3);
			printf("%s:SERVER: Chat Message: \"%s\"\n", calctime(), buffer+1);
			snprintf(message, BUFFER_LEN, "[%s] %s", clients[pos].name,buffer+1);
			
			if (strlen(buffer)) {
				for (int i = 0; i < n_clients; i++) {
					if (clients[i].data.sin_family != AF_INET) continue;
					sendto(
						sock, 
						message, 
						strlen(message), 
						0, 
						(struct sockaddr*)&clients[i].data, 
						clientlen
						);
					inet_ntop(AF_INET, &clients[i].data.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN);
					if(debug) printf("%s:DEBUG: Sending message to %d of %d possible clients. Target IP: %s:%d: Message \"%s\"\n", 
						calctime(), i+1, n_clients, ip_str,ntohs(clients[i].data.sin_port), message);
				}
			}
		}
	}
	close (sock);
	cleanup();
	return 0;
}

