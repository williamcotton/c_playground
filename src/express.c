#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <dispatch/dispatch.h>
#include <regex.h>
#include <Block.h>

typedef struct
{
  char *path;
} request_t;

typedef struct
{
  void (^send)(char *);
} response_t;

typedef void (^requestHandler)(request_t req, response_t res);

typedef struct
{
  void (^get)(char *path, requestHandler);
  void (^listen)(int port, void (^handler)());
} app_t;

typedef struct
{
  char *path;
  requestHandler handler;
} route_handler_t;

static route_handler_t *routeHandlers = NULL;
static int routeHandlerCount = 0;

static void initRouteHandlers()
{
  routeHandlers = malloc(sizeof(route_handler_t));
}

static void addRouteHandler(char *path, requestHandler handler)
{
  routeHandlers = realloc(routeHandlers, sizeof(route_handler_t) * (routeHandlerCount + 1));
  routeHandlers[routeHandlerCount++] = (route_handler_t){.path = path, .handler = handler};
}

static int servSock = -1;

static dispatch_queue_t serverQueue = NULL;

static void initServerQueue()
{
  serverQueue = dispatch_queue_create("serverQueue", DISPATCH_QUEUE_CONCURRENT);
}

typedef struct
{
  int socket;
  char *ip;
} client_t;

client_t acceptClientConnection(int servSock)
{
  int clntSock = -1;
  struct sockaddr_in echoClntAddr;
  unsigned int clntLen = sizeof(echoClntAddr);
  if ((clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen)) < 0)
  {
    // perror("accept() failed");
    return (client_t){.socket = -1, .ip = NULL};
  }

  // Make the socket non-blocking
  if (fcntl(clntSock, F_SETFL, O_NONBLOCK) < 0)
  {
    // perror("fcntl() failed");
    shutdown(clntSock, SHUT_RDWR);
    close(clntSock);
  }

  char *client_ip = inet_ntoa(echoClntAddr.sin_addr);

  return (client_t){.socket = clntSock, .ip = client_ip};
}

void initServerSocket()
{
  if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    printf("socket() failed");
  }

  int flag = 1;
  if (-1 == setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
  {
    perror("setsockopt fail");
  }
}

void initServerListen(int port)
{
  struct sockaddr_in servAddr;
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(port);

  if (bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
  {
    printf("bind() failed");
  }

  // Make the socket non-blocking
  if (fcntl(servSock, F_SETFL, O_NONBLOCK) < 0)
  {
    shutdown(servSock, SHUT_RDWR);
    close(servSock);
    perror("fcntl() failed");
    exit(3);
  }

  if (listen(servSock, 10000) < 0)
  {
    printf("listen() failed");
  }
};

void initClientAcceptEventHandler()
{
  dispatch_source_t acceptSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, servSock, 0, serverQueue);

  dispatch_source_set_event_handler(acceptSource, ^{
    const unsigned long numPendingConnections = dispatch_source_get_data(acceptSource);
    for (unsigned long i = 0; i < numPendingConnections; i++)
    {
      client_t client = acceptClientConnection(servSock);
      if (client.socket < 0)
        continue;

      dispatch_source_t readSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, client.socket, 0, serverQueue);
      dispatch_source_set_event_handler(readSource, ^{
        char request[1024];
        ssize_t numBytes = read(client.socket, request, sizeof(request));
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

        route_handler_t rh = routeHandlers[routeHandlerCount - 1];
        request_t req;
        response_t res;
        res.send = ^(char *data) {
          char *response = malloc(strlen("HTTP/1.1 200 OK\r\n\r\n") + strlen(data) + 1);
          sprintf(response, "HTTP/1.1 200 OK\r\n\r\n%s", data);

          printf("Response\n===\n%s", response);
          write(client.socket, response, strlen(response));
        };
        req.path = rh.path;
        rh.handler(req, res);

        shutdown(client.socket, SHUT_RDWR);
        close(client.socket);
      });
      dispatch_resume(readSource);
    }
  });

  dispatch_resume(acceptSource);
}

app_t express()
{
  initRouteHandlers();
  initServerQueue();
  initServerSocket();

  void (^_listen)(int, void (^)()) = ^(int port, void (^handler)()) {
    initServerListen(port);
    initClientAcceptEventHandler();
    handler();
    dispatch_main();
  };

  void (^_get)(char *, void (^)(request_t, response_t)) = ^(char *path, requestHandler handler) {
    // request_t req;
    // response_t res;
    // res.send = ^(char *data) {
    //   printf("send - %s\n", data);
    // };
    // req.path = path;
    // dispatch_block_t block = Block_copy(^{
    //   handler(req, res);
    // });
    addRouteHandler(path, handler);
  };

  app_t app = {.get = _get, .listen = _listen};
  return app;
};

int main()
{
  app_t app = express();
  int port = 3000;

  app.get("/", ^(request_t req, response_t res) {
    res.send("Hello World!");
  });

  app.listen(port, ^{
    printf("Example app listening at http://localhost:%d\n", port);
  });

  return 0;
}
