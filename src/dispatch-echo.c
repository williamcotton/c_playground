#include <stdio.h>     // perror, printf
#include <stdlib.h>    // exit, atoi
#include <unistd.h>    // read, write, close
#include <arpa/inet.h> // sockaddr_in, AF_INET, SOCK_STREAM, INADDR_ANY, socket etc...
#include <string.h>    // memset
#include <regex.h>
#include <dispatch/dispatch.h>

struct server_connections
{
  int count;
};

int main(int argc, char const *argv[])
{
  // https://mohsensy.github.io/programming/2019/09/25/echo-server-and-client-using-sockets-in-c.html
  int serverFd;
  struct server_connections connections;
  connections.count = 0;
  struct sockaddr_in server;
  socklen_t len;
  int port = 1234;
  if (argc == 2)
  {
    port = atoi(argv[1]);
  }
  serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd < 0)
  {
    perror("Cannot create socket");
    exit(1);
  }
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);

  int flag = 1;
  if (-1 == setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
  {
    perror("setsockopt fail");
  }

  len = sizeof(server);
  if (bind(serverFd, (struct sockaddr *)&server, len) < 0)
  {
    perror("Cannot bind socket");
    exit(2);
  }
  if (listen(serverFd, 1000) < 0)
  {
    perror("Listen error");
    exit(3);
  }

  dispatch_semaphore_t exitsignal = dispatch_semaphore_create(0);
  // dispatch_queue_t dq = dispatch_queue_create("data", NULL); // serial queue
  dispatch_queue_t dq = dispatch_queue_create("data", DISPATCH_QUEUE_CONCURRENT); // concurrent queue
  // dispatch_queue_t dq = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);     // concurrent queue
  // dispatch_queue_t dq = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);  // concurrent queue
  // dispatch_queue_t dqc = dispatch_queue_create("client", NULL); // serial queue
  dispatch_queue_t dqc = dispatch_queue_create("client", DISPATCH_QUEUE_CONCURRENT); // concurrent queue

  // dispatch_queue_t dqc = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0); // concurrent queue
  // dispatch_queue_t dqc = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0); // concurrent queue
  dispatch_source_t ds = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, serverFd, 0, dq);

  dispatch_set_context(ds, &connections);

  dispatch_source_set_event_handler(ds, ^{
    printf("new source\n");
    struct server_connections *connections = dispatch_get_context(ds);

    if (connections->count > 100)
    {
      printf("too many connections\n");
      // 429: Too Many Requests
      return;
    }

    struct sockaddr_in client;
    int clientFd;

    socklen_t len = sizeof(client);

    if ((clientFd = accept(serverFd, (struct sockaddr *)&client, &len)) < 0)
    {
      perror("accept error");
      exit(4);
    }

    connections->count++;
    dispatch_set_context(ds, connections);
    printf("total_connections: %d\n", connections->count);
    char *client_ip = inet_ntoa(client.sin_addr);
    printf("Accepted new connection from a client %s:%d\n", client_ip, ntohs(client.sin_port));

    dispatch_async(dqc, ^{
      char buffer[1024];
      memset(buffer, 0, sizeof(buffer));
      int size = read(clientFd, buffer, sizeof(buffer));
      if (size < 0)
      {
        perror("read error");
        exit(5);
      }

      printf("%s", buffer);

      // a regular expression to match GET /:pathname HTTP/1.1
      char *pattern = "GET /(.*) HTTP/1.[0-1]";
      regex_t regex;
      int reti;
      size_t nmatch = 2;
      regmatch_t pmatch[2];
      reti = regcomp(&regex, pattern, REG_EXTENDED);
      if (reti)
      {
        fprintf(stderr, "Could not compile regex\n");
        exit(6);
      }
      reti = regexec(&regex, buffer, nmatch, pmatch, 0);
      if (reti == 0)
      {
        printf("Matched\n");
        printf("%lld %lld\n", pmatch[1].rm_so, pmatch[1].rm_eo);
        char *path = buffer + pmatch[1].rm_so;
        path[pmatch[1].rm_eo - pmatch[1].rm_so] = 0;
        printf("path: %s\n", path);
        char *file = malloc(strlen(path) + 1);
        strcpy(file, path);
        printf("file: %s\n", file);
        char *file_path = malloc(strlen(file) + strlen("./") + 1);
        strcpy(file_path, "./");
        strcat(file_path, file);
        printf("file_path: %s\n", file_path);
        FILE *fp = fopen(file_path, "r");
        if (fp == NULL)
        {
          printf("File not found\n");
          char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
          write(clientFd, response, strlen(response));
          close(clientFd);
          return;
        }
        char *response = "HTTP/1.1 200 OK\r\n\r\n";
        write(clientFd, response, strlen(response));
        while (1)
        {
          memset(buffer, 0, sizeof(buffer));
          size = fread(buffer, 1, sizeof(buffer), fp);
          if (size < 0)
          {
            perror("read error");
            exit(5);
          }
          if (size == 0)
          {
            break;
          }
          write(clientFd, buffer, size);
        }
        free(file_path);
        free(file);
        fclose(fp);
      }
      else
      {
        printf("No match\n");
        char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
        write(clientFd, response, strlen(response));
      }

      close(clientFd);

      struct server_connections *connections = dispatch_get_context(ds);
      connections->count--;
      dispatch_set_context(ds, connections);

      regfree(&regex);
    });
  });

  dispatch_source_set_cancel_handler(ds, ^{
    printf("cancel\n");
    int rfd = (int)dispatch_source_get_handle(ds);
    close(rfd);
    dispatch_semaphore_signal(exitsignal);
  });

  printf("waiting for clients\n");

  dispatch_resume(ds);
  dispatch_main(); // suspend run loop waiting for blocks

  dispatch_semaphore_wait(exitsignal, dispatch_walltime(NULL, DISPATCH_TIME_FOREVER));

  dispatch_release(ds);
  dispatch_release(dq);
  dispatch_release(exitsignal);

  close(serverFd);

  printf("server exit\n");

  return 0;
}
