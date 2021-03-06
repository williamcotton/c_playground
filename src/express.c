#include <assert.h>
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
#include <sys/stat.h>
#include <hash/hash.h>
#include <signal.h>

#define UNUSED __attribute__((unused))

char *curl(char *cmd)
{
  system(cmd);
  FILE *file = fopen("./test/test-response.html", "r");
  char *html = malloc(4096);
  size_t bytesRead = fread(html, 1, 4096, file);
  html[bytesRead] = '\0';
  fclose(file);
  system("rm ./test/test-response.html");
  return html;
}

char *curlGet(char *url)
{
  char *curlCmd = "curl -s -o ./test/test-response.html http://127.0.0.1:3000";
  char *cmd = malloc(strlen(curlCmd) + strlen(url) + 1);
  sprintf(cmd, "%s%s", curlCmd, url);
  return curl(cmd);
}

char *curlPost(char *url, char *data)
{
  char *curlCmd = "curl -X POST -s -o ./test/test-response.html -H \"Content-Type: application/x-www-form-urlencoded\" -d \"\" http://127.0.0.1:3000";
  char *cmd = malloc(strlen(curlCmd) + strlen(url) + strlen(data) + 1);
  sprintf(cmd, "curl -X POST -s -o ./test/test-response.html -H \"Content-Type: application/x-www-form-urlencoded\" -d \"%s\" http://127.0.0.1:3000%s", data, url);
  return curl(cmd);
}

int testEq(char *testName, char *response, char *expected)
{
  int status = 0;
  if (strcmp(response, expected) != 0)
  {
    printf("%s: FAIL\n", testName);
    printf("\tExpected: %s\n", expected);
    printf("\tReceived: %s\n", response);
    status = 1;
  }
  else
  {
    printf("%s: PASS\n", testName);
  }
  free(response);
  return status;
}

static char *errorHTML = "<!DOCTYPE html>\n"
                         "<html lang=\"en\">\n"
                         "<head>\n"
                         "<meta charset=\"utf-8\">\n"
                         "<title>Error</title>\n"
                         "</head>\n"
                         "<body>\n"
                         "<pre>Cannot GET %s</pre>\n"
                         "</body>\n"
                         "</html>\n";

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

size_t fileSize(char *filePath)
{
  struct stat st;
  stat(filePath, &st);
  return st.st_size;
}

static void parseQueryString(hash_t *hash, char *string)
{
  char *query = strdup(string);
  char *tokens = query;
  char *p;
  while ((p = strsep(&tokens, "&\n")))
  {
    char *key = strtok(p, "=");
    char *value = NULL;
    if (key && (value = strtok(NULL, "=")))
      hash_set(hash, key, value);
    else
      hash_set(hash, key, "");
  }
}

typedef struct client_t
{
  int socket;
  char *ip;
} client_t;

typedef struct request_t
{
  char *path;
  char *method;
  char *url;
  char *queryString;
  hash_t *queryHash;
  char * (^query)(char *queryKey);
  hash_t *headersHash;
  char * (^get)(char *headerKey);
  hash_t *paramsHash;
  char * (^param)(char *paramKey);
  char *bodyString;
  hash_t *bodyHash;
  char * (^body)(char *bodyKey);
  char *rawRequest;
} request_t;

typedef struct response_t
{
  void (^send)(char *);
  void (^sendf)(char *, ...);
  void (^sendFile)(char *);
  int status;
} response_t;

typedef struct param_match_t
{
  char *regexRoute;
  char **keys;
  int count;
} param_match_t;

