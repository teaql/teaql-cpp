use std::{collections::BTreeMap, io::{self, BufRead}};
use serde_json::{json, Value as Json};
use teaql_core::*;
#[path = "../fixture_value.rs"]
mod fixture_value;

fn formatted<T: std::fmt::Debug>(v: &T) -> Json {
    json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),
        "lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),
        "pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),
        "width_precision":format!("{v:>40.2?}"),"fancy":format!("{v:🧊^+18.3?}"),
        "zero_pad":format!("{v:+012?}")})
}

fn evaluate(input: Json) -> Json {
    let flags = input["flags"].as_u64().unwrap();
    let version: i64 = input["version"].as_str().unwrap().parse().unwrap();
    let payload = fixture_value::value(&input["value"]);
    let mut fields = BTreeMap::new();
    if flags & 4 != 0 {
        fields.insert("".into(), Value::Text("empty-key".into()));
        fields.insert("private\n雪\0".into(), payload.clone());
        fields.insert("nested".into(), Value::Object(BTreeMap::from([
            ("items".into(), Value::List(vec![payload.clone(),Value::Decimal("1.00".parse().unwrap())]))])));
    }
    let values = MutationValues::from(fields.clone());
    let generated = GeneratedValues::from(fields.clone());
    let snapshot = EntitySnapshot::from(fields);
    let empty_snapshot = EntitySnapshot::new();
    let old = match flags % 3 {0=>None,1=>Some(empty_snapshot.clone()),_=>Some(snapshot.clone())};
    let expected = if flags & 2 != 0 {Some(version)} else {None};
    let trace = TraceNode::typed(TraceKind::AuditReason,"Order",Some(u64::MAX),"private reason\n雪");
    let traces = if flags & 1 != 0 {vec![trace]} else {Vec::new()};
    let mut insert = InsertCommand::new("Order\n雪");
    insert.values=values.clone();insert.trace_chain=traces.clone();
    let mut update = UpdateCommand::new("Order\n雪",payload.clone());
    update.expected_version=expected;update.values=values.clone();update.trace_chain=traces.clone();update.old_values=old.clone();
    let mut delete = DeleteCommand::new("Order\n雪",payload.clone());
    delete.expected_version=expected;delete.soft_delete=flags & 1 == 0;delete.trace_chain=traces.clone();
    let mut recover = RecoverCommand::new("Order\n雪",payload.clone(),version);
    recover.trace_chain=traces.clone();
    let mut batch_insert = BatchInsertCommand::new("Order\n雪");
    batch_insert.batch_values=vec![values.clone(),MutationValues::new()];
    batch_insert.trace_chains=vec![traces.clone(),Vec::new(),traces.clone()];
    let mut batch_update = BatchUpdateCommand::new("Order\n雪",vec!["z".into(),"a".into(),"z".into(),String::new()]);
    batch_update.batch_ids=vec![payload.clone(),Value::Null];
    batch_update.batch_expected_versions=vec![None,expected,Some(version)];
    batch_update.batch_values=vec![values.clone()];batch_update.trace_chains=vec![traces,Vec::new()];
    batch_update.batch_old_values=vec![None,Some(empty_snapshot),old];
    let mut output=json!({"values":formatted(&values),"generated":formatted(&generated),
        "snapshot":formatted(&snapshot),"insert":formatted(&insert),"update":formatted(&update),
        "delete":formatted(&delete),"recover":formatted(&recover),
        "batch_insert":formatted(&batch_insert),"batch_update":formatted(&batch_update)});
    if input["defaults"] == true {
        output["defaults"]=json!({"insert":formatted(&InsertCommand::new("")),
            "update":formatted(&UpdateCommand::new("",Value::Null)),
            "delete":formatted(&DeleteCommand::new("",Value::Null)),
            "recover":formatted(&RecoverCommand::new("",Value::Null,0)),
            "batch_insert":formatted(&BatchInsertCommand::new("")),
            "batch_update":formatted(&BatchUpdateCommand::new("",Vec::new()))});
        output["kinds"]=[MutationKind::Insert,MutationKind::Update,MutationKind::Delete,MutationKind::Recover]
            .iter().map(formatted).collect();
    }
    output
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
