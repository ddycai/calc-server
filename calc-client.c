#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_RESPONSE	50

struct addrinfo* get_sockaddr(const char* hostname, const char* port)
{
  struct addrinfo hints;
  struct addrinfo* results;
  int retval;

  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = AF_INET;        // Return socket addresses for the server's IPv4 addresses
  hints.ai_socktype = SOCK_STREAM;  // Return TCP socket addresses

  retval = getaddrinfo(hostname, port, &hints, &results);

  if (retval != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
    exit(EXIT_FAILURE);
  }

  return results;
}

int open_connection(struct addrinfo* addr_list)
{
  struct addrinfo* addr;
  int sockfd;

  // Iterate through each addrinfo in the list; stop when we successfully
  // connect to one
  for (addr = addr_list; addr != NULL; addr = addr->ai_next)
  {
    // Open a socket
    sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

    // Try the next address if we couldn't open a socket
    if (sockfd == -1)
      continue;

    // Stop iterating if we're able to connect to the server
    if (connect(sockfd, addr->ai_addr, addr->ai_addrlen) != -1)
      break;
  }

  // Free the memory allocated to the addrinfo list
  freeaddrinfo(addr_list);

  // If addr is NULL, we tried every addrinfo and weren't able to connect to any
  if (addr == NULL)
  {
    perror("Unable to connect");
    exit(EXIT_FAILURE);
  }
  else
  {
    return sockfd;
  }

}

int main(int argc, char** argv)
{
  int c;
  int bytes_read;      // Number of bytes read from the server
  char response[MAX_RESPONSE];   // Buffer to store received message
  char *server, *port, *expr; //Stores the arguments
  server = port = expr = NULL;

  //Parse the command line arguments
  while(1)
  {
    static struct option long_options[] = 
    {
      {"server", required_argument, 0, 's'},
      {"port", required_argument, 0, 'p'},
      {"expr", required_argument, 0, 'e'},
      {0, 0, 0, 0}
    };
    int option_index = 0;

    c = getopt_long(argc, argv, "s:p:e:", long_options, &option_index);
    if(c == -1)
      break;

    switch(c)
    {
      case 's':
        server = optarg;
        break;
      case 'p':
        port = optarg;
        break;
      case 'e':
        expr = optarg;
        break;
      case '?':
        exit(EXIT_FAILURE);
        break;
    }
  }

  if(server == NULL || port == NULL || expr == NULL)
  {
    printf("You must specify a server, port and expression.\n");
    exit(EXIT_FAILURE);
  }

  // Connect to the server
  struct addrinfo* results = get_sockaddr(server, port);
  int sockfd = open_connection(results);

	// Create the request string	
	// Allocating 3 extra spaces for \r\n\0
	char *request = malloc(strlen(expr) + 3);
	sprintf(request, "%s\r\n", expr);
	printf("Request: %s", request);

  // Send the message
  if (send(sockfd, request, strlen(request), 0) == -1)
  {
		free(request);
    perror("Unable to send");
    exit(EXIT_FAILURE);
  }
	free(request);

  // Read the reponse
  bytes_read = recv(sockfd, response, sizeof(response), 0);
  
  if (bytes_read == -1)
  {
    perror("Unable to read");
    exit(EXIT_FAILURE);
  }

	// Allocate 5 extra spaces for " Code" (plus 1 for \0)
	char *output = malloc(bytes_read + 6);
	response[bytes_read] = '\0';

	// Construct and print the output string
	strcpy(output, "Status Code");
	strcat(output, &response[6]);
  printf("%s", output);
	free(output);

  // Close the connection
  close(sockfd);

  exit(EXIT_SUCCESS);
}
