#include <windows.h>


DWORD WINAPI f( LPVOID x ) {
  int i;
  for (i = 0; i < 100; i++) {
    printf("thread %d said %dth hello\n", x, i);
    Sleep(1);
  }
  return 0;
}

main()
{
  DWORD tidx, tidy;
  HANDLE hx = CreateThread( 0, 0, f, (LPVOID)0, 0, &tidx );
  HANDLE hy = CreateThread( 0, 0, f, (LPVOID)1, 0, &tidy );
  WaitForSingleObject( hx, INFINITE );
  printf( "main said hello\n" );
  WaitForSingleObject( hy, INFINITE );
  printf( "main said goodbye\n" );
}

