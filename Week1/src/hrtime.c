#include <windows.h>

void main()
{
  int i;
  LARGE_INTEGER Frequency, Count[2];
  BOOL b = QueryPerformanceFrequency(&Frequency);
  if (b == 0) {
    printf("no high resolution timer\n");
    exit(1);
  } else {
    printf("timer frequency = %d / sec\n", Frequency);
  }
  QueryPerformanceCounter(Count);
  for (i = 0; i < 1000000; i++) {
    do_nothing();
  }
  QueryPerformanceCounter(Count + 1);
  printf("%d\n", Count[1].QuadPart - Count[0].QuadPart);
}

int do_nothing()
{
  return 3;
}
