#include<stdio.h>
#include<stdlib.h>
main(){
	char a[10];
	printf( "name " );
	fflush( stdout );
	gets(a);
	printf( "hello, %s\n", a );
}