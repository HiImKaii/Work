#include <stdio.h>
int B[5];
int nSS, nCS, nDS;
int nIP;
char strFormat[] = "EAX = %d\n"; 
int main(){
	int i;
	int arr_of_int[5];
	int *a;
	nSS = 0;
	nCS = 0;
	nDS = 0;
	nIP = 0;
	for( i = 0; i < 5; i++ ) {
		arr_of_int[i] = i;
	}
	a = (int*)malloc( 5 * sizeof(int) );
	for( i = 0; i < 5; i++ ) {
		a[i] = i;
	}
	for( i = 0; i < 5; i++ ) {
		B[i] = i;
	}
	printf( "arr_of_int: %d\n", arr_of_int );
	printf( "&arr_of_int: %d\n", &arr_of_int );
	printf( "&i: %d\n", &i );
	printf( "a: %d\n", a );
	printf( "&a: %d\n", &a );
	printf( "B: %d\n", B );
	printf( "&B: %d\n", &B );
	//printf( "a[0] = %d\n", *((int*)4260592) );
	//printf( "a[1] = %d\n", *((int*)4260596) );
	__asm{
		mov WORD PTR offset nSS, SS
		mov WORD PTR offset nCS, CS
		mov WORD PTR offset nDS, DS
	}
	printf( "SS: %d, CS: %d, DS: %d, IP: %d\n", nSS, nCS, nDS, nIP );
	printf( "main: %x\n", main );
	printf( "printf: %x\n", &printf );
	return 0;
}