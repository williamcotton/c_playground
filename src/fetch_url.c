#include <stdio.h>
#include <stdlib.h>
#include <dotenv-c/dotenv.h>
// #include "deps/dotenv-c/dotenv.h"
#include "helpers.h"

// Make sure to install libdotenv:
// /usr/local/lib/libdotenv.so
// /usr/local/include/dotenv.h

int main(int argc, char **argv)
{
  // get session from .env file.
  env_load(".", false);
  char *session = getenv("SESSION");

  // get url from command line.
  if (argc < 2)
  {
    printf("need url to make http request.\n");
    return 0;
  }
  char *url = argv[1];

  printf("trying to make http request to: %s\n", url);
  struct memory_struct m;
  http_get(url, session, &m);
  // printf("\nresponse:\n%s", m.buffer);

  int newlines = count_of_newlines_in_string(m.buffer);
  char *array[newlines];
  process_array(m.buffer, array);

  print_all_elements_in_array(array);

  // print length of array
  printf("\nlength of array: %d\n", length_of_array(array));
  return 0;
}
