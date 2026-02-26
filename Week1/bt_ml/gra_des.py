import numpy as np
import random as rd

w0 = [5, 0, 2, 4, 4] # Khởi tạo trọng số ban đầu để sinh dữ liệu

x = []
y = []

w_train = np.ones(5) # Trọng số thay đổi trong quá trình huấn luyện

def f(x, w):
    return w[0] + w[1] * x + w[2] * x * x + np.sin(w[3] * x) + (np.sin(w[4] * x) ** 2)

for i in range (5000):
    x_val = rd.uniform(-1, 1)
    y_val = f(x_val, w0) + rd.uniform(-0.05, 0.05)

    x.append(x_val)
    y.append(y_val)

x = np.array(x)
y = np.array(y)

def loss(w):
    y_t = f(x, w)
    return np.mean((y_t - y) ** 2)

# def derivative(f, x):
#     dx = 0.00001
#     df = (f(x+dx) - f(x)) / dx
#     return df

gamma = 0.05
dx = 0.00001
for i in range (1000):
    grad = np.zeros(5)
    loss_ht = loss(w_train)
    
    # Tính gradient cho từng chiều w_j
    for j in range (5):
        # def loss_wj(val):
        #     w_tmp = w_train.copy()
        #     w_tmp[j] = val
        #     return loss(w_tmp)
        # grad[j] = derivative(loss_wj, w_train[j])
        w_tmp = w_train.copy()
        w_tmp[j] += dx

        grad[j] = (loss(w_tmp) - loss_ht) / dx
        
    w_train = w_train - gamma * grad
    
print ("w0: ", w0, "w_t", w_train)