#include<windows.h>
#include<winbase.h>
#include<math.h>

#define TRUE 1
#define FALSE 0
long t0, t1;

int is_prime( int n )
{
	int j = 0;
	int upper = (int)sqrt( n );
	for( j = 2; j <= upper; j++ ) {
		if( n % j == 0 ) return FALSE;
	}
	return TRUE;
}

void go_sleep() {
	Sleep( 30 );
}
int main()
{

	int i = 2;
	long count = 0;
	long dwPID = GetCurrentProcessId();
	t0 = GetTickCount();
	while( TRUE ) {
		if( is_prime( i ) ) {
			count++;
			if( count % 5000 == 0 ) {
				printf( "%ld %ld primes found, %d\n", dwPID, count, t1-t0 );
			}
		}
		i++;
		t1 = GetTickCount();
		if( t1 - t0 >= 100 ){ 
			printf( "%ld: go_sleep\n", dwPID );
			go_sleep();
			t0 = GetTickCount();
		}
	}
}