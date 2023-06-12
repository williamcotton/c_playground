#include <stdio.h>

int add(int a, int b) {
  int c;
  c = a + b;
  return c;
};
  
int main() {
  int x, y, z;
  x = 1;
  y = 2;
  z = add(x, y);
  printf("%d + %d = %d\n", x, y, z);
}
