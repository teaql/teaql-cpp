use serde_json::{json, Value};
use std::io::{self, BufRead};
use teaql_core::{default_table_name, TraceKind, TraceNode};

fn formatted<T: std::fmt::Debug>(value: &T) -> Value {
    json!({
        "compact": format!("{value:?}"),
        "pretty": format!("{value:#?}"),
        "lower_hex": format!("{value:x?}"),
        "upper_hex": format!("{value:X?}"),
        "pretty_lower_hex": format!("{value:#x?}"),
        "pretty_upper_hex": format!("{value:#X?}"),
        "width_precision": format!("{value:>40.2?}"),
        "fancy": format!("{value:🧊^+18.3?}"),
        "zero_pad": format!("{value:+012?}")
    })
}

fn observe(input: Value) -> Value {
    if input["mode"] == "name" {
        return json!({"table": default_table_name(input["name"].as_str().unwrap())});
    }
    let kinds = [TraceKind::Operation, TraceKind::Request, TraceKind::Relation,
        TraceKind::Entity, TraceKind::Provider, TraceKind::Sql, TraceKind::Comment,
        TraceKind::Purpose, TraceKind::AuditReason];
    let index = input["kind"].as_str().unwrap().parse::<usize>().unwrap();
    let name = input["name"].as_str().unwrap();
    let comment = input["comment"].as_str().unwrap();
    let id = input["id"].as_str().map(|value| value.parse::<u64>().unwrap());
    let node = TraceNode::typed(kinds[index], name, id, comment);
    let default = TraceNode::new(name, id, comment);
    let mut changes = Vec::new();
    for field in 0..4 {
        let mut changed = node.clone();
        match field {
            0 => changed.kind = kinds[(index + 1) % kinds.len()],
            1 => changed.entity_type.push('!'),
            2 => changed.entity_id = if id == Some(0) { None } else { Some(0) },
            _ => changed.comment.push('!'),
        }
        changes.push(json!({"equal": node == changed, "changed": formatted(&changed)}));
    }
    json!({"kind": formatted(&kinds[index]), "node": formatted(&node),
        "default": formatted(&default), "clone_equal": node == node.clone(),
        "default_equal": node == default, "changes": changes})
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(input));
    }
}
