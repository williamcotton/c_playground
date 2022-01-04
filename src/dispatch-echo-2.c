#include <stdio.h>     // perror, printf
#include <stdlib.h>    // exit, atoi
#include <unistd.h>    // read, write, close
#include <arpa/inet.h> // sockaddr_in, AF_INET, SOCK_STREAM, INADDR_ANY, socket etc...
#include <string.h>    // memset
#include <regex.h>
#include <dispatch/dispatch.h>

int main()
{
  int port = 1234;

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

  // Set up the dispatch source that will alert us to new incoming connections
  dispatch_queue_t q = dispatch_queue_create("server_queue", DISPATCH_QUEUE_CONCURRENT);
  dispatch_source_t acceptSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, servSock, 0, q);
  dispatch_source_set_event_handler(acceptSource, ^{
    printf("acceptSource\n");
    const unsigned long numPendingConnections = dispatch_source_get_data(acceptSource);
    for (unsigned long i = 0; i < numPendingConnections; i++)
    {
      int clntSock = -1;
      struct sockaddr_in echoClntAddr;
      unsigned int clntLen = sizeof(echoClntAddr);
      if ((clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen)) < 0)
      {
        perror("accept() failed");
        continue;
      }

      // Make the socket non-blocking
      if (fcntl(clntSock, F_SETFL, O_NONBLOCK) < 0)
      {
        shutdown(clntSock, SHUT_RDWR);
        close(clntSock);
        printf("fcntl() failed");
      }

      // Set up the dispatch source that will alert us to incoming data
      dispatch_source_t readSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, clntSock, 0, q);
      dispatch_source_set_event_handler(readSource, ^{
        char buffer[1024];
        ssize_t numBytes = read(clntSock, buffer, sizeof(buffer));
        if (numBytes < 0)
        {
          perror("read() failed");
          shutdown(clntSock, SHUT_RDWR);
          close(clntSock);
          return;
        }
        else if (numBytes == 0)
        {
          shutdown(clntSock, SHUT_RDWR);
          close(clntSock);
          return;
        }

        // Echo the data back to the client
        if (write(clntSock, buffer, numBytes) != numBytes)
        {
          perror("write() failed");
          shutdown(clntSock, SHUT_RDWR);
          close(clntSock);
          return;
        }
        shutdown(clntSock, SHUT_RDWR);
        close(clntSock);
      });
      dispatch_resume(readSource);
    }
  });
  printf("waiting for clients\n");
  dispatch_resume(acceptSource);

  dispatch_main();

  return 0;
}
