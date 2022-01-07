#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <dispatch/dispatch.h>
#include <regex.h>
#include <Block.h>
#include <picohttpparser/picohttpparser.h>

#define UNUSED __attribute__((unused))

static char *getStatusMessage(int status)
{
  switch (status)
  {
  case 100:
    return "Continue";
  case 101:
    return "Switching Protocols";
  case 102:
    return "Processing";
  case 103:
    return "Early Hints";
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 202:
    return "Accepted";
  case 203:
    return "Non-Authoritative Information";
  case 204:
    return "No Content";
  case 205:
    return "Reset Content";
  case 206:
    return "Partial Content";
  case 207:
    return "Multi-Status";
  case 208:
    return "Already Reported";
  case 226:
    return "IM Used";
  case 300:
    return "Multiple Choices";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 303:
    return "See Other";
  case 304:
    return "Not Modified";
  case 305:
    return "Use Proxy";
  case 306:
    return "Switch Proxy";
  case 307:
    return "Temporary Redirect";
  case 308:
    return "Permanent Redirect";
  case 400:
    return "Bad Request";
  case 401:
    return "Unauthorized";
  case 402:
    return "Payment Required";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 406:
    return "Not Acceptable";
  case 407:
    return "Proxy Authentication Required";
  case 408:
    return "Request Timeout";
  case 409:
    return "Conflict";
  case 410:
    return "Gone";
  case 411:
    return "Length Required";
  case 412:
    return "Precondition Failed";
  case 413:
    return "Payload Too Large";
  case 414:
    return "URI Too Long";
  case 415:
    return "Unsupported Media Type";
  case 416:
    return "Range Not Satisfiable";
  case 417:
    return "Expectation Failed";
  case 418:
    return "I'm a teapot";
  case 421:
    return "Misdirected Request";
  case 422:
    return "Unprocessable Entity";
  case 423:
    return "Locked";
  case 424:
    return "Failed Dependency";
  case 425:
    return "Too Early";
  case 426:
    return "Upgrade Required";
  case 428:
    return "Precondition Required";
  case 429:
    return "Too Many Requests";
  case 431:
    return "Request Header Fields Too Large";
  case 451:
    return "Unavailable For Legal Reasons";
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  case 502:
    return "Bad Gateway";
  case 503:
    return "Service Unavailable";
  case 504:
    return "Gateway Timeout";
  case 505:
    return "HTTP Version Not Supported";
  case 506:
    return "Variant Also Negotiates";
  case 507:
    return "Insufficient Storage";
  case 508:
    return "Loop Detected";
  case 510:
    return "Not Extended";
  case 511:
    return "Network Authentication Required";
  default:
    return "Unknown";
  }
}

typedef struct
{
  char *path;
  char *method;
  char *url;
  char *queryString;
  struct phr_header *headers;
  int numHeaders;
  char *rawRequest;
  char * (^get)(char *headerKey);
} request_t;

