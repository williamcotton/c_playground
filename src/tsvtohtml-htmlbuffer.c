#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ROWS 1000
#define MAX_COLS 1000

char *tsv_to_html() {
  char buffer[1024];

  // Generate HTML table
  char *html = malloc(1024 * MAX_ROWS * MAX_COLS);
  html[0] = '\0';
  
  strcat(html, "<table>\n");

  // Read input from stdin
  while (fgets(buffer, sizeof(buffer), stdin)) {
    strcat(html, "  <tr>\n");

    // Remove trailing newline
    int col_index = 0;
    buffer[strcspn(buffer, "\n")] = 0;

    // Split row into columns
    char *token = strtok(buffer, "\t");
    while (token != NULL) {
      strcat(html, "    <td>");
      strcat(html, token);
      strcat(html, "</td>\n");
      token = strtok(NULL, "\t");
    }

    strcat(html, "  </tr>\n");
  }

  strcat(html, "</table>\n");

  return html;
}

int main() {
  char *html = tsv_to_html();
  printf("%s", html);
  free(html);
  return 0;
}
