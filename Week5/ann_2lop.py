import numpy as np
import math

# Sinh du lieu
def sinh_du_lieu(n = 100):
    X = np.random.uniform(-math.pi / 4, math.pi / 4, size = n)
    Y = np.sin(X) + np.random.uniform(-0.1, 0.1, size = n)
    return X, Y

X, Y = sinh_du_lieu(100)

for i in range(5):
    print("X: ", X[i], " Y: ", Y[i])
    
# Sinh trong so
def init_w(m, n):
    weigh = np.random.randn(m, n)
    return weigh

# Truyen thang
def relu(z):
    return np.maximum(0, z)

def forward(x, W1, B1, W2, B2, W3, B3):
    Z1 = W1 @ x + B1
    A1 = relu(Z1)
    
    Z2 = W2 @ A1 + B2
    A2 = relu(Z2)
    
    Z3 = W3 @ A2 + B3
    
    return Z3, A1, A2, Z1, Z2


# Tinh loss va dao ham ham loss
def loss_fn(X, Y):
    x = X[i].reshape(1, 1)
    y = Y[i].reshape(1, 1)
    
    z3 = forward(x, W1, B1, W2, B2, W3, B3)
    
    y_pred = z3[0][0]
    
    return np.mean((y_pred - y) ** 2)

def gra_loss(y_pred, y):
    return (y_pred - y)

# Truyen nguoc
def gra_relu(z):
    return (z > 0).astype(float)

def truyen_nguoc(x, y, y_pred, a1, a2, z1, z2, W2, W3):
    dl_dypred = gra_loss(y_pred, y).reshape(1, 1)
    dl_dz3 = dl_dypred
    
    dl_dw3 = dl_dz3 @ a2.T
    dl_db3 = dl_dz3
    
    dl_da2 = W3.T @ dl_dz3
    dl_dz2 = dl_da2 * gra_relu(z2)
    dl_dw2 = dl_dz2 @ a1.T
    dl_db2 = dl_dz2
    
    dl_da1 = W2.T @ dl_dz2
    dl_dz1 = dl_da1 * gra_relu(z1)
    dl_dw1 = dl_dz1 @ x.T
    dl_db1 = dl_dz1
    
    grads =  {
        "W1": dl_dw1, "B1": dl_db1,
        "W2": dl_dw2, "B2": dl_db2,
        "W3": dl_dw3, "B3": dl_db3
    }
    return grads

def huan_luyen(X, Y, its = 500, lr = 1e-5):
    W1 = init_w(4, 1)
    B1 = init_w(4, 1)

    W2 = init_w(4, 4)
    B2 = init_w(4, 1)

    W3 = init_w(1, 4)
    B3 = init_w(1, 1)
    
    for _ in range(its):
        for i in range (len(X)):
            x = X[i].reshape(1, 1)
            y = Y[i].reshape(1, 1)
            
            y_pred, A1, A2, Z1, Z2 = forward(x, W1, B1, W2, B2, W3, B3)
            
            grads = truyen_nguoc(x, y, y_pred, A1, A2, Z1, Z2, W2, W3)
            
            W1 = W1 - lr * grads["W1"]
            B1 = B1 - lr * grads["B1"]
            
            W2 = W2 - lr * grads["W2"]
            B2 = B2 - lr * grads["B2"]
            
            W3 = W3 - lr * grads["W3"]
            B3 = B3 - lr * grads["B3"]
            
    return W1, B1, W2, B2, W3, B3

W1, B1, W2, B2, W3, B3 = huan_luyen(X, Y, its = 500, lr = 1e-5)

print("W1: ", W1)
print("B1: ", B1)
print("W2: ", W2)
print ("B2: ", B2)
print ("W3: ", W3)
print ("B3: ", B3)

loss_cuoi = loss_fn(X, Y)
print("Loss cuoi: ", loss_cuoi)