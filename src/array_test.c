#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helpers.h"

int *shiftRight(int *arr, int start, int *arrSize)
{
  int new_size = *arrSize + 1;
  int *new_arr = realloc(arr, new_size * sizeof(int));
  for (int i = (new_size - 2); i > start; i--)
  {
    new_arr[i + 1] = new_arr[i];
  }
  *arrSize = new_size;
  return new_arr;
}

int main()
{

  int arrSize = 8;
  // arr of pointers with values [1,0,2,3,0,4,5,0]
  int *arr = malloc(sizeof(int) * arrSize);
  arr[0] = 1;
  arr[1] = 0;
  arr[2] = 2;
  arr[3] = 3;
  arr[4] = 0;
  arr[5] = 4;
  arr[6] = 5;
  arr[7] = 0;

  // arr = shiftRight(arr, 1, &arrSize);
  // arr[2] = 0;

  for (int i = 0; i < arrSize; i++)
  {
    if (arr[i] == 0)
    {
      arr = shiftRight(arr, i, &arrSize);
      arr[i + 1] = 0;
      i++;
    }
  }

  // print all elements in arr
  for (int i = 0; i < arrSize; i++)
  {
    printf("%d ", arr[i]);
  }

  return 0;
}