param_match_t *paramMatch(char *route)
{
  param_match_t *pm = malloc(sizeof(param_match_t));
  pm->keys = malloc(sizeof(char *));
  pm->count = 0;
  char regexRoute[4096];
  regexRoute[0] = '\0';
  char *source = route;
  char *regexString = ":([A-Za-z0-9_]*)";
  size_t maxMatches = 100;
  size_t maxGroups = 100;

  regex_t regexCompiled;
  regmatch_t groupArray[maxGroups];
  unsigned int m;
  char *cursor;

  if (regcomp(&regexCompiled, regexString, REG_EXTENDED))
  {
    printf("Could not compile regular expression.\n");
    return NULL;
  };

  cursor = source;
  for (m = 0; m < maxMatches; m++)
  {
    if (regexec(&regexCompiled, cursor, maxGroups, groupArray, 0))
      break; // No more matches

    unsigned int g = 0;
    unsigned int offset = 0;
    for (g = 0; g < maxGroups; g++)
    {
      if (groupArray[g].rm_so == (long long)(size_t)-1)
        break; // No more groups

      char cursorCopy[strlen(cursor) + 1];
      strcpy(cursorCopy, cursor);
      cursorCopy[groupArray[g].rm_eo] = 0;

      if (g == 0)
      {
        offset = groupArray[g].rm_eo;
        sprintf(regexRoute + strlen(regexRoute), "%.*s(.*)", (int)groupArray[g].rm_so, cursorCopy);
      }
      else
      {
        pm->keys = realloc(pm->keys, sizeof(char *) * (m + 1));
        pm->count++;
        char *key = malloc(sizeof(char) * (groupArray[g].rm_eo - groupArray[g].rm_so + 1));
        strncpy(key, cursorCopy + groupArray[g].rm_so, groupArray[g].rm_eo - groupArray[g].rm_so);
        key[groupArray[g].rm_eo - groupArray[g].rm_so] = '\0';
        pm->keys[m] = key;
      }
    }
    cursor += offset;
  }

  sprintf(regexRoute + strlen(regexRoute), "%s", cursor);

  regfree(&regexCompiled);

  pm->regexRoute = malloc(strlen(regexRoute) + 1);
  strcpy(pm->regexRoute, regexRoute);

  return pm;
}

void routeMatch(char *path, param_match_t *pm, char **values, int *match)
{
  char *source = path;
  char *regexString = pm->regexRoute;

  size_t maxMatches = 100;
  size_t maxGroups = 100;

  regex_t regexCompiled;
  regmatch_t groupArray[maxGroups];
  unsigned int m;
  char *cursor;

  if (regcomp(&regexCompiled, regexString, REG_EXTENDED))
  {
    printf("Could not compile regular expression.\n");
    return;
  };

  cursor = source;
  for (m = 0; m < maxMatches; m++)
  {
    if (regexec(&regexCompiled, cursor, maxGroups, groupArray, 0))
      break; // No more matches

    unsigned int g = 0;
    unsigned int offset = 0;
    for (g = 0; g < maxGroups; g++)
    {
      if (groupArray[g].rm_so == (long long)(size_t)-1)
        break; // No more groups

      if (g == 0)
      {
        offset = groupArray[g].rm_eo;
        *match = 1;
      }
      else
      {
        int index = g - 1;
        values[index] = malloc(sizeof(char) * (groupArray[g].rm_eo - groupArray[g].rm_so + 1));
        strncpy(values[index], cursor + groupArray[g].rm_so, groupArray[g].rm_eo - groupArray[g].rm_so);
        values[index][groupArray[g].rm_eo - groupArray[g].rm_so] = '\0';
      }

      char cursorCopy[strlen(cursor) + 1];
      strcpy(cursorCopy, cursor);
      cursorCopy[groupArray[g].rm_eo] = 0;
    }
    cursor += offset;
  }

  regfree(&regexCompiled);
}

typedef void (^requestHandler)(request_t *req, response_t *res);
typedef void (^middlewareHandler)(request_t *req, response_t *res, void (^next)());

typedef char * (^getHashBlock)(char *key);
static getHashBlock reqQueryFactory(request_t *req)
{
  return Block_copy(^(char *key) {
    return (char *)hash_get(req->queryHash, key);
  });
}

static getHashBlock reqGetHeaderFactory(request_t *req)
{
  return Block_copy(^(char *key) {
    return (char *)hash_get(req->headersHash, key);
  });
}

