#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <syslog.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "parser.h"

#define BACKLOG 25
#define MAX_RESPONSE 50
#define MAX_REQUEST 80

#define MAX_LENGTH_EXCEEDED 4
#define MALFORMED_REQ 5

struct addrinfo* get_server_sockaddr(const char* port)
{
  struct addrinfo hints;
  struct addrinfo* results;
  int retval;

  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = AF_INET;        // Return socket addresses for our local IPv4 addresses
  hints.ai_socktype = SOCK_STREAM;  // Return TCP socket addresses
  hints.ai_flags = AI_PASSIVE;      // Socket addresses should be listening sockets

  retval = getaddrinfo(NULL, port, &hints, &results);

  if (retval != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
    exit(EXIT_FAILURE);
  }

  return results;
}

int bind_socket(struct addrinfo* addr_list)
{
  struct addrinfo* addr;
  int sockfd;
  char yes = '1';

  // Iterate through each addrinfo in the list; stop when we successfully bind
  // to one
  for (addr = addr_list; addr != NULL; addr = addr->ai_next)
  {
    // Open a socket
    sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

    // Try the next address if we couldn't open a socket
    if (sockfd == -1)
      continue;

    // Allow the port to be re-used if currently in the TIME_WAIT state
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
      perror("Unable to set socket option");
      exit(EXIT_FAILURE);
    }

    // Try to bind the socket to the address/port
    if (bind(sockfd, addr->ai_addr, addr->ai_addrlen) == -1)
    {
      // If binding fails, close the socket, and try the next address
      close(sockfd);
      continue;
    }
    else
    {
      // Otherwise, we've bound the address/port to the socket, so stop
      // processing
      break;
    }
  }

  // Free the memory allocated to the addrinfo list
  freeaddrinfo(addr_list);

  // If addr is NULL, we tried every addrinfo and weren't able to bind to any
  if (addr == NULL)
  {
    perror("Unable to bind");
    exit(EXIT_FAILURE);
  }
  else
  {
    // Otherwise, return the socket descriptor
    return sockfd;
  }

}

int wait_for_connection(int sockfd)
{
  struct sockaddr_in client_addr;            // Remote IP that is connecting to us
  int addr_len = sizeof(struct sockaddr_in); // Length of the remote IP structure
  char ip_address[INET_ADDRSTRLEN];          // Buffer to store human-friendly IP address
  int connectionfd;                          // Socket file descriptor for the new connection

  // Wait for a new connection
  connectionfd = accept(sockfd, (struct sockaddr*)&client_addr, &addr_len);

  // Make sure the connection was established successfully
  if (connectionfd == -1)
  {
    perror("Unable to accept connection");
    exit(EXIT_FAILURE);
  }

  // Convert the connecting IP to a human-friendly form and print it
  inet_ntop(client_addr.sin_family, &client_addr.sin_addr, ip_address, sizeof(ip_address));
  syslog(LOG_INFO, "Request received from client %s", ip_address);

  // Return the socket file descriptor for the new connection
  return connectionfd;
}

/*
 * Converts a status code to a string representation
 *
 * code: the status code
 *
 * return: string representation of the status code
 */
char * status_code_to_str(int code)
{
	switch(code)
	{
		case MALFORMED_REQ:
			return "malformed-req";
		case MAX_LENGTH_EXCEEDED:
			return "max-length-exceeded";
		case MISMATCH:
			return "mismatch";
		case INVALID_EXPR:
			return "invalid-expr";
		case OK:
			return "ok";
		default:
			printf("Invalid status code.\n");
			exit(EXIT_FAILURE);
	}
}

/*
 * Reads the CTP request from the client and processes it
 * Sends a response to the client with the result
 */
void handle_connection(int connectionfd)
{
	int status_code = OK;
  char request[MAX_REQUEST];		// Stores the request from client
	char response[MAX_RESPONSE];	// Stores the response to client
	int result;					// Stores the result of parsing
  int bytes_read;

  // Read up to 80 bytes from the client
  bytes_read = recv(connectionfd, request, sizeof(request), 0);  

  // If the data was read successfully
  if (bytes_read > 0)
  {
		// If we read 80 bytes, must end on newline
		// If we read 2 bytes, then expression is empty
		if(bytes_read == MAX_REQUEST && request[MAX_REQUEST  - 1] != '\n')
		{
			status_code = MAX_LENGTH_EXCEEDED;
		}
		// Otherwise, must end with \r\n and be non-empty
		else if(bytes_read == 2 || request[bytes_read - 1] != '\n' || request[bytes_read - 2] != '\r')
		{
			status_code = MALFORMED_REQ;
		}

    // Add a terminating NULL character to indicate end of string
    request[bytes_read - 2] = '\0';
    syslog(LOG_INFO, "Expression was: %s", request);

		// If the status code is OK so far, parse the expression
		if(status_code == OK)
		{
			status_code = parse_expr(request, &result);
		}

		// Parse succesful, construct OK response
		if(status_code == OK)
		{
			sprintf(response, "Status: ok\r\nResult: %d\r\n", result);	
			syslog(LOG_INFO, "Status: ok, result: %d", result);
		// Parse error, construct error code response
		}
		else
		{
			sprintf(response, "Status: %s\r\n", status_code_to_str(status_code));
			syslog(LOG_INFO, "Status: %s", status_code_to_str(status_code));
		}

		// Send response to client
		if (send(connectionfd, response, strlen(response), 0) == -1)
		{
			perror("Unable to send to socket");
			exit(EXIT_FAILURE);
		}

  }
  else if (bytes_read == -1)
  {
    // Otherwise, if the read failed,
    perror("Unable to read from socket");
    exit(EXIT_FAILURE);
  }

  // Close the connection
  close(connectionfd);
}

int main(int argc, char** argv)
{

	openlog("calc-server", LOG_PERROR | LOG_PID | LOG_NDELAY, LOG_USER);
	setlogmask(LOG_UPTO(LOG_INFO));

  int c;
	static int debug_flag = 0;
  char * port = NULL;	// Stores the port number

  // Parse the command line arguments
  while(1)
  {
    static struct option long_options[] = 
    {
      {"port", required_argument, 0, 'p'},
			{"debug", no_argument, &debug_flag, 1},
      {0, 0, 0, 0}
    };
    int option_index = 0;

    c = getopt_long(argc, argv, "dp:", long_options, &option_index);
    if(c == -1)
      break;

    switch(c)
    {
      case 'p':
        port = optarg;
        break;
			case 'd':
				debug_flag = 1;
				break;
      case '?':
        exit(EXIT_FAILURE);
        break;
    }
  }

	// Set debug mode
	if(debug_flag)
		setlogmask(LOG_UPTO(LOG_DEBUG));

  // Argument passed should be port number
  if(port == NULL)
  {
    printf("Must specify port number with -p or --port.\n");
    exit(EXIT_FAILURE);
  }

  struct addrinfo* results = get_server_sockaddr(port);

  // Create a listening socket
  int sockfd = bind_socket(results);

  // Start listening on the socket
  if (listen(sockfd, BACKLOG) == -1)
  {
    perror("Unable to listen on socket");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    // Wait for a connection and handle it
    int connectionfd = wait_for_connection(sockfd);
    handle_connection(connectionfd);
  }

	closelog();
  exit(EXIT_SUCCESS);
}

