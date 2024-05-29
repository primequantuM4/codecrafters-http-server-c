#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

char *response(char *request_target);
char *echo_response(char *buff);

char **split_tokens(char *buff);
char *send_response(char buffer[]);
char *html_content(char *message);
char *copy_str(char str[]);

void *send_response_wrapper(void *args);

typedef struct thread_args {
    int client_socket_fd;
    char buffer[BUFFER_SIZE];
}THREAD_ARGS;

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

  pthread_t tid;
  while (1) {
      int client_socket_fd =
          accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
      printf("Client connected\n");

      if (client_socket_fd < 0) { break; }
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

      THREAD_ARGS *args = malloc(sizeof(THREAD_ARGS));
      args->client_socket_fd = client_socket_fd;
      strncpy(args->buffer, buffer, BUFFER_SIZE);

      int result = pthread_create(&tid, NULL, send_response_wrapper, (void *) args);

      if(result != 0) {
          perror("Thread creation failed");
          free(args);
          exit(EXIT_FAILURE);
      }

      pthread_detach(tid);
      
  }
  close(server_fd);

  return 0;
}

void *send_response_wrapper(void *arg){
    THREAD_ARGS *args = (THREAD_ARGS *)arg; 
    int client_socket_fd = args->client_socket_fd;
    char* buffer = args->buffer;

    char *res = send_response(buffer);
    send(client_socket_fd, res, strlen(res), 0);

    free(args);

    close(client_socket_fd);

}
char *copy_str(char str[]){
    size_t len = strlen(str);
    char *copy_arr = malloc(len + 1);

    if (copy_arr == NULL) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    strcpy(copy_arr, str);
    return copy_arr;
}

char *response(char *request_target) {
  if (strcmp(request_target, "/") == 0) {
    return "HTTP/1.1 200 OK\r\n\r\n";
  } else {
    return "HTTP/1.1 404 Not Found\r\n\r\n";
  }
}

char *send_response(char buffer[]){
    char *buffer_cpy = copy_str(buffer);
    const char *user_agent = "/user-agent";

    char *request_line = strtok(buffer, "\r\n");
    char *host_line = strtok(NULL, "\r\n");
    char *user_agent_line = (host_line != NULL) ? strtok(NULL, "\r\n") : NULL;

    char *request_target = strtok(request_line, " ");
    request_target = strtok(NULL, " ");

    if(strcmp(request_target, user_agent) != 0) { return echo_response(buffer_cpy); } 

    char *user_agents = strtok(user_agent_line, ": ");
    user_agents = strtok(NULL, ": ");

    return html_content(user_agents);

    
}
char *echo_response(char *buff) {

  char **split_buff = split_tokens(buff);
  const char *slash = "/";
  const char *echo = "echo";

  if (strcmp(split_buff[1], slash) == 0)
    return response(split_buff[1]);

  char *echo_word = strtok(split_buff[1], "/");
  if (strcmp(echo_word, echo) != 0) {
    return response(split_buff[1]);
  }

  echo_word = strtok(NULL, "/");
  if (echo_word == NULL) {
    return response("/Not Found");
  }
    
  return html_content(echo_word);
}

char *html_content(char *message){
    int content_length = strlen(message);
  int buffer_size = snprintf(NULL, 0,
                             "HTTP/1.1 200 OK\r\nContent-Type: "
                             "text/plain\r\nContent-Length: %d\r\n\r\n%s",
                             content_length, message);

  char *buffer = malloc(buffer_size + 1);
  sprintf(buffer,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/plain\r\n"
          "Content-Length: %d\r\n\r\n%s",
          content_length, message);

  return buffer;
}
char **split_tokens(char *buff) {
  char *request_target;
  const char delim[] = "\r\n";
  request_target = strtok(buff, delim);
  int index = 0;
  for (int i = 0; request_target[i] != '\0'; i++) {
    if (request_target[i] == ' ') {
      index++;
    }
  }
  index++;

  if (index < 3) {
    return NULL;
  }
  char **buff_arr = malloc((index + 1) * sizeof(char *));

  char *token = strtok(request_target, " ");
  for (int i = 0; i < index; i++) {
    buff_arr[i] = malloc(strlen(token) + 1);
    strcpy(buff_arr[i], token);
    token = strtok(NULL, " ");
  }

  return buff_arr;
}
