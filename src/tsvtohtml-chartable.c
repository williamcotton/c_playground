#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ROWS 1000
#define MAX_COLS 1000

char *tsv_to_html() {
  char buffer[1024];
  char *table[MAX_ROWS][MAX_COLS];
  int row_count = 0;
  int col_count = 0;

  // Generate HTML table
  char *html = malloc(1024 * MAX_ROWS * MAX_COLS);
  html[0] = '\0';

  // Read input from stdin
  while (fgets(buffer, sizeof(buffer), stdin)) {
    // Remove trailing newline
    int col_index = 0;
    buffer[strcspn(buffer, "\n")] = 0;

    // Split row into columns
    char *token = strtok(buffer, "\t");
    while (token != NULL) {
      table[row_count][col_index++] = strdup(token);
      token = strtok(NULL, "\t");
    }
    if (row_count == 0) {
      col_count = col_index;
    }

    row_count++;
  }

  strcat(html, "<table>\n");
  for (int i = 0; i < row_count; i++) {
    strcat(html, "  <tr>\n");
    for (int j = 0; j < col_count; j++) {
      strcat(html, "    <td>");
      strcat(html, table[i][j]);
      strcat(html, "</td>\n");
    }
    strcat(html, "  </tr>\n");
  }
  strcat(html, "</table>\n");

  // Free memory
  for (int i = 0; i < row_count; i++) {
    for (int j = 0; j < col_count; j++) {
      free(table[i][j]);
    }
  }

  return html;
}

int main() {
  char *html = tsv_to_html();
  printf("%s", html);
  free(html);
  return 0;
}