static getHashBlock reqParamFactory(request_t *req)
{
  return Block_copy(^(char *key) {
    return (char *)hash_get(req->paramsHash, key);
  });
}

static getHashBlock reqBodyFactory(request_t *req)
{
  req->bodyHash = hash_new();
  if (strncmp(req->method, "POST", 4) == 0)
  {
    char *copy = strdup(req->rawRequest);
    req->bodyString = strstr(copy, "\r\n\r\n");

    if (req->bodyString && strlen(req->bodyString) > 4)
    {
      req->bodyString += 4;
      if (strncmp(req->get("Content-Type"), "application/x-www-form-urlencoded", 33) == 0)
      {
        parseQueryString(req->bodyHash, req->bodyString);
      }
      else if (strncmp(req->get("Content-Type"), "application/json", 16) == 0)
      {
        printf("%s\n", req->bodyString);
      }
      else if (strncmp(req->get("Content-Type"), "multipart/form-data", 20) == 0)
      {
        printf("%s\n", req->bodyString);
      }
    }
    else
    {
      req->bodyString = "";
    }
    free(copy);
  }
  return Block_copy(^(char *key) {
    return (char *)hash_get(req->bodyHash, key);
  });
}

static char *buildResponseString(char *body, response_t *res)
{
  char *contentType = "text/html; charset=utf-8";
  char *contentLength = malloc(sizeof(char) * 20);
  sprintf(contentLength, "%zu", strlen(body));
  char *statusMessage = getStatusMessage(res->status);
  char *status = malloc(sizeof(char) * (strlen(statusMessage) + 5));
  sprintf(status, "%d %s", res->status, statusMessage);
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

typedef void (^sendBlock)(char *body);
static sendBlock sendFactory(client_t client, response_t *res)
{
  return Block_copy(^(char *body) {
    char *response = buildResponseString(body, res);
    write(client.socket, response, strlen(response));
    free(response);
  });
}

typedef void (^sendfBlock)(char *format, ...);
static sendfBlock sendfFactory(response_t *res)
{
  return Block_copy(^(char *format, ...) {
    char body[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(body, 4096, format, args);
    res->send(body);
    va_end(args);
  });
}

typedef void (^sendFileBlock)(char *body);
static sendFileBlock sendFileFactory(client_t client, request_t *req, response_t *res)
{
  return Block_copy(^(char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
      res->status = 404;
      res->sendf(errorHTML, req->path);
      return;
    }
    char *response = malloc(sizeof(char) * (strlen("HTTP/1.1 200 OK\r\nContent-Length: \r\n\r\n") + 20));
    // TODO: mimetype
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %zu\r\n\r\n", fileSize(path));
    write(client.socket, response, strlen(response));
    char *bufferfer = malloc(4096);
    size_t bytesRead = fread(bufferfer, 1, 4096, file);
    while (bytesRead > 0)
    {
      write(client.socket, bufferfer, bytesRead);
      bytesRead = fread(bufferfer, 1, 4096, file);
    }
    free(bufferfer);
    free(response);
    fclose(file);
  });
}

typedef struct app_t
{
  void (^get)(char *path, requestHandler);
  void (^post)(char *path, requestHandler);
  void (^listen)(int port, void (^handler)());
  void (^use)(middlewareHandler);
} app_t;

typedef struct route_handler_t
{
  char *method;
  char *path;
  int regex;
  param_match_t *paramMatch;
  requestHandler handler;
} route_handler_t;

typedef struct middleware_t
{
  char *path;
  middlewareHandler handler;
} middleware_t;

static route_handler_t *routeHandlers = NULL;
static int routeHandlerCount = 0;
static middleware_t *middlewares = NULL;
static int middlewareCount = 0;
static int servSock = -1;
static dispatch_queue_t serverQueue = NULL;

char *matchFilepath(request_t *req, char *path)
{
  regex_t regex;
  int reti;
  size_t nmatch = 2;
  regmatch_t pmatch[2];
  char *pattern = malloc(sizeof(char) * (strlen(path) + strlen("//(.*)") + 1));
  sprintf(pattern, "/%s/(.*)", path);
  char *bufferfer = malloc(sizeof(char) * (strlen(req->url) + 1));
  strcpy(bufferfer, req->path);
  reti = regcomp(&regex, pattern, REG_EXTENDED);
  if (reti)
  {
    fprintf(stderr, "Could not compile regex\n");
    exit(6);
  }
  reti = regexec(&regex, bufferfer, nmatch, pmatch, 0);
  if (reti == 0)
  {
    char *fileName = bufferfer + pmatch[1].rm_so;
    fileName[pmatch[1].rm_eo - pmatch[1].rm_so] = 0;
    char *file_path = malloc(sizeof(char) * (strlen(fileName) + strlen(".//") + strlen(path) + 1));
    sprintf(file_path, "./%s/%s", path, fileName);
    return file_path;
  }
  else
  {
    return NULL;
  }
}

static void initRouteHandlers()
{
  routeHandlers = malloc(sizeof(route_handler_t));
}

static void addRouteHandler(char *method, char *path, requestHandler handler)
{
  int regex = strchr(path, ':') != NULL;
  routeHandlers = realloc(routeHandlers, sizeof(route_handler_t) * (routeHandlerCount + 1));
  route_handler_t routeHandler = {
      .method = method,
      .path = path,
      .regex = regex,
      .paramMatch = regex ? paramMatch(path) : NULL,
      .handler = handler,
  };
  routeHandlers[routeHandlerCount++] = routeHandler;
}

static void initMiddlewareHandlers()
{
  middlewares = malloc(sizeof(middleware_t));
}

static void addMiddlewareHandler(middlewareHandler handler)
{
  middlewares = realloc(middlewares, sizeof(middleware_t) * (middlewareCount + 1));
  middlewares[middlewareCount++] = (middleware_t){.handler = handler};
}

static void runMiddleware(int index, request_t *req, response_t *res, void (^next)())
{
  if (index < middlewareCount)
  {
    middlewares[index].handler(req, res, ^{
      runMiddleware(index + 1, req, res, next);
    });
  }
  else
  {
    next();
  }
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
  request_t req = {.url = NULL, .queryString = "", .path = NULL, .method = NULL, .rawRequest = NULL};

  char buffer[4096];
  memset(buffer, 0, sizeof(buffer));
  char *method, *url;
  int parseBytes, minorVersion;
  struct phr_header headers[100];
  size_t bufferLen = 0, prevBufferLen = 0, methodLen, urlLen, numHeaders;
  ssize_t readBytes;

  // TODO: timeout support
  while (1)
  {
    while ((readBytes = read(client.socket, buffer + bufferLen, sizeof(buffer) - bufferLen)) == -1)
      ;
    if (readBytes <= 0)
      return req;
    prevBufferLen = bufferLen;
    bufferLen += readBytes;
    numHeaders = sizeof(headers) / sizeof(headers[0]);
    parseBytes = phr_parse_request(buffer, bufferLen, (const char **)&method, &methodLen, (const char **)&url, &urlLen,
                                   &minorVersion, headers, &numHeaders, prevBufferLen);
    if (parseBytes > 0)
    {
      if (method[0] == 'P' && parseBytes == readBytes)
        while ((read(client.socket, buffer + bufferLen, sizeof(buffer) - bufferLen)) == -1)
          ;
      break;
    }
    else if (parseBytes == -1)
      return req;
    assert(parseBytes == -2);
    if (bufferLen == sizeof(buffer))
      return req;
  }

  req.rawRequest = buffer;

  req.headersHash = hash_new();
  for (size_t i = 0; i != numHeaders; ++i)
  {
    char *key = malloc(headers[i].name_len + 1);
    sprintf(key, "%.*s", (int)headers[i].name_len, headers[i].name);
    char *value = malloc(headers[i].value_len + 1);
    sprintf(value, "%.*s", (int)headers[i].value_len, headers[i].value);
    hash_set(req.headersHash, key, value);
  }

  req.method = malloc(methodLen + 1);
  memcpy(req.method, method, methodLen);
  req.method[methodLen] = '\0';

  req.url = malloc(urlLen + 1);
  memcpy(req.url, url, urlLen);
  req.url[urlLen] = '\0';

  char *copy = strdup(req.url);
  char *queryStringStart = strchr(copy, '?');

  if (queryStringStart)
  {
    int queryStringLen = strlen(queryStringStart + 1);
    req.queryString = malloc(queryStringLen + 1);
    memcpy(req.queryString, queryStringStart + 1, queryStringLen);
    req.queryString[queryStringLen] = '\0';
    *queryStringStart = '\0';
    req.queryHash = hash_new();
    parseQueryString(req.queryHash, req.queryString);
  }

  req.path = malloc(strlen(copy) + 1);
  memcpy(req.path, copy, strlen(copy));
  req.path[strlen(copy)] = '\0';

  req.get = reqGetHeaderFactory(&req);
  req.query = reqQueryFactory(&req);
  req.body = reqBodyFactory(&req);

  free(copy);

  return req;
}

static route_handler_t matchRouteHandler(request_t *req)
{
  for (int i = 0; i < routeHandlerCount; i++)
  {
    if (strcmp(routeHandlers[i].method, req->method) != 0)
      continue;
    param_match_t *pm = routeHandlers[i].paramMatch;
    if (pm != NULL)
    {
      char *values[pm->count];
      int match = 0;
      routeMatch(req->path, pm, values, &match);
      if (match)
      {
        req->paramsHash = hash_new();
        for (int i = 0; i < pm->count; i++)
        {
          hash_set(req->paramsHash, pm->keys[i], values[i]);
        }
        req->param = reqParamFactory(req);
        match = 0;
        return routeHandlers[i];
      }
    }

    if (strcmp(routeHandlers[i].method, req->method) == 0 && strcmp(routeHandlers[i].path, req->path) == 0)
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
  hash_free(req.queryHash);
  hash_free(req.headersHash);
  hash_free(req.paramsHash);
  hash_free(req.bodyHash);
  Block_release(req.get);
  Block_release(req.query);
  Block_release(req.param);
  if (strlen(req.queryString) > 0)
    free(req.queryString);
}

static void closeClientConnection(client_t client, request_t req)
{
  shutdown(client.socket, SHUT_RDWR);
  close(client.socket);
  freeRequest(req);
}

static response_t buildResponse(client_t client, request_t *req)
{
  response_t res;
  res.status = 200;
  res.send = sendFactory(client, &res);
  res.sendf = sendfFactory(&res);
  res.sendFile = sendFileFactory(client, req, &res);
  return res;
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

      request_t req = parseRequest(client);

      if (req.method == NULL)
      {
        closeClientConnection(client, req);
        return;
      }

      __block response_t res = buildResponse(client, &req);

      runMiddleware(0, &req, &res, ^{
        route_handler_t routeHandler = matchRouteHandler((request_t *)&req);
        if (routeHandler.handler == NULL)
        {
          res.status = 404;
          res.sendf(errorHTML, req.path);
        }
        else
        {
          routeHandler.handler((request_t *)&req, (response_t *)&res);
        }
      });

      closeClientConnection(client, req);
    }
  });

  dispatch_resume(acceptSource);
}

