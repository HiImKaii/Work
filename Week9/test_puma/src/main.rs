//! Nurse Scheduling Problem (NSP) — solved with Puma Optimizer.
//!
//! Mô hình hoá bài toán phân ca y tá:
//! - NUM_NURSES = 100 y tá
//! - NUM_DAYS   = 7 ngày
//! - 3 ca/ngày: sang, chieu, toi
//!
//! Yêu cầu:
//!  1. Đủ số y tá mỗi ca
//!  2. Min/max ca mỗi y tá
//!  3. Head nurse ≥ 150 mỗi ca sáng
//!  4. Head nurse không làm chiều/tối
//!  5. ≥ 1 nữ mỗi ca
//!  6. Không làm 2 ca liên tiếp
//!  7. Trong 5 ca liên tiếp: ≤ 2 ca tối

use puma_optimizer::{ObjectiveFunction, PumaSolver, PumaParams};

const NUM_NURSES: usize = 20;
const NUM_DAYS: usize = 3;
const NUM_SHIFTS: usize = 3; // 0=sang, 1=chieu, 2=toi
const NUM_SLOTS: usize = NUM_DAYS * NUM_SHIFTS; // 9

const NUM_HEAD: usize = 5;
const NUM_NOR: usize = NUM_NURSES - NUM_HEAD; // 15

const COST_NORMAL: f64 = 1000.0;
const COST_OVERTIME: f64 = 1200.0;
const COST_HEAD: f64 = 1500.0;

const MIN_SHIFT_NOR: usize = 2;
const MAX_SHIFT_NOR: usize = 6;
const MIN_SHIFT_HEAD: usize = 0;
const MAX_SHIFT_HEAD: usize = 6;

const MIN_HEAD_PER_SANG: usize = 1; // tối thiểu head mỗi ca sáng

const MIN_CHIEU: usize = 1;
const MIN_TOI: usize = 1;

const NEED_SANG: usize = 5;
const NEED_CHIEU: usize = 4;
const NEED_TOI: usize = 3;

// ─────────────────────────────────────────────────────────────────────────────
// NSP Objective Function
// ─────────────────────────────────────────────────────────────────────────────

struct NSPObjective;

