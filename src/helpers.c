#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <dotenv-c/dotenv.h>
#include "helpers.h"

int compare_int(const void *a, const void *b)
{
  return *(int *)a - *(int *)b;
}

int compare_double(const void *a, const void *b)
{
  double da = *(double *)a;
  double db = *(double *)b;

  if (da > db)
    return 1;
  else if (da < db)
    return -1;
  else
    return 0;
}

int compare_strings(const void *a, const void *b)
{
  return *(char *)a - *(char *)b;
}

// write response data to the memory buffer.
static size_t
mem_write(void *contents, size_t size, size_t nmemb, void *userp)
{
  // initialize memory_struct
  size_t realsize = size * nmemb;
  struct memory_struct *mem = (struct memory_struct *)userp;

  char *ptr = realloc(mem->buffer, mem->size + realsize + 1);
  if (ptr == NULL)
  {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  // copy the response contents to memory_struct buffer.
  mem->buffer = ptr;
  memcpy(&(mem->buffer[mem->size]), contents, realsize); // NOLINT
  mem->size += realsize;
  mem->buffer[mem->size] = 0;

  // return the size of content that is copied.
  return realsize;
}

void http_get(char *url, char *session, struct memory_struct *mem)
{
  CURL *curl_handle;
  CURLcode res;

  mem->buffer = malloc(1);
  mem->size = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  // initialize curl
  curl_handle = curl_easy_init();

  char *cookie_string;
  cookie_string = malloc(strlen("session=") + strlen(session) + strlen(";"));
  strlcpy(cookie_string, "session=", 8);
  strlcat(cookie_string, session, strlen(session) + strlen(cookie_string));

  // specify url, callback function to receive response, buffer to hold
  // response and lastly user agent for http request.
  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  curl_easy_setopt(curl_handle, CURLOPT_COOKIE, cookie_string);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, mem_write);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)mem);
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "custom-agent");

  // make the http request.
  res = curl_easy_perform(curl_handle);

  // check for errors.
  if (res != CURLE_OK)
  {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
  }

  // cleanup
  free(cookie_string);
  curl_easy_cleanup(curl_handle);
  curl_global_cleanup();
}

char *fetch(char *url)
{
  env_load(".", false);
  char *session = getenv("SESSION");
  struct memory_struct mem;
  http_get(url, session, &mem);
  return mem.buffer;
}

void resize_int_array(int **array, int *size, int new_size)
{
  int *new_array = realloc(*array, new_size * sizeof(int));
  if (new_array == NULL)
  {
    fprintf(stderr, "Error: realloc failed\n");
    exit(1);
  }
  *array = new_array;
  *size = new_size;
}

int *comma_seperated_string_to_integer_array(char *input, int count)
{
  char *copy = strdup(input);
  int *numbers = malloc(sizeof(int) * count);
  int i = 0;
  char *token = strtok(copy, ",");
  free(copy);
  while (token != NULL)
  {
    numbers[i] = atoi(token);
    token = strtok(NULL, ",");
    i++;
  }
  return numbers;
}

char *string(char *input)
{
  int len = strlen(input) + 1;
  char *output = NULL;
  output = malloc(sizeof(char) * (len));
  strlcpy(output, input, len);
  return output;
}

// convert a string to an array of strings.
void process_array(char *str, char **array)
{
  char *token;
  char *delimiter = "\n";
  int i = 0;

  // get the first token
  token = strtok(str, delimiter);

  // walk through other tokens
  while (token != NULL)
  {
    array[i] = token;
    token = strtok(NULL, delimiter);
    i++;
  }
  array[i] = NULL;
}

void print_first_element_of_array(char **array)
{
  printf("%s\n", array[0]);
}

void print_all_elements_in_array(char **array)
{
  int i = 0;
  while (array[i] != NULL)
  {
    printf("%s\n", array[i]);
    i++;
  }
}

int length_of_array(char **array)
{
  int i = 0;
  while (array[i] != NULL)
  {
    i++;
  }
  return i;
}

int count_of_newlines_in_string(char *str)
{
  int count = 0;
  char *p = str;
  while (*p != '\0')
  {
    if (*p == '\n')
    {
      count++;
    }
    p++;
  }
  return count;
}

int count_of_commas_in_string(char *str)
{
  int count = 0;
  char *p = str;
  while (*p != '\0')
  {
    if (*p == ',')
    {
      count++;
    }
    p++;
  }
  return count;
}

int split_count(char *str, char delimiter)
{
  int count = 1;
  char *p = str;
  while (*p != '\0')
  {
    if (*p == delimiter)
    {
      count++;
    }
    p++;
  }
  return count;
}

int *split_int(char *str, char delimiter)
{
  int count = split_count(str, delimiter);
  int *array = malloc(sizeof(int) * (count));
  char *p = str;
  int i = 0;
  while (*p != '\0')
  {
    if (*p == delimiter)
    {
      array[i] = atoi(str);
      str = p + 1;
      i++;
    }
    p++;
  }
  array[i] = atoi(str);
  return array;
}

void shift_left(long *array)
{
  int i;
  for (i = 0; i < 8; i++)
  {
    array[i] = array[i + 1];
  }
}

// given an array and an integer find the difference between the integer and each element in the array.
int *difference_array(int *array, int size, int value)
{
  int *differences = malloc(sizeof(int) * size);
  int i;
  for (i = 0; i < size; i++)
  {
    differences[i] = abs(array[i] - value);
  }
  return differences;
}

