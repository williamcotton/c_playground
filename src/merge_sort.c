#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "helpers.h"

void merge(int *array, int left, int mid, int right)
{
  int i, j, k;
  int n1 = mid - left + 1;
  int n2 = right - mid;
  int *L = malloc(sizeof(int) * n1);
  int *R = malloc(sizeof(int) * n2);
  for (i = 0; i < n1; i++)
  {
    L[i] = array[left + i];
    printf("L[%d] = %d\n", i, L[i]);
  }
  for (j = 0; j < n2; j++)
  {
    R[j] = array[mid + 1 + j];
    printf("R[%d] = %d\n", j, R[j]);
  }
  i = 0;
  j = 0;
  k = left;
  while (i < n1 && j < n2)
  {
    if (L[i] <= R[j])
    {
      array[k] = L[i];
      i++;
    }
    else
    {
      array[k] = R[j];
      j++;
    }
    k++;
  }
  while (i < n1)
  {
    array[k] = L[i];
    i++;
    k++;
  }
  while (j < n2)
  {
    array[k] = R[j];
    j++;
    k++;
  }
  free(L);
  free(R);
}

void merge_sort_recursive(int *array, int left, int right)
{
  printf("sort segment (%d %d): ", left, right);
  for (int i = 0; i < 10; i++)
  {
    printf("%d ", array[i]);
  }
  printf("\n");
  if (left < right)
  {
    int mid = (left + (right - 1)) / 2;
    merge_sort_recursive(array, left, mid);
    merge_sort_recursive(array, mid + 1, right);
    merge(array, left, mid, right);
  }
}

void merge_sort(int *array, int size)
{
  printf(" input: ");
  for (int i = 0; i < 10; i++)
  {
    printf("%d ", array[i]);
  }
  printf("\n");
  printf(" count: %d\n", size);
  merge_sort_recursive(array, 0, size - 1);
  printf("output: ");
  for (int i = 0; i < 10; i++)
  {
    printf("%d ", array[i]);
  }
  printf("\n");
}

int main()
{
  int numbers[10] = {9, 3, 5, 0, 8, 7, 6, 2, 1, 4};
  int number_size = sizeof(numbers) / sizeof(int);
  merge_sort(numbers, number_size);

  return 0;
}
