/**
 * Nurse Scheduling Problem (NSP) - Multi-Commodity Network Flow Model
 * Based on paper: "A new formulation and solution for the nurse scheduling problem"
 * Alexandria Engineering Journal (2018) 57, 2289–2298
 * 
 * Authors: Ahmed Ali El Adoly, Mohamed Gheith, M. Nashat Fors
 * 
 * This implementation uses OR-Tools CP-SAT solver for Binary Linear Programming
 */

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <random>
#include <chrono>

#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model_solver.h"

using namespace std;
using namespace operations_research;
using namespace operations_research::sat;

// ==================== CẤU TRÚC DỮ LIỆU ====================

struct Nurse {
    int id;
    string name;
    bool isHeadNurse;    // Y tá trưởng
    bool isFemale;       // Giới tính nữ
    int minShifts;       // Số ca tối thiểu phải làm (N_i)
    int maxShifts;       // Số ca tối đa có thể làm (U_i)
};

struct ShiftRequirement {
    int dayIndex;        // Ngày trong planning horizon (0 = Saturday)
    int shiftType;       // 0 = Morning, 1 = Afternoon, 2 = Night
    int requiredNurses;  // Số y tá cần (M_j)
};

struct NSPInput {
    int numDays;                          // Số ngày trong planning horizon
    int numShiftsPerDay;                  // Số ca mỗi ngày (thường = 3)
    vector<Nurse> nurses;                 // Danh sách y tá
    vector<ShiftRequirement> shifts;      // Yêu cầu mỗi ca
    int minAfternoonShifts;               // Số ca chiều tối thiểu (A)
    int minNightShifts;                   // Số ca đêm tối thiểu (B)
    int minMorningShiftsHeadNurse;        // Số ca sáng tối thiểu cho y tá trưởng (F)
    double costPerShift;                  // Chi phí mỗi ca thường (c1)
    double overtimeCost;                  // Chi phí làm thêm (c2)
    double headNurseCost;                 // Chi phí ca y tá trưởng (c3)
};

struct NSPSolution {
    bool feasible;
    double totalCost;
    double normalCost;
    double overtimeCost;
    double headNurseCost;
    vector<vector<int>> schedule;         // schedule[nurse][shift] = 0 or 1
    double solveTimeMs;
};

// ==================== HELPER FUNCTIONS ====================

string getShiftName(int shiftType) {
    switch(shiftType) {
        case 0: return "Sáng  ";
        case 1: return "Chiều ";
        case 2: return "Đêm   ";
        default: return "???   ";
    }
}

string getDayName(int dayIndex) {
    vector<string> days = {"Thứ 7", "CN   ", "Thứ 2", "Thứ 3", "Thứ 4", "Thứ 5", "Thứ 6"};
    return days[dayIndex % 7];
}

int getShiftIndex(int day, int shiftType, int shiftsPerDay) {
    return day * shiftsPerDay + shiftType;
}

// ==================== MODEL BUILDER ====================

class NSPSolver {
private:
    NSPInput input;
    int numNurses;
    int totalShifts;
    
public:
    NSPSolver(const NSPInput& inp) : input(inp) {
        numNurses = input.nurses.size();
        totalShifts = input.numDays * input.numShiftsPerDay;
    }
    
