#include <windows.h>

/* shared data */
int isAssigning = 0;
int counter = 0;


int test_and_set( int* x )
{
	int retVal;
	__asm
	{
		push eax
		mov eax, x
		bts dword ptr [eax], 0
		jc occupied
		mov retVal, 0
		jmp getout
occupied:
		mov retVal, 1
getout:
		pop eax
	}
	//printf( "retVal = %d\n", retVal );
	return retVal;
}

DWORD WINAPI f( LPVOID x ) {
  int i;
  for (i = 0; i < 1000000; i++) {
    //printf("thread %d: counter = %d\n", x, counter);
	  if( test_and_set( &isAssigning ) == 0 ) {
		counter++;
		isAssigning = 0;
	  }
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

