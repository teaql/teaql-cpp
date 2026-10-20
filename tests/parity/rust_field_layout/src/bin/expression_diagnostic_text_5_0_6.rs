use serde_json::{Value as Json,json};
use std::io::{self,BufRead};
fn main() {
    std::panic::set_hook(Box::new(|_|{}));
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
        let captured=std::panic::catch_unwind(||cpp_dynamic_json_core::trigger_logic_bug_panic(
            input["root"].as_str().unwrap(),input["node"].as_str().unwrap(),input["path"].as_str().unwrap())).unwrap_err();
        let text=captured.downcast_ref::<String>().unwrap();
        println!("{}",json!({"raw":text}));
    }
}
