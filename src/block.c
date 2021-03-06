#include <stdio.h>
#include <dispatch/dispatch.h>

int with_callback(int (^callback)(int x))
{
  return callback(42);
}

int main()
{
  int d = 5;

  void (^hello)(void) = ^(void) {
    printf("Hello, block!\n");
  };

  int (^add)(int, int) = ^(int a, int b) {
    hello();
    return a + b + d;
  };

  char * (^str)(int) = ^(int a) {
    return "Hello, block 123!";
  };

  printf("%s", str(42));

  printf("%d\n", add(1, 2));

  hello();

  dispatch_queue_t myCustomQueue;
  myCustomQueue = dispatch_queue_create("com.example.MyCustomQueue", NULL);

  dispatch_async(myCustomQueue, ^{
    printf("Do some work here.\n");
    printf("%d\n", add(1, 2));
  });

  printf("The first block may or may not have run.\n");

  dispatch_async(myCustomQueue, ^{
    printf("Do some more work here.\n");
  });
  printf("Neither blocks have completed.\n");

  sleep(1);

  dispatch_async(myCustomQueue, ^{
    printf("Do some work here.\n");
  });

  printf("The first block may or may not have run.\n");

  dispatch_sync(myCustomQueue, ^{
    printf("Do some more work here.\n");
  });
  printf("Both blocks have completed.\n");

  int count = 30;

  dispatch_apply(count, myCustomQueue, ^(size_t i) {
    printf("s: %zu\n", i);
  });

  for (int i = 0; i < count; i++)
  {
    dispatch_async(myCustomQueue, ^{
      printf("a: %d\n", i);
    });
  }

  printf("All iterations have completed.\n");

  dispatch_sync(myCustomQueue, ^{
    printf("Do some more work here.\n");
  });

  dispatch_release(myCustomQueue);

  printf("with_callback: %d\n", with_callback(^(int x) {
           return x + 1;
         }));

  return 0;
}
