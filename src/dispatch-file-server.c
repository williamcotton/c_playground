#include <stdio.h>     // perror, printf
#include <stdlib.h>    // exit, atoi
#include <unistd.h>    // read, write, close
#include <arpa/inet.h> // sockaddr_in, AF_INET, SOCK_STREAM, INADDR_ANY, socket etc...
#include <string.h>    // memset
#include <dispatch/dispatch.h>
#include "server-helpers.h"

int main()
{
  int port = 1234;
  int servSock = listenServer(port);

  __block int clientCount = 0;
  dispatch_queue_t clientCounter = dispatch_queue_create("clientCounter", NULL);

  dispatch_queue_t serverQueue = dispatch_queue_create("serverQueue", DISPATCH_QUEUE_CONCURRENT);
  dispatch_source_t acceptSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, servSock, 0, serverQueue);

  dispatch_source_set_event_handler(acceptSource, ^{
    const unsigned long numPendingConnections = dispatch_source_get_data(acceptSource);
    for (unsigned long i = 0; i < numPendingConnections; i++)
    {
      int clntSock = acceptClient(servSock);
      if (clntSock < 0)
        continue;

      dispatch_source_t readSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, clntSock, 0, serverQueue);
      dispatch_source_set_event_handler(readSource, ^{
        char request[1024];
        ssize_t numBytes = read(clntSock, request, sizeof(request));
        if (numBytes < 0)
        {
          // perror("read() failed");
          return;
        }
        else if (numBytes == 0)
        {
          return;
        }

        printf("Request\n===\n%s\n", request);

        dispatch_async(clientCounter, ^{
          clientCount++;
        });
        printf("Client count: %d\n", clientCount);

        FILE *fp = matchFile("GET /files/(.*) HTTP/1.[0-1]", request);
        if (fp != NULL)
        {
          char *response = "HTTP/1.1 200 OK\r\n\r\n";
          printf("Response\n===\n%s", response);
          write(clntSock, response, strlen(response));
          writeFile(fp, clntSock);
          fclose(fp);
        }
        else
        {
          char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
          printf("Response\n===\n%s", response);
          write(clntSock, response, strlen(response));
        }

        dispatch_async(clientCounter, ^{
          clientCount--;
        });
        shutdown(clntSock, SHUT_RDWR);
        close(clntSock);
      });
      dispatch_resume(readSource);
    }
  });

  printf("Server listening on port %d\n\n", port);
  dispatch_resume(acceptSource);

  dispatch_main();

  return 0;
}
