#include <stdio.h> // perror, printf
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#import <dispatch/dispatch.h>

#define PORT "2305" // the port client will be connecting to

#define MAXDATASIZE 100 // max number of bytes we can get at once

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  if (argc != 2)
  {
    fprintf(stderr, "usage: client hostname\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop to obtain a socket
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1)
    {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("client: connect");
      continue;
    }

    break;
  }

  if (p == NULL)
  {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
  printf("client: connecting to %s\n", s);

  freeaddrinfo(servinfo);

  // use a libdispatch queue to read data from the socket
  dispatch_semaphore_t exitsignal = dispatch_semaphore_create(0);
  dispatch_queue_t dq = dispatch_queue_create("data", NULL);
  dispatch_source_t ds = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, sockfd, 0, dq);

  dispatch_source_set_event_handler(ds, ^{
    int numbytes;
    char buf[MAXDATASIZE];

    int rfd = (int)dispatch_source_get_handle(ds);
    size_t bytesAvail = dispatch_source_get_data(ds);
    if (bytesAvail == 0)
    {
      dispatch_source_cancel(ds);
      return;
    }
    else
    {
      printf("..dispatch source %d has %lu bytes available.\n", rfd, bytesAvail);
    }

    if ((numbytes = recv(rfd, buf, MAXDATASIZE - 1, 0)) == -1)
    {
      perror("recv");
      exit(1);
    }
    buf[numbytes] = '\0';
    printf("client: received '%s'\n", buf);
  });

  dispatch_source_set_cancel_handler(ds, ^{
    // handler to close the socket - then signal to exit
    int rfd = (int)dispatch_source_get_handle(ds);
    close(rfd);
    dispatch_semaphore_signal(exitsignal);
  });

  dispatch_resume(ds);
  // block until the semaphore is signalled - when socket closes
  // second parameter is a timeout - nanoseconds
  dispatch_semaphore_wait(exitsignal, dispatch_walltime(NULL, 5 * 1000000000));

  dispatch_release(ds);
  dispatch_release(dq);
  dispatch_release(exitsignal);

  return 0;
}
