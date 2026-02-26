int process_id[50];
int schedule(int x){
	int a,b,c;
	a = x;
	b = 1;
	c = 5;
	x = a + b * x;
	c = x - a;
	a = x + c;
	return a;
}
int main(){
	int f, g, h;
	f = 5;
	h = schedule( f );
	g = 3;
	f = schedule( h );
	h = schedule( f );
	return 0;
}