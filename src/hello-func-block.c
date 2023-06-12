#include <stdio.h>

typedef void (^printBlock)(void);

printBlock print() {
  return ^(void) {
    printf("Hello, Blocks!!\n");
  };
};
  
int main() {
  print()();
}
