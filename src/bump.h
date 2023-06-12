// a c header file for a bump allocator

#ifndef BUMP_H
#define BUMP_H

#include <stdio.h>
#include <stdlib.h>

// a bump allocator
struct bump {
  void *start;
  void *end;
  void *cur;
};
void bump_init(struct bump *b, size_t size);

void *bump_alloc(struct bump *b, size_t size);

void *bump_realloc(struct bump *b, void *p, size_t size);

void bump_free(struct bump *b);

#endif // BUMP_H