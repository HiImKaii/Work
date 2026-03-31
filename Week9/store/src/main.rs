use good_lp::{highs, variable, variables, Solution, SolverModel};

fn main() {
    let mut vars = variables!();
    let x = vars.add(variable().integer().min(0.0));
    let y = vars.add(variable().integer().min(0.0));

    let obj = -(400.0 * x + 700.0 * y);

    let mut model = vars.minimise(obj).using(highs);

    model = model.with((1000.0 * x + 1500.0 * y).leq(100000.0));
    model = model.with((x * 1.0).geq(15.0));
    model = model.with((x * 1.0).leq(80.0));
    model = model.with((x * 1.0).geq(2 * y));

    let solution = model.solve().unwrap();

    println!("Desktop: {}", solution.value(x));
    println!("Laptop: {}", solution.value(y));

    let z = 400.0 * solution.value(x) + 700.0 * solution.value(y);
    println!("Z =: {}", z);
}
