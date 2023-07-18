mod engine;

fn main() {
    let engine = engine::init_engine();
    engine.run();

    println!("Hello, world!");
}
