#include<windows.h>
#include<math.h>
#define TRUE 1
#define FALSE 0

int is_prime( int n )
{
	int j = 0;
	int upper = (int)sqrt( n );
	for( j = 2; j <= upper; j++ ) {
		if( n % j == 0 ) return FALSE;
	}
	return TRUE;
}

int main()
{
	int i = 2;
	long count = 0;
	long dwPID = GetCurrentProcessId();
	while( TRUE ) {
		if( is_prime( i ) ) {
			count++;
			if( count % 5000 == 0 ) {
				printf( "%ld %ld primes found\n", dwPID, count );
			}
		}
		i++;
	}
}