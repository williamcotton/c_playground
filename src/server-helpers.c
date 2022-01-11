#include <stdio.h>     // perror, printf
#include <stdlib.h>    // exit, atoi
#include <arpa/inet.h> // sockaddr_in, AF_INET, SOCK_STREAM, INADDR_ANY, socket etc...
#include <string.h>    // memset
#include <regex.h>
#include <dispatch/dispatch.h>
#include "server-helpers.h"

void get(char *path, char request[1024], void (^handler)())
{
  char *pattern = malloc(strlen("GET (") + strlen(path) + strlen(") HTTP/1.[0-1]") + 1);
  sprintf(pattern, "GET (%s) HTTP/1.[0-1]", path);

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
  reti = regexec(&regex, request, nmatch, pmatch, 0);
  if (reti == 0)
  {
    printf("Matched\n");
    printf("%d %d\n", (int)pmatch[1].rm_so, (int)pmatch[1].rm_eo);
    char *mpath = request + pmatch[1].rm_so;
    mpath[pmatch[1].rm_eo - pmatch[1].rm_so] = 0;
    printf("path: %s\n", mpath);
    handler();
  }
};

int listenServer(int port)
{

  // Create the socket
  int servSock = -1;
  if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
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
  if (-1 == setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
  {
    perror("setsockopt fail");
  }

  if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
  {
    perror("Cannot bind socket");
    exit(2);
  }

  // Make the socket non-blocking
  if (fcntl(servSock, F_SETFL, O_NONBLOCK) < 0)
  {
    shutdown(servSock, SHUT_RDWR);
    close(servSock);
    perror("fcntl() failed");
    exit(3);
  }

  if (listen(servSock, 1000) < 0)
  {
    perror("Listen error");
    exit(4);
  }

  return servSock;
}

int acceptClient(int servSock)
{
  int clntSock = -1;
  struct sockaddr_in echoClntAddr;
  unsigned int clntLen = sizeof(echoClntAddr);
  if ((clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen)) < 0)
  {
    // perror("accept() failed");
    return clntSock;
  }

  // Make the socket non-blocking
  if (fcntl(clntSock, F_SETFL, O_NONBLOCK) < 0)
  {
    // perror("fcntl() failed");
    shutdown(clntSock, SHUT_RDWR);
    close(clntSock);
  }

  char *client_ip = inet_ntoa(echoClntAddr.sin_addr);
  printf("Accepted new connection from a client %s:%d\n", client_ip, ntohs(echoClntAddr.sin_port));

  return clntSock;
}

FILE *matchFile(char *pattern, char buffer[1024])
{
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
    // printf("Matched\n");
    // printf("%lld %lld\n", pmatch[1].rm_so, pmatch[1].rm_eo);
    char *path = buffer + pmatch[1].rm_so;
    path[pmatch[1].rm_eo - pmatch[1].rm_so] = 0;
    // printf("path: %s\n", path);
    char *file = malloc(strlen(path) + 1);
    strcpy(file, path);
    // printf("file: %s\n", file);
    char *file_path = malloc(strlen(file) + strlen("./files/") + 1);
    strcpy(file_path, "./files/");
    strcat(file_path, file);
    // printf("file_path: %s\n", file_path);
    FILE *fp = fopen(file_path, "r");
    free(file_path);
    free(file);
    return fp;
  }
  else
  {
    return NULL;
  }
}

void writeFile(FILE *fp, int fd)
{
  char buffer[1024];
  while (1)
  {
    memset(buffer, 0, sizeof(buffer));
    size_t size = fread(buffer, 1, sizeof(buffer), fp);
    if (size < 0)
    {
      perror("read error");
      exit(5);
    }
    if (size == 0)
    {
      break;
    }
    write(fd, buffer, size);
  }
}