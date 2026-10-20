use std::io::{self, BufRead};
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", teaql_sql::__cpp_diagnostic_guards_506(input));
    }
}
