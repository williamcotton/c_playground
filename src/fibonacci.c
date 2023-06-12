#include <stdio.h>

unsigned long long fibonacci(int n) {
  if (n < 0) {
    printf("Input should be a positive integer\n");
    return -1;
  }

  if (n == 0)
    return 0;
  else if (n == 1)
    return 1;

  unsigned long long prevPrev = 0, prev = 1, result = 0;
  for (int i = 2; i <= n; i++) {
    result = prev + prevPrev;
    prevPrev = prev;
    prev = result;
  }
  return result;
}

int main() {
  int n = 40;
  printf("Fibonacci number at position %d is %llu\n", n, fibonacci(n));
  return 0;
}
