import random as rd
import math

# Sinh du lieu;
N = 60

X = [rd.uniform(-math.pi / 4, math.pi / 4) for _ in range(N)]
Y = [math.sin(x) + rd.uniform(-0.1, 0.1) for x in X]

w = rd.uniform(-0.1, 0.1)
b = rd.uniform(-0.1, 0.1)

# Thuc hien du doan
def predict(x, w, b):
    y = w * x + b
    return y

# Tinh loss
def loss(X, Y, w, b):
    total = 0
    for x, y in zip(X, Y):
        y_pre = predict(x, w, b)
        total += (y_pre - y) * (y_pre - y)
    return total / 2

# Tinh Gradient
def gradient(X, Y, w, b):
    dw = 0
    db = 0
    for x, y in zip(X, Y):
        y_pre = predict(x, w, b)
        err = y_pre - y
        dw += err * x
        db += err
    return dw, db

loss_test = loss(X, Y, w, b)
gra_test = gradient(X, Y, w, b)
print("Loss ban đầu: ", loss_test, "Gradient ban đầu: ", gra_test)

# Vong lap huan luyen
gamma = 1e-3
it = 400

for _ in range(it):
    dw, db = gradient(X, Y, w, b)
    
    w = w - gamma * dw
    b = b - gamma * db
loss_cuoi = loss(X, Y, w, b)
print ("w: ", w, " b: ", b, " Loss cuoi: ", loss_cuoi)