static void closeServer()
{
  printf("\nClosing server...\n");
  free(routeHandlers);
  free(middlewares);
  close(servSock);
  dispatch_release(serverQueue);
  exit(EXIT_SUCCESS);
}

app_t express()
{
  initMiddlewareHandlers();
  initRouteHandlers();
  initServerQueue();
  initServerSocket();

  if (signal(SIGINT, closeServer) == SIG_ERR)
    ;

  app_t app;

  app.listen = ^(int port, void (^handler)()) {
    initServerListen(port);
    initClientAcceptEventHandler();
    handler();
    dispatch_main();
  };

  app.get = ^(char *path, requestHandler handler) {
    addRouteHandler("GET", path, handler);
  };

  app.post = ^(char *path, requestHandler handler) {
    addRouteHandler("POST", path, handler);
  };

  app.use = ^(middlewareHandler handler) {
    addMiddlewareHandler(handler);
  };

  return app;
};

middlewareHandler expressStatic(char *path)
{
  return Block_copy(^(request_t *req, response_t *res, void (^next)()) {
    char *filePath = matchFilepath(req, path);
    if (filePath != NULL)
    {
      res->sendFile(filePath);
      free(filePath);
    }
    else
    {
      next();
    }
  });
}

void runTests()
{
  testEq("root", curlGet("/"), "Hello World!");
  testEq("basic route", curlGet("/test"), "Testing, testing!");
  testEq("query string", curlGet("/qs\?value1=123\\&value2=345"), "<h1>Query String</h1><p>Value 1: 123</p><p>Value 2: 345</p>");
  testEq("headers", curlGet("/headers"), "<h1>Headers</h1><p>Host: 127.0.0.1:3000</p><p>Accept: */*</p>");
  testEq("route params", curlGet("/one/123/two/345/567.jpg"), "<h1>Params</h1><p>One: 123</p><p>Two: 345</p><p>Three: 567</p>");
  testEq("send file", curlGet("/file"), "hello, world!\n");
  testEq("static file middleware", curlGet("/files/test2.txt"), "this is a test!!!");
  testEq("form data", curlPost("/post/form123", "param1=123&param2=345"), "<h1>Form</h1><p>Param 1: 123</p><p>Param 2: 345</p>");
  exit(EXIT_SUCCESS);
}

