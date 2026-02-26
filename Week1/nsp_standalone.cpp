/**
 * Nurse Scheduling Problem (NSP) - Standalone Version (No External Dependencies)
 * Based on paper: "A new formulation and solution for the nurse scheduling problem"
 * Alexandria Engineering Journal (2018) 57, 2289–2298
 * 
 * This implementation uses a Greedy + Local Search heuristic approach
 * to solve the Binary Linear Programming problem without external libraries.
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

using namespace std;

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
    int dayIndex;        // Ngày trong planning horizon
    int shiftType;       // 0 = Morning, 1 = Afternoon, 2 = Night
    int requiredNurses;  // Số y tá cần (M_j)
};

struct NSPInput {
    int numDays;
    int numShiftsPerDay;
    vector<Nurse> nurses;
    vector<ShiftRequirement> shifts;
    int minAfternoonShifts;
    int minNightShifts;
    int minMorningShiftsHeadNurse;
    double costPerShift;
    double overtimeCost;
    double headNurseCost;
};

struct NSPSolution {
    bool feasible;
    double totalCost;
    double normalCost;
    double overtimeCostValue;
    double headNurseCostValue;
    vector<vector<int>> schedule;  // schedule[nurse][shift] = 0 or 1
    double solveTimeMs;
    int violations;
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

int getShiftType(int shiftIndex, int shiftsPerDay) {
    return shiftIndex % shiftsPerDay;
}

int getDay(int shiftIndex, int shiftsPerDay) {
    return shiftIndex / shiftsPerDay;
}

// ==================== NSP SOLVER CLASS ====================

class NSPSolver {
private:
    NSPInput input;
    int numNurses;
    int totalShifts;
    mt19937 rng;
    
    // Đếm số vi phạm ràng buộc
    int countViolations(const vector<vector<int>>& schedule) {
        int violations = 0;
        
        // Constraint (1): Đủ số y tá cho mỗi ca
        for (int j = 0; j < totalShifts; j++) {
            int count = 0;
            for (int i = 0; i < numNurses; i++) {
                count += schedule[i][j];
            }
            if (count < input.shifts[j].requiredNurses) {
                violations += (input.shifts[j].requiredNurses - count) * 10;
            }
        }
        
        for (int i = 0; i < numNurses; i++) {
            int totalWorked = 0;
            for (int j = 0; j < totalShifts; j++) {
                totalWorked += schedule[i][j];
            }
            
            // Constraint (2): Số ca tối thiểu
            if (totalWorked < input.nurses[i].minShifts) {
                violations += (input.nurses[i].minShifts - totalWorked) * 5;
            }
            
            // Constraint (3): Số ca tối đa
            if (totalWorked > input.nurses[i].maxShifts) {
                violations += (totalWorked - input.nurses[i].maxShifts) * 5;
            }
            
            if (!input.nurses[i].isHeadNurse) {
                // Constraint (4): Số ca chiều tối thiểu
                int afternoonCount = 0;
                for (int day = 0; day < input.numDays; day++) {
                    afternoonCount += schedule[i][getShiftIndex(day, 1, input.numShiftsPerDay)];
                }
                if (afternoonCount < input.minAfternoonShifts) {
                    violations += (input.minAfternoonShifts - afternoonCount) * 3;
                }
                
                // Constraint (5): Số ca đêm tối thiểu
                int nightCount = 0;
                for (int day = 0; day < input.numDays; day++) {
                    nightCount += schedule[i][getShiftIndex(day, 2, input.numShiftsPerDay)];
                }
                if (nightCount < input.minNightShifts) {
                    violations += (input.minNightShifts - nightCount) * 3;
                }
                
                // Constraint (9): Nghỉ 2 ca sau khi làm 1 ca
                for (int j = 0; j < totalShifts - 2; j++) {
                    if (schedule[i][j] && schedule[i][j + 2]) {
                        violations += 2;
                    }
                }
                
                // Constraint (10): Nghỉ 3 ca sau khi làm 2 ca liên tiếp
                for (int j = 0; j < totalShifts - 4; j++) {
                    int sum = schedule[i][j] + schedule[i][j+1] + schedule[i][j+2] 
                            + schedule[i][j+3] + schedule[i][j+4];
                    if (sum > 2) {
                        violations += (sum - 2) * 2;
                    }
                }
            } else {
                // Constraint (6)-(7): Y tá trưởng chỉ làm ca sáng
                int morningCount = 0;
                for (int day = 0; day < input.numDays; day++) {
                    morningCount += schedule[i][getShiftIndex(day, 0, input.numShiftsPerDay)];
                    
                    // Không được làm ca chiều và đêm
                    if (schedule[i][getShiftIndex(day, 1, input.numShiftsPerDay)]) {
                        violations += 10;
                    }
                    if (schedule[i][getShiftIndex(day, 2, input.numShiftsPerDay)]) {
                        violations += 10;
                    }
                }
                
                if (morningCount < input.minMorningShiftsHeadNurse) {
                    violations += (input.minMorningShiftsHeadNurse - morningCount) * 3;
                }
            }
        }
        
        // Constraint (8): Mỗi ca có ít nhất 1 y tá nữ
        for (int j = 0; j < totalShifts; j++) {
            int femaleCount = 0;
            for (int i = 0; i < numNurses; i++) {
                if (input.nurses[i].isFemale && schedule[i][j]) {
                    femaleCount++;
                }
            }
            if (femaleCount < 1) {
                violations += 5;
            }
        }
        
        return violations;
    }
    
    // Tính chi phí
    double calculateCost(const vector<vector<int>>& schedule) {
        double normalCost = 0, overtime = 0, headCost = 0;
        
        for (int i = 0; i < numNurses; i++) {
            int totalWorked = 0;
            for (int j = 0; j < totalShifts; j++) {
                totalWorked += schedule[i][j];
            }
            
            if (input.nurses[i].isHeadNurse) {
                headCost += totalWorked * input.headNurseCost;
            } else {
                normalCost += totalWorked * input.costPerShift;
                int ot = max(0, totalWorked - input.nurses[i].minShifts);
                overtime += ot * input.overtimeCost;
            }
        }
        
        return normalCost + overtime + headCost;
    }
    
    // Khởi tạo lời giải greedy
    vector<vector<int>> greedyInitialize() {
        vector<vector<int>> schedule(numNurses, vector<int>(totalShifts, 0));
        vector<int> nurseShiftCount(numNurses, 0);
        
        // Bước 1: Gán y tá trưởng vào ca sáng
        for (int i = 0; i < numNurses; i++) {
            if (input.nurses[i].isHeadNurse) {
                vector<int> morningShifts;
                for (int day = 0; day < input.numDays; day++) {
                    morningShifts.push_back(getShiftIndex(day, 0, input.numShiftsPerDay));
                }
                shuffle(morningShifts.begin(), morningShifts.end(), rng);
                
                int assigned = 0;
                for (int j : morningShifts) {
                    if (assigned >= input.nurses[i].minShifts) break;
                    schedule[i][j] = 1;
                    nurseShiftCount[i]++;
                    assigned++;
                }
            }
        }
        
        // Bước 2: Gán y tá thường, ưu tiên đảm bảo yêu cầu mỗi ca
        for (int j = 0; j < totalShifts; j++) {
            int shiftType = getShiftType(j, input.numShiftsPerDay);
            int required = input.shifts[j].requiredNurses;
            
            // Đếm số đã gán
            int current = 0;
            for (int i = 0; i < numNurses; i++) {
                current += schedule[i][j];
            }
            
            // Tạo danh sách ứng viên
            vector<pair<int, int>> candidates;  // (priority, nurse_id)
            for (int i = 0; i < numNurses; i++) {
                if (schedule[i][j]) continue;  // Đã gán
                if (input.nurses[i].isHeadNurse && shiftType != 0) continue;
                if (nurseShiftCount[i] >= input.nurses[i].maxShifts) continue;
                
                // Kiểm tra ràng buộc 9 và 10
                bool canAssign = true;
                if (j >= 2 && schedule[i][j - 2]) canAssign = false;
                if (j < totalShifts - 2 && schedule[i][j + 2]) canAssign = false;
                
                if (canAssign) {
                    int priority = nurseShiftCount[i];  // Ưu tiên người ít ca hơn
                    candidates.push_back({priority, i});
                }
            }
            
            // Sắp xếp theo priority (ít ca hơn ưu tiên hơn)
            sort(candidates.begin(), candidates.end());
            
            // Gán cho đến khi đủ
            for (auto& [priority, i] : candidates) {
                if (current >= required) break;
                schedule[i][j] = 1;
                nurseShiftCount[i]++;
                current++;
            }
        }
        
        // Bước 3: Đảm bảo số ca tối thiểu cho mỗi y tá
        for (int i = 0; i < numNurses; i++) {
            while (nurseShiftCount[i] < input.nurses[i].minShifts) {
                // Tìm ca có thể gán
                bool assigned = false;
                for (int j = 0; j < totalShifts && !assigned; j++) {
                    if (schedule[i][j]) continue;
                    
                    int shiftType = getShiftType(j, input.numShiftsPerDay);
                    if (input.nurses[i].isHeadNurse && shiftType != 0) continue;
                    
                    // Kiểm tra ràng buộc
                    bool canAssign = true;
                    if (!input.nurses[i].isHeadNurse) {
                        if (j >= 2 && schedule[i][j - 2]) canAssign = false;
                        if (j < totalShifts - 2 && schedule[i][j + 2]) canAssign = false;
                    }
                    
                    if (canAssign) {
                        schedule[i][j] = 1;
                        nurseShiftCount[i]++;
                        assigned = true;
                    }
                }
                
                if (!assigned) break;  // Không thể gán thêm
            }
        }
        
        // Bước 4: Đảm bảo số ca chiều và đêm tối thiểu
        for (int i = 0; i < numNurses; i++) {
            if (input.nurses[i].isHeadNurse) continue;
            
            // Ca chiều
            int afternoonCount = 0;
            for (int day = 0; day < input.numDays; day++) {
                afternoonCount += schedule[i][getShiftIndex(day, 1, input.numShiftsPerDay)];
            }
            
            while (afternoonCount < input.minAfternoonShifts) {
                bool assigned = false;
                for (int day = 0; day < input.numDays && !assigned; day++) {
                    int j = getShiftIndex(day, 1, input.numShiftsPerDay);
                    if (schedule[i][j]) continue;
                    if (nurseShiftCount[i] >= input.nurses[i].maxShifts) break;
                    
                    // Swap với ca khác nếu cần
                    for (int k = 0; k < totalShifts && !assigned; k++) {
                        if (getShiftType(k, input.numShiftsPerDay) != 0) continue;  // Chỉ swap với ca sáng
                        if (!schedule[i][k]) continue;
                        
                        schedule[i][k] = 0;
                        schedule[i][j] = 1;
                        afternoonCount++;
                        assigned = true;
                    }
                    
                    if (!assigned && nurseShiftCount[i] < input.nurses[i].maxShifts) {
                        schedule[i][j] = 1;
                        nurseShiftCount[i]++;
                        afternoonCount++;
                        assigned = true;
                    }
                }
                if (!assigned) break;
            }
            
            // Ca đêm (tương tự)
            int nightCount = 0;
            for (int day = 0; day < input.numDays; day++) {
                nightCount += schedule[i][getShiftIndex(day, 2, input.numShiftsPerDay)];
            }
            
            while (nightCount < input.minNightShifts) {
                bool assigned = false;
                for (int day = 0; day < input.numDays && !assigned; day++) {
                    int j = getShiftIndex(day, 2, input.numShiftsPerDay);
                    if (schedule[i][j]) continue;
                    if (nurseShiftCount[i] >= input.nurses[i].maxShifts) break;
                    
                    if (nurseShiftCount[i] < input.nurses[i].maxShifts) {
                        schedule[i][j] = 1;
                        nurseShiftCount[i]++;
                        nightCount++;
                        assigned = true;
                    }
                }
                if (!assigned) break;
            }
        }
        
        return schedule;
    }
    
    // Local Search để cải thiện lời giải
    vector<vector<int>> localSearch(vector<vector<int>> schedule, int maxIterations) {
        vector<vector<int>> bestSchedule = schedule;
        int bestViolations = countViolations(schedule);
        double bestCost = calculateCost(schedule);
        
        for (int iter = 0; iter < maxIterations; iter++) {
            // Chọn ngẫu nhiên một y tá và thử swap hoặc thay đổi
            int nurse = rng() % numNurses;
            int shift1 = rng() % totalShifts;
            int shift2 = rng() % totalShifts;
            
            if (shift1 == shift2) continue;
            
            // Thử swap
            vector<vector<int>> newSchedule = schedule;
            swap(newSchedule[nurse][shift1], newSchedule[nurse][shift2]);
            
            int newViolations = countViolations(newSchedule);
            double newCost = calculateCost(newSchedule);
            
            // Chấp nhận nếu tốt hơn
            if (newViolations < bestViolations || 
                (newViolations == bestViolations && newCost < bestCost)) {
                schedule = newSchedule;
                bestSchedule = newSchedule;
                bestViolations = newViolations;
                bestCost = newCost;
            }
            
            // Thử flip
            newSchedule = schedule;
            newSchedule[nurse][shift1] = 1 - newSchedule[nurse][shift1];
            
            newViolations = countViolations(newSchedule);
            newCost = calculateCost(newSchedule);
            
            if (newViolations < bestViolations || 
                (newViolations == bestViolations && newCost < bestCost)) {
                schedule = newSchedule;
                bestSchedule = newSchedule;
                bestViolations = newViolations;
                bestCost = newCost;
            }
        }
        
        return bestSchedule;
    }
    
public:
    NSPSolver(const NSPInput& inp) : input(inp) {
        numNurses = input.nurses.size();
        totalShifts = input.numDays * input.numShiftsPerDay;
        rng.seed(chrono::steady_clock::now().time_since_epoch().count());
    }
    
    NSPSolution solve() {
        NSPSolution solution;
        solution.feasible = false;
        
        auto startTime = chrono::high_resolution_clock::now();
        
        cout << "  [1/3] Khởi tạo lời giải greedy..." << endl;
        vector<vector<int>> schedule = greedyInitialize();
        
        int initialViolations = countViolations(schedule);
        cout << "        Vi phạm ban đầu: " << initialViolations << endl;
        
        cout << "  [2/3] Cải thiện bằng Local Search..." << endl;
        int maxIter = 50000;
        schedule = localSearch(schedule, maxIter);
        
        int finalViolations = countViolations(schedule);
        cout << "        Vi phạm sau cải thiện: " << finalViolations << endl;
        
        cout << "  [3/3] Tính toán kết quả..." << endl;
        
        auto endTime = chrono::high_resolution_clock::now();
        solution.solveTimeMs = chrono::duration<double, milli>(endTime - startTime).count();
        
        // Xác định feasibility
        solution.violations = finalViolations;
        solution.feasible = (finalViolations == 0);
        solution.schedule = schedule;
        
        // Tính chi phí
        double normalCost = 0, overtimeCostVal = 0, headCost = 0;
        for (int i = 0; i < numNurses; i++) {
            int totalWorked = 0;
            for (int j = 0; j < totalShifts; j++) {
                totalWorked += schedule[i][j];
            }
            
            if (input.nurses[i].isHeadNurse) {
                headCost += totalWorked * input.headNurseCost;
            } else {
                normalCost += totalWorked * input.costPerShift;
                int ot = max(0, totalWorked - input.nurses[i].minShifts);
                overtimeCostVal += ot * input.overtimeCost;
            }
        }
        
        solution.normalCost = normalCost;
        solution.overtimeCostValue = overtimeCostVal;
        solution.headNurseCostValue = headCost;
        solution.totalCost = normalCost + overtimeCostVal + headCost;
        
        return solution;
    }
};

// ==================== HIỂN THỊ KẾT QUẢ ====================

void printSchedule(const NSPInput& input, const NSPSolution& solution) {
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
    cout << "  Chi phí làm thêm:     " << setw(15) << solution.overtimeCostValue << endl;
    cout << "  Chi phí y tá trưởng:  " << setw(15) << solution.headNurseCostValue << endl;
    cout << string(60, '-') << endl;
    cout << "  TỔNG CHI PHÍ:         " << setw(15) << solution.totalCost << endl;
    cout << string(60, '=') << endl;
    
    cout << "\nSố vi phạm ràng buộc: " << solution.violations << endl;
    cout << "Thời gian giải: " << solution.solveTimeMs << " ms" << endl;
    
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
    
    input.numDays = 7;
    input.numShiftsPerDay = 3;
    input.minAfternoonShifts = 2;
    input.minNightShifts = 1;
    input.minMorningShiftsHeadNurse = 5;
    
    input.costPerShift = 100;
    input.overtimeCost = 150;
    input.headNurseCost = 120;
    
    int numHeadNurses = 6;
    int numRegularNurses = 40;
    
    for (int i = 0; i < numHeadNurses; i++) {
        Nurse n;
        n.id = i;
        n.name = "YT Trưởng " + to_string(i + 1);
        n.isHeadNurse = true;
        n.isFemale = true;
        n.minShifts = 5;
        n.maxShifts = 7;
        input.nurses.push_back(n);
    }
    
    int numMale = 17;
    for (int i = 0; i < numRegularNurses; i++) {
        Nurse n;
        n.id = numHeadNurses + i;
        n.name = "Y tá " + to_string(i + 1);
        n.isHeadNurse = false;
        n.isFemale = (i >= numMale);
        n.minShifts = 6;
        n.maxShifts = 8;
        input.nurses.push_back(n);
    }
    
    for (int day = 0; day < input.numDays; day++) {
        for (int shift = 0; shift < input.numShiftsPerDay; shift++) {
            ShiftRequirement req;
            req.dayIndex = day;
            req.shiftType = shift;
            
            if (shift == 0) {
                req.requiredNurses = 10;
            } else if (shift == 1) {
                req.requiredNurses = 7;
            } else {
                req.requiredNurses = 5;
            }
            
            input.shifts.push_back(req);
        }
    }
    
    return input;
}

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
        n.isFemale = (i % 2 == 0);
        n.minShifts = 5;
        n.maxShifts = 7;
        input.nurses.push_back(n);
    }
    
    for (int day = 0; day < input.numDays; day++) {
        for (int shift = 0; shift < input.numShiftsPerDay; shift++) {
            ShiftRequirement req;
            req.dayIndex = day;
            req.shiftType = shift;
            req.requiredNurses = (shift == 0) ? 4 : 3;
            input.shifts.push_back(req);
        }
    }
    
    return input;
}

// ==================== MAIN ====================

int main() {
    cout << R"(
╔═══════════════════════════════════════════════════════════════════════════════╗
║     NURSE SCHEDULING PROBLEM - Standalone Version (No Dependencies)          ║
║     Based on: El Adoly et al., Alexandria Engineering Journal (2018)          ║
║     Algorithm: Greedy + Local Search Heuristic                                ║
╚═══════════════════════════════════════════════════════════════════════════════╝
)" << endl;

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
    
    cout << "\nĐang giải bài toán...\n" << endl;
    
    NSPSolver solver(input);
    NSPSolution solution = solver.solve();
    
    printSchedule(input, solution);
    
    if (solution.feasible) {
        cout << "\n✓ Tìm được lời giải khả thi!" << endl;
    } else {
        cout << "\n⚠ Lời giải có " << solution.violations << " vi phạm (heuristic solution)" << endl;
    }
    
    return 0;
}
