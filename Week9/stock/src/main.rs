use good_lp::{highs, variable, variables, Solution, SolverModel};

fn main() {
    let mut vars = variables!();
    let f1 = vars.add(variable().min(0.0));
    let f2 = vars.add(variable().min(0.0));
    let f3 = vars.add(variable().min(0.0));

    let obj = -(1.02 * f1 + 1.04 * f2 + 1.05 * f3);

    let mut model = vars.minimise(obj).using(highs);

    model = model.with((f3 * 1.0).leq(3000.0));
    model = model.with((f1 * 1.0).geq(f2 * 2.0));
    model = model.with((f1 + f2 + f3).leq(20000.0));

    let solution = model.solve().unwrap();

    println!("Gói F1: {}", solution.value(f1));
    println!("Gói F2: {}", solution.value(f2));
    println!("Gói F3: {}", solution.value(f3));

    let z = 1.02 * solution.value(f1) + 1.04 * solution.value(f2) + 1.05 * solution.value(f3);
    println!("Z =: {}", z);
}
