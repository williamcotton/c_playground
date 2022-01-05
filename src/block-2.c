#include <stdio.h>
#include <dispatch/dispatch.h>
#include <Block.h>

typedef void (^requestHandler)(char *path);

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

  void (^_get)(char *, void (^)(char *)) = ^(char *path, requestHandler handler) {
    b = Block_copy(^{
      handler(path);
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

  app.get("/", ^(char *path) {
    printf("path test 12: %s\n", path);
  });

  app.listen(port, ^{
    printf("Example app listening at http://localhost:%d\n", port);
  });

  return 0;
}
