#include <stdio.h>

// emcc -o build/hello2.html src/hello.c -O3 -fblocks --shell-file \
// src/shell_minimal.html

// source ../emsdk/emsdk_env.sh

int main() { printf("Hello, wasm!!\n"); }
