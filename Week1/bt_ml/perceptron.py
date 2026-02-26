import math
import random as rd
import numpy as np

def sinh_du_lieu(n = 1):
    data = []
    for i in range(n):
        x = rd.uniform(-math.pi/4, math.pi/4)
        noise = rd.uniform(-0.1, 0.1)
        y = math.sin(x) + noise
        data.append((x, y))
    return data

def sigmoid(x):
    if x < -100:
        return 0
    if x > 100:
        return 1
    return 1.0 / (1.0 + math.exp(-x))

def d_sigmoid(x):
    s = sigmoid(x)
    return s * (1 - s)

class ANN:
    def __init__(self, hidden_size = 1, learning_rate = 0.1):
        self.hidden_size = hidden_size
        self.learning_rate = learning_rate

        self.weights_ih = [rd.uniform(-1, 1) for _ in range (hidden_size)] #i: input; h: hidden layer: weight_ih = trọng số nối từ input đến lớp ẩn
        self.bias_h = [rd.uniform(-1, 1) for _ in range (hidden_size)]

        self.weights_ho = [rd.uniform(-1, 1) for _ in range (hidden_size)]
        self.bias_o = rd.uniform(-1, 1)

        self.hidden_inputs = [0.0] * hidden_size
        self.hidden_outputs = [0.0] * hidden_size
        self.output_input = 0.0
        self.output = 0.0
    
    def truyen_thang(self, x):
        for i in range(self.hidden_size):
            self.hidden_inputs[i] = self.weights_ih[i] * x + self.bias_h[i]
            self.hidden_outputs[i] = sigmoid(self.hidden_inputs[i])

        self.output = sum(
            self.weights_ho[i] * self.hidden_outputs[i]
            for i in range (self.hidden_size)
        ) + self.bias_o

        # self.output = self.output_input

        return self.output

    def truyen_nguoc(self, x, y_true):
        y_pred = self.output
        output_error = y_pred - y_true

        for i in range(self.hidden_size):
            grad_w_ho = output_error * self.hidden_outputs[i] #Tính độ dốc để cập nhật trọng số. delta(E)/delta(W_ho)
            self.weights_ho[i] -= self.learning_rate * grad_w_ho

        self.bias_o -= self.learning_rate * output_error

        d_hidden = []
        
        for i in range(self.hidden_size):
            d_a_h = output_error * self.weights_ho[i]
            d_z_h = d_a_h * d_sigmoid(self.hidden_inputs[i])
            d_hidden.append(d_z_h)

        for i in range(self.hidden_size):
            grad_w_ih = d_hidden[i] * x
            self.weights_ih[i] -= self.learning_rate * grad_w_ih

            self.bias_h[i] -= self.learning_rate * d_hidden[i]

    def huan_luyen_1step(self, x, y_true):
        y_pred = self.truyen_thang(x)
        self.truyen_nguoc(x, y_true)

        loss = 0.5 * (y_pred - y_true) ** 2
        return loss

    def huan_luyen(self, data, it = 1000, verbose=True):
        for i in range(it):
            total_loss = 0.0

            rd.shuffle(data)

            for x, y in data:
                loss = self.huan_luyen_1step(x, y)
                total_loss += loss
            ss_tb = total_loss / len(data) # Sai số trung bình

            if verbose and (i + 1) % 100 == 0:
                print ("Vong lap: ", i + 1, " loss: ", ss_tb)
        return ss_tb

    def predict(self, x):
        return self.truyen_thang(x)

def mse(predictions, actuals):
    n = len(predictions)
    return sum((p - a) ** 2 for p, a in zip(predictions, actuals)) / n

def mae (predictions, actuals):
    n = len(predictions)
    return sum(abs(p-a) for p, a in zip(predictions, actuals)) / n

def main():
    train_data = sinh_du_lieu(n = 1000)
    test_data = sinh_du_lieu(n = 200)

    for i in range (5):
        print (train_data[i])

    ann = ANN(hidden_size = 1, learning_rate = 0.1)

    final_loss = ann.huan_luyen(train_data, it = 1000, verbose = True)

    print ("Loss cuối: ", final_loss)

    predictions = []
    actuals = []

    for x, y_true in test_data:
        y_pred = ann.predict(x)
        predictions.append(y_pred)
        actuals.append(y_true)
    
    mse_rsl = mse(predictions, actuals)
    mae_rsl = mae(predictions, actuals)

    print ("MSE: ", mse_rsl, "; MAE: ", mae_rsl)

    test_points = [
        -math.pi / 4,
        -math.pi / 8,
        0,
        math.pi / 8,
        math.pi / 4
    ]

    for x in test_points:
        y_true = math.sin(x)
        y_pred = ann.predict(x)
        error = abs(y_pred - y_true)
        print(f"x = {x:>7.4f}: sin(x) = {y_true:>7.4f}, "
            f"dự đoán = {y_pred:>7.4f}, sai số = {error:.4f}")

if __name__ == "__main__":
    main()