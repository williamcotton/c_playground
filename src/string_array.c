#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "helpers.h"

int main()
{
  int array_i[10];
  for (int i = 0; i < 10; i++)
  {
    array_i[i] = arc4random() % 10;
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%d ", array_i[i]);
  }
  printf("\n");

  int *array_ip = malloc(sizeof(int) * 10);
  for (int i = 0; i < 10; i++)
  {
    array_ip[i] = arc4random() % 10;
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%d ", array_ip[i]);
  }
  printf("\n");

  int *array_ipa[10];
  for (int i = 0; i < 10; i++)
  {
    array_ipa[i] = malloc(sizeof(int));
    *array_ipa[i] = arc4random() % 10;
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%d ", *array_ipa[i]);
  }
  printf("\n");

  char array_c[10];
  for (int i = 0; i < 10; i++)
  {
    array_c[i] = arc4random() % 26 + 'a';
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%c ", array_c[i]);
  }
  printf("\n");

  char *array_cp = malloc(sizeof(char) * 10);
  for (int i = 0; i < 10; i++)
  {
    array_cp[i] = arc4random() % 26 + 'a';
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%c ", array_cp[i]);
  }
  printf("\n");

  char *array_cpa[10];
  for (int i = 0; i < 10; i++)
  {
    array_cpa[i] = malloc(sizeof(char));
    *array_cpa[i] = arc4random() % 26 + 'a';
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%c ", *array_cpa[i]);
  }
  printf("\n");

  char *array_str[10];
  for (int i = 0; i < 10; i++)
  {
    array_str[i] = malloc(sizeof(char) * 2);
    sprintf(array_str[i], "%c", arc4random() % 26 + 'a');
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%s ", array_str[i]);
  }
  printf("\n");

  char *array_strc[10];
  for (int i = 0; i < 10; i++)
  {
    array_strc[i] = malloc(sizeof(char) * 2);
    array_strc[i][0] = arc4random() % 26 + 'a';
    array_strc[i][1] = '\0';
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%s ", array_str[i]);
  }
  printf("\n");

  char **array_strp = malloc(sizeof(char *) * 10);
  for (int i = 0; i < 10; i++)
  {
    array_strp[i] = malloc(sizeof(char) * 2);
    sprintf(array_strp[i], "%c", arc4random() % 26 + 'a');
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%s ", array_strp[i]);
  }
  printf("\n");

  char **array_strpa[10];
  for (int i = 0; i < 10; i++)
  {
    array_strpa[i] = malloc(sizeof(char *));
    *array_strpa[i] = malloc(sizeof(char) * 2);
    sprintf(*array_strpa[i], "%c", arc4random() % 26 + 'a');
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%s ", *array_strpa[i]);
  }
  printf("\n");

  char **array_strpai[10];
  for (int i = 0; i < 10; i++)
  {
    array_strpai[i] = malloc(sizeof(char *));
    array_strpai[i][0] = malloc(sizeof(char) * 2);
    sprintf(array_strpai[i][0], "%c", arc4random() % 26 + 'a');
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%s ", array_strpai[i][0]);
  }
  printf("\n");

  char ***array_strpaa[10];
  for (int i = 0; i < 10; i++)
  {
    array_strpaa[i] = malloc(sizeof(char **));
    *array_strpaa[i] = malloc(sizeof(char *));
    **array_strpaa[i] = malloc(sizeof(char) * 2);
    sprintf(**array_strpaa[i], "%c", arc4random() % 26 + 'a');
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%s ", **array_strpaa[i]);
  }
  printf("\n");

  char ***array_strpaai[10];
  for (int i = 0; i < 10; i++)
  {
    array_strpaai[i] = malloc(sizeof(char **));
    array_strpaai[i][0] = malloc(sizeof(char *));
    array_strpaai[i][0][0] = malloc(sizeof(char) * 2);
    sprintf(array_strpaai[i][0][0], "%c", arc4random() % 26 + 'a');
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%s ", array_strpaai[i][0][0]);
  }
  printf("\n");

  char ***array_strpaaic[10];
  for (int i = 0; i < 10; i++)
  {
    array_strpaaic[i] = malloc(sizeof(char **));
    array_strpaaic[i][0] = malloc(sizeof(char *));
    array_strpaaic[i][0][0] = malloc(sizeof(char) * 2);
    array_strpaaic[i][0][0][0] = arc4random() % 26 + 'a';
    array_strpaaic[i][0][0][1] = '\0';
  }
  for (int i = 0; i < 10; i++)
  {
    printf("%s ", array_strpaaic[i][0][0]);
  }
  printf("\n");

  return 0;
}
