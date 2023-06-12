#include <stdio.h>
#include <Block.h>

typedef void (^printBlock)(void);

printBlock print() {
  return Block_copy(^(void) {
    printf("Hello, Blocks!!\n");
  });
};
  
int main() {
  print()();
}
