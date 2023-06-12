#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct fib_args {
  int n;
  unsigned long long result;
};

void *fib_runner(void *arg) {
  struct fib_args *args = arg;

  if (args->n == 0)
    args->result = 0;
  else if (args->n == 1)
    args->result = 1;
  else {
    unsigned long long prevPrev = 0, prev = 1, result = 0;
    for (int i = 2; i <= args->n; i++) {
      result = prev + prevPrev;
      prevPrev = prev;
      prev = result;
    }
    args->result = result;
  }
  pthread_exit(0);
}

#define NUM_THREADS 5

int main() {
  pthread_t threads[NUM_THREADS];
  struct fib_args args[NUM_THREADS];

  // launch the threads
  for (int i = 0; i < NUM_THREADS; i++) {
    args[i].n = i * 10; // calculate Fibonacci numbers for n = 0, 10, 20, 30, 40
    pthread_create(&threads[i], NULL, fib_runner, &args[i]);
  }

  // wait for the threads to finish and print their results
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
    printf("Fibonacci number at position %d is %llu\n", args[i].n,
           args[i].result);
  }

  return 0;
}
