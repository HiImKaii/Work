/**
 * Nurse Scheduling Problem (NSP) - C++ gọi HiGHS solver
 * Dùng HiGHS C API, cùng data như Rust/Python
 * Compile: g++ -O3 -std=c++17 nsp_highs.cpp -lhighs -o nsp_highs
 */

#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <cstring>
#include <cstdlib>

// HiGHS C API
extern "C" {
#include "highs/interfaces/highs_c_api.h"
}

using namespace std;

// ==================== CẤU HÌNH ====================

const int NUM_NURSES    = 1983;
const int NUM_DAYS      = 7;
const int NUM_SHIFTS    = 3;           // sang, chieu, toi
const int NUM_HEAD_NUR  = 1234;

const double COST_NORMAL  = 1000.0;
const double COST_OVERTIME = 1200.0;
const double COST_HEAD    = 1500.0;

const double MIN_AFTERNOON = 2.0;
const double MIN_NIGHT     = 1.0;
const double MIN_HEAD      = 150.0;

const double DEMAND[3]    = {542.0, 438.0, 225.0};

const int NUM_VARIABLES  = NUM_NURSES * NUM_DAYS * NUM_SHIFTS;  // 41643

// ==================== CẤU TRÚC ====================

struct Nurse {
    int id;
    bool isHead;
    bool isFemale;
    double minShift;
    double maxShift;
};

// ==================== BUILD MODEL ====================