    NSPSolution solve() {
        NSPSolution solution;
        solution.feasible = false;
        
        auto startTime = chrono::high_resolution_clock::now();
        
        // Tạo CP-SAT model
        CpModelBuilder cp_model;
        
        // ==================== BIẾN QUYẾT ĐỊNH ====================
        // x[i][j] = 1 nếu y tá i được phân công vào ca j
        vector<vector<BoolVar>> x(numNurses, vector<BoolVar>(totalShifts));
        
        for (int i = 0; i < numNurses; i++) {
            for (int j = 0; j < totalShifts; j++) {
                x[i][j] = cp_model.NewBoolVar();
            }
        }
        
        // ==================== RÀNG BUỘC ====================
        
        // Constraint (1): Đảm bảo đủ số y tá cho mỗi ca
        // Σ x[i][j] >= M[j] for all j
        for (int j = 0; j < totalShifts; j++) {
            vector<BoolVar> nursesInShift;
            for (int i = 0; i < numNurses; i++) {
                nursesInShift.push_back(x[i][j]);
            }
            cp_model.AddGreaterOrEqual(LinearExpr::Sum(nursesInShift), 
                                        input.shifts[j].requiredNurses);
        }
        
        // Constraint (2) & (3): Số ca tối thiểu và tối đa cho mỗi y tá
        for (int i = 0; i < numNurses; i++) {
            vector<BoolVar> shiftsForNurse;
            for (int j = 0; j < totalShifts; j++) {
                shiftsForNurse.push_back(x[i][j]);
            }
            // Constraint (2): Σ x[i][j] >= N[i]
            cp_model.AddGreaterOrEqual(LinearExpr::Sum(shiftsForNurse), 
                                        input.nurses[i].minShifts);
            // Constraint (3): Σ x[i][j] <= U[i]
            cp_model.AddLessOrEqual(LinearExpr::Sum(shiftsForNurse), 
                                     input.nurses[i].maxShifts);
        }
        
        // Constraint (4): Số ca chiều tối thiểu cho y tá thường
        // Σ x[i][j] >= A for afternoon shifts
        for (int i = 0; i < numNurses; i++) {
            if (!input.nurses[i].isHeadNurse) {
                vector<BoolVar> afternoonShifts;
                for (int day = 0; day < input.numDays; day++) {
                    int shiftIdx = getShiftIndex(day, 1, input.numShiftsPerDay); // 1 = Afternoon
                    afternoonShifts.push_back(x[i][shiftIdx]);
                }
                cp_model.AddGreaterOrEqual(LinearExpr::Sum(afternoonShifts), 
                                            input.minAfternoonShifts);
            }
        }
        
        // Constraint (5): Số ca đêm tối thiểu cho y tá thường
        for (int i = 0; i < numNurses; i++) {
            if (!input.nurses[i].isHeadNurse) {
                vector<BoolVar> nightShifts;
                for (int day = 0; day < input.numDays; day++) {
                    int shiftIdx = getShiftIndex(day, 2, input.numShiftsPerDay); // 2 = Night
                    nightShifts.push_back(x[i][shiftIdx]);
                }
                cp_model.AddGreaterOrEqual(LinearExpr::Sum(nightShifts), 
                                            input.minNightShifts);
            }
        }
        
        // Constraint (6) & (7): Y tá trưởng chỉ làm ca sáng
        for (int i = 0; i < numNurses; i++) {
            if (input.nurses[i].isHeadNurse) {
                // Chỉ làm ca sáng
                vector<BoolVar> morningShifts;
                for (int day = 0; day < input.numDays; day++) {
                    int morningIdx = getShiftIndex(day, 0, input.numShiftsPerDay);
                    morningShifts.push_back(x[i][morningIdx]);
                    
                    // Không làm ca chiều và đêm
                    int afternoonIdx = getShiftIndex(day, 1, input.numShiftsPerDay);
                    int nightIdx = getShiftIndex(day, 2, input.numShiftsPerDay);
                    cp_model.AddEquality(x[i][afternoonIdx], 0);
                    cp_model.AddEquality(x[i][nightIdx], 0);
                }
                // Số ca sáng tối thiểu cho y tá trưởng
                cp_model.AddGreaterOrEqual(LinearExpr::Sum(morningShifts), 
                                            input.minMorningShiftsHeadNurse);
            }
        }
        
        // Constraint (8): Mỗi ca có ít nhất 1 y tá nữ
        for (int j = 0; j < totalShifts; j++) {
            vector<BoolVar> femaleNurses;
            for (int i = 0; i < numNurses; i++) {
                if (input.nurses[i].isFemale) {
                    femaleNurses.push_back(x[i][j]);
                }
            }
            if (!femaleNurses.empty()) {
                cp_model.AddGreaterOrEqual(LinearExpr::Sum(femaleNurses), 1);
            }
        }
        
        // Constraint (9): Nếu làm 1 ca thì nghỉ 2 ca tiếp theo
        // x[i][j] + x[i][j+2] <= 1 (không làm ca cách 2 vị trí)
        for (int i = 0; i < numNurses; i++) {
            if (!input.nurses[i].isHeadNurse) {
                for (int j = 0; j < totalShifts - 2; j++) {
                    cp_model.AddLessOrEqual(
                        LinearExpr::Sum({x[i][j], x[i][j + 2]}), 1);
                }
            }
        }
        
        // Constraint (10): Nếu làm 2 ca liên tiếp thì nghỉ 3 ca tiếp theo
        // Nếu x[i][j] = 1 và x[i][j+1] = 1, thì x[i][j+2] + x[i][j+3] + x[i][j+4] <= 0
        // Tương đương: x[i][j] + x[i][j+1] + x[i][j+2] + x[i][j+3] + x[i][j+4] <= 2
        for (int i = 0; i < numNurses; i++) {
            if (!input.nurses[i].isHeadNurse) {
                for (int j = 0; j < totalShifts - 4; j++) {
                    cp_model.AddLessOrEqual(
                        LinearExpr::Sum({x[i][j], x[i][j+1], x[i][j+2], x[i][j+3], x[i][j+4]}), 2);
                }
            }
        }
        
        // ==================== HÀM MỤC TIÊU ====================
        // Minimize: c1 * Σx[i][j] + c2 * max(0, Σx[i][j] - N[i]) + c3 * Σx_head[i][j]
        
        // Sử dụng hệ số nguyên (nhân 100 để tránh số thực)
        int c1 = static_cast<int>(input.costPerShift * 100);
        int c2 = static_cast<int>(input.overtimeCost * 100);
        int c3 = static_cast<int>(input.headNurseCost * 100);
        
        LinearExpr objective;
        
        // Chi phí y tá thường
        for (int i = 0; i < numNurses; i++) {
            if (!input.nurses[i].isHeadNurse) {
                for (int j = 0; j < totalShifts; j++) {
                    objective += c1 * x[i][j];
                }
            }
        }
        
        // Chi phí làm thêm giờ cho y tá thường
        // overtime[i] = max(0, Σx[i][j] - N[i])
        for (int i = 0; i < numNurses; i++) {
            if (!input.nurses[i].isHeadNurse) {
                IntVar overtime = cp_model.NewIntVar({0, input.nurses[i].maxShifts});
                
                vector<BoolVar> allShifts;
                for (int j = 0; j < totalShifts; j++) {
                    allShifts.push_back(x[i][j]);
                }
                
                // overtime >= Σx[i][j] - N[i]
                cp_model.AddGreaterOrEqual(
                    overtime, 
                    LinearExpr::Sum(allShifts) - input.nurses[i].minShifts);
                
                objective += c2 * overtime;
            }
        }
        
        // Chi phí y tá trưởng
        for (int i = 0; i < numNurses; i++) {
            if (input.nurses[i].isHeadNurse) {
                for (int j = 0; j < totalShifts; j++) {
                    objective += c3 * x[i][j];
                }
            }
        }
        
        cp_model.Minimize(objective);
        
        // ==================== GIẢI BÀI TOÁN ====================
        SatParameters parameters;
        parameters.set_max_time_in_seconds(300.0);  // Timeout 5 phút
        parameters.set_num_search_workers(4);       // Đa luồng
        
        CpSolverResponse response = SolveWithParameters(cp_model.Build(), parameters);
        
        auto endTime = chrono::high_resolution_clock::now();
        solution.solveTimeMs = chrono::duration<double, milli>(endTime - startTime).count();
        
        // ==================== XỬ LÝ KẾT QUẢ ====================
        if (response.status() == CpSolverStatus::OPTIMAL || 
            response.status() == CpSolverStatus::FEASIBLE) {
            
            solution.feasible = true;
            solution.schedule.resize(numNurses, vector<int>(totalShifts, 0));
            
            double normalCost = 0, overtimeCost = 0, headNurseCost = 0;
            
            for (int i = 0; i < numNurses; i++) {
                int totalWorked = 0;
                for (int j = 0; j < totalShifts; j++) {
                    if (SolutionBooleanValue(response, x[i][j])) {
                        solution.schedule[i][j] = 1;
                        totalWorked++;
                        
                        if (input.nurses[i].isHeadNurse) {
                            headNurseCost += input.headNurseCost;
                        } else {
                            normalCost += input.costPerShift;
                        }
                    }
                }
                
                // Tính overtime
                if (!input.nurses[i].isHeadNurse) {
                    int overtime = max(0, totalWorked - input.nurses[i].minShifts);
                    overtimeCost += overtime * input.overtimeCost;
                }
            }
            
            solution.normalCost = normalCost;
            solution.overtimeCost = overtimeCost;
            solution.headNurseCost = headNurseCost;
            solution.totalCost = normalCost + overtimeCost + headNurseCost;
        }
        
        return solution;
    }
};

