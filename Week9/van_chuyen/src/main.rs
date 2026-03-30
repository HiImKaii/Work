use good_lp::{default_solver, variable, variables, Solution, SolverModel};

fn main() {
    let mut vars = variables!();

    let x = vars.add(variable().integer().min(0.0));
    let y = vars.add(variable().integer().min(0.0));

    let objective = 30.0 * x + 40.0 * y;

    let mut model = vars.minimise(objective).using(default_solver);

    model = model.with((20.0 * x + 30.0 * y).geq(3000.0));
    model = model.with((40.0 * x + 30.0 * y).geq(4000.0));

    let solution = model.solve().unwrap();

    println!("Xe A: {}", solution.value(x));
    println!("Xe B: {}", solution.value(y));

    let chi_phi = 30.0 * solution.value(x) + 40.0 * solution.value(y);
    println!("Chi phí: {}", chi_phi);
}