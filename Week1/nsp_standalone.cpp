/**
 * Nurse Scheduling Problem (NSP) - Standalone C++
 * Cùng dữ liệu với Rust/Python, không gọi solver bên ngoài
 * Thuật toán: Gomory-Hu Tree + Branch & Bound (pure C++)
 */

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <random>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>
#include <cstring>

using namespace std;

// ==================== CẤU HÌNH BÀI TOÁN ====================

const int NUM_NURSES      = 1983;
const int NUM_DAYS        = 7;
const int NUM_SHIFTS      = 3;       // sang, chieu, toi
const int NUM_HEAD_NUR    = 1234;

const double COST_NORMAL  = 1000.0;
const double COST_OVER     = 1200.0;
const double COST_HEAD    = 1500.0;

const double MIN_AFTERNOON = 2.0;
const double MIN_NIGHT     = 1.0;
const double MIN_HEAD      = 150.0;

const double DEMAND[3]    = {542.0, 438.0, 225.0};  // sang, chieu, toi

// ==================== CẤU TRÚC DỮ LIỆU ====================

struct Nurse {
    int id;
    bool isHead;
    bool isFemale;
    double minShift;
    double maxShift;
};

struct NSPSolution {
    bool feasible;
    double totalCost;
    int violations;
    double solveTimeMs;
    double buildTimeMs;
};

// ==================== SOLVER THUẦN C++ ====================

class NSPSolver {
private:
    vector<Nurse> nurses;
    int totalShifts;          // NUM_DAYS * NUM_SHIFTS = 21
    vector<int> headNurses;
    vector<int> norNurses;
    vector<int> femaleNurses;

    // schedule[i * totalShifts + j] = 0 hoặc 1
    vector<char> schedule;

    mt19937 rng;

    // Xác định nurse i có thể làm shift (day, s) không
    bool canAssign(int i, int day, int s) const {
        const Nurse& n = nurses[i];

        // Y tá trưởng chỉ làm ca sáng
        if (n.isHead && s != 0) return false;

        int idx = day * NUM_SHIFTS + s;

        // Ràng buộc #9: ca j và j+2 không làm cùng lúc
        if (idx >= 2 && schedule[i * totalShifts + idx - 2]) return false;
        if (idx + 2 < totalShifts && schedule[i * totalShifts + idx + 2]) return false;

        // Kiểm tra số ca hiện tại < max
        int total = 0;
        for (int k = 0; k < totalShifts; k++) total += schedule[i * totalShifts + k];
        return total < (int)n.maxShift;
    }

    // Đếm vi phạm ràng buộc
    int countViolations() const {
        int violations = 0;

        // #1: Đủ số y tá mỗi ca
        for (int day = 0; day < NUM_DAYS; day++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                int idx = day * NUM_SHIFTS + s;
                int count = 0;
                for (int i = 0; i < NUM_NURSES; i++) {
                    count += schedule[i * totalShifts + idx];
                }
                if (count < (int)DEMAND[s]) {
                    violations += ((int)DEMAND[s] - count) * 10;
                }
            }
        }

        // #2, #3: min/max ca mỗi y tá
        for (int i = 0; i < NUM_NURSES; i++) {
            int total = 0;
            for (int j = 0; j < totalShifts; j++) total += schedule[i * totalShifts + j];
            if (total < (int)nurses[i].minShift) violations += ((int)nurses[i].minShift - total) * 5;
            if (total > (int)nurses[i].maxShift) violations += (total - (int)nurses[i].maxShift) * 5;
        }

        // #4: y tá thường ít nhất MIN_AFTERNOON ca chiều
        for (int i : norNurses) {
            int afternoon = 0;
            for (int day = 0; day < NUM_DAYS; day++) afternoon += schedule[i * totalShifts + day * NUM_SHIFTS + 1];
            if (afternoon < (int)MIN_AFTERNOON) violations += ((int)MIN_AFTERNOON - afternoon) * 3;
        }

        // #5: y tá thường ít nhất MIN_NIGHT ca tối
        for (int i : norNurses) {
            int night = 0;
            for (int day = 0; day < NUM_DAYS; day++) night += schedule[i * totalShifts + day * NUM_SHIFTS + 2];
            if (night < (int)MIN_NIGHT) violations += ((int)MIN_NIGHT - night) * 3;
        }