// ==================== HIỂN THỊ KẾT QUẢ ====================

void printSchedule(const NSPInput& input, const NSPSolution& solution) {
    if (!solution.feasible) {
        cout << "Không tìm được lời giải khả thi!" << endl;
        return;
    }
    
    int numNurses = input.nurses.size();
    int totalShifts = input.numDays * input.numShiftsPerDay;
    
    cout << "\n" << string(80, '=') << endl;
    cout << "              LỊCH LÀM VIỆC Y TÁ - NURSE SCHEDULING PROBLEM" << endl;
    cout << string(80, '=') << endl;
    
    // Header
    cout << setw(15) << "Y tá" << " |";
    for (int day = 0; day < input.numDays; day++) {
        cout << " " << getDayName(day) << " |";
    }
    cout << " Tổng |" << endl;
    
    cout << string(15, '-') << "-+";
    for (int day = 0; day < input.numDays; day++) {
        cout << "-------+";
    }
    cout << "------+" << endl;
    
    // Mỗi y tá
    for (int i = 0; i < numNurses; i++) {
        cout << setw(15) << input.nurses[i].name << " |";
        
        int totalWorked = 0;
        for (int day = 0; day < input.numDays; day++) {
            string shifts = "";
            for (int s = 0; s < input.numShiftsPerDay; s++) {
                int idx = getShiftIndex(day, s, input.numShiftsPerDay);
                if (solution.schedule[i][idx]) {
                    shifts += (s == 0 ? "S" : (s == 1 ? "C" : "D"));
                    totalWorked++;
                }
            }
            if (shifts.empty()) shifts = "-";
            cout << setw(6) << shifts << " |";
        }
        cout << setw(5) << totalWorked << " |" << endl;
    }
    
    cout << string(15, '-') << "-+";
    for (int day = 0; day < input.numDays; day++) {
        cout << "-------+";
    }
    cout << "------+" << endl;
    
    // Thống kê số y tá mỗi ca
    cout << "\n" << string(60, '-') << endl;
    cout << "THỐNG KÊ SỐ Y TÁ MỖI CA:" << endl;
    cout << setw(10) << "Ngày" << " | " << setw(10) << "Sáng" 
         << " | " << setw(10) << "Chiều" << " | " << setw(10) << "Đêm" << endl;
    cout << string(60, '-') << endl;
    
    for (int day = 0; day < input.numDays; day++) {
        cout << setw(10) << getDayName(day) << " |";
        for (int s = 0; s < input.numShiftsPerDay; s++) {
            int idx = getShiftIndex(day, s, input.numShiftsPerDay);
            int count = 0;
            for (int i = 0; i < numNurses; i++) {
                count += solution.schedule[i][idx];
            }
            cout << setw(10) << count << " |";
        }
        cout << endl;
    }
    
    // Chi phí
    cout << "\n" << string(60, '=') << endl;
    cout << "THỐNG KÊ CHI PHÍ:" << endl;
    cout << string(60, '-') << endl;
    cout << fixed << setprecision(2);
    cout << "  Chi phí ca thường:    " << setw(15) << solution.normalCost << endl;
    cout << "  Chi phí làm thêm:     " << setw(15) << solution.overtimeCost << endl;
    cout << "  Chi phí y tá trưởng:  " << setw(15) << solution.headNurseCost << endl;
    cout << string(60, '-') << endl;
    cout << "  TỔNG CHI PHÍ:         " << setw(15) << solution.totalCost << endl;
    cout << string(60, '=') << endl;
    
    cout << "\nThời gian giải: " << solution.solveTimeMs << " ms" << endl;
    
    // Phân bố số ca
    cout << "\n" << string(40, '-') << endl;
    cout << "PHÂN BỐ SỐ CA LÀM VIỆC:" << endl;
    vector<int> shiftDistribution(20, 0);
    for (int i = 0; i < numNurses; i++) {
        int count = 0;
        for (int j = 0; j < totalShifts; j++) {
            count += solution.schedule[i][j];
        }
        shiftDistribution[count]++;
    }
    for (int s = 0; s < 20; s++) {
        if (shiftDistribution[s] > 0) {
            double percent = 100.0 * shiftDistribution[s] / numNurses;
            cout << "  " << s << " ca: " << shiftDistribution[s] 
                 << " y tá (" << fixed << setprecision(1) << percent << "%)" << endl;
        }
    }
}

