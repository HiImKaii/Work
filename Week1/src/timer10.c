#include<stdio.h>
#include<time.h>

int y;

void set_value( int x ) {
	y = x;
}

int main( int argc, char* argv[] ){
	time_t start, end;
	int i;
	char* str = "abc";
	i = 0;
	
	set_value( 0 );
	time( &start );			// get current time, in sec. from 0:0:0, 1/1/1970
	printf( " y = %d, wait for 10 seconds...\n", y );
	while( 1 ) {
		// do something (ie. control something)
		i++;
		if( i >= 100 ) i = 0;
		// get current time
		time( &end );
		if( end - start >= 10 ) {	// if 10 sec. passed, change value of y 
			set_value( y + 1 );
			printf( " y = %d, i = %d,  wait for 10 seconds...\n", y, i );
			// reset the start time
			time( &start );
		}
	}
	return 0;
}