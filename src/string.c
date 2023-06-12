#include <Block.h>
#include <ctype.h>
#include <dispatch/dispatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct string_t;
struct string_collection_t;
struct string_t *string(const char *str);
struct string_collection_t *stringCollection(size_t size,
                                             struct string_t **arr);
typedef void (^eachCallback)(struct string_t *string);
typedef void (^eachWithIndexCallback)(struct string_t *string, int index);
typedef void * (^reducerCallback)(void *accumulator, struct string_t *string);
typedef void * (^mapCallback)(struct string_t *string);

typedef struct string_collection_t {
  size_t size;
  struct string_t **arr;
  void (^each)(eachCallback);
  void (^eachWithIndex)(eachWithIndexCallback);
  void * (^reduce)(void *accumulator, reducerCallback);
  void ** (^map)(mapCallback);
  void (^reverse)(void);
  void (^sort)(void);
  void (^free)(void);
  int (^indexOf)(const char *str);
  struct string_t * (^join)(const char *delim);
} string_collection_t;

typedef struct string_t {
  char *str;
  size_t size;
  void (^print)(void);
  struct string_t * (^concat)(const char *str);
  struct string_t * (^upcase)(void);
  struct string_t * (^downcase)(void);
  struct string_t * (^capitalize)(void);
  struct string_t * (^reverse)(void);
  struct string_t * (^trim)(void);
  string_collection_t * (^split)(const char *delim);
  struct string_t * (^replace)(const char *str1, const char *str2);
  struct string_t * (^chomp)(void);
  struct string_t * (^slice)(size_t start, size_t length);
  struct string_t * (^delete)(const char *str);
  int (^indexOf)(const char *str);
  int (^lastIndexOf)(const char *str);
  int (^eql)(const char *str);
  int (^to_i)(void);
  void (^free)(void);
} string_t;

string_collection_t *stringCollection(size_t size, string_t **arr) {
  string_collection_t *collection = malloc(sizeof(string_collection_t));
  collection->size = size;
  collection->arr = arr;

  collection->each = Block_copy(^(eachCallback callback) {
    for (size_t i = 0; i < collection->size; i++) {
      callback(collection->arr[i]);
    }
  });

  collection->eachWithIndex = Block_copy(^(eachWithIndexCallback callback) {
    for (size_t i = 0; i < collection->size; i++) {
      callback(collection->arr[i], i);
    }
  });

  collection->reduce =
      Block_copy(^(void *accumulator, reducerCallback reducer) {
        for (size_t i = 0; i < collection->size; i++) {
          accumulator = reducer(accumulator, collection->arr[i]);
        }
        return accumulator;
      });

  collection->map = Block_copy(^(mapCallback callback) {
    void *arr[collection->size];
    for (size_t i = 0; i < collection->size; i++) {
      arr[i] = callback(collection->arr[i]);
    }
    return arr;
  });

  collection->reverse = Block_copy(^(void) {
    for (size_t i = 0; i < collection->size / 2; i++) {
      struct string_t *tmp = collection->arr[i];
      collection->arr[i] = collection->arr[collection->size - i - 1];
      collection->arr[collection->size - i - 1] = tmp;
    }
  });

  collection->join = Block_copy(^(const char *delim) {
    size_t len = 0;
    for (size_t i = 0; i < collection->size; i++) {
      len += collection->arr[i]->size;
    }
    len += collection->size - 1;
    char *str = malloc(len + 1);
    str[0] = '\0';
    for (size_t i = 0; i < collection->size; i++) {
      strcat(str, collection->arr[i]->str);
      if (i < collection->size - 1) {
        strcat(str, delim);
      }
    }
    return string(str);
  });

  collection->indexOf = Block_copy(^(const char *str) {
    for (int i = 0; i < collection->size; i++) {
      if (strcmp(collection->arr[i]->str, str) == 0) {
        return i;
      }
    }
    return -1;
  });

  collection->free = Block_copy(^(void) {
    for (size_t i = 0; i < collection->size; i++) {
      collection->arr[i]->free();
    }
    free(collection->arr);
    Block_release(collection->each);
    Block_release(collection->eachWithIndex);
    Block_release(collection->reduce);
    Block_release(collection->map);
    Block_release(collection->reverse);
    Block_release(collection->join);
    dispatch_async(dispatch_get_main_queue(), ^() {
      Block_release(collection->free);
      free(collection);
    });
  });

  return collection;
}

