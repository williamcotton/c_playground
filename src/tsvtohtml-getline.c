#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tsv_to_html() {
  // Generate HTML table
  printf("<table>\n");

  // Read input from stdin
  char *line = NULL;
  size_t len = 0;
  while (getline(&line, &len, stdin) != -1) {
    printf("  <tr>\n");

    // Remove trailing newline
    line[strcspn(line, "\n")] = 0;

    // Split row into columns
    char *token = strtok(line, "\t");
    while (token != NULL) {
      printf("    <td>%s</td>\n", token);
      token = strtok(NULL, "\t");
    }

    printf("  </tr>\n");

    fflush(stdout);
  }

  free(line);

  printf("</table>\n");
}

int main() {
  tsv_to_html();
}
