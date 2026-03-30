use good_lp::{highs, variable, variables, Solution, SolverModel};

fn main() {
    let mut vars = variables!();

    let x = vars.add(variable().integer().min(0.0));
    let y = vars.add(variable().integer().min(0.0));

    let objective = 800.0 * x + 600.0 * y;

    let mut model = vars.minimise(objective).using(highs);

    model = model.with((x * 1.0).leq(10.0));
    model = model.with((y * 1.0).leq(8.0));
    model = model.with((x + y).leq(9.0));
    model = model.with((50 * x + 40 * y).geq(400.0));

    let solution = model.solve().unwrap();

    println!("Xe A: {}", solution.value(x));
    println!("Xe B: {}", solution.value(y));

    let chi_phi = 800.0 * solution.value(x) + 600.0 * solution.value(y);
    println!("Chi phí: {}", chi_phi);
}