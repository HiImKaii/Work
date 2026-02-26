#include <windows.h>

int foo(DWORD);

hoge()
{
  int x = 1;
  int y = 0;
  int z = x / y;
  printf("ok\n");
}

mainx()
{
  hoge();
}

main()
{
  __try {
    hoge();
  } __except(foo(GetExceptionCode())) {
    printf("divide by zero\n");
  }
}

int foo(DWORD c) 
{
  if (c == STATUS_INTEGER_DIVIDE_BY_ZERO) {
    return EXCEPTION_EXECUTE_HANDLER; /* 1 */
  } else {
    return EXCEPTION_CONTINUE_SEARCH;
  }
}
