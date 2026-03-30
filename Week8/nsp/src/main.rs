use anyhow::Result;
use good_lp::{default_solver, variable, variables, Expression, Solution, SolverModel};
use std::time::Instant;

const NUM_NURSES: usize = 1983;
const NUM_DAYS: usize = 7;
const SHIFTS: [&str; 3] = ["sang", "chieu", "toi"];
const NUM_SHIFTS_PER_DAY: usize = SHIFTS.len();
const NUM_HEAD_NUR: usize = 1234;

const COST_NORMAL: f64 = 1000.0; 
const COST_OVERTIME: f64 = 1200.0;
const COST_HEAD: f64 = 1500.0;

const MIN_AFTERNOON: f64 = 2.0;
const MIN_NIGHT: f64 = 1.0;
const MIN_HEAD: f64 = 150.0;

#[derive(Clone, Debug)]
struct Nurse {
    id: usize,
    is_head: bool,
    is_female: bool,
    min_shift: f64,
    max_shift: f64,
}

fn shift_idx(name: &str) -> usize {
    match name {
        "sang" => 0,
        "chieu" => 1,
        "toi" => 2,
        _ => unreachable!("Unknown shift: {name}"),
    }
}

fn x_index(i: usize, d: usize, s_idx: usize) -> usize {
    i * NUM_DAYS * NUM_SHIFTS_PER_DAY + d * NUM_SHIFTS_PER_DAY + s_idx
}

fn build_nurses_data() -> Vec<Nurse> {
    let mut nurses_data = Vec::with_capacity(NUM_NURSES);

    for i in 0..NUM_HEAD_NUR {
        nurses_data.push(Nurse {
            id: i,
            is_head: true,
            is_female: true,
            min_shift: 5.0,
            max_shift: 9.0,
        });
    }

    for i in 0..(NUM_NURSES - NUM_HEAD_NUR) {
        nurses_data.push(Nurse {
            id: i + NUM_HEAD_NUR,
            is_head: false,
            is_female: i >= 664,
            min_shift: 6.0,
            max_shift: 9.0,
        });
    }

    nurses_data
}