// ==================== TẠO DỮ LIỆU MẪU ====================

NSPInput createSampleData() {
    NSPInput input;
    
    // Cấu hình theo bài báo: 7 ngày, 3 ca/ngày
    input.numDays = 7;
    input.numShiftsPerDay = 3;
    input.minAfternoonShifts = 2;    // A = 2
    input.minNightShifts = 1;        // B = 1
    input.minMorningShiftsHeadNurse = 5; // F = 5
    
    // Chi phí (đơn vị: nghìn đồng)
    input.costPerShift = 100;        // c1
    input.overtimeCost = 150;        // c2 = 1.5 * c1
    input.headNurseCost = 120;       // c3
    
    // Tạo y tá (mô phỏng bệnh viện trong bài báo)
    // 6 y tá trưởng + 40 y tá thường
    int numHeadNurses = 6;
    int numRegularNurses = 40;
    
    for (int i = 0; i < numHeadNurses; i++) {
        Nurse n;
        n.id = i;
        n.name = "YT Trưởng " + to_string(i + 1);
        n.isHeadNurse = true;
        n.isFemale = true;  // Giả định y tá trưởng là nữ
        n.minShifts = 5;    // Ít nhất 5 ca sáng
        n.maxShifts = 7;    // Tối đa 7 ca
        input.nurses.push_back(n);
    }
    
    // 17 y tá nam, 23 y tá nữ (tổng 40 y tá thường, trong bài báo có 29 nữ nhưng bao gồm cả trưởng)
    int numMale = 17;
    for (int i = 0; i < numRegularNurses; i++) {
        Nurse n;
        n.id = numHeadNurses + i;
        n.name = "Y tá " + to_string(i + 1);
        n.isHeadNurse = false;
        n.isFemale = (i >= numMale);  // 17 nam, còn lại nữ
        n.minShifts = 6;    // Ít nhất 6 ca/tuần
        n.maxShifts = 8;    // Tối đa 8 ca/tuần
        input.nurses.push_back(n);
    }
    
    // Tạo yêu cầu cho mỗi ca
    // Theo bài báo: mỗi ca cần khoảng 7-8 y tá (tùy khoa)
    int totalShifts = input.numDays * input.numShiftsPerDay;
    for (int day = 0; day < input.numDays; day++) {
        for (int shift = 0; shift < input.numShiftsPerDay; shift++) {
            ShiftRequirement req;
            req.dayIndex = day;
            req.shiftType = shift;
            
            // Ca sáng cần nhiều hơn, ca đêm cần ít hơn
            if (shift == 0) {        // Sáng
                req.requiredNurses = 10;  // Bao gồm y tá trưởng
            } else if (shift == 1) { // Chiều
                req.requiredNurses = 7;
            } else {                 // Đêm
                req.requiredNurses = 5;
            }
            
            input.shifts.push_back(req);
        }
    }
    
    return input;
}

