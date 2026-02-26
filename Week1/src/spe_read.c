#include<stdio.h>
#include<conio.h>

#define ESC 27
#define DOWN_ARROW 80
#define UP_ARROW 72
#define RIGHT_ARROW 77
#define LEFT_ARROW 75
#define CTRL_A 1
#define CTRL_B 2
#define CTRL_C 3
int main(){
	int ch = 0;
	printf( "Press ESC to quit!\n" );
	do {
		ch = getch();
		if( ch == 244 ) ch = getch();	// Arrows: generate 2 code: 244 and xx
		switch( ch ) {
		case CTRL_A:
			printf( "Ctrl+A pressed\n" );
			break;
		case CTRL_B:
			printf( "Ctrl+B pressed\n" );
			break;
		case CTRL_C:
			printf( "Ctrl+C pressed\n" );
			break;
		case UP_ARROW:
			printf( "Up Arrow pressed\n" );
			break;
		case DOWN_ARROW:
			printf( "Down Arrow pressed\n" );
			break;
		case RIGHT_ARROW:
			printf( "Right Arrow pressed\n" );
			break;
		case LEFT_ARROW:
			printf( "Left Arrow pressed\n" );
			break;
		case ESC:
			printf( "Goodbye!\n" );
			break;
		}
	} while( ch != ESC );	
	return 0;
}