typedef struct
{
  void (^send)(char *);
  void (^sendf)(char *, ...);
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

static request_t parseRequest(client_t client)
{
  request_t req = {.url = NULL, .queryString = "", .path = NULL, .method = NULL, .headers = NULL, .rawRequest = NULL};
  char buf[4096];
  char *method, *url;
  int pret, minor_version;
  struct phr_header headers[100];
  size_t buflen = 0, prevbuflen = 0, method_len, queryString_len, url_len, num_headers;
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

  req.rawRequest = buf;

  // copy to request struct

  req.method = malloc(method_len + 1);
  memcpy(req.method, method, method_len);
  req.method[method_len] = '\0';

  req.url = malloc(url_len + 1);
  memcpy(req.url, url, url_len);
  req.url[url_len] = '\0';

  char *copy = strdup(req.url);
  char *queryStringStart = strchr(copy, '?');

  if (queryStringStart)
  {
    queryString_len = strlen(queryStringStart + 1);
    req.queryString = malloc(queryString_len + 1);
    memcpy(req.queryString, queryStringStart + 1, queryString_len);
    req.queryString[queryString_len] = '\0';
    *queryStringStart = '\0';
  }

  req.path = malloc(strlen(copy) + 1);
  memcpy(req.path, copy, strlen(copy));
  req.path[strlen(copy)] = '\0';

  req.headers = headers;
  req.numHeaders = num_headers;

  req.get = ^(char *headerKey) {
    for (int i = 0; i < req.numHeaders; i++)
    {
      char *key = malloc(req.headers[i].name_len + 1);
      char *value = malloc(req.headers[i].value_len + 1);
      memcpy(key, req.headers[i].name, req.headers[i].name_len);
      key[req.headers[i].name_len] = '\0';
      if (strcasecmp(key, headerKey) == 0)
      {

        memcpy(value, req.headers[i].value, req.headers[i].value_len);
        value[req.headers[i].value_len] = '\0';
        return value;
      }
      free(key);
      free(value);
    }
    return (char *)NULL;
  };

  free(copy);

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

static void freeRequest(request_t req)
{
  free(req.method);
  free(req.path);
  free(req.url);
  if (strlen(req.queryString) > 0)
    free(req.queryString);
}

static void closeClientConnection(client_t client, request_t req)
{
  shutdown(client.socket, SHUT_RDWR);
  close(client.socket);
  freeRequest(req);
}

static void buildResponse(response_t *res)
{
  res->status = 200;
}

static char *buildResponseString(char *body, response_t res)
{
  char *contentType = "text/html; charset=utf-8";
  char *contentLength = malloc(sizeof(char) * 20);
  sprintf(contentLength, "%zu", strlen(body));
  char *statusMessage = getStatusMessage(res.status);
  char *status = malloc(sizeof(char) * (strlen(statusMessage) + 5));
  sprintf(status, "%d %s", res.status, statusMessage);
  char *headers = malloc(sizeof(char) * (strlen("HTTP/1.1 \r\nContent-Type: \r\nContent-Length: \r\n\r\n") + strlen(status) + strlen(contentType) + strlen(contentLength) + 1));
  sprintf(headers, "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %s\r\n\r\n", status, contentType, contentLength);
  char *responseString = malloc(sizeof(char) * (strlen(headers) + strlen(body) + 1));
  strcpy(responseString, headers);
  strcat(responseString, body);
  free(status);
  free(headers);
  free(contentLength);
  return responseString;
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
        request_t req = parseRequest(client);

        if (req.method == NULL)
        {
          closeClientConnection(client, req);
          return;
        }

        route_handler_t routeHandler = matchRouteHandler(req);

        if (routeHandler.handler == NULL)
        {
          char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
          write(client.socket, response, strlen(response));
          closeClientConnection(client, req);
          return;
        }

        __block response_t res;
        buildResponse(&res);
        res.send = ^(char *body) {
          char *response = buildResponseString(body, res);
          write(client.socket, response, strlen(response));
          free(response);
        };

        res.sendf = ^(char *format, ...) {
          char body[4096];
          va_list args;
          va_start(args, format);
          vsnprintf(body, 255, format, args);
          res.send(body);
          va_end(args);
        };

        routeHandler.handler(&req, &res);

        closeClientConnection(client, req);
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

  app.get("/", ^(UNUSED request_t *req, response_t *res) {
    res->send("Hello World!");
  });

  app.get("/test", ^(UNUSED request_t *req, response_t *res) {
    res->send("Testing, testing!");
  });

  app.get("/qs_test", ^(request_t *req, response_t *res) {
    res->sendf("<h1>Testing!</h1><p>Query string: %s</p>", req->queryString);
  });

  app.get("/headers", ^(request_t *req, response_t *res) {
    res->sendf("<h1>Testing!</h1><p>User-Agent: %s</p><p>Host: %s</p>", req->get("User-Agent"), req->get("Host"));
  });

  app.listen(port, ^{
    printf("Example app listening at http://localhost:%d\n", port);
  });

  return 0;
}
