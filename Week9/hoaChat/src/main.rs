use good_lp::{highs, variable, variables, Solution, SolverModel};

fn main() {
    let mut vars = variables!();
    let a = vars.add(variable().min(0.0));
    let b = vars.add(variable().min(0.0));

    let obj = 10.0 * a + 15.0 * b;
    let mut model = vars.minimise(obj).using(highs);

    model = model.with((a * 1.0).geq(30.0));
    model = model.with((b * 1.0).geq(40.0));
    model = model.with((a + b).geq(100.0));

    let solution = model.solve().unwrap();

    println!("Gói A: {}", solution.value(a));
    println!("Gói B: {}", solution.value(b));

    let chi_phi = 10.0 * solution.value(a) + 15.0 * solution.value(b);
    println!("Chi phí: {}", chi_phi);
}
