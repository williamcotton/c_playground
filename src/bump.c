#include "bump.h"
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

void bump_init(struct bump *b, size_t size) {
  b->start = mmap(NULL, size, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  b->end = (char *)b->start + size;
  b->cur = b->start;
}

void *bump_alloc(struct bump *b, size_t size) {
  void *p = b->cur;
  b->cur = (char *)b->cur + size;
  if (b->cur > b->end) {
    return NULL;
  }
  return p;
}

void *bump_realloc(struct bump *b, void *p, size_t size) {
  void *q = bump_alloc(b, size);
  if (q == NULL) {
    return NULL;
  }
  memmove(q, p, size);
  return q;
}

void bump_free(struct bump *b) {
  munmap(b->start, (char *)b->end - (char *)b->start);
}

int main() {
  struct bump b;
  bump_init(&b, 1024);
  char *p = bump_alloc(&b, 100);
  strcpy(p, "hello");
  printf("%s\n", p);
  char *q = bump_realloc(&b, p, 200);
  strcpy(q, "world");
  printf("%s\n", q);
  bump_free(&b);
  return 0;
}