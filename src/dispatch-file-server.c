#include <stdio.h>     // perror, printf
#include <stdlib.h>    // exit, atoi
#include <unistd.h>    // read, write, close
#include <arpa/inet.h> // sockaddr_in, AF_INET, SOCK_STREAM, INADDR_ANY, socket etc...
#include <string.h>    // memset
#include <regex.h>
#include <dispatch/dispatch.h>

void new_server(int *serverFd, int port)
{
  // Create the socket
  *serverFd = -1;
  if ((*serverFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    printf("socket() failed");
  }

  // Bind the socket - if the port we want is in use, increment until we find one that isn't
  struct sockaddr_in echoServAddr;
  memset(&echoServAddr, 0, sizeof(echoServAddr));
  echoServAddr.sin_family = AF_INET;
  echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  echoServAddr.sin_port = htons(port);

  int flag = 1;
  if (-1 == setsockopt(*serverFd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
  {
    perror("setsockopt fail");
  }

  if (bind(*serverFd, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
  {
    perror("Cannot bind socket");
    exit(2);
  }

  if (listen(*serverFd, 1000) < 0)
  {
    perror("Listen error");
    exit(3);
  }
}

int main()
{
  int serverFd;

  new_server(&serverFd, 1234);

  dispatch_semaphore_t exitsignal = dispatch_semaphore_create(0);

  dispatch_queue_t dq = dispatch_queue_create("data", DISPATCH_QUEUE_CONCURRENT);    // concurrent queue
  dispatch_queue_t dqc = dispatch_queue_create("client", DISPATCH_QUEUE_CONCURRENT); // concurrent queue

  dispatch_source_t ds = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, serverFd, 0, dq);

  dispatch_source_set_event_handler(ds, ^{
    printf("new source\n");

    struct sockaddr_in client;
    int clientFd;

    socklen_t len = sizeof(client);

    if ((clientFd = accept(serverFd, (struct sockaddr *)&client, &len)) < 0)
    {
      perror("accept error");
      exit(4);
    }

    // set setsockopt SO_NOSIGPIPE
    int flag = 1;
    if (-1 == setsockopt(clientFd, SOL_SOCKET, SO_NOSIGPIPE, &flag, sizeof(flag)))
    {
      perror("setsockopt fail");
    }

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