int main() {
    cout << R"(
╔════════════════════════════════════════════════════════════════╗
║     NSP - C++ gọi HiGHS solver (C API)                     ║
║     So sánh với Rust + good_lp + highs-sys                 ║
╚════════════════════════════════════════════════════════════════╝
)" << endl;

    cout << "Data: " << NUM_NURSES << " nurses, " << NUM_DAYS << " days, "
         << NUM_SHIFTS << " shifts" << endl;
    cout << "Variables: " << NUM_VARIABLES << " binary" << endl;

    // ========== XÂY DỰNG DỮ LIỆU ==========

    vector<Nurse> nurses;
    vector<int> headNurses, norNurses, femaleNurses;

    for (int i = 0; i < NUM_HEAD_NUR; i++) {
        nurses.push_back({i, true, true, 5.0, 9.0});
    }
    for (int i = 0; i < NUM_NURSES - NUM_HEAD_NUR; i++) {
        nurses.push_back({i + NUM_HEAD_NUR, false, i >= 664, 6.0, 9.0});
    }

    for (int i = 0; i < NUM_NURSES; i++) {
        if (nurses[i].isHead) headNurses.push_back(i);
        else norNurses.push_back(i);
        if (nurses[i].isFemale) femaleNurses.push_back(i);
    }

    // ========== XÂY DỰNG MODEL CHO HIGHS ==========

    auto buildStart = chrono::high_resolution_clock::now();

    // Tạo model
    HighsInt numVars = NUM_VARIABLES;
    HighsInt numIntegers = NUM_VARIABLES;  // tất cả binary

    // Overtime variables: numNorNurses biến continuous >= 0
    int numNor = norNurses.size();
    int numVarsTotal = numVars + numNor;

    // Chi phí: x[i,d,s] + overtime cost
    vector<double> costs(numVarsTotal, 0.0);
    for (int i = 0; i < NUM_NURSES; i++) {
        double c = nurses[i].isHead ? COST_HEAD : COST_NORMAL;
        for (int d = 0; d < NUM_DAYS; d++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                int idx = i * NUM_DAYS * NUM_SHIFTS + d * NUM_SHIFTS + s;
                costs[idx] = c;
            }
        }
    }

    // Overtime: COST_OVERTIME - COST_NORMAL = 200 (vì normal cost đã tính rồi)
    // Theo logic Rust: cost_nor += COST_NORMAL * overtime[i]
    //                 cost_overtime += (COST_OVERTIME - COST_NORMAL) * overtime[i]
    //                 → overtime cost = (COST_OVERTIME - COST_NORMAL) = 200
    for (int k = 0; k < numNor; k++) {
        costs[numVars + k] = COST_OVERTIME - COST_NORMAL;  // 200
    }

    // Bounds: x[i,d,s] in [0,1], overtime[k] >= 0
    vector<double> colLower(numVarsTotal, 0.0);
    vector<double> colUpper(numVarsTotal, 1.0);
    for (int k = 0; k < numNor; k++) {
        colLower[numVars + k] = 0.0;   // overtime >= 0
        colUpper[numVars + k] = 1e30;
    }

    // Integrality: x[] = INTEGER, overtime[] = CONTINUOUS
    vector<HighsInt> integrality(numVarsTotal, kHighsVarTypeContinuous);
    for (int i = 0; i < numVars; i++) {
        integrality[i] = kHighsVarTypeInteger;
    }

    // ========== ĐẾM SỐ RÀNG BUỘC ==========

    // #1: đủ số y tá mỗi ca         → 7 * 3 = 21 ràng buộc
    // #2,#3: min/max ca mỗi y tá   → 1983 * 2 = 3966 ràng buộc
    // #4: min afternoon             → 749 ràng buộc
    // #5: min night                 → 749 ràng buộc
    // #6: head không chiều/tối       → 1234 * 7 * 2 = 17276 ràng buộc
    // #7: min head mỗi ca sáng      → 7 ràng buộc
    // #8: >= 1 nữ mỗi ca           → 7 * 3 = 21 ràng buộc
    // #9: ca j và j+2 không cùng   → 749 * 19 = 14231 ràng buộc
    // #10: 5 ca liên tiếp <= 2     → 749 * 17 = 12733 ràng buộc
    // overtime: numNor ràng buộc
    // Tổng: 50502 ràng buộc

    int numConstraints = 0;
    // Đếm để tính
    int cnt1 = 0, cnt2 = 0, cnt4 = 0, cnt5 = 0, cnt6 = 0;
    int cnt7 = 0, cnt8 = 0, cnt9 = 0, cnt10 = 0;

    cnt1 = NUM_DAYS * NUM_SHIFTS;                      // 21
    cnt2 = NUM_NURSES * 2;                            // 3966
    cnt4 = numNor;                                     // 749
    cnt5 = numNor;                                     // 749
    cnt6 = NUM_HEAD_NUR * NUM_DAYS * 2;                // 17276
    cnt7 = NUM_DAYS;                                   // 7
    cnt8 = NUM_DAYS * NUM_SHIFTS;                      // 21
    cnt9 = numNor * (NUM_DAYS * NUM_SHIFTS - 2);       // 749 * 19 = 14231
    cnt10 = numNor * (NUM_DAYS * NUM_SHIFTS - 4);      // 749 * 17 = 12733

    numConstraints = cnt1 + cnt2 + cnt4 + cnt5 + cnt6 + cnt7 + cnt8 + cnt9 + cnt10 + numNor;

    cout << "Constraints: " << numConstraints << endl;

    // ========== XÂY DỰNG RÀNG BUỘC ==========

    vector<double> rowLower(numConstraints, 0.0);
    vector<double> rowUpper(numConstraints, 1e30);

    // Sparse matrix: lưu theo cột (col-wise)
    vector<int> aStart;   // chỉ mục bắt đầu mỗi cột
    vector<int> aIndex;    // row indices
    vector<double> aValue; // hệ số

    aStart.reserve(numVarsTotal + 1);
    aStart.push_back(0);

    // Helper: x_index
    auto xIdx = [&](int i, int d, int s) -> int {
        return i * NUM_DAYS * NUM_SHIFTS + d * NUM_SHIFTS + s;
    };

    int constraintId = 0;
    int constraintBase = 0;

    // ---- OVERTIME CONSTRAINTS ----
    // overtime[i] >= sum(x[i]) - minShift[i]
    // <=> sum(x[i]) - overtime[i] <= minShift[i]
    for (int k = 0; k < numNor; k++) {
        int i = norNurses[k];
        // Các hệ số cho x[i,d,s] = 1, overtime = -1
        for (int d = 0; d < NUM_DAYS; d++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                aIndex.push_back(constraintId);
                aValue.push_back(1.0);
            }
        }
        // -1 * overtime[i]
        aIndex.push_back(constraintId);
        aValue.push_back(-1.0);
        // Ràng buộc: sum(x) - overtime <= minShift
        rowUpper[constraintId] = nurses[i].minShift;
        constraintId++;
    }

    // ---- CONSTRAINT #1: đủ số y tá mỗi ca ----
    constraintBase = constraintId;
    for (int d = 0; d < NUM_DAYS; d++) {
        for (int s = 0; s < NUM_SHIFTS; s++) {
            for (int i = 0; i < NUM_NURSES; i++) {
                aIndex.push_back(constraintId);
                aValue.push_back(1.0);
            }
            rowLower[constraintId] = DEMAND[s];
            constraintId++;
        }
    }

    // ---- CONSTRAINT #2,#3: min/max ca mỗi y tá ----
    for (int i = 0; i < NUM_NURSES; i++) {
        for (int d = 0; d < NUM_DAYS; d++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                aIndex.push_back(constraintId);
                aValue.push_back(1.0);
            }
        }
        // <= max
        rowUpper[constraintId] = nurses[i].maxShift;
        constraintId++;
    }
    // Lại một lần nữa cho min
    for (int i = 0; i < NUM_NURSES; i++) {
        for (int d = 0; d < NUM_DAYS; d++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                aIndex.push_back(constraintId);
                aValue.push_back(1.0);
            }
        }
        rowLower[constraintId] = nurses[i].minShift;
        constraintId++;
    }

    // ---- CONSTRAINT #4: min afternoon cho y tá thường ----
    for (int k = 0; k < numNor; k++) {
        int i = norNurses[k];
        for (int d = 0; d < NUM_DAYS; d++) {
            int idx = xIdx(i, d, 1); // chiều
            aIndex.push_back(constraintId);
            aValue.push_back(1.0);
        }
        rowLower[constraintId] = MIN_AFTERNOON;
        constraintId++;
    }

    // ---- CONSTRAINT #5: min night cho y tá thường ----
    for (int k = 0; k < numNor; k++) {
        int i = norNurses[k];
        for (int d = 0; d < NUM_DAYS; d++) {
            int idx = xIdx(i, d, 2); // tối
            aIndex.push_back(constraintId);
            aValue.push_back(1.0);
        }
        rowLower[constraintId] = MIN_NIGHT;
        constraintId++;
    }

    // ---- CONSTRAINT #6: y tá trưởng không làm chiều/tối ----
    for (int k = 0; k < (int)headNurses.size(); k++) {
        int i = headNurses[k];
        for (int d = 0; d < NUM_DAYS; d++) {
            // chiều = 0
            int idx1 = xIdx(i, d, 1);
            aIndex.push_back(constraintId);
            aValue.push_back(1.0);
            rowUpper[constraintId] = 0.0;
            constraintId++;

            // tối = 0
            int idx2 = xIdx(i, d, 2);
            aIndex.push_back(constraintId);
            aValue.push_back(1.0);
            rowUpper[constraintId] = 0.0;
            constraintId++;
        }
    }

    // ---- CONSTRAINT #7: >= MIN_HEAD y tá trưởng mỗi ca sáng ----
    for (int d = 0; d < NUM_DAYS; d++) {
        for (int k = 0; k < (int)headNurses.size(); k++) {
            int i = headNurses[k];
            int idx = xIdx(i, d, 0); // sáng
            aIndex.push_back(constraintId);
            aValue.push_back(1.0);
        }
        rowLower[constraintId] = MIN_HEAD;
        constraintId++;
    }

    // ---- CONSTRAINT #8: >= 1 nữ mỗi ca ----
    for (int d = 0; d < NUM_DAYS; d++) {
        for (int s = 0; s < NUM_SHIFTS; s++) {
            for (int k = 0; k < (int)femaleNurses.size(); k++) {
                int i = femaleNurses[k];
                int idx = xIdx(i, d, s);
                aIndex.push_back(constraintId);
                aValue.push_back(1.0);
            }
            rowLower[constraintId] = 1.0;
            constraintId++;
        }
    }

    // ---- CONSTRAINT #9: ca j và j+2 không cùng làm ----
    int totalShift = NUM_DAYS * NUM_SHIFTS;
    for (int k = 0; k < numNor; k++) {
        int i = norNurses[k];
        for (int j = 0; j < totalShift - 2; j++) {
            int d1 = j / NUM_SHIFTS;
            int s1 = j % NUM_SHIFTS;
            int d2 = (j + 2) / NUM_SHIFTS;
            int s2 = (j + 2) % NUM_SHIFTS;

            aIndex.push_back(constraintId);
            aValue.push_back(1.0);

            int idx2 = xIdx(i, d2, s2);
            aIndex.push_back(constraintId);
            aValue.push_back(1.0);

            rowUpper[constraintId] = 1.0;
            constraintId++;
        }
    }

    // ---- CONSTRAINT #10: 5 ca liên tiếp tối đa 2 ----
    for (int k = 0; k < numNor; k++) {
        int i = norNurses[k];
        for (int j = 0; j < totalShift - 4; j++) {
            for (int t = 0; t < 5; t++) {
                int d = (j + t) / NUM_SHIFTS;
                int s = (j + t) % NUM_SHIFTS;
                int idx = xIdx(i, d, s);
                aIndex.push_back(constraintId);
                aValue.push_back(1.0);
            }
            rowUpper[constraintId] = 2.0;
            constraintId++;
        }
    }

    // Hoàn thiện aStart
    // Tính aStart cho mỗi cột (col-wise)
    vector<int> colCount(numVarsTotal, 0);

    // Reset: đếm lại
    // Xây dựng col-wise: cần biết mỗi cột có bao nhiêu phần tử
    // Cách đơn giản: dùng row-wise thay vì col-wise

    // ========== CHUYỂN SANG ROW-WISE ==========
    // HiGHS C API hỗ trợ cả col-wise và row-wise
    // Row-wise: aStart = row bắt đầu ở đâu trong aIndex/aValue

    // Đếm số phần tử non-zero mỗi row
    vector<int> rowCount(numConstraints, 0);

    // Đếm lại constraints để biết vị trí
    int c_overtime = 0;
    int c_demand = c_overtime + numNor;
    int c_max = c_demand + NUM_DAYS * NUM_SHIFTS;
    int c_min = c_max + NUM_NURSES;
    int c_afternoon = c_min + NUM_NURSES;
    int c_night = c_afternoon + numNor;
    int c_head_exclude = c_night + numNor;
    int c_head_min = c_head_exclude + NUM_HEAD_NUR * NUM_DAYS * 2;
    int c_female = c_head_min + NUM_DAYS;
    int c_consec = c_female + NUM_DAYS * NUM_SHIFTS;
    int c_window = c_consec + numNor * (totalShift - 2);

    // Overtime constraints
    for (int k = 0; k < numNor; k++) {
        rowCount[c_overtime + k] = NUM_DAYS * NUM_SHIFTS + 1;
    }

    // Demand constraints
    for (int d = 0; d < NUM_DAYS; d++) {
        for (int s = 0; s < NUM_SHIFTS; s++) {
            rowCount[c_demand + d * NUM_SHIFTS + s] = NUM_NURSES;
        }
    }

    // Max shift constraints
    for (int i = 0; i < NUM_NURSES; i++) {
        rowCount[c_max + i] = NUM_DAYS * NUM_SHIFTS;
    }

    // Min shift constraints
    for (int i = 0; i < NUM_NURSES; i++) {
        rowCount[c_min + i] = NUM_DAYS * NUM_SHIFTS;
    }

    // Afternoon constraints
    for (int k = 0; k < numNor; k++) {
        rowCount[c_afternoon + k] = NUM_DAYS;
    }

    // Night constraints
    for (int k = 0; k < numNor; k++) {
        rowCount[c_night + k] = NUM_DAYS;
    }

    // Head exclude constraints
    for (int k = 0; k < (int)headNurses.size(); k++) {
        for (int d = 0; d < NUM_DAYS; d++) {
            rowCount[c_head_exclude + k * NUM_DAYS + d] = 1;
            rowCount[c_head_exclude + k * NUM_DAYS + d + (int)headNurses.size() * NUM_DAYS] = 1;
        }
    }

    // Head min constraints
    for (int d = 0; d < NUM_DAYS; d++) {
        rowCount[c_head_min + d] = headNurses.size();
    }

    // Female constraints
    for (int d = 0; d < NUM_DAYS; d++) {
        for (int s = 0; s < NUM_SHIFTS; s++) {
            rowCount[c_female + d * NUM_SHIFTS + s] = femaleNurses.size();
        }
    }

    // Consecutive constraints
    for (int k = 0; k < numNor; k++) {
        for (int j = 0; j < totalShift - 2; j++) {
            rowCount[c_consec + k * (totalShift - 2) + j] = 2;
        }
    }

    // Window constraints
    for (int k = 0; k < numNor; k++) {
        for (int j = 0; j < totalShift - 4; j++) {
            rowCount[c_window + k * (totalShift - 4) + j] = 5;
        }
    }

    // Tính aStart (row-wise)
    vector<int> aStartRow(numConstraints + 1, 0);
    for (int r = 0; r < numConstraints; r++) {
        aStartRow[r + 1] = aStartRow[r] + rowCount[r];
    }

    int numNnz = aStartRow[numConstraints];
    vector<int> aIndexRow(numNnz);
    vector<double> aValueRow(numNnz);
    vector<int> aPos(numConstraints, 0);

    auto addCoeff = [&](int row, int col, double val) {
        int pos = aStartRow[row] + aPos[row];
        aIndexRow[pos] = col;
        aValueRow[pos] = val;
        aPos[row]++;
    };

    // ========== ĐIỀN HỆ SỐ VÀO MA TRẬN ==========

    // Overtime constraints
    for (int k = 0; k < numNor; k++) {
        int i = norNurses[k];
        int row = c_overtime + k;
        for (int d = 0; d < NUM_DAYS; d++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                addCoeff(row, xIdx(i, d, s), 1.0);
            }
        }
        addCoeff(row, numVars + k, -1.0); // -overtime
    }

    // Demand constraints
    for (int d = 0; d < NUM_DAYS; d++) {
        for (int s = 0; s < NUM_SHIFTS; s++) {
            int row = c_demand + d * NUM_SHIFTS + s;
            for (int i = 0; i < NUM_NURSES; i++) {
                addCoeff(row, xIdx(i, d, s), 1.0);
            }
        }
    }

    // Max shift constraints
    for (int i = 0; i < NUM_NURSES; i++) {
        int row = c_max + i;
        for (int d = 0; d < NUM_DAYS; d++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                addCoeff(row, xIdx(i, d, s), 1.0);
            }
        }
    }

    // Min shift constraints
    for (int i = 0; i < NUM_NURSES; i++) {
        int row = c_min + i;
        for (int d = 0; d < NUM_DAYS; d++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                addCoeff(row, xIdx(i, d, s), 1.0);
            }
        }
    }

    // Afternoon constraints
    for (int k = 0; k < numNor; k++) {
        int i = norNurses[k];
        int row = c_afternoon + k;
        for (int d = 0; d < NUM_DAYS; d++) {
            addCoeff(row, xIdx(i, d, 1), 1.0);
        }
    }

    // Night constraints
    for (int k = 0; k < numNor; k++) {
        int i = norNurses[k];
        int row = c_night + k;
        for (int d = 0; d < NUM_DAYS; d++) {
            addCoeff(row, xIdx(i, d, 2), 1.0);
        }
    }

    // Head exclude constraints
    for (int k = 0; k < (int)headNurses.size(); k++) {
        int i = headNurses[k];
        for (int d = 0; d < NUM_DAYS; d++) {
            addCoeff(c_head_exclude + k * NUM_DAYS + d, xIdx(i, d, 1), 1.0);
            addCoeff(c_head_exclude + (int)headNurses.size() * NUM_DAYS + k * NUM_DAYS + d, xIdx(i, d, 2), 1.0);
        }
    }

    // Head min constraints
    for (int d = 0; d < NUM_DAYS; d++) {
        int row = c_head_min + d;
        for (int k = 0; k < (int)headNurses.size(); k++) {
            int i = headNurses[k];
            addCoeff(row, xIdx(i, d, 0), 1.0);
        }
    }

    // Female constraints
    for (int d = 0; d < NUM_DAYS; d++) {
        for (int s = 0; s < NUM_SHIFTS; s++) {
            int row = c_female + d * NUM_SHIFTS + s;
            for (int fi : femaleNurses) {
                addCoeff(row, xIdx(fi, d, s), 1.0);
            }
        }
    }

    // Consecutive constraints
    for (int k = 0; k < numNor; k++) {
        int i = norNurses[k];
        for (int j = 0; j < totalShift - 2; j++) {
            int d1 = j / NUM_SHIFTS;
            int s1 = j % NUM_SHIFTS;
            int d2 = (j + 2) / NUM_SHIFTS;
            int s2 = (j + 2) % NUM_SHIFTS;
            int row = c_consec + k * (totalShift - 2) + j;
            addCoeff(row, xIdx(i, d1, s1), 1.0);
            addCoeff(row, xIdx(i, d2, s2), 1.0);
        }
    }

    // Window constraints
    for (int k = 0; k < numNor; k++) {
        int i = norNurses[k];
        for (int j = 0; j < totalShift - 4; j++) {
            int row = c_window + k * (totalShift - 4) + j;
            for (int t = 0; t < 5; t++) {
                int d = (j + t) / NUM_SHIFTS;
                int s = (j + t) % NUM_SHIFTS;
                addCoeff(row, xIdx(i, d, s), 1.0);
            }
        }
    }

    auto buildEnd = chrono::high_resolution_clock::now();
    double buildMs = chrono::duration<double, milli>(buildEnd - buildStart).count();
    cout << "BUILD_MS=" << fixed << setprecision(2) << buildMs << endl;

    // ========== GỌI HIGHS ==========

    auto solveStart = chrono::high_resolution_clock::now();

    HighsInt runStatus;
    HighsInt modelStatus;
    double objectiveValue = 0.0;
    HighsInt numCols = numVarsTotal;
    HighsInt numRows = numConstraints;

    vector<double> colValue(numCols);
    vector<double> rowValue(numRows);

    // Highs_mipCall trả về status, không phải objective
    // objective = sum(costs[i] * colValue[i])
    runStatus = Highs_mipCall(
        numCols, numRows, numNnz,
        kHighsMatrixFormatRowwise,
        kHighsObjSenseMinimize,
        0.0,
        costs.data(),
        colLower.data(),
        colUpper.data(),
        rowLower.data(),
        rowUpper.data(),
        aStartRow.data(),
        aIndexRow.data(),
        aValueRow.data(),
        integrality.data(),
        colValue.data(),
        rowValue.data(),
        &modelStatus
    );

    // Tính objective value từ kết quả
    objectiveValue = 0.0;
    double normalCost = 0.0;
    double overtimeCost = 0.0;
    double headCost = 0.0;
    double totalNorShifts = 0.0;
    double totalHeadShifts = 0.0;
    double totalOT = 0.0;
    for (int i = 0; i < numCols; i++) {
        objectiveValue += costs[i] * colValue[i];
    }
    // Tính chi tiết
    for (int i = 0; i < NUM_NURSES; i++) {
        double nurseShifts = 0;
        for (int d = 0; d < NUM_DAYS; d++) {
            for (int s = 0; s < NUM_SHIFTS; s++) {
                nurseShifts += colValue[xIdx(i, d, s)];
            }
        }
        if (nurses[i].isHead) {
            headCost += nurseShifts * COST_HEAD;
            totalHeadShifts += nurseShifts;
        } else {
            normalCost += nurseShifts * COST_NORMAL;
            totalNorShifts += nurseShifts;
        }
    }
    for (int k = 0; k < numNor; k++) {
        double ot = colValue[numVars + k];
        overtimeCost += ot * (COST_OVERTIME - COST_NORMAL);
        totalOT += ot;
    }
    cout << "DEBUG: headCost=" << fixed << setprecision(0) << headCost
         << " normalCost=" << normalCost
         << " overtimeCost=" << overtimeCost
         << " totalHeadShifts=" << totalHeadShifts
         << " totalNorShifts=" << totalNorShifts
         << " totalOT=" << totalOT << endl;

    auto solveEnd = chrono::high_resolution_clock::now();
    double solveMs = chrono::duration<double, milli>(solveEnd - solveStart).count();
    double totalMs = buildMs + solveMs;

    // ========== KẾT QUẢ ==========

    cout << "\n--- RESULTS ---" << endl;
    if (runStatus == kHighsStatusOk && modelStatus == kHighsModelStatusOptimal) {
        cout << "STATUS=SUCCESS" << endl;
        cout << "BUILD_MS=" << fixed << setprecision(2) << buildMs << endl;
        cout << "SOLVE_MS=" << fixed << setprecision(2) << solveMs << endl;
        cout << "TOTAL_MS=" << fixed << setprecision(2) << totalMs << endl;
        cout << "TOTAL_COST=" << fixed << setprecision(0) << objectiveValue << endl;
    } else {
        cout << "STATUS=FAILED" << endl;
        cout << "RunStatus=" << runStatus << endl;
        cout << "ModelStatus=" << modelStatus << endl;
    }

    return 0;
}
