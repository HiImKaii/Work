def f(x):
    return x ** 2

def derivative(f, x):
    dx = 0.0000001
    df = (f(x+dx) - f(x)) / dx
    return df

x = 3
print(derivative(f, x))