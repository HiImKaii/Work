import numpy as np
import pandas as pd

import yfinance as yf
import matplotlib.pyplot as plt

import torch    # frame work chinh;
import torch.nn as nn   #cac lop mang, cac mo hinh trien khai;
from torch.utils.data import DataLoader, TensorDataset

from sklearn.preprocessing import MinMaxScaler

TICKER     = "NVDA"
START      = "2010-01-01"
END        = "2026-03-01"
SPLIT_DATE = "2024-01-01"
SEQ_LEN    = 60

df = yf.download(TICKER, start = START, end = END)
close = df["Close"].dropna()

prices = close.values.reshape(-1,1)
scaler = MinMaxScaler(feature_range = (0,1))
scaled = scaler.fit_transform(prices).flatten()

# Tạo chuỗi
def make_lables(data, seq_len):
    X, y = [], []
    for i in range(len(data) - seq_len):
        X.append(data[i: i + seq_len])
        y.append(data[i + seq_len])
    return np.array(X), np.array(y)

X, y = make_lables(scaled, SEQ_LEN)

split_idx = close.index.get_loc(
    close.index[close.index >= SPLIT_DATE][0]
)
split = split_idx - SEQ_LEN

X_train, X_test = X[:split], X[split:]
y_train, y_test = y[:split], y[split:]

# Thêm chiều features + chuyển sang tensor
X_train = X_train[:, :, np.newaxis]  # (N, 60, 1)
X_test  = X_test[:,  :, np.newaxis]
y_train = y_train[:, np.newaxis]     # (N, 1)
y_test  = y_test[:,  np.newaxis]

to_tensor = lambda a: torch.tensor(a, dtype = torch.float32)
X_train_t, y_train_t = to_tensor(X_train), to_tensor(y_train)
X_test_t, y_test_t = to_tensor(X_test), to_tensor(y_test)

# Xây model
class LSTMModel(nn.Module):
    def __init__(self, input_size = 1, hidden_size = 64, num_layers = 2, dropout = 0.2):
        super().__init__()

        self.lstm = nn.LSTM(
            input_size = input_size,
            hidden_size = hidden_size,
            num_layers = num_layers,
            dropout = dropout,
            batch_first = True
        )

        self.fc = nn.Linear(hidden_size, 1)

    def forward(self, x):
        out, _ =self.lstm(x)
        out = out[:, -1, :]
        return self.fc(out)
    
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = LSTMModel().to(DEVICE)
print(model)

# Huan luyen
BATCH_SIZE = 32
its = 100   
lr = 1e-4

train_ds = TensorDataset(X_train_t, y_train_t)
test_ds = TensorDataset(X_test_t, y_test_t)
train_dl = DataLoader(train_ds, batch_size = BATCH_SIZE, shuffle = True)
test_dl = DataLoader(test_ds, batch_size = BATCH_SIZE)

criterion = nn.MSELoss()   # đo sai số bình phương trung bình
optimizer = torch.optim.Adam(model.parameters(), lr=lr)

print(f"\nShape X_train : {X_train_t.shape}")  # [N, 60, 1]
print(f"Shape X_test  : {X_test_t.shape}")

train_losses, test_losses = [], []

for epoch in range(1, its + 1):

    # --- TRAIN ---
    model.train()
    total_train = 0.0
    for xb, yb in train_dl:
        xb, yb = xb.to(DEVICE), yb.to(DEVICE)

        optimizer.zero_grad()   # xóa gradient cũ
        pred = model(xb)        # dự đoán
        loss = criterion(pred, yb)  # tính loss
        loss.backward()         # lan truyền ngược
        optimizer.step()        # cập nhật trọng số

        total_train += loss.item() * len(xb)

    train_loss = total_train / len(train_ds)

    # --- EVALUATE trên test (không cập nhật trọng số) ---
    model.eval()
    total_test = 0.0
    with torch.no_grad():       # không tính gradient
        for xb, yb in test_dl:
            xb, yb = xb.to(DEVICE), yb.to(DEVICE)
            pred = model(xb)
            total_test += criterion(pred, yb).item() * len(xb)

    test_loss = total_test / len(test_ds)

    train_losses.append(train_loss)
    test_losses.append(test_loss)

    if epoch % 10 == 0 or epoch == 1:
        print(f"Epoch {epoch:3d}/{its} | Train Loss: {train_loss:.6f} | Test Loss: {test_loss:.6f}")


model.eval()
with torch.no_grad():
    y_pred_scaled = model(X_test_t.to(DEVICE)).cpu().numpy()

y_pred = scaler.inverse_transform(y_pred_scaled)
y_true = scaler.inverse_transform(y_test)

mae  = np.mean(np.abs(y_true - y_pred))
rmse = np.sqrt(np.mean((y_true - y_pred) ** 2))
print(f"MAE  = ${mae:.2f}")
print(f"RMSE = ${rmse:.2f}")

dates_test = close.index[split_idx:]

fig, axes = plt.subplots(2, 1, figsize=(14, 8))

axes[0].plot(dates_test, y_true, label="Thật",    color="#76b900", linewidth=1.5)
axes[0].plot(dates_test, y_pred, label="Dự đoán", color="#FF5722", linewidth=1.5, linestyle="--")
axes[0].set_title("Giá thật vs Dự đoán (tập test)")
axes[0].set_ylabel("Giá ($)")
axes[0].legend()
axes[0].grid(alpha=0.3)
axes[0].text(0.02, 0.95, f"MAE=${mae:.2f}  RMSE=${rmse:.2f}",
            transform=axes[0].transAxes,
            bbox=dict(boxstyle="round", facecolor="wheat", alpha=0.5))

axes[1].plot(range(1, its+1), train_losses, label="Train Loss", color="#2196F3")
axes[1].plot(range(1, its+1), test_losses,  label="Test Loss",  color="#F44336")
axes[1].set_title("Loss qua các epoch")
axes[1].set_xlabel("Epoch")
axes[1].set_ylabel("MSE Loss")
axes[1].legend()
axes[1].grid(alpha=0.3)

plt.tight_layout()
plt.show()