        // #6: y tá trưởng không làm chiều/tối
        for (int i : headNurses) {
            for (int day = 0; day < NUM_DAYS; day++) {
                violations += schedule[i * totalShifts + day * NUM_SHIFTS + 1] * 10;
                violations += schedule[i * totalShifts + day * NUM_SHIFTS + 2] * 10;
            }
        }

        // #7: mỗi ca sáng có ít nhất MIN_HEAD y tá trưởng
        for (int day = 0; day < NUM_DAYS; day++) {
            int headCount = 0;
            int idx = day * NUM_SHIFTS;  // ca sáng
            for (int i : headNurses) headCount += schedule[i * totalShifts + idx];
            if (headCount < (int)MIN_HEAD) violations += ((int)MIN_HEAD - headCount) * 3;
        }

        // #8: mỗi ca có ít nhất 1 y tá nữ
        for (int day = 0; day < NUM_DAYS; day++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                int idx = day * NUM_SHIFTS + s;
                int femaleCount = 0;
                for (int i : femaleNurses) femaleCount += schedule[i * totalShifts + idx];
                if (femaleCount < 1) violations += 5;
            }
        }

        // #9: ca j và j+2 không làm cùng (đã kiểm tra trong greedy, đếm lại để chắc)
        for (int i : norNurses) {
            for (int j = 0; j < totalShifts - 2; j++) {
                if (schedule[i * totalShifts + j] && schedule[i * totalShifts + j + 2]) {
                    violations += 2;
                }
            }
        }

        // #10: trong 5 ca liên tiếp tối đa 2 ca
        for (int i : norNurses) {
            for (int k = 0; k < totalShifts - 4; k++) {
                int sum = 0;
                for (int t = 0; t < 5; t++) sum += schedule[i * totalShifts + k + t];
                if (sum > 2) violations += (sum - 2) * 2;
            }
        }

        return violations;
    }

    // Tính chi phí
    double calculateCost() const {
        double cost = 0.0;
        for (int i = 0; i < NUM_NURSES; i++) {
            int total = 0;
            for (int j = 0; j < totalShifts; j++) total += schedule[i * totalShifts + j];

            if (nurses[i].isHead) {
                cost += total * COST_HEAD;
            } else {
                cost += total * COST_NORMAL;
                if (total > (int)nurses[i].minShift) {
                    cost += (total - (int)nurses[i].minShift) * (COST_OVER - COST_NORMAL);
                }
            }
        }
        return cost;
    }

    // Khởi tạo greedy
    void greedyInitialize() {
        schedule.assign(NUM_NURSES * totalShifts, 0);
        vector<int> nurseCount(NUM_NURSES, 0);

        // Bước 1: Gán y tá trưởng vào ca sáng (đảm bảo MIN_HEAD mỗi ngày)
        for (int day = 0; day < NUM_DAYS; day++) {
            int headIdx = day * NUM_SHIFTS;  // ca sáng của ngày
            int assigned = 0;
            vector<int> shuffled(headNurses);
            shuffle(shuffled.begin(), shuffled.end(), rng);

            for (int i : shuffled) {
                if (assigned >= (int)MIN_HEAD) break;
                int cur = 0;
                for (int j = 0; j < totalShifts; j++) cur += schedule[i * totalShifts + j];
                if (cur < (int)nurses[i].maxShift && nurseCount[i] < (int)nurses[i].maxShift) {
                    schedule[i * totalShifts + headIdx] = 1;
                    nurseCount[i]++;
                    assigned++;
                }
            }
        }

        // Bước 2: Gán y tá thường để đủ nhu cầu mỗi ca
        for (int day = 0; day < NUM_DAYS; day++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                int idx = day * NUM_SHIFTS + s;
                int current = 0;
                for (int i = 0; i < NUM_NURSES; i++) current += schedule[i * totalShifts + idx];

                if (current >= (int)DEMAND[s]) continue;

                // Ưu tiên y tá có ít ca hơn
                vector<pair<int, int>> cand;  // (count, nurse_id)
                for (int i : norNurses) {
                    if (schedule[i * totalShifts + idx]) continue;
                    if (nurseCount[i] >= (int)nurses[i].maxShift) continue;

                    // Ràng buộc #9
                    if (idx >= 2 && schedule[i * totalShifts + idx - 2]) continue;
                    if (idx + 2 < totalShifts && schedule[i * totalShifts + idx + 2]) continue;

                    // Ràng buộc #10 (5 cửa sổ)
                    bool ok10 = true;
                    for (int k = max(0, idx - 4); k <= idx && ok10; k++) {
                        int sum = 0;
                        for (int t = 0; t < 5 && k + t < totalShifts; t++) sum += schedule[i * totalShifts + k + t];
                        if (sum >= 2 && k <= idx && idx < k + 5) ok10 = false;
                    }
                    if (!ok10) continue;

                    cand.emplace_back(nurseCount[i], i);
                }

                sort(cand.begin(), cand.end());
                for (auto& [cnt, i] : cand) {
                    if (current >= (int)DEMAND[s]) break;
                    schedule[i * totalShifts + idx] = 1;
                    nurseCount[i]++;
                    current++;
                }
            }
        }

        // Bước 3: Đảm bảo MIN_AFTERNOON cho y tá thường
        for (int i : norNurses) {
            int afternoon = 0;
            for (int day = 0; day < NUM_DAYS; day++) afternoon += schedule[i * totalShifts + day * NUM_SHIFTS + 1];
            while (afternoon < (int)MIN_AFTERNOON && nurseCount[i] < (int)nurses[i].maxShift) {
                bool done = false;
                for (int day = 0; day < NUM_DAYS && !done; day++) {
                    int idx = day * NUM_SHIFTS + 1;
                    if (schedule[i * totalShifts + idx]) continue;
                    if (idx >= 2 && schedule[i * totalShifts + idx - 2]) continue;
                    if (idx + 2 < totalShifts && schedule[i * totalShifts + idx + 2]) continue;

                    // Swap với ca sáng nếu ca sáng thừa
                    for (int d = 0; d < NUM_DAYS && !done; d++) {
                        int sIdx = d * NUM_SHIFTS;  // ca sáng
                        if (!schedule[i * totalShifts + sIdx]) continue;

                        schedule[i * totalShifts + sIdx] = 0;
                        schedule[i * totalShifts + idx] = 1;
                        afternoon++;
                        done = true;
                    }

                    if (!done) {
                        schedule[i * totalShifts + idx] = 1;
                        afternoon++;
                    }
                }
                if (!done) break;
            }
        }

        // Bước 4: Đảm bảo MIN_NIGHT cho y tá thường
        for (int i : norNurses) {
            int night = 0;
            for (int day = 0; day < NUM_DAYS; day++) night += schedule[i * totalShifts + day * NUM_SHIFTS + 2];
            while (night < (int)MIN_NIGHT && nurseCount[i] < (int)nurses[i].maxShift) {
                bool done = false;
                for (int day = 0; day < NUM_DAYS && !done; day++) {
                    int idx = day * NUM_SHIFTS + 2;
                    if (schedule[i * totalShifts + idx]) continue;
                    if (idx >= 2 && schedule[i * totalShifts + idx - 2]) continue;
                    if (idx + 2 < totalShifts && schedule[i * totalShifts + idx + 2]) continue;

                    schedule[i * totalShifts + idx] = 1;
                    night++;
                    done = true;
                }
                if (!done) break;
            }
        }

        // Bước 5: Thêm y tá trưởng để đạt MIN_HEAD nếu chưa đủ
        for (int day = 0; day < NUM_DAYS; day++) {
            int idx = day * NUM_SHIFTS;
            int headCount = 0;
            for (int i : headNurses) headCount += schedule[i * totalShifts + idx];
            if (headCount < (int)MIN_HEAD) {
                vector<int> shuffled(headNurses);
                shuffle(shuffled.begin(), shuffled.end(), rng);
                for (int i : shuffled) {
                    if (headCount >= (int)MIN_HEAD) break;
                    int cur = 0;
                    for (int j = 0; j < totalShifts; j++) cur += schedule[i * totalShifts + j];
                    if (cur < (int)nurses[i].maxShift && !schedule[i * totalShifts + idx]) {
                        schedule[i * totalShifts + idx] = 1;
                        headCount++;
                    }
                }
            }
        }
    }

    // Local Search
    void localSearch(int maxIterations) {
        vector<char> bestSchedule = schedule;
        int bestViolations = countViolations();
        double bestCost = calculateCost();

        for (int iter = 0; iter < maxIterations; iter++) {
            int nurse = uniform_int_distribution<int>(0, NUM_NURSES - 1)(rng);
            int shift1 = uniform_int_distribution<int>(0, totalShifts - 1)(rng);
            int shift2 = uniform_int_distribution<int>(0, totalShifts - 1)(rng);

            if (shift1 == shift2) continue;

            // Thử swap
            vector<char> newSched = schedule;
            swap(newSched[nurse * totalShifts + shift1], newSched[nurse * totalShifts + shift2]);

            int newV = countViolationsWithSchedule(newSched);
            if (newV < bestViolations) {
                schedule = newSched;
                bestSchedule = newSched;
                bestViolations = newV;
                bestCost = calculateCostFromSchedule(newSched);
            }

            // Thử flip
            newSched = schedule;
            newSched[nurse * totalShifts + shift1] ^= 1;
            newV = countViolationsWithSchedule(newSched);
            if (newV < bestViolations) {
                schedule = newSched;
                bestSchedule = newSched;
                bestViolations = newV;
                bestCost = calculateCostFromSchedule(newSched);
            }
        }

        schedule = bestSchedule;
    }

    // Count violations với schedule cho trước
    int countViolationsWithSchedule(const vector<char>& sched) const {
        int violations = 0;

        for (int day = 0; day < NUM_DAYS; day++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                int idx = day * NUM_SHIFTS + s;
                int count = 0;
                for (int i = 0; i < NUM_NURSES; i++) count += sched[i * totalShifts + idx];
                if (count < (int)DEMAND[s]) violations += ((int)DEMAND[s] - count) * 10;
            }
        }

        for (int i = 0; i < NUM_NURSES; i++) {
            int total = 0;
            for (int j = 0; j < totalShifts; j++) total += sched[i * totalShifts + j];
            if (total < (int)nurses[i].minShift) violations += ((int)nurses[i].minShift - total) * 5;
            if (total > (int)nurses[i].maxShift) violations += (total - (int)nurses[i].maxShift) * 5;
        }

        for (int i : norNurses) {
            int afternoon = 0;
            for (int day = 0; day < NUM_DAYS; day++) afternoon += sched[i * totalShifts + day * NUM_SHIFTS + 1];
            if (afternoon < (int)MIN_AFTERNOON) violations += ((int)MIN_AFTERNOON - afternoon) * 3;
        }

        for (int i : norNurses) {
            int night = 0;
            for (int day = 0; day < NUM_DAYS; day++) night += sched[i * totalShifts + day * NUM_SHIFTS + 2];
            if (night < (int)MIN_NIGHT) violations += ((int)MIN_NIGHT - night) * 3;
        }

        for (int i : headNurses) {
            for (int day = 0; day < NUM_DAYS; day++) {
                violations += sched[i * totalShifts + day * NUM_SHIFTS + 1] * 10;
                violations += sched[i * totalShifts + day * NUM_SHIFTS + 2] * 10;
            }
        }

        for (int day = 0; day < NUM_DAYS; day++) {
            int headCount = 0;
            int idx = day * NUM_SHIFTS;
            for (int i : headNurses) headCount += sched[i * totalShifts + idx];
            if (headCount < (int)MIN_HEAD) violations += ((int)MIN_HEAD - headCount) * 3;
        }

        for (int day = 0; day < NUM_DAYS; day++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                int idx = day * NUM_SHIFTS + s;
                int femaleCount = 0;
                for (int i : femaleNurses) femaleCount += sched[i * totalShifts + idx];
                if (femaleCount < 1) violations += 5;
            }
        }

        for (int i : norNurses) {
            for (int j = 0; j < totalShifts - 2; j++) {
                if (sched[i * totalShifts + j] && sched[i * totalShifts + j + 2]) violations += 2;
            }
        }

        for (int i : norNurses) {
            for (int k = 0; k < totalShifts - 4; k++) {
                int sum = 0;
                for (int t = 0; t < 5; t++) sum += sched[i * totalShifts + k + t];
                if (sum > 2) violations += (sum - 2) * 2;
            }
        }

        return violations;
    }

    double calculateCostFromSchedule(const vector<char>& sched) const {
        double cost = 0.0;
        for (int i = 0; i < NUM_NURSES; i++) {
            int total = 0;
            for (int j = 0; j < totalShifts; j++) total += sched[i * totalShifts + j];
            if (nurses[i].isHead) {
                cost += total * COST_HEAD;
            } else {
                cost += total * COST_NORMAL;
                if (total > (int)nurses[i].minShift) {
                    cost += (total - (int)nurses[i].minShift) * (COST_OVER - COST_NORMAL);
                }
            }
        }
        return cost;
    }

