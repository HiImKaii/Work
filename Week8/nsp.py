from pulp import *

NUM_NURSES = 1983
NUM_DAYS = 7
SHIFT = ["sang", "chieu", "toi"]
NUM_SHIFTS_PER_DAY = len(SHIFT)
NUM_HEAD_NUR = 1234


COST_NORMAL = 1000
COST_OVERTIME = 1200
COST_HEAD = 1500

MIN_AFTERNOON = 2   
MIN_NIGHT = 1 
MIN_HEAD = 150 # Mỗi ca sáng phải có ít nhất từng này y tá trưởng

nurses_data = []

for i in range(NUM_HEAD_NUR):
    nurses_data.append({
        "id": i,
        "is_head": True,
        "is_female": True,
        "min_shift": 5,
        "max_shift": 9,
    })

for i in range(NUM_NURSES - NUM_HEAD_NUR):
    nurses_data.append({
        "id": i + NUM_HEAD_NUR,
        "is_head": False,
        "is_female": (i >= 664),
        "min_shift": 6,
        "max_shift": 9
    })

so_y_ta_yeu_cau_moi_ca = {
    "sang": 542,
    "chieu": 438,
    "toi": 225
}


nurses = [n["id"] for n in nurses_data]
days = list(range(NUM_DAYS))

num_female = sum(1 for n in nurses_data if n["is_female"])
num_male = NUM_NURSES - num_female


problem = LpProblem("NSP", LpMinimize)

x = LpVariable.dicts(
    "x",
    [(i, d, s) for i in nurses for d in days for s in SHIFT],
    cat = "Binary"
)

overtime = LpVariable.dicts(
    "overtime",
    nurses,
    lowBound = 0,
    cat = "Continuous"
)

head_nurses = [n["id"] for n in nurses_data if n["is_head"]]
nor_nurses = [n["id"] for n in nurses_data if not n["is_head"]]

cost_nor = lpSum(
    COST_NORMAL * x[i, d, s]
    for i in nor_nurses
    for d in days
    for s in SHIFT
)

cost_overtime = lpSum(
    (COST_OVERTIME - COST_NORMAL) * overtime[i]
    for i in nor_nurses
)

cost_head = lpSum(
    COST_HEAD * x[i, d, s]
    for i in head_nurses
    for d in days
    for s in SHIFT
)

problem += cost_nor + cost_overtime + cost_head, "Tong Chi Phi"

nurse_lookup = {n["id"]: n for n in nurses_data}

for i in nor_nurses:
    min_s = nurse_lookup[i]["min_shift"]
    problem += (
        overtime[i] >= lpSum(x[i, d, s] for d in days for s in SHIFT) - min_s,
        f"overtime_{i}"
    )

#1: đủ số y tá mỗi ca
for d in days:
    for s in SHIFT:
        problem += (
            lpSum(x[i, d, s] for i in nurses) >= so_y_ta_yeu_cau_moi_ca[s],
            f"yeu_cau_ca{d}_{s}"
        )

#2, 3: min/max ca mỗi y tá
for n in nurses_data:
    i = n["id"]
    total_worker = lpSum(x[i, d, s] for d in days for s in SHIFT)
    problem += (total_worker >= n["min_shift"], f"min_ca_{i}")
    problem += (total_worker <= n["max_shift"], f"max_ca_{i}")

#4: y tá thường ít nhất 2 ca chiều
for i in nor_nurses:
    problem += (
        lpSum(x[i, d, "chieu"] for d in days) >= MIN_AFTERNOON,
        f"min_ca_chieu_{i}"
    )

#5 y tá thường ít nhất 1 ca tối
for i in nor_nurses:
    problem += (
        lpSum(x[i, d, "toi"] for d in days) >= MIN_NIGHT,
        f"min_ca_toi_{i}"
    )

#6 y tá trưởng không làm ca chiều và tối
for i in head_nurses:
    for d in days:
        problem += (x[i, d, "chieu"] == 0)
        problem += (x[i, d, "toi"] == 0)

#7 ít nhất 5 y tá trưởng mỗi ca sáng
for d in days:
    problem += (
        lpSum(x[i, d, "sang"] for i in head_nurses) >= MIN_HEAD
    )

#8 mỗi ca có ýt nhất 1 y tá nữ
female_nurse = [n["id"] for n in nurses_data if n["is_female"]]
for d in days:
    for s in SHIFT:
        problem += (
            lpSum(x[i, d, s] for i in female_nurse) >= 1
        )

#9 Ca làm thứ 2 tính từ sau ca làm hiện tại không được làm
total_shift = NUM_DAYS * NUM_SHIFTS_PER_DAY
all_shift = [(d, s) for d in days for s in SHIFT]

for i in nor_nurses:
    for k in range(total_shift - 2):
        d1, s1 = all_shift[k]
        d2, s2 = all_shift[k + 2]
        problem += (
            x[i, d1, s1] + x[i, d2, s2] <= 1
        )

#10: trong 5 ca liên tiếp chỉ được làm tối đa 2 ca
for i in nor_nurses:
    for k in range(total_shift - 4):
        window = all_shift[k:k+5] # cửa sổ trượt qua các ca, trong cái cửa sổ đó chỉ có tối đa 2 ca được làm
        problem += (
            lpSum(x[i, d, s] for d, s in window) <= 2
        )

problem.solve(HiGHS())

if problem.status == 1:
    print("SUCCESS")
    print(f"TOTAL_COST={value(problem.objective):.0f}")
else:
    print("FAILED")


