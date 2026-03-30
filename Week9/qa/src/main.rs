use good_lp::{highs, variable, variables, Solution, SolverModel};

fn main() {
    let mut vars = variables!();

    let a = vars.add(variable().integer().min(0.0));
    let b = vars.add(variable().integer().min(0.0));

    let objective = -(30.0 * a + 50.0 * b);

    let mut model = vars.minimise(objective).using(highs);
    model = model.with((a * 1.0).geq(20.0));
    model = model.with((b * 1.0).geq(10.0));
    model = model.with((a + 3 * b).leq(200.0));
    model = model.with((a + b).leq(100.0));

    let solution = model.solve().unwrap();

    println!("Gói A: {}", solution.value(a));
    println!("Gói B: {}", solution.value(b));

    let chi_phi = 30.0 * solution.value(a) + 50.0 * solution.value(b);
    println!("Chi phí: {}", chi_phi);
}