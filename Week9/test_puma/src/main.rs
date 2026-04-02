//! Nurse Scheduling Problem (NSP) — solved with Puma Optimizer.
//! ĐÚNG scale như Week8/nsp.py: 1983 nurses × 7 days × 3 shifts = 41643 vars

use puma_optimizer::{ObjectiveFunction, PumaSolver, PumaParams};

const NUM_NURSES: usize = 1983;
const NUM_DAYS: usize = 7;
const NUM_SHIFTS: usize = 3; // 0=sang, 1=chieu, 2=toi
const NUM_SLOTS: usize = NUM_DAYS * NUM_SHIFTS; // 21

const NUM_HEAD: usize = 1234; // từ nsp.py: NUM_HEAD_NUR = 1234
const NUM_NOR: usize = NUM_NURSES - NUM_HEAD; // 749

const COST_NORMAL: f64 = 1000.0;
const COST_OVERTIME: f64 = 1200.0;
const COST_HEAD: f64 = 1500.0;

const MIN_SHIFT_NOR: usize = 6;
const MAX_SHIFT_NOR: usize = 9;

const MIN_HEAD_PER_SANG: usize = 150; // từ nsp.py: MIN_HEAD = 150

const MIN_CHIEU: usize = 2;
const MIN_TOI: usize = 1;

const NEED_SANG: usize = 542;  // từ nsp.py
const NEED_CHIEU: usize = 438;
const NEED_TOI: usize = 225;

// ─────────────────────────────────────────────────────────────────────────────
// NSP Objective Function
// ─────────────────────────────────────────────────────────────────────────────

struct NSPObjective;

impl ObjectiveFunction for NSPObjective {
    fn evaluate(&self, x: &[f64]) -> f64 {
        // x có 21 × NUM_NURSES phần tử
        // x[slot * NUM_NURSES + nurse] = continuous [0,1]
        // slot = d * NUM_SHIFTS + s

        let big_m = 1e12;
        let lambda = 1e10; // penalty weight — phải >> salary cost (~14M)

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

        // Ràng buộc 8: ≥ 1 nữ mỗi ca
        // Từ nsp.py:
        // - Head nurses (0..1233): is_female=True
        // - Normal nurses (1234..1982): is_female=True if offset >= 664 → ids 1898..1982
        // Female ids: 0..1233 (head) + 1898..1982 (normal) = 1234 + 85 = 1319 nurses
        let nor_female_start = NUM_HEAD + 664; // 1234 + 664 = 1898
        let nor_female_end = NUM_NURSES; // 1983
        let mut females: Vec<usize> = (0..NUM_HEAD).collect();
        females.extend(nor_female_start..nor_female_end);
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
// Chạy 1 lần, trả về best_cost + raw_salary + violations_count
// ─────────────────────────────────────────────────────────────────────────────

fn run_once(run_idx: usize) -> (f64, f64, usize) {
    let dim = NUM_NURSES * NUM_SLOTS;
    let params = PumaParams {
        n_pumas: 100,
        max_iter: 2000,
        dim,
        lb: vec![0.0; dim],
        ub: vec![1.0; dim],
        seed: Some(run_idx as u64 * 12345),
        verbose: false,
        ..Default::default()
    };

    let solver = PumaSolver::with_params(params);
    let result = solver.optimize(&NSPObjective);
    let x = &result.best_x;

    // ── Raw salary ─────────────────────────────────────────────────────
    let mut total_cost = 0.0;
    for nurse in 0..NUM_NURSES {
        let is_head = nurse < NUM_HEAD;
        let mut n_shifts = 0i32;
        for slot in 0..NUM_SLOTS {
            let val = x[slot * NUM_NURSES + nurse].clamp(0.0, 1.0).round() as i32;
            n_shifts += val;
        }
        if is_head {
            total_cost += COST_HEAD * n_shifts as f64;
        } else {
            let ot = (n_shifts - MIN_SHIFT_NOR as i32).max(0);
            total_cost += COST_NORMAL * n_shifts as f64
                        + (COST_OVERTIME - COST_NORMAL) * ot as f64;
        }
    }

    // ── Count violations ────────────────────────────────────────────────
    let mut violations = 0_usize;

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
            if (count.round() as usize) < need {
                violations += 1;
            }
        }
    }

    for d in 0..NUM_DAYS {
        let slot = d * NUM_SHIFTS;
        let count: f64 = (0..NUM_HEAD)
            .map(|n| x[slot * NUM_NURSES + n].clamp(0.0, 1.0).round())
            .sum();
        if (count.round() as usize) < MIN_HEAD_PER_SANG {
            violations += 1;
        }
    }

    (result.best_cost, total_cost.round() as f64, violations)
}

// ─────────────────────────────────────────────────────────────────────────────

fn main() {
    println!("╔══════════════════════════════════════════════════════╗");
    println!("║     Nurse Scheduling Problem — Puma Optimizer       ║");
    println!("╚══════════════════════════════════════════════════════╝");
    println!();
    println!("Problem: {} nurses × {} days × {} shifts = {} vars",
        NUM_NURSES, NUM_DAYS, NUM_SLOTS, NUM_NURSES * NUM_SLOTS);
    println!("Config: pumas=100, iter=2000, λ=1e10, big_M=1e12");
    println!("Runs: 10 (lấy best fitness)");
    println!("{}", "─".repeat(60));

    let runs = 10;
    let mut best_cost = f64::INFINITY;
    let mut best_raw = f64::INFINITY;
    let mut best_viol = usize::MAX;

    for i in 0..runs {
        let (cost, raw, viol) = run_once(i);
        print!("\r  Run {:2}/{} | fitness={:>20.2} | raw={:>10.0} | viol={:>3}",
            i + 1, runs, cost, raw, viol);
        if cost < best_cost {
            best_cost = cost;
            best_raw = raw;
            best_viol = viol;
        }
    }

    println!();
    println!("{}", "─".repeat(60));
    println!("  BEST fitness = {:.2}", best_cost);
    println!("  BEST raw salary = {:.0}", best_raw);
    println!("  BEST violations = {}", best_viol);
    println!();
    println!("  vs Python PuLP: total_cost = 13,925,400 | violations = 0");
    println!("  vs Rust good_lp: total_cost = 13,925,400 | violations = 0");
    println!();
    println!("══════════════════════════════════════════════════════");
    println!("Đã hoàn thành!");
}
