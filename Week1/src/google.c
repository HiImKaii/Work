#include<stdio.h>

int main(){
	//wwwdot - google = dotcom
	int w, d, o, t, g, l, e, c, m;
	int alpha, beta, gama;
	printf( "begin\n" );
	for( w = 1; w <= 9; w++ ) {
		for( g = 1; g < w; g++ ) {
			for( d = 1; d <= (w-g); d++ ) {
				if( d == w || d == g ) continue;
				for( o = 0; o <= 9; o++ ) {
					if( o == w || o == g || o == d ) continue;
					for( t = 0; t <= 9; t++ ) {
						if( t == o || t == w || t == g || t == d ) continue;
						for( l = 0; l <= 9; l++ ) {
							if( l == t || l == o || l == g || l == d || l == w ) continue;
							for( c = 0; c <= 9; c++ ){
								if( c == l || c == t || c == o || c == d || c == g || c == w ) continue;
								for( e = 0; e <= 9; e++ ) {
									if( e == c || e == l || e == t || e == o || e == d ||
										e == g || e == w ) continue;
									for( m = 0; m <= 9; m++ ) {
										if( m == e || m == c || m == l || m == t || m == o ||
											m == d || m == g || m == w ) continue;
										alpha = 111000*w + 100*d + 10*o + t;
										beta = 100100*g + 11000*o +10*l + e;
										gama = 100000*d + 10010*o + 1000*t + 100*c + m;
										if( alpha - beta == gama ) {
											beta = 100100*g + 11000*o +10*l + m;
											gama = 100000*d + 10010*o + 1000*t + 100*c + e;
											if( alpha - beta == gama ) {
												printf( "%d%d%d%d%d%d - %d%d%d%d%d%d = %d%d%d%d%d%d\n",
														w,w,w,d,o,t, g,o,o,g,l,e, d,o,t,c,o,m );
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	printf( "end\n" );
	return 0;
}