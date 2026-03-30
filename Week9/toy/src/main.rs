use good_lp::{highs, default_solver, variable, variables, Solution, SolverModel};

fn main() {
    let mut vars = variables!();
    let a = vars.add(variable().integer().min(0.0));
    let b = vars.add(variable().integer().min(0.0));

    let objective = - (2 * a + 3 * b);
    let mut model = vars.minimise(objective).using(highs);

    model = model.with((a + b).leq(2000.0));
    model = model.with((8 * a + 14 * b).leq(20000.0));

    let solution = model.solve().unwrap();

    println!("Gói A: {}", solution.value(a));
    println!("Gói B: {}", solution.value(b));

    let chi_phi = 2.0 * solution.value(a) + 3.0 * solution.value(b);
    println!("Chi phí: {}", chi_phi);
}