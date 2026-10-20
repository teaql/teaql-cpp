//! Actual generated pagination-option constructors; no execution or replacement algorithms.
use cpp_inheritance_graph_core::Q;
use serde_json::{json,Value as Json};
use std::io::{self,BufRead};
use teaql_core::SelectQuery;
fn observe(input:&Json)->Json {
    macro_rules! run {($query:expr)=>{{
        let mut query=$query;
        let mut steps=Vec::new();
        for action in input["actions"].as_array().unwrap() {
            let before:SelectQuery=query.clone().into();
            let next=std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                let query=query.clone();
                match action["op"].as_str().unwrap() {
                    "continuous_default"=>query.optimize_for_continuous_page_fetch(),
                    "continuous_config"=>query.optimize_for_continuous_page_fetch_with(action["namespace"].as_str().unwrap(),action["ttl"].as_str().unwrap().parse::<u64>().unwrap()),
                    "id_set_default"=>query.optimize_pagination_with_id_set(),
                    "id_set_config"=>query.optimize_pagination_with_id_set_config(action["namespace"].as_str().unwrap(),action["ttl"].as_str().unwrap().parse::<u64>().unwrap(),action["max"].as_str().unwrap().parse::<u64>().unwrap()),
                    "top_n"=>query.top_n_probe_parent_threshold(action["threshold"].as_str().unwrap().parse::<usize>().unwrap()),
                    _=>panic!("Unknown option action"),
                }
            }));
            let error=match next {Ok(next)=>{query=next;None},Err(p)=>Some(p.downcast_ref::<String>().cloned().or_else(||p.downcast_ref::<&str>().map(|s|s.to_string())).unwrap())};
            let after:SelectQuery=query.clone().into();
            steps.push(json!({"before":format!("{before:?}"),"after":format!("{after:?}"),"error":error}));
        }
        json!(steps)
    }};}
    match input["entity"].as_str().unwrap() {
        "school"=>run!(Q::schools()),"academy"=>run!(Q::academies()),
        "research_academy"=>run!(Q::research_academies()),_=>run!(Q::enrollments()),
    }
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){let input=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
