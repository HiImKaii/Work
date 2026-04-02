use good_lp::{highs, variable, variables, Solution, SolverModel};

fn main() {
    let cost = [
        [
            13, 4, 7, 6, 10, 8, 5, 9, 3, 11, 7, 6, 8, 4, 12, 9, 5, 7, 6, 10,
        ],
        [9, 7, 3, 2, 5, 6, 11, 4, 8, 7, 6, 9, 3, 5, 8, 4, 7, 6, 10, 5],
        [6, 8, 5, 9, 4, 7, 3, 6, 10, 5, 8, 4, 7, 9, 6, 3, 11, 5, 8, 4],
        [11, 3, 8, 4, 6, 9, 7, 5, 4, 8, 3, 7, 6, 10, 5, 8, 4, 9, 3, 7],
        [7, 6, 9, 5, 8, 3, 4, 10, 6, 4, 9, 5, 3, 7, 8, 6, 4, 10, 5, 9],
        [5, 9, 4, 7, 3, 8, 6, 4, 9, 3, 7, 8, 5, 6, 4, 10, 8, 3, 7, 6],
        [8, 5, 6, 10, 7, 4, 9, 3, 5, 6, 4, 8, 10, 3, 7, 5, 9, 4, 6, 8],
        [4, 10, 8, 3, 9, 5, 7, 6, 4, 9, 5, 3, 8, 7, 6, 4, 10, 8, 3, 5],
        [9, 3, 6, 8, 4, 10, 5, 7, 3, 6, 8, 4, 9, 5, 10, 7, 3, 6, 8, 4],
        [6, 7, 4, 5, 9, 3, 8, 10, 7, 4, 6, 9, 5, 8, 3, 7, 6, 4, 9, 10],
        [3, 8, 10, 7, 6, 4, 5, 9, 8, 3, 7, 10, 4, 6, 9, 5, 8, 3, 7, 6],
        [
            10, 4, 5, 9, 3, 7, 6, 8, 5, 10, 4, 6, 7, 3, 8, 9, 5, 10, 4, 7,
        ],
        [7, 6, 9, 4, 8, 5, 3, 7, 6, 8, 9, 5, 3, 10, 4, 6, 7, 9, 5, 3],
        [5, 9, 3, 8, 7, 6, 4, 5, 10, 7, 3, 8, 6, 4, 9, 3, 8, 5, 10, 8],
        [8, 4, 7, 6, 5, 9, 10, 3, 4, 5, 8, 7, 9, 6, 3, 10, 4, 7, 6, 5],
        [4, 7, 6, 10, 8, 3, 9, 5, 7, 6, 4, 3, 8, 9, 5, 7, 6, 10, 3, 9],
        [9, 5, 8, 3, 6, 10, 4, 7, 9, 3, 5, 6, 10, 8, 7, 4, 9, 5, 8, 3],
        [
            6, 3, 10, 5, 4, 8, 7, 9, 6, 4, 10, 5, 3, 7, 8, 6, 3, 9, 4, 10,
        ],
        [3, 8, 4, 7, 9, 5, 6, 10, 3, 8, 7, 4, 5, 9, 6, 3, 10, 8, 7, 4],
        [10, 6, 9, 4, 7, 3, 8, 5, 10, 9, 6, 7, 4, 3, 8, 5, 7, 6, 9, 8],
    ];
    let num_workers = cost.len();
    let num_works = cost[0].len();

    let mut vars = variables!();
    let x: Vec<Vec<_>> = (0..num_workers)
        .map(|_| {
            (0..num_works)
                .map(|_| vars.add(variable().binary()))
                .collect()
        })
        .collect();

    let objective: good_lp::Expression = (0..num_workers)
        .flat_map(|i| {
            let x_ref = &x;
            (0..num_works).map(move |d| cost[i][d] as f64 * x_ref[i][d])
        })
        .sum();

    let mut model = vars.minimise(objective).using(highs);

    for d in 0..num_works {
        let tong: good_lp::Expression = x.iter().map(|xi| xi[d]).sum();
        model = model.with((tong * 1.0).eq(1.0));
    }
    for i in 0..num_workers {
        let tong: good_lp::Expression = x[i].iter().copied().sum();
        model = model.with((tong * 1.0).eq(1.0));
    }

    let solution = model.solve().unwrap();

    for i in 0..num_workers {
        for d in 0..num_works {
            if solution.value(x[i][d]) > 0.5 {
                println!(
                    "Công nhân {} → Việc {} | Chi phí: ${}",
                    i + 1,
                    d + 1,
                    cost[i][d]
                );
            }
        }
    }
}