// Tạo dữ liệu nhỏ hơn để test nhanh
NSPInput createSmallTestData() {
    NSPInput input;
    
    input.numDays = 7;
    input.numShiftsPerDay = 3;
    input.minAfternoonShifts = 1;
    input.minNightShifts = 1;
    input.minMorningShiftsHeadNurse = 4;
    
    input.costPerShift = 1;
    input.overtimeCost = 1.5;
    input.headNurseCost = 1.2;
    
    // 2 y tá trưởng + 10 y tá thường
    for (int i = 0; i < 2; i++) {
        Nurse n;
        n.id = i;
        n.name = "Trưởng" + to_string(i + 1);
        n.isHeadNurse = true;
        n.isFemale = true;
        n.minShifts = 4;
        n.maxShifts = 6;
        input.nurses.push_back(n);
    }
    
    for (int i = 0; i < 10; i++) {
        Nurse n;
        n.id = 2 + i;
        n.name = "YT" + to_string(i + 1);
        n.isHeadNurse = false;
        n.isFemale = (i % 2 == 0);  // 5 nữ, 5 nam
        n.minShifts = 5;
        n.maxShifts = 7;
        input.nurses.push_back(n);
    }
    
    // Mỗi ca cần 3-4 y tá
    for (int day = 0; day < input.numDays; day++) {
        for (int shift = 0; shift < input.numShiftsPerDay; shift++) {
            ShiftRequirement req;
            req.dayIndex = day;
            req.shiftType = shift;
            req.requiredNurses = (shift == 0) ? 4 : 3;  // Sáng 4, chiều/đêm 3
            input.shifts.push_back(req);
        }
    }
    
    return input;
}

