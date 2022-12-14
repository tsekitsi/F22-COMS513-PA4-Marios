#include <stdio.h>
#include <stdlib.h>

int main(void) {
  int r = rand() % 100;
  printf("%d\n", r);
  if (r < 17)
    printf("1\n");
  else if (r < 33)
    printf("2\n");
  else if (r < 50)
    printf("3\n");
  else if (r < 66)
    printf("4\n");
  else if (r < 83)
    printf("5\n");
  else
    printf("6\n");
  return 0;
}