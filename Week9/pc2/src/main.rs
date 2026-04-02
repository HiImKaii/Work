use good_lp::{highs, variable, variables, Solution, SolverModel};

fn main() {
    let mut vars = variables!();
    let vl: Vec<_> = (0..3) // Vars list;
        .map(|_| vars.add(variable().integer().min(0.0)))
        .collect();
    let loi_nhuan = vec![20.0, 6.0, 8.0];

    let objective: good_lp::Expression = ((0..3).map(|i| vl[i] * loi_nhuan[i])).sum();
    let objective = -objective;

    let mut model = vars.minimise(objective).using(highs);

    model = model.with((0.8 * vl[0] + 0.2 * vl[1] + 0.3 * vl[2]).leq(4.0));
    model = model.with((0.4 * vl[0] + 0.3 * vl[1]).leq(2.0));
    model = model.with((0.2 * vl[0] + 0.1 * vl[2]).leq(1.0));

    let solution = model.solve().unwrap();

    println!("A = {}", solution.value(vl[0]));
    println!("B = {}", solution.value(vl[1]));
    println!("C = {}", solution.value(vl[2]));
    println!(
        "Lợi nhuận = {}",
        (0..3)
            .map(|i| loi_nhuan[i] * solution.value(vl[i]))
            .sum::<f64>()
    );
}
