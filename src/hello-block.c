#include <stdio.h>

int main() {

  void (^print)(void) = ^(void) {
    printf("Hello, Blocks!!\n");
  };

  print();
}
