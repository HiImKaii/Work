use good_lp::{highs, variable, variables, Solution, SolverModel};

fn main() {
    let cp = vec![400.0, 300.0, 500.0];
    let sl = vec![500.0, 400.0, 600.0];
    // let tong_sl: f64 = (0..sl.len()).map(|x| sl[x]).sum();
    let nc = vec![80.0, 270.0, 250.0, 160.0, 180.0]; // Nhu cầu;
                                                     // let tong_nc: f64 = (0..nc.len()).map(|x| nc[x]).sum();
    let ct: Vec<Vec<_>> = vec![
        vec![2, 3, 1, 4, 5], // Kho 1
        vec![3, 2, 4, 1, 3], // Kho 2
        vec![1, 4, 3, 2, 2], // Kho 3
    ];

    let mut vars = variables!();

    let var_x: Vec<Vec<_>> = (0..cp.len())
        .map(|_| {
            (0..nc.len())
                .map(|_| vars.add(variable().integer().min(0.0)))
                .collect()
        })
        .collect();

    let var_y: Vec<_> = (0..cp.len())
        .map(|_| vars.add(variable().binary()))
        .collect();

    let objective = (0..cp.len())
        .map(|x| cp[x] * var_y[x])
        .sum::<good_lp::Expression>()
        + (0..cp.len())
            .map(|i| {
                (0..nc.len())
                    .map(|j| var_x[i][j] * (ct[i][j] as f64))
                    .sum::<good_lp::Expression>()
            })
            .sum::<good_lp::Expression>();

    let mut model = vars.minimise(&objective).using(highs);

    for i in 0..cp.len() as usize {
        let tong: good_lp::Expression = (0..nc.len()).map(|j| var_x[i][j]).sum();
        model = model.with(tong.leq(sl[i] * var_y[i]));
    }
    for j in 0..nc.len() as usize {
        let tong: good_lp::Expression = (0..cp.len()).map(|i| var_x[i][j]).sum();
        model = model.with(tong.geq(nc[j] * 1.0));
    }

    let solution = model.solve().unwrap();
    println!("=== KẾT QUẢ ===");
    for i in 0..cp.len() {
        if solution.value(var_y[i]) > 0.5 {
            println!("Kho {} MỞ (chi phí cố định: {})", i + 1, cp[i]);
            for j in 0..nc.len() {
                let val = solution.value(var_x[i][j]);
                if val > 0.0 {
                    println!("  -> Điểm {} : {} đơn vị", j + 1, val);
                }
            }
        } else {
            println!("Kho {} ĐÓNG", i + 1);
        }
    }
    println!("Tổng chi phí: {}", solution.eval(&objective));
}