int main()
{
  app_t app = express();
  int port = 3000;

  app.use(expressStatic("files"));

  app.use(^(UNUSED request_t *req, UNUSED response_t *res, void (^next)()) {
    next();
  });

  app.get("/", ^(UNUSED request_t *req, response_t *res) {
    res->send("Hello World!");
  });

  app.get("/test", ^(UNUSED request_t *req, response_t *res) {
    res->status = 201;
    res->send("Testing, testing!");
  });

  app.get("/qs", ^(request_t *req, response_t *res) {
    res->sendf("<h1>Query String</h1><p>Value 1: %s</p><p>Value 2: %s</p>", req->query("value1"), req->query("value2"));
  });

  app.get("/headers", ^(request_t *req, response_t *res) {
    res->sendf("<h1>Headers</h1><p>Host: %s</p><p>Accept: %s</p>", req->get("Host"), req->get("Accept"));
  });

  app.get("/file", ^(UNUSED request_t *req, response_t *res) {
    res->sendFile("./files/test.txt");
  });

  app.get("/one/:one/two/:two/:three.jpg", ^(request_t *req, response_t *res) {
    res->sendf("<h1>Params</h1><p>One: %s</p><p>Two: %s</p><p>Three: %s</p>", req->param("one"), req->param("two"), req->param("three"));
  });

  app.get("/form", ^(UNUSED request_t *req, response_t *res) {
    res->send("<form method=\"POST\" action=\"/post/new\">"
              "  <input type=\"text\" name=\"param1\">"
              "  <input type=\"text\" name=\"param2\">"
              "  <input type=\"submit\" value=\"Submit\">"
              "</form>");
  });

  app.post("/post/:form", ^(request_t *req, response_t *res) {
    res->status = 201;
    res->sendf("<h1>Form</h1><p>Param 1: %s</p><p>Param 2: %s</p>", req->body("param1"), req->body("param2"));
  });

  app.listen(port, ^{
    printf("Example app listening at http://localhost:%d\n", port);
    runTests();
  });

  return 0;
}
