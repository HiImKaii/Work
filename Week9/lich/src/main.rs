use good_lp::{highs, variable, variables, Solution, SolverModel};

fn main() {
    let pc = vec![3, 2, 4, 3, 5]; // pc = phân công;
    let num_days = pc.len();
    let max_workers: i32 = pc.iter().sum();

    let mut vars = variables!();

    let x: Vec<Vec<_>> = (0..max_workers)
        .map(|_| {
            (0..num_days)
                .map(|_| vars.add(variable().binary()))
                .collect()
        })
        .collect();

    let y: Vec<_> = (0..max_workers)
        .map(|_| vars.add(variable().binary()))
        .collect();

    let obj: good_lp::Expression = y.iter().copied().sum();

    let mut model = vars.minimise(obj).using(highs);

    for d in 0..num_days {
        let tong: good_lp::Expression = x.iter().map(|xi| xi[d]).sum();
        model = model.with(tong.geq(pc[d] as f64));
    }
    for i in 0..max_workers as usize {
        let tong: good_lp::Expression = x[i].iter().copied().sum();
        model = model.with(tong.leq(3.0 * y[i]));
    }

    let solution = model.solve().unwrap();

    let tong_nv: f64 = y.iter().map(|yi| solution.value(*yi)).sum();
    println!("Số nhân viên tối thiểu: {}", tong_nv as i32);

    let ten_ngay = ["T2", "T3", "T4", "T5", "T6"];
    for i in 0..max_workers as usize {
        if solution.value(y[i]) > 0.5 {
            print!("Nhân viên {}: ", i + 1);
            for d in 0..num_days {
                if solution.value(x[i][d]) > 0.5 {
                    print!("{} ", ten_ngay[d]);
                }
            }
            println!();
        }
    }
}