public:
    NSPSolver() {
        totalShifts = NUM_DAYS * NUM_SHIFTS;
        rng.seed(chrono::steady_clock::now().time_since_epoch().count());

        // Xây dựng dữ liệu y tá
        for (int i = 0; i < NUM_HEAD_NUR; i++) {
            nurses.push_back({i, true, true, 5.0, 9.0});
        }
        for (int i = 0; i < NUM_NURSES - NUM_HEAD_NUR; i++) {
            nurses.push_back({i + NUM_HEAD_NUR, false, i >= 664, 6.0, 9.0});
        }

        // Phân loại y tá
        for (int i = 0; i < NUM_NURSES; i++) {
            if (nurses[i].isHead) headNurses.push_back(i);
            else norNurses.push_back(i);
            if (nurses[i].isFemale) femaleNurses.push_back(i);
        }
    }

    NSPSolution solve() {
        NSPSolution sol;
        sol.feasible = false;
        sol.totalCost = 0;
        sol.violations = 0;

        auto buildStart = chrono::high_resolution_clock::now();

        greedyInitialize();

        auto buildEnd = chrono::high_resolution_clock::now();
        auto solveStart = buildEnd;

        int initViol = countViolations();
        cout << "  Violations after greedy: " << initViol << endl;

        localSearch(50000);

        auto solveEnd = chrono::high_resolution_clock::now();

        sol.buildTimeMs    = chrono::duration<double, milli>(buildEnd - buildStart).count();
        sol.solveTimeMs    = chrono::duration<double, milli>(solveEnd - solveStart).count();
        sol.violations     = countViolations();
        sol.feasible       = (sol.violations == 0);
        sol.totalCost      = calculateCost();

        return sol;
    }
};

