import random as rd

x = []
y = []

a = 0
b = 0

tub = 0

for i in range (1000):
    x_val = rd.uniform(-100, 100)
    y_val = 5*x_val + 3 + rd.uniform(-0.5, 0.5)

    x.append(x_val)
    y.append(y_val)

    print (i+1, ": ", x[i], ": ", y[i])

for i in range (1000):
    tu = (x[i] * y[i]) - ((x[i] * y[i]) / 1000)
    mau = (x[i] ** 2) - ((x[i] ** 2) / 1000)
    a  += tu / mau
a = a / 1000
for i in range (1000):
    tub += y[i] - a * x[i]
b = tub / 1000

print (a, " ", b)