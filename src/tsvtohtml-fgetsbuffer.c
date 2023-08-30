#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ROWS 1000
#define MAX_COLS 1000

void tsv_to_html() {
  char buffer[1024];

  // Generate HTML table
  printf("<table>\n");

  // Read input from stdin
  while (fgets(buffer, sizeof(buffer), stdin)) {
    printf("  <tr>\n");

    // Remove trailing newline
    int col_index = 0;
    buffer[strcspn(buffer, "\n")] = 0;

    // Split row into columns
    char *token = strtok(buffer, "\t");
    while (token != NULL) {
      printf("    <td>%s</td>\n", token);
      token = strtok(NULL, "\t");
    }

    printf("  </tr>\n");
    fflush(stdout);
  }

  printf("</table>\n");
}

int main() {
  tsv_to_html();
  return 0;
}
