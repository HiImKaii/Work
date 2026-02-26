#include<windows.h>

void set_value( int data, int* pFreq ) {
	int tmp_f = 0;
	if( data >= 'A' && data <= 'Z' ) data += 0x20; // to-lower-case
	switch( data ) {
	case 'a':
		tmp_f = 440;	// doof
		break;
	case 's':
		tmp_f = 494;	// re
		break;
	case 'd':
		tmp_f = 523; 
		break;
	case 'f':
		tmp_f = 587;
		break;
	case 'j':
		tmp_f = 659;
		break;
	case 'k':
		tmp_f = 698;
		break;
	case 'l':
		tmp_f = 784;
		break;
	case ';':
		tmp_f = 880;
		break;
	default:
		tmp_f = 0;
		break;
	}
	*pFreq = tmp_f;
}

int main() {
	int f;
	int ch;
	f = 0;
	ch = 0;
	printf( "Type q for quit!\n" );
	ch = _getch();
	while( ch != 'q' && ch != 'Q' ) {
		set_value( ch, &f ); 
		Beep( f, 500 );	// generate a beep to the speaker( f is frequency, in 1000ms )
		ch = _getch();
	} 
	return 0;
}