#include <stdio.h> // perror, printf
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#import <dispatch/dispatch.h>
#define PORT "2305" // the port users will be connecting to
#define BACKLOG 10  // how many pending connections queue will hold

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(void)
{
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int yes = 1;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
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
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int)) == -1)
    {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  if (p == NULL)
  {
    fprintf(stderr, "server: failed to bind\n");
    return 2;
  }

  freeaddrinfo(servinfo);

  if (listen(sockfd, BACKLOG) == -1)
  {
    perror("listen");
    exit(1);
  }
  // NSLog(@"Listening...");

  // use a libdispatch queue to read data from the socket
  dispatch_semaphore_t exitsignal = dispatch_semaphore_create(0);
  dispatch_queue_t dq = dispatch_queue_create("data", NULL);
  dispatch_source_t ds = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, sockfd, 0, dq);

  dispatch_source_set_event_handler(ds, ^{
    int new_fd;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    sin_size = sizeof their_addr;

    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1)
    {
      perror("accept");
    }

    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    // NSLog(@"server: got connection from %s\n", s);

    // reply to client
    if (send(new_fd, "Hello, world!", 13, 0) == -1)
      perror("send");
    close(new_fd);
  });

  dispatch_source_set_cancel_handler(ds, ^{
    int rfd = (int)dispatch_source_get_handle(ds);
    close(rfd);
    dispatch_semaphore_signal(exitsignal);
  });

  dispatch_resume(ds);
  dispatch_main(); // suspend run loop waiting for blocks

  dispatch_semaphore_wait(exitsignal, dispatch_walltime(NULL, DISPATCH_TIME_FOREVER));

  dispatch_release(ds);
  dispatch_release(dq);
  dispatch_release(exitsignal);

  return 0;
}