fn main() -> Result<()> {
    let nurses_data = build_nurses_data();

    let nurses: Vec<usize> = nurses_data.iter().map(|n| n.id).collect();
    let days: Vec<usize> = (0..NUM_DAYS).collect();

    let head_nurses: Vec<usize> = nurses_data.iter().filter(|n| n.is_head).map(|n| n.id).collect();
    let nor_nurses: Vec<usize> = nurses_data.iter().filter(|n| !n.is_head).map(|n| n.id).collect();
    let female_nurses: Vec<usize> = nurses_data
        .iter()
        .filter(|n| n.is_female)
        .map(|n| n.id)
        .collect();

    let demand = |s: &str| -> f64 {
        match s {
            "sang" => 542.0,
            "chieu" => 438.0,
            "toi" => 225.0,
            _ => unreachable!("Unknown shift: {s}"),
        }
    };

    let mut vars = variables!();

    // x[i,d,s] in {0,1}
    let x = vars.add_vector(
        variable().binary(),
        NUM_NURSES * NUM_DAYS * NUM_SHIFTS_PER_DAY,
    );

    // overtime[i] >= 0
    let overtime = vars.add_vector(variable().min(0.0), NUM_NURSES);

    // ===== OBJECTIVE =====
    let mut cost_nor = Expression::from(0.0);
    for &i in &nor_nurses {
        for &d in &days {
            for s in SHIFTS {
                let sid = shift_idx(s);
                cost_nor += COST_NORMAL * x[x_index(i, d, sid)];
            }
        }
    }

    let mut cost_overtime = Expression::from(0.0);
    for &i in &nor_nurses {
        cost_overtime += (COST_OVERTIME - COST_NORMAL) * overtime[i];
    }

    let mut cost_head = Expression::from(0.0);
    for &i in &head_nurses {
        for &d in &days {
            for s in SHIFTS {
                let sid = shift_idx(s);
                cost_head += COST_HEAD * x[x_index(i, d, sid)];
            }
        }
    }

    let objective = cost_nor.clone() + cost_overtime.clone() + cost_head.clone();

    let build_start = Instant::now();
    let mut model = vars.minimise(objective.clone()).using(default_solver);

    // ===== CONSTRAINTS =====

    // overtime[i] >= sum_shifts(i) - min_shift(i), for normal nurses
    for &i in &nor_nurses {
        let min_s = nurses_data[i].min_shift;
        let mut total = Expression::from(0.0);
        for &d in &days {
            for s in SHIFTS {
                total += x[x_index(i, d, shift_idx(s))];
            }
        }
        model = model.with((overtime[i] - total + min_s) >> 0.0);
    }

    // #1: đủ số y tá mỗi ca
    for &d in &days {
        for s in SHIFTS {
            let sid = shift_idx(s);
            let mut sum_staff = Expression::from(0.0);
            for &i in &nurses {
                sum_staff += x[x_index(i, d, sid)];
            }
            model = model.with(sum_staff.geq(demand(s)));
        }
    }

    // #2, #3: min/max ca mỗi y tá
    for n in &nurses_data {
        let i = n.id;
        let mut total_work = Expression::from(0.0);
        for &d in &days {
            for s in SHIFTS {
                total_work += x[x_index(i, d, shift_idx(s))];
            }
        }
        model = model.with(total_work.clone().geq(n.min_shift));
        model = model.with(total_work.leq(n.max_shift));
    }

    // #4: y tá thường ít nhất MIN_AFTERNOON ca chiều
    for &i in &nor_nurses {
        let mut sum_afternoon = Expression::from(0.0);
        for &d in &days {
            sum_afternoon += x[x_index(i, d, shift_idx("chieu"))];
        }
        model = model.with(sum_afternoon.geq(MIN_AFTERNOON));
    }

    // #5: y tá thường ít nhất MIN_NIGHT ca tối
    for &i in &nor_nurses {
        let mut sum_night = Expression::from(0.0);
        for &d in &days {
            sum_night += x[x_index(i, d, shift_idx("toi"))];
        }
        model = model.with(sum_night.geq(MIN_NIGHT));
    }

    // #6: y tá trưởng không làm ca chiều/tối
    for &i in &head_nurses {
        for &d in &days {
            model = model.with((x[x_index(i, d, shift_idx("chieu"))]) << 0.0);
            model = model.with((x[x_index(i, d, shift_idx("toi"))]) << 0.0);
        }
    }

    // #7: mỗi ca sáng có ít nhất MIN_HEAD y tá trưởng
    for &d in &days {
        let mut sum_heads = Expression::from(0.0);
        for &i in &head_nurses {
            sum_heads += x[x_index(i, d, shift_idx("sang"))];
        }
        model = model.with(sum_heads.geq(MIN_HEAD));
    }

    // #8: mỗi ca có ít nhất 1 y tá nữ
    for &d in &days {
        for s in SHIFTS {
            let sid = shift_idx(s);
            let mut sum_female = Expression::from(0.0);
            for &i in &female_nurses {
                sum_female += x[x_index(i, d, sid)];
            }
            model = model.with(sum_female.geq(1.0));
        }
    }

    // Flattened shifts
    let total_shift = NUM_DAYS * NUM_SHIFTS_PER_DAY;
    let mut all_shift: Vec<(usize, usize)> = Vec::with_capacity(total_shift);
    for &d in &days {
        for s in SHIFTS {
            all_shift.push((d, shift_idx(s)));
        }
    }

    // #9: ca thứ 2 sau ca hiện tại không được làm (k và k+2)
    for &i in &nor_nurses {
        for k in 0..(total_shift - 2) {
            let (d1, s1) = all_shift[k];
            let (d2, s2) = all_shift[k + 2];
            model = model.with((x[x_index(i, d1, s1)] + x[x_index(i, d2, s2)]) << 1.0);
        }
    }

    // #10: trong 5 ca liên tiếp chỉ được làm tối đa 2 ca
    for &i in &nor_nurses {
        for k in 0..(total_shift - 4) {
            let mut window_sum = Expression::from(0.0);
            for &(d, s) in &all_shift[k..(k + 5)] {
                window_sum += x[x_index(i, d, s)];
            }
            model = model.with(window_sum.leq(2.0));
        }
    }

    // Solve
    let build_ms = build_start.elapsed().as_secs_f64() * 1000.0;
    let solve_start = Instant::now();
    let solution = model.solve();
    let solve_ms = solve_start.elapsed().as_secs_f64() * 1000.0;
    let total_ms = build_ms + solve_ms;

    match solution {
        Ok(sol) => {
            println!("SUCCESS");
            let obj = sol.eval(objective);
            println!("BUILD_MS={:.2}", build_ms);
            println!("SOLVE_MS={:.2}", solve_ms);
            println!("TOTAL_MS={:.2}", total_ms);
            println!("TOTAL_COST={:.0}", obj);
        }
        Err(_) => {
            println!("FAILED");
        }
    }

    Ok(())
}