impl ObjectiveFunction for NSPObjective {
    fn evaluate(&self, x: &[f64]) -> f64 {
        // x có 21 × NUM_NURSES phần tử
        // x[slot * NUM_NURSES + nurse] = continuous [0,1]
        // slot = d * NUM_SHIFTS + s

        let big_m = 1e6;
        let lambda = 5000.0; // penalty weight

        // ── 1. Chi phí lương ────────────────────────────────────────────────
        let mut cost = 0.0;

        for nurse in 0..NUM_NURSES {
            let is_head = nurse < NUM_HEAD;
            let mut n_shifts = 0.0;

            for slot in 0..NUM_SLOTS {
                let val = x[slot * NUM_NURSES + nurse].clamp(0.0, 1.0);
                if is_head {
                    cost += COST_HEAD * val;
                } else {
                    cost += COST_NORMAL * val;
                }
                n_shifts += val;
            }

            // Overtime cho y tá thường
            if !is_head && n_shifts > MIN_SHIFT_NOR as f64 {
                let ot = n_shifts - MIN_SHIFT_NOR as f64;
                cost += (COST_OVERTIME - COST_NORMAL) * ot;
            }
        }

        // ── 2. Ràng buộc phạt ────────────────────────────────────────────────

        // Ràng buộc 1: đủ số y tá mỗi ca
        for slot in 0..NUM_SLOTS {
            let _d = slot / NUM_SHIFTS;
            let s = slot % NUM_SHIFTS;
            let need = match s {
                0 => NEED_SANG,
                1 => NEED_CHIEU,
                _ => NEED_TOI,
            };
            let actual: f64 = (0..NUM_NURSES)
                .map(|n| x[slot * NUM_NURSES + n].clamp(0.0, 1.0))
                .sum();
            let viol = (need as f64 - actual).max(0.0);
            cost += lambda * viol * viol;
        }

        // Ràng buộc 3: head nurse ≥ MIN_HEAD_PER_SANG mỗi ca sáng
        for d in 0..NUM_DAYS {
            let slot = d * NUM_SHIFTS; // ca sáng
            let actual: f64 = (0..NUM_HEAD)
                .map(|n| x[slot * NUM_NURSES + n].clamp(0.0, 1.0))
                .sum();
            let viol = (MIN_HEAD_PER_SANG as f64 - actual).max(0.0);
            cost += lambda * viol * viol;
        }

        // Ràng buộc 4: head nurse không làm chiều và tối
        for nurse in 0..NUM_HEAD {
            for d in 0..NUM_DAYS {
                let slot_chieu = d * NUM_SHIFTS + 1;
                let slot_toi   = d * NUM_SHIFTS + 2;
                let chieu = x[slot_chieu * NUM_NURSES + nurse].clamp(0.0, 1.0);
                let toi   = x[slot_toi   * NUM_NURSES + nurse].clamp(0.0, 1.0);
                cost += big_m * (chieu * chieu + toi * toi);
            }
        }

        // Ràng buộc 5: min/max ca mỗi y tá thường
        for nurse in NUM_HEAD..NUM_NURSES {
            let n: f64 = (0..NUM_SLOTS)
                .map(|slot| x[slot * NUM_NURSES + nurse].clamp(0.0, 1.0))
                .sum();
            let viol_min = (n - MIN_SHIFT_NOR as f64).max(0.0);
            let viol_max = (n - MAX_SHIFT_NOR as f64).max(0.0);
            cost += lambda * (viol_min * viol_min + viol_max * viol_max);
        }

        // Ràng buộc 6: ≥ MIN_CHIEU ca chiều mỗi y tá thường
        for nurse in NUM_HEAD..NUM_NURSES {
            let chieu_count: f64 = (0..NUM_DAYS)
                .map(|d| {
                    let slot = d * NUM_SHIFTS + 1;
                    x[slot * NUM_NURSES + nurse].clamp(0.0, 1.0)
                })
                .sum();
            let viol = (MIN_CHIEU as f64 - chieu_count).max(0.0);
            cost += lambda * viol * viol;
        }

        // Ràng buộc 7: ≥ MIN_TOI ca tối mỗi y tá thường
        for nurse in NUM_HEAD..NUM_NURSES {
            let toi_count: f64 = (0..NUM_DAYS)
                .map(|d| {
                    let slot = d * NUM_SHIFTS + 2;
                    x[slot * NUM_NURSES + nurse].clamp(0.0, 1.0)
                })
                .sum();
            let viol = (MIN_TOI as f64 - toi_count).max(0.0);
            cost += lambda * viol * viol;
        }

        // Ràng buộc 8: ≥ 1 nữ mỗi ca (giả sử 60% là nữ, chọn 12 nurse cuối)
        let females: Vec<usize> = (NUM_NURSES - 12..NUM_NURSES).collect();
        for slot in 0..NUM_SLOTS {
            let n_female: f64 = females.iter()
                .map(|&n| x[slot * NUM_NURSES + n].clamp(0.0, 1.0))
                .sum();
            let viol = (1.0 - n_female).max(0.0);
            cost += lambda * viol * viol;
        }

        // Ràng buộc 9: không làm 2 ca liên tiếp
        // Ca k và k+2 không thể cùng làm
        for nurse in NUM_HEAD..NUM_NURSES {
            for k in 0..(NUM_SLOTS - 2) {
                let v1 = x[k       * NUM_NURSES + nurse].clamp(0.0, 1.0);
                let v2 = x[(k + 2) * NUM_NURSES + nurse].clamp(0.0, 1.0);
                if v1 + v2 > 1.0 + 1e-6 {
                    cost += big_m * (v1 + v2 - 1.0).powi(2);
                }
            }
        }

        // Ràng buộc 10: trong 5 ca liên tiếp, ≤ 2 ca tối
        for nurse in NUM_HEAD..NUM_NURSES {
            for k in 0..(NUM_SLOTS - 4) {
                let toi_count: f64 = (0..5)
                    .map(|offset| {
                        let slot = k + offset;
                        let s = slot % NUM_SHIFTS;
                        if s == 2 {
                            x[slot * NUM_NURSES + nurse].clamp(0.0, 1.0)
                        } else {
                            0.0
                        }
                    })
                    .sum();
                let viol = (toi_count - 2.0).max(0.0);
                cost += lambda * viol * viol;
            }
        }

        cost
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Giải & in kết quả
// ─────────────────────────────────────────────────────────────────────────────

fn main() {
    println!("╔══════════════════════════════════════════════════════╗");
    println!("║     Nurse Scheduling Problem — Puma Optimizer       ║");
    println!("╚══════════════════════════════════════════════════════╝");
    println!();
    println!("Problem size: {} nurses × {} days × {} shifts = {} vars",
        NUM_NURSES, NUM_DAYS, NUM_SLOTS, NUM_NURSES * NUM_SLOTS);

    let dim = NUM_NURSES * NUM_SLOTS;
    let params = PumaParams {
        n_pumas: 50,
        max_iter: 500,
        dim,
        lb: vec![0.0; dim],
        ub: vec![1.0; dim],
        seed: Some(42),
        verbose: true,
        ..Default::default()
    };

    println!("\nRunning Puma Optimizer...");
    println!("{}", "-".repeat(55));

    let solver = PumaSolver::with_params(params);
    let result = solver.optimize(&NSPObjective);

    println!("{}", "-".repeat(55));
    println!();
    println!("Best cost = {:.2}", result.best_cost);
    println!("Iterations = {}", result.n_iterations);
    println!("Function evals = {}", result.n_function_evaluations);

    // ── Phân tích kết quả ─────────────────────────────────────────────────
    println!();
    println!("══════════ KẾT QUẢ CHI TIẾT ══════════");
    let x = &result.best_x;

    let shift_names = ["Sang", "Chieu", "Toi"];

    println!("\n--- Số y tá mỗi ca ---");
    for d in 0..NUM_DAYS {
        for s in 0..NUM_SHIFTS {
            let slot = d * NUM_SHIFTS + s;
            let count: f64 = (0..NUM_NURSES)
                .map(|n| x[slot * NUM_NURSES + n].clamp(0.0, 1.0).round())
                .sum();
            let need = match s {
                0 => NEED_SANG,
                1 => NEED_CHIEU,
                _ => NEED_TOI,
            };
            let ok = if count.round() as usize >= need { "✅" } else { "❌" };
            println!("  Ngày {} ca {}: {:>4.0}/{}  {}",
                d + 1, shift_names[s], count, need, ok);
        }
    }

    println!("\n--- Ca sáng: head nurses ---");
    for d in 0..NUM_DAYS {
        let slot = d * NUM_SHIFTS;
        let count: f64 = (0..NUM_HEAD)
            .map(|n| x[slot * NUM_NURSES + n].clamp(0.0, 1.0).round())
            .sum();
        let ok = if count.round() as usize >= MIN_HEAD_PER_SANG { "✅" } else { "❌" };
        println!("  Ngày {}: {:>4.0}/{} head nurses  {}", d + 1, count, MIN_HEAD_PER_SANG, ok);
    }

    println!("\n--- Tổng chi phí (≈) ---");
    let mut total_cost = 0.0;
    let mut total_shifts = 0;
    for nurse in 0..NUM_NURSES {
        let is_head = nurse < NUM_HEAD;
        let mut n_shifts = 0i32;
        for slot in 0..NUM_SLOTS {
            let val = x[slot * NUM_NURSES + nurse].clamp(0.0, 1.0).round() as i32;
            n_shifts += val;
        }
        total_shifts += n_shifts;

        if is_head {
            total_cost += (COST_HEAD * n_shifts as f64).round();
        } else {
            let ot = (n_shifts - MIN_SHIFT_NOR as i32).max(0);
            total_cost += COST_NORMAL * n_shifts as f64
                        + (COST_OVERTIME - COST_NORMAL) * ot as f64;
        }
    }
    println!("  Tổng chi phí ≈ {:.0}", total_cost.round());
    println!("  Tổng số ca làm = {}", total_shifts);

    println!();
    println!("══════════════════════════════════════════════════════");
    println!("Đã hoàn thành!");
}
