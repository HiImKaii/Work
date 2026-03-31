use good_lp::{highs, variable, variables, Solution, SolverModel};

fn main() {
    let mut vars = variables!();

    let a = vars.add(variable().min(0.0));
    let b = vars.add(variable().min(0.0));

    let objective = 10.0 * a + 12.0 * b;

    let mut model = vars.minimise(objective).using(highs);

    model = model.with((40.0 * a + 30.0 * b).geq(150.0));
    model = model.with((20.0 * a + 20.0 * b).geq(90.0));
    model = model.with((10.0 * a + 30.0 * b).geq(60.0));

    let solution = model.solve().unwrap();

    println!("Gói A: {}", solution.value(a));
    println!("Gói B: {}", solution.value(b));

    let z = 10.0 * solution.value(a) + 12.0 * solution.value(b);
    println!("Z =: {}", z);
}
