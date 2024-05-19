#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
char *response();
int main() {
  // Disable output buffering
  setbuf(stdout, NULL);

  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  printf("Logs from your program will appear here!\n");

  // Uncomment this block to pass the first stage
  //
  int server_fd, client_addr_len;
  struct sockaddr_in client_addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    printf("Socket creation failed: %s...\n", strerror(errno));
    return 1;
  }

  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) <
      0) {
    printf("SO_REUSEPORT failed: %s \n", strerror(errno));
    return 1;
  }

  struct sockaddr_in serv_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(4221),
      .sin_addr = {htonl(INADDR_ANY)},
  };

  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }

  printf("Waiting for a client to connect...\n");
  client_addr_len = sizeof(client_addr);

  int client_socket_fd =
      accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
  printf("Client connected\n");

  char buffer[BUFFER_SIZE];
  size_t recieved_buff = recv(client_socket_fd, buffer, sizeof(buffer) - 1, 0);

  if (recieved_buff < 0) {
    printf("Recieved Error while parsing client request Error: %s \n",
           strerror(errno));
    close(server_fd);
    close(client_socket_fd);
    return 1;
  }

  buffer[recieved_buff] = '\0';

  char *parse = strtok(buffer, "\n");
  char* request = strtok(parse, " ");

  request = strtok(NULL, " ");

  char *empty_response = response(request);
  send(client_socket_fd, empty_response, strlen(empty_response), 0);
  close(server_fd);

  return 0;
}

char *response(char* request_target) {
    if(strcmp(request_target, "/") == 0){ return "HTTP/1.1 200 OK\r\n\r\n";  }
    else { return "HTTP/1.1 404 Not Found\r\n\r\n"; }
    
}
