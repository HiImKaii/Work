#include<windows.h>


// global shared data
int y;	// updating, using of y must be mutual exclusive (-->mutex)
int turn;

DWORD WINAPI set_value( int val_to_set ) {
	// wait user for input value, 
	// if input data available, change the value of y
	// this likes the "signal" model in Linux
	int data;
	while( 1 ) {
		printf( "data = " );
		scanf( "%d", &data );
		if( turn == 1 ) {
			/* critical section */
			y = data;
			turn = 0;
		}
	}
}

int main() {

	DWORD thId;
	HANDLE hSetVal;
	int f;
	// do something ( ie. control something )
	y = 420;
	f = y;
	turn = 0;
	hSetVal = CreateThread( 0, 0, set_value, (LPVOID)0, 0, &thId );
	while( 1 ) {
		Beep( f, 1000 );	// generate a beep to the speaker( f is frequency, in 1000ms )
		if( turn == 0 ) {
			/* critical section */
			f = y;
			turn = 1;
		}
	}
	return 0;

}