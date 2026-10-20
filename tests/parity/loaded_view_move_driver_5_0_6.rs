use std::io::{self, BufRead};
fn main() {
    for line in io::stdin().lock().lines() {
        let input = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", teaql_runtime::EntityRuntimeState::__cpp_loaded_view_move_506(input));
    }
}
