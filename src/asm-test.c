#include <unistd.h>

int main() {
  const char msg[] = "Hello, ARM!\n";
  write(0, msg, sizeof(msg));
  // exit(0);
}