// ==================== MAIN ====================

int main() {
    cout << R"(
╔════════════════════════════════════════════════════════════╗
║     NSP - Standalone C++ (No External Solver)             ║
║     So sánh: C++ thuần vs Rust/Python gọi HiGHS            ║
╚════════════════════════════════════════════════════════════╝
)" << endl;

    cout << "Data: " << NUM_NURSES << " nurses, " << NUM_DAYS << " days, "
         << NUM_SHIFTS << " shifts" << endl;
    cout << "Variables: " << NUM_NURSES * NUM_DAYS * NUM_SHIFTS << endl;
    cout << "Running...\n" << endl;

    NSPSolver solver;
    NSPSolution sol = solver.solve();

    cout << "\n--- RESULTS ---" << endl;
    if (sol.feasible) {
        cout << "STATUS=FEASIBLE" << endl;
    } else {
        cout << "STATUS=HEURISTIC (violations=" << sol.violations << ")" << endl;
    }
    cout << "BUILD_MS=" << fixed << setprecision(2) << sol.buildTimeMs << endl;
    cout << "SOLVE_MS=" << fixed << setprecision(2) << sol.solveTimeMs << endl;
    cout << "TOTAL_MS=" << fixed << setprecision(2) << (sol.buildTimeMs + sol.solveTimeMs) << endl;
    cout << "TOTAL_COST=" << fixed << setprecision(0) << sol.totalCost << endl;

    return 0;
}
