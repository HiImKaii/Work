#include <windows.h>

/* shared data */
int counter = 0;


DWORD WINAPI f( LPVOID x ) {
  int i;
  for (i = 0; i < 100000; i++) {
    counter++;
  }
  return 0;
}

int main()
{
  DWORD tidx, tidy;
  HANDLE hx, hy;
  counter = 0;

  hx = CreateThread( 0, 0, f, (LPVOID)0, 0, &tidx );
  hy = CreateThread( 0, 0, f, (LPVOID)1, 0, &tidy );
  WaitForSingleObject( hx, INFINITE );
  WaitForSingleObject( hy, INFINITE );
  printf( "counter = %d\n", counter );
  return 0;
}

