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
    printf("fcntl() failed");
  }

  // Set up the dispatch source that will alert us to new incoming connections
  dispatch_queue_t q = dispatch_queue_create("server_queue", DISPATCH_QUEUE_CONCURRENT);
  dispatch_source_t acceptSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, servSock, 0, q);
  dispatch_source_set_event_handler(acceptSource, ^{
    const unsigned long numPendingConnections = dispatch_source_get_data(acceptSource);
    for (unsigned long i = 0; i < numPendingConnections; i++)
    {
      int clntSock = -1;
      struct sockaddr_in echoClntAddr;
      unsigned int clntLen = sizeof(echoClntAddr);

      // Wait for a client to connect
      if ((clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen)) >= 0)
      {

        dispatch_queue_t dqc = dispatch_queue_create("client", NULL);

        printf("server sock: %d accepted\n", clntSock);

        dispatch_io_t channel = dispatch_io_create(DISPATCH_IO_STREAM, clntSock, dqc, ^(int error) {
          if (error)
          {
            fprintf(stderr, "Error: %s", strerror(error));
          }
          printf("server sock: %d closing\n", clntSock);
          close(clntSock);
        });

        // Configure the channel...
        dispatch_io_set_low_water(channel, 1);
        dispatch_io_set_high_water(channel, SIZE_MAX);

        // Setup read handler
        dispatch_io_read(channel, 0, SIZE_MAX, dqc, ^(bool done, dispatch_data_t data, int error) {
          int close_channel = 0;
          if (error)
          {
            fprintf(stderr, "Error: %s", strerror(error));
            close_channel = 1;
          }

          const size_t rxd = data ? dispatch_data_get_size(data) : 0;
          if (rxd)
          {
            // echo...
            printf("server sock: %d received: %ld bytes\n", clntSock, (long)rxd);
            // write it back out; echo!
            dispatch_io_write(channel, 0, data, dqc, ^(bool done, dispatch_data_t data, int error) {
              dispatch_io_close(channel, 0);
            });
          }
          else
          {
            close_channel = 1;
          }

          if (close_channel)
          {
            dispatch_io_close(channel, DISPATCH_IO_STOP);
            dispatch_release(channel);
          }
        });
      }
      else
      {
        printf("accept() failed;\n");
      }
    }
  });

  // Resume the source so we're ready to accept once we listen()
  dispatch_resume(acceptSource);

  // Listen() on the socket
  if (listen(servSock, SOMAXCONN) < 0)
  {
    shutdown(servSock, SHUT_RDWR);
    close(servSock);
    printf("listen() failed");
  }

  dispatch_main(); // suspend run loop waiting for blocks

  printf("server exit\n");

  return 0;
}
