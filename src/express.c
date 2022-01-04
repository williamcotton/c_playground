#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <dispatch/dispatch.h>
#include <regex.h>
#include "server-helpers.h"

typedef struct
{
  char *path;
} request_t;

typedef struct
{
  void (^send)(char *);
} response_t;

typedef struct
{
  void (^get)(char *path, void (^handler)(request_t *req, response_t *res));
  void (^listen)(int port, void (^handler)());
} app;

app express()
{

  __block int servSock = -1;
  if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    printf("socket() failed");
  }

  int flag = 1;
  if (-1 == setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
  {
    perror("setsockopt fail");
  }

  void (^_listen)(int, void (^)()) = ^(int port, void (^handler)()) {
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

    if (bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
      printf("bind() failed");
    }

    if (listen(servSock, 5) < 0)
    {
      printf("listen() failed");
    }

    handler();
  };

  void (^_get)(char *, void (^)(request_t *, response_t *)) = ^(char *path, void (^handler)(request_t *req, response_t *res)) {
    request_t *req = malloc(sizeof(request_t));
    response_t *res = malloc(sizeof(response_t));
    res->send = ^(char *data) {
      printf("%s\n", data);
    };
    req->path = path;
    handler(req, res);
    free(req);
    free(res);
  };

  app app = {
      .get = _get,
      .listen = _listen};
  return app;
}

// const express = require('express')
// const app = express()
// const port = 3000

// app.get('/', (req, res) => {
//   res.send('Hello World!')
// })

// app.listen(port, () => {
//   console.log(`Example app listening at http://localhost:${port}`)
// })

int main()
{
  app app = express();
  int port = 3000;

  app.get("/", ^(request_t *req, response_t *res) {
    res->send("Hello World!");
  });

  app.listen(port, ^() {
    printf("Example app listening at http://localhost:%d\n", port);
  });

  return 0;
}