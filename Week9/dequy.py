n = 9

def test(n):
    if n == 1 or n == 0: 
        return 1
    return test(n - 1) + test(n - 2)

print(test(n))