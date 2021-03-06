int compare_int(const void *a, const void *b);
int compare_double(const void *a, const void *b);
int compare_strings(const void *a, const void *b);
struct memory_struct
{
  char *buffer;
  size_t size;
};
void http_get(char *url, char *session, struct memory_struct *mem);
char *fetch(char *url);
void resize_int_array(int **array, int *size, int new_size);
int *comma_seperated_string_to_integer_array(char *input, int count);
char *string(char *input);
void process_array(char *str, char **array);
void print_first_element_of_array(char **array);
void print_all_elements_in_array(char **array);
int length_of_array(char **array);
int count_of_newlines_in_string(char *str);
int count_of_commas_in_string(char *str);
int split_count(char *str, char delimiter);
int *split_int(char *str, char delimiter);
void shift_left(long *array);
int *difference_array(int *array, int size, int value);
int sum_array(int *array, int size);
int difference_sum(int *array, int size, int value);
int difference_triangular(int *array, int size, int value);
int max(int a, int b);
int min(int a, int b);
char **split(char *str, char *delim, int *count);
int count_characters(char *str);
char *difference_string(char *str1, char *str2);
char *intersection_string(char *str1, char *str2);
int string_contains(char *str, char character);
int string_contains_all(char *str, char *characters);
int strings_are_equal(char *str1, char *str2);
char *insert_char(char *str, char replacement, int index);
int count_of_char_in_string(char *str, char character);
int find_index(char *line, int size, char c);
int max_in_array(int *array, int size);
int min_in_array(int *array, int size);
char *itoa(int n);
char *ltoa(long n);
char *lltoa(long long n);