string_t *string(const char *str) {
  string_t *s = malloc(sizeof(string_t));
  s->str = strdup(str);
  s->size = strlen(str);

  s->print = Block_copy(^(void) {
    printf("%s\n", s->str);
  });

  s->concat = Block_copy(^(const char *str) {
    size_t size = s->size + strlen(str);
    char *new_str = malloc(size + 1);
    strcpy(new_str, s->str);
    strcat(new_str, str);
    free(s->str);
    s->str = new_str;
    s->size = size;
    return s;
  });

  s->upcase = Block_copy(^(void) {
    for (size_t i = 0; i < s->size; i++) {
      s->str[i] = toupper(s->str[i]);
    }
    return s;
  });

  s->downcase = Block_copy(^(void) {
    for (size_t i = 0; i < s->size; i++) {
      s->str[i] = tolower(s->str[i]);
    }
    return s;
  });

  s->capitalize = Block_copy(^(void) {
    for (size_t i = 0; i < s->size; i++) {
      if (i == 0) {
        s->str[i] = toupper(s->str[i]);
      } else {
        s->str[i] = tolower(s->str[i]);
      }
    }
    return s;
  });

  s->reverse = Block_copy(^(void) {
    char *new_str = malloc(s->size + 1);
    for (size_t i = 0; i < s->size; i++) {
      new_str[s->size - i - 1] = s->str[i];
    }
    new_str[s->size] = '\0';
    free(s->str);
    s->str = new_str;
    return s;
  });

  s->trim = Block_copy(^(void) {
    size_t start = 0;
    size_t end = s->size - 1;
    while (isspace(s->str[start])) {
      start++;
    }
    while (isspace(s->str[end])) {
      end--;
    }
    size_t size = end - start + 1;
    char *new_str = malloc(size + 1);
    strncpy(new_str, s->str + start, size);
    new_str[size] = '\0';
    free(s->str);
    s->str = new_str;
    s->size = size;
    return s;
  });

  s->split = Block_copy(^(const char *delim) {
    string_collection_t *collection = stringCollection(0, NULL);
    collection->size = 0;
    collection->arr = malloc(sizeof(string_t *));
    char *token = strtok(s->str, delim);
    while (token != NULL) {
      collection->size++;
      collection->arr =
          realloc(collection->arr, collection->size * sizeof(string_t *));
      collection->arr[collection->size - 1] = string(token);
      token = strtok(NULL, delim);
    }
    return collection;
  });

  s->to_i = Block_copy(^(void) {
    return atoi(s->str);
  });

  s->replace = Block_copy(^(const char *str1, const char *str2) {
    char *new_str = malloc(s->size + 1);
    size_t i = 0;
    size_t j = 0;
    while (i < s->size) {
      if (strncmp(s->str + i, str1, strlen(str1)) == 0) {
        strcpy(new_str + j, str2);
        i += strlen(str1);
        j += strlen(str2);
      } else {
        new_str[j] = s->str[i];
        i++;
        j++;
      }
    }
    new_str[j] = '\0';
    free(s->str);
    s->str = new_str;
    s->size = j;
    return s;
  });

  s->chomp = Block_copy(^(void) {
    if (s->str[s->size - 1] == '\n') {
      s->str[s->size - 1] = '\0';
      s->size--;
    }
    return s;
  });

  s->slice = Block_copy(^(size_t start, size_t length) {
    if (start >= s->size) {
      return string("");
    }
    if (start + length > s->size) {
      length = s->size - start;
    }
    char *new_str = malloc(length + 1);
    strncpy(new_str, s->str + start, length);
    new_str[length] = '\0';
    return string(new_str);
  });

  s->indexOf = Block_copy(^(const char *str) {
    for (int i = 0; i < s->size; i++) {
      if (strncmp(s->str + i, str, strlen(str)) == 0) {
        return i;
      }
    }
    return -1;
  });

  s->lastIndexOf = Block_copy(^(const char *str) {
    for (int i = s->size - 1; i > 0; i--) {
      if (strncmp(s->str + i, str, strlen(str)) == 0) {
        return i;
      }
    }
    return -1;
  });

  s->eql = Block_copy(^(const char *str) {
    return strcmp(s->str, str) == 0;
  });

  s->free = Block_copy(^(void) {
    free(s->str);
    Block_release(s->print);
    Block_release(s->concat);
    Block_release(s->upcase);
    Block_release(s->downcase);
    Block_release(s->capitalize);
    Block_release(s->reverse);
    Block_release(s->trim);
    Block_release(s->split);
    Block_release(s->to_i);
    Block_release(s->replace);
    Block_release(s->chomp);
    Block_release(s->slice);
    dispatch_async(dispatch_get_main_queue(), ^() {
      Block_release(s->free);
      free(s);
    });
  });
  return s;
}

int main() {
  string_t *s = string("Hello");
  s->print();

  s->concat(" World");
  s->print();

  s->concat("!");
  s->print();

  s->upcase();
  s->print();

  s->downcase();
  s->print();

  s->capitalize();
  s->print();

  s->reverse();
  s->print();

  s->trim();
  s->print();

  s->free();

  string_t *s2 = string("one,two,three");
  string_collection_t *c = s2->split(",");
  s2->free();
  c->each(^(string_t *string) {
    string->print();
  });

  printf("%d %d %d\n", c->indexOf("one"), c->indexOf("two"),
         c->indexOf("three"));

  c->reverse();
  c->each(^(string_t *string) {
    string->print();
  });

  string_t *s3 = c->join("-");
  s3->print();

  s3->replace("three", "four");
  s3->print();

  s3->replace("-", "=");
  s3->print();

  string_t *s4 = s3->slice(5, 3);
  s3->free();
  s4->print();
  s4->free();

  string_t *s5 = string("");
  c->reduce((void *)s5, ^(void *accumulator, string_t *str) {
    string_t *acc = (string_t *)accumulator;
    acc->concat(str->reverse()->str)->concat("+");
    return (void *)acc;
  });
  s5->print();
  s5->free();

  string_t **sa = (string_t **)c->map(^(string_t *str) {
    return (void *)str->concat("!!!")->reverse()->concat("!!!");
  });
  for (size_t i = 0; i < c->size; i++) {
    sa[i]->print();
  }
  c->free();
}