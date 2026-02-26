#include <iostream>
using namespace std;

double f(double x)
{
    return x*x;
}

double derivative(double (*func)(double), double x)
{
    double dx = 0.000001;
    double df = (func(x + dx) - func(x)) / dx;
    return df;
}

int main()
{
    double x = 3;
    cout << derivative(f, x) << endl;

    return 0;
}