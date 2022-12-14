#include <stdio.h>
#include <stdlib.h>

int main(void) {
  int r = rand() % 100;
  printf("%d\n", r);
  if (r < 50)
    printf("Heads\n");
  else
    printf("Tails\n");
  return 0;
}