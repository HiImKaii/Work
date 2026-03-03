use rand::RngExt;

fn predict(w: f64, b: f64, x: f64) -> f64
{
    w * x + b
}

fn loss(w: f64, b: f64, data: &Vec<(f64, f64)>) -> f64
{
    let mut total = 0.0;
    for &(x, y) in data
    {
        let y_pre = predict(w, b, x);
        let err = (y_pre - y).powi(2);
        total += err
    }
    total / 2 as f64
}

fn gradient(w: f64, b: f64, data: &Vec<(f64, f64)>) -> (f64, f64)
{
    let mut dw = 0.0;
    let mut db = 0.0;

    for &(x, y) in data
    {
        let y_pre = predict(w, b, x);
        let l = y_pre - y;
        dw += l * x;
        db += l;
    }
    (dw, db)
}

fn main()
{
    let n = 60;
    let mut data: Vec<(f64, f64)> = Vec::new();
    let mut rng = rand::rng();
    let pi = std::f64::consts::PI;

    for _ in 0..n
    {
        let x = rng.random_range((-pi / 4.0)..(pi / 4.0));
        let noise = rng.random_range(-0.1..0.1);
        let y = x.sin() + noise;
        data.push((x, y));
    }

    for i in 0..5
    {
        println!("Du lieu {i}: {:?}", data[i]);
    }

    let mut w = 0.0;
    let mut b = 0.0;

    let loss_ht = loss(w, b, &data);
    println!("Loss hien tai: {loss_ht}");

    let gamma = 0.005;

    for _ in 0..250
    {
        let (dw, db) = gradient(w, b, &data);
        w = w - gamma * dw;
        b = b - gamma * db;
    }
    let loss_cuoi = loss(w, b, &data);

    println!("Loss hien tai: {loss_cuoi}, w: {w}, b: {b}");
}