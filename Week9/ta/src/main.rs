use good_lp::{highs, variable, variables, Solution, SolverModel};

fn main() {
    let mut vars = variables!();

    let t1 = vars.add(variable().integer().min(0.0));
    let t2 = vars.add(variable().integer().min(0.0));

    let objective = -(90.0 * t1 + 110.0 * t2);

    let mut model = vars.minimise(objective).using(highs);

    model = model.with((2.0 * t1 + 4.0 * t2).leq(7000.0));
    model = model.with((t1 + 2.5 * t2).leq(4000.0));
    model = model.with((2.0 * t1 + 1.5 * t2).leq(5500.0));

    let solution = model.solve().unwrap();

    println!("Gói A: {}", solution.value(t1));
    println!("Gói B: {}", solution.value(t2));

    let z = 90.0 * solution.value(t1) + 110.0 * solution.value(t2);
    println!("Z =: {}", z);
}