int sum_array(int *array, int size)
{
  int sum = 0;
  int i;
  for (i = 0; i < size; i++)
  {
    sum += array[i];
  }
  return sum;
}

int difference_sum(int *array, int size, int value)
{
  int sum = 0;
  int i;
  for (i = 0; i < size; i++)
  {
    sum += abs(array[i] - value);
  }
  return sum;
}

int triangular_number(int n)
{
  return n * (n + 1) / 2;
}

int difference_triangular(int *array, int size, int value)
{
  int sum = 0;
  int i;
  for (i = 0; i < size; i++)
  {
    sum += triangular_number(abs(array[i] - value));
  }
  return sum;
}

int max(int a, int b)
{
  return a > b ? a : b;
}

int min(int a, int b)
{
  return a < b ? a : b;
}

char **split(char *str, char *delim, int *count)
{
  char **result = NULL;
  char *token = NULL;
  int i = 0;

  token = strtok(str, delim);
  while (token != NULL)
  {
    result = realloc(result, sizeof(char *) * (i + 1));
    result[i] = token;
    i++;
    token = strtok(NULL, delim);
  }

  *count = i;
  return result;
}

// count the number of characters in a string
int count_characters(char *str)
{
  int count = 0;
  char *p = str;
  while (*p != '\0')
  {
    count++;
    p++;
  }
  return count;
}

// the characters not contained in two strings are returned in a new string.
char *difference_string(char *str1, char *str2)
{
  int i = 0;
  int j = 0;
  int count = 0;
  char *difference = malloc(sizeof(char) * (count_characters(str1) + 1));
  while (str1[i] != '\0')
  {
    int found = 0;
    j = 0;
    while (str2[j] != '\0')
    {
      if (str1[i] == str2[j])
      {
        found = 1;
        break;
      }
      j++;
    }
    if (found == 0)
    {
      difference[count] = str1[i];
      count++;
    }
    i++;
  }
  difference[count] = '\0';
  return difference;
}

// the characters contained in two strings are returned in a new string.
char *intersection_string(char *str1, char *str2)
{
  int i = 0;
  int j = 0;
  int count = 0;
  char *intersection = malloc(sizeof(char) * (count_characters(str1) + 1));
  while (str1[i] != '\0')
  {
    int found = 0;
    j = 0;
    while (str2[j] != '\0')
    {
      if (str1[i] == str2[j])
      {
        found = 1;
        break;
      }
      j++;
    }
    if (found == 1)
    {
      intersection[count] = str1[i];
      count++;
    }
    i++;
  }
  intersection[count] = '\0';
  return intersection;
}

int string_contains_all_characters(char *str, char *characters)
{
  int i = 0;
  int j = 0;
  int found = 0;
  while (str[i] != '\0')
  {
    j = 0;
    while (characters[j] != '\0')
    {
      if (str[i] == characters[j])
      {
        found = 1;
        break;
      }
      j++;
    }
    if (found == 0)
    {
      return 0;
    }
    i++;
  }
  return 1;
}

int string_contains(char *str, char character)
{
  int i = 0;
  int found = 0;
  while (str[i] != '\0')
  {
    if (str[i] == character)
    {
      found = 1;
      break;
    }
    i++;
  }
  return found;
}

int string_contains_all(char *str, char *characters)
{
  int i = 0;
  while (characters[i] != '\0')
  {
    if (!string_contains(str, characters[i]))
    {
      return 0;
    }
    i++;
  }
  return 1;
}

int strings_are_equal(char *str1, char *str2)
{
  if (strlen(str1) != strlen(str2))
  {
    return 0;
  }
  int i = 0;
  while (str1[i] != '\0')
  {
    if (str1[i] != str2[i])
    {
      return 0;
    }
    i++;
  }
  return 1;
}

char *insert_char(char *str, char replacement, int index)
{
  char *new_str = malloc(sizeof(char) * (strlen(str) + 2));
  char *repl = malloc(sizeof(char) * 2);
  sprintf(repl, "%c", replacement); // NOLINT
  strcpy(new_str, str);             // NOLINT
  strlcpy(new_str + index, repl, 2);
  strlcpy(new_str + index + 1, str + index, strlen(str) - index + 1); // NOLINT
  return new_str;
}

int count_of_char_in_string(char *str, char character)
{
  int count = 0;
  int i = 0;
  while (str[i] != '\0')
  {
    if (str[i] == character)
    {
      count++;
    }
    i++;
  }
  return count;
}

int find_index(char *line, int size, char c)
{
  int i;
  // int size = strlen(line);
  // printf("%d\n", size);
  for (i = 0; i < size; i++)
  {
    if (line[i] == c)
    {
      return i;
    }
  }
  return -1;
}

int max_in_array(int *array, int size)
{
  int max = array[0];
  int i;
  for (i = 1; i < size; i++)
  {
    if (array[i] > max)
    {
      max = array[i];
    }
  }
  return max;
}

int min_in_array(int *array, int size)
{
  int min = array[0];
  int i;
  for (i = 1; i < size; i++)
  {
    if (array[i] < min)
    {
      min = array[i];
    }
  }
  return min;
}

char *itoa(int n)
{
  char *str = malloc(sizeof(char) * 12);
  sprintf(str, "%d", n); // NOLINT
  return str;
}

char *ltoa(long n)
{
  char *str = malloc(sizeof(char) * 12);
  sprintf(str, "%ld", n); // NOLINT
  return str;
}

char *lltoa(long long n)
{
  char *str = malloc(sizeof(char) * 12);
  sprintf(str, "%lld", n); // NOLINT
  return str;
}