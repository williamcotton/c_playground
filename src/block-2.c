#include <stdio.h>
#include <dispatch/dispatch.h>
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

dispatch_block_t b;

app_t express()
{
  void (^_listen)(int, void (^)()) = ^(int port, void (^handler)()) {
    handler();
    b();
  };

  void (^_get)(char *, void (^)(request_t, response_t)) = ^(char *path, requestHandler handler) {
    request_t req;
    response_t res;
    res.send = ^(char *data) {
      printf("%s\n", data);
    };
    req.path = path;
    b = Block_copy(^{
      handler(req, res);
    });
  };

  app_t app = {.get = _get, .listen = _listen};
  return app;
};

int main()
{
  __block dispatch_block_t _b;

  void (^listen)(int, void (^)()) = ^(int port, void (^callback)()) {
    _b = Block_copy(^{
      callback(port);
    });
  };

  dispatch_block_t block = ^{
    printf("Hello, World!\n");
  };

  listen(80, ^(int port) {
    printf("test 123 - %d\n", port);
  });

  void (^call)(void) = ^{
    printf("%d\n", 123);
    _b();
  };

  block();

  call();

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
