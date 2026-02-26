def f(x):
    return x ** 2

def derivative(f, x):
    dx = 0.0000001
    df = (f(x+dx) - f(x)) / dx
    return df

def main():
    x_input = float(input())
    result = derivative(f, x_input)
    print(result)

if __name__ == "__main__":
    main()