#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <dispatch/dispatch.h>
#include <regex.h>
#include <Block.h>
#include <picohttpparser/picohttpparser.h>

typedef struct
{
  char *path;
  char *method;
  char *url;
  char *queryString;
  struct phr_header *headers;
  int numHeaders;
  char *rawRequest;
} request_t;

typedef struct
{
  void (^send)(char *);
  int status;
} response_t;

typedef void (^requestHandler)(request_t *req, response_t *res);

typedef struct
{
  void (^get)(char *path, requestHandler);
  void (^listen)(int port, void (^handler)());
} app_t;

typedef struct
{
  char *method;
  char *path;
  requestHandler handler;
} route_handler_t;

typedef struct
{
  int socket;
  char *ip;
} client_t;

static route_handler_t *routeHandlers = NULL;
static int routeHandlerCount = 0;
static int servSock = -1;
static dispatch_queue_t serverQueue = NULL;

static void initRouteHandlers()
{
  routeHandlers = malloc(sizeof(route_handler_t));
}

static void addRouteHandler(char *method, char *path, requestHandler handler)
{
  routeHandlers = realloc(routeHandlers, sizeof(route_handler_t) * (routeHandlerCount + 1));
  routeHandlers[routeHandlerCount++] = (route_handler_t){.method = method, .path = path, .handler = handler};
}

static void initServerQueue()
{
  serverQueue = dispatch_queue_create("serverQueue", DISPATCH_QUEUE_CONCURRENT);
}

static client_t acceptClientConnection(int servSock)
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

static void initServerSocket()
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

static void initServerListen(int port)
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

static request_t parseRequest(char *rawRequest, client_t client)
{
  request_t req = {.url = NULL, .queryString = NULL, .path = NULL, .method = NULL, .headers = NULL, .rawRequest = rawRequest};
  char buf[4096];
  char *method, *url, *path, *queryString;
  int pret, minor_version;
  struct phr_header headers[100];
  size_t buflen = 0, prevbuflen = 0, method_len, path_len, queryString_len, url_len, num_headers;
  ssize_t rret;

  while (1)
  {
    /* read the request */
    while ((rret = read(client.socket, buf + buflen, sizeof(buf) - buflen)) == -1)
      ;
    if (rret <= 0)
      return req;
    prevbuflen = buflen;
    buflen += rret;
    /* parse the request */
    num_headers = sizeof(headers) / sizeof(headers[0]);
    pret = phr_parse_request(buf, buflen, (const char **)&method, &method_len, (const char **)&url, &url_len,
                             &minor_version, headers, &num_headers, prevbuflen);
    if (pret > 0)
      break; /* successfully parsed the request */
    else if (pret == -1)
      return req;
    /* request is incomplete, continue the loop */
    // assert(pret == -2);
    if (buflen == sizeof(buf))
      return req;
  }

  req.url = malloc(url_len + 1);
  memcpy(req.url, url, url_len);
  req.url[url_len] = '\0';
  // char *copy = strdup(req.url);
  // req.path = strtok(copy, "?");
  // req.queryString = strtok(NULL, "?");
  req.path = req.url;
  req.method = malloc(method_len + 1);
  memcpy(req.method, method, method_len);
  req.method[method_len] = '\0';
  req.headers = headers;
  req.numHeaders = num_headers;
  return req;
}

static route_handler_t matchRouteHandler(request_t req)
{
  for (int i = 0; i < routeHandlerCount; i++)
  {
    if (strcmp(routeHandlers[i].method, req.method) == 0 && strcmp(routeHandlers[i].path, req.path) == 0)
    {
      return routeHandlers[i];
    }
  }
  return (route_handler_t){.method = NULL, .path = NULL, .handler = NULL};
}

static void closeClientConnection(client_t client)
{
  shutdown(client.socket, SHUT_RDWR);
  close(client.socket);
}

static void freeRequest(request_t req)
{
  free(req.method);
  free(req.path);
}

static response_t buildResponse()
{
  __block response_t res;
  res.status = 200;
  return res;
}

// char *responseString = malloc(sizeof(char) * (strlen(data) + strlen(res.status) + strlen(res.contentType) + strlen(res.contentLength) + strlen(res.body) + 10));
// sprintf(responseString, "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %s\r\n\r\n%s", res.status, res.contentType, res.contentLength, res.body);
// return responseString;

static char *buildResponseString(char *data, response_t res)
{
  char *response = malloc(strlen("HTTP/1.1 200 OK\r\n\r\n") + strlen(data) + 1);
  sprintf(response, "HTTP/1.1 %d OK\r\n\r\n%s", res.status, data);
  return response;
}

static void initClientAcceptEventHandler()
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
        char buf[4096];
        request_t req = parseRequest(buf, client);

        if (req.method == NULL)
        {
          closeClientConnection(client);
          return;
        }

        route_handler_t routeHandler = matchRouteHandler(req);

        if (routeHandler.handler == NULL)
        {
          char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
          write(client.socket, response, strlen(response));
          freeRequest(req);
          closeClientConnection(client);
          return;
        }

        __block response_t res = buildResponse();
        res.send = ^(char *data) {
          char *response = buildResponseString(data, res);
          write(client.socket, response, strlen(response));
          free(response);
        };

        routeHandler.handler(&req, &res);

        freeRequest(req);
        closeClientConnection(client);
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

  void (^_get)(char *, void (^)(request_t *, response_t *)) = ^(char *path, requestHandler handler) {
    addRouteHandler("GET", path, handler);
  };

  app_t app = {.get = _get, .listen = _listen};
  return app;
};

int main()
{
  app_t app = express();
  int port = 3000;

  app.get("/", ^(request_t *req, response_t *res) {
    res->send("Hello World!");
  });

  app.get("/test", ^(request_t *req, response_t *res) {
    // char *response = malloc(strlen("<h1>Testing!</h1><p>Query string: </p>") + strlen(req->queryString) + 1);
    // sprintf(response, "<h1>Testing!</h1><p>Query string: %s</p>", req->queryString);

    // res->status = 201;
    // res->send(response);
    res->send("testing!");
  });

  app.listen(port, ^{
    printf("Example app listening at http://localhost:%d\n", port);
  });

  return 0;
}