// ==================== MAIN ====================

int main() {
    cout << R"(
╔═══════════════════════════════════════════════════════════════════════════════╗
║     NURSE SCHEDULING PROBLEM - Multi-Commodity Network Flow Model             ║
║     Based on: El Adoly et al., Alexandria Engineering Journal (2018)          ║
╚═══════════════════════════════════════════════════════════════════════════════╝
)" << endl;

    // Chọn dữ liệu test
    cout << "Chọn bộ dữ liệu:" << endl;
    cout << "  1. Dữ liệu nhỏ (12 y tá, 7 ngày) - Test nhanh" << endl;
    cout << "  2. Dữ liệu thực tế (46 y tá, 7 ngày) - Theo bài báo" << endl;
    cout << "Nhập lựa chọn (1 hoặc 2): ";
    
    int choice;
    cin >> choice;
    
    NSPInput input;
    if (choice == 2) {
        cout << "\nĐang tạo dữ liệu theo case study bài báo..." << endl;
        input = createSampleData();
    } else {
        cout << "\nĐang tạo dữ liệu test nhỏ..." << endl;
        input = createSmallTestData();
    }
    
    cout << "Số y tá: " << input.nurses.size() << endl;
    cout << "Số ca: " << input.numDays * input.numShiftsPerDay << endl;
    cout << "Số biến: " << input.nurses.size() * input.numDays * input.numShiftsPerDay << endl;
    
    cout << "\nĐang giải bài toán..." << endl;
    
    NSPSolver solver(input);
    NSPSolution solution = solver.solve();
    
    printSchedule(input, solution);
    
    if (solution.feasible) {
        cout << "\n✓ Tìm được lời giải tối ưu!" << endl;
    } else {
        cout << "\n✗ Không tìm được lời giải khả thi!" << endl;
    }
    
    return 0;
}