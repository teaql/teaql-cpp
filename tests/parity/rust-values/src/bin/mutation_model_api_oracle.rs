//! Observe original public mutation/trace constructors, PartialEq and map traits.
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::*;

fn value(input: &Json) -> Value {
    let data = &input["value"];
    match input["kind"].as_str().unwrap() {
        "null" => Value::Null,
        "typed_null" => Value::TypedNull(DataType::Text),
        "bool" => Value::Bool(data.as_bool().unwrap()),
        "text" => Value::Text(data.as_str().unwrap().to_owned()),
        "i64" => Value::I64(data.as_str().unwrap().parse().unwrap()),
        "u64" => Value::U64(data.as_str().unwrap().parse().unwrap()),
        "f64" => Value::F64(data.as_str().unwrap().parse().unwrap()),
        "decimal" => Value::Decimal(data.as_str().unwrap().parse().unwrap()),
        "date" => Value::Date(Value::from(data.as_str().unwrap()).try_date().unwrap()),
        "timestamp" => Value::Timestamp(teaql_core::time::Timestamp::from(
            data.as_str().unwrap().parse::<i64>().unwrap(),
        )),
        "json" => Value::Json(data.clone()),
        "list" => Value::List(data.as_array().unwrap().iter().map(value).collect()),
        "object" => Value::Object(
            data.as_object()
                .unwrap()
                .iter()
                .map(|(k, v)| (k.clone(), value(v)))
                .collect(),
        ),
        _ => panic!("invalid value fixture"),
    }
}
fn record(input: &Json) -> Record {
    input
        .as_object()
        .unwrap()
        .iter()
        .map(|(k, v)| (k.clone(), value(v)))
        .collect()
}
fn record_output(values: &Record) -> Json {
    json!(
        values
            .iter()
            .map(|(k, v)| (k, format!("{v:?}")))
            .collect::<Vec<_>>()
    )
}
fn version(input: &Json) -> Option<i64> {
    input.as_str().map(|v| v.parse().unwrap())
}
fn trace(input: &Json) -> TraceNode {
    let kind = [
        TraceKind::Operation,
        TraceKind::Request,
        TraceKind::Relation,
        TraceKind::Entity,
        TraceKind::Provider,
        TraceKind::Sql,
        TraceKind::Comment,
        TraceKind::Purpose,
        TraceKind::AuditReason,
    ][input["trace_kind"].as_u64().unwrap() as usize];
    let entity = input["entity"].as_str().unwrap();
    let id = input["id"].as_str().map(|v| v.parse::<u64>().unwrap());
    let comment = input["comment"].as_str().unwrap();
    if input["default"].as_bool().unwrap() {
        TraceNode::new(entity, id, comment)
    } else {
        TraceNode::typed(kind, entity, id, comment)
    }
}
fn traces(input: &Json) -> Vec<TraceNode> {
    input.as_array().unwrap().iter().map(trace).collect()
}
fn equality<T: Clone + PartialEq>(left: T, right: T) -> Json {
    json!([
        left == right,
        left == left.clone(),
        right == right.clone(),
        left == left,
        right == right
    ])
}
fn command(input: &Json) -> Json {
    let l = &input["left"];
    let r = &input["right"];
    match input["type"].as_str().unwrap() {
        "insert" => {
            let parse = |x: &Json| {
                let mut c = InsertCommand::new(x["entity"].as_str().unwrap());
                c.values = record(&x["values"]).into();
                c.trace_chain = traces(&x["trace"]);
                c
            };
            equality(parse(l), parse(r))
        }
        "update" => {
            let parse = |x: &Json| {
                let mut c = UpdateCommand::new(x["entity"].as_str().unwrap(), value(&x["id"]));
                c.values = record(&x["values"]).into();
                c.expected_version = version(&x["version"]);
                c.old_values = (!x["old"].is_null()).then(|| record(&x["old"]).into());
                c.trace_chain = traces(&x["trace"]);
                c
            };
            equality(parse(l), parse(r))
        }
        "delete" => {
            let parse = |x: &Json| {
                let mut c = DeleteCommand::new(x["entity"].as_str().unwrap(), value(&x["id"]));
                c.expected_version = version(&x["version"]);
                c.soft_delete = x["soft"].as_bool().unwrap();
                c.trace_chain = traces(&x["trace"]);
                c
            };
            equality(parse(l), parse(r))
        }
        "recover" => {
            let parse = |x: &Json| {
                let mut c = RecoverCommand::new(
                    x["entity"].as_str().unwrap(),
                    value(&x["id"]),
                    version(&x["version"]).unwrap(),
                );
                c.trace_chain = traces(&x["trace"]);
                c
            };
            equality(parse(l), parse(r))
        }
        "batch_insert" => {
            let parse = |x: &Json| {
                let mut c = BatchInsertCommand::new(x["entity"].as_str().unwrap());
                c.batch_values = x["values"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(|v| record(v).into())
                    .collect();
                c.trace_chains = x["trace"].as_array().unwrap().iter().map(traces).collect();
                c
            };
            equality(parse(l), parse(r))
        }
        "batch_update" => {
            let parse = |x: &Json| {
                let fields = x["fields"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(|v| v.as_str().unwrap().to_owned())
                    .collect();
                let mut c = BatchUpdateCommand::new(x["entity"].as_str().unwrap(), fields);
                c.batch_ids = x["ids"].as_array().unwrap().iter().map(value).collect();
                c.batch_expected_versions = x["versions"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(version)
                    .collect();
                c.batch_values = x["values"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(|v| record(v).into())
                    .collect();
                c.trace_chains = x["trace"].as_array().unwrap().iter().map(traces).collect();
                c.batch_old_values = x["old"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(|v| (!v.is_null()).then(|| record(v).into()))
                    .collect();
                c
            };
            equality(parse(l), parse(r))
        }
        _ => panic!("invalid command fixture"),
    }
}
fn builders(input: &Json) -> Json {
    let entity = input["entity"].as_str().unwrap();
    let steps = input["steps"].as_array().unwrap();
    let mut states = Vec::new();
    match input["type"].as_str().unwrap() {
        "insert" => {
            let mut c = InsertCommand::new(entity);
            states.push(format!("{c:?}"));
            for step in steps {
                c = c.value(step["field"].as_str().unwrap(), value(&step["value"]));
                states.push(format!("{c:?}"));
            }
        }
        "update" => {
            let mut c = UpdateCommand::new(entity, value(&input["id"]));
            states.push(format!("{c:?}"));
            for step in steps {
                c = if step["op"] == "version" {
                    c.expected_version(version(&step["version"]).unwrap())
                } else {
                    c.value(step["field"].as_str().unwrap(), value(&step["value"]))
                };
                states.push(format!("{c:?}"));
            }
        }
        "delete" => {
            let mut c = DeleteCommand::new(entity, value(&input["id"]));
            states.push(format!("{c:?}"));
            for step in steps {
                c = if step["op"] == "version" {
                    c.expected_version(version(&step["version"]).unwrap())
                } else {
                    c.hard_delete()
                };
                states.push(format!("{c:?}"));
            }
        }
        _ => panic!("invalid builder fixture"),
    }
    json!(states)
}
fn map_methods(input: &Json) -> Json {
    fn run(values: &mut Record) -> Json {
        let mut states = vec![record_output(values)];
        values.insert("z".into(), Value::Text("first".into()));
        values.insert("a".into(), Value::U64(u64::MAX));
        states.push(record_output(values));
        let replaced = values.insert("z".into(), Value::Bool(false));
        states.push(json!({"replaced":format!("{replaced:?}"),"fields":record_output(values)}));
        *values.get_mut("z").unwrap() = Value::Null;
        states.push(record_output(values));
        let removed = values.remove("a");
        let missing = values.remove("missing");
        states.push(json!({"removed":format!("{removed:?}"),"missing":format!("{missing:?}"),
            "fields":record_output(values)}));
        values.clear();
        states.push(record_output(values));
        json!(states)
    }
    match input["type"].as_str().unwrap() {
        "mutation" => {
            let mut values = MutationValues::new();
            assert_eq!(values, MutationValues::default());
            run(&mut values)
        }
        "generated" => {
            let mut values = GeneratedValues::new();
            assert_eq!(values, GeneratedValues::default());
            run(&mut values)
        }
        "snapshot" => {
            let mut values = EntitySnapshot::new();
            assert_eq!(values, EntitySnapshot::default());
            run(&mut values)
        }
        _ => panic!("invalid map fixture"),
    }
}
fn observe(input: Json) -> Json {
    match input["kind"].as_str().unwrap() {
        "builders" => builders(&input),
        "map_methods" => map_methods(&input),
        "trace" => {
            let l = trace(&input["left"]);
            let r = trace(&input["right"]);
            json!({"equal":equality(l.clone(),r),"kind":format!("{:?}",l.kind),"entity":l.entity_type,
                   "id":l.entity_id.map(|v|v.to_string()),"comment":l.comment})
        }
        "wrapper" => {
            let left = record(&input["left"]);
            let right = record(&input["right"]);
            let (eq, roundtrip, iteration) = match input["type"].as_str().unwrap() {
                "mutation" => {
                    let l = MutationValues::from(left);
                    let r = MutationValues::from(right);
                    let borrowed = (&l)
                        .into_iter()
                        .map(|(k, v)| (k.clone(), format!("{v:?}")))
                        .collect::<Vec<_>>();
                    let owned = l
                        .clone()
                        .into_iter()
                        .map(|(k, v)| (k, format!("{v:?}")))
                        .collect::<Vec<_>>();
                    assert_eq!(borrowed, owned);
                    let result: Record = l.clone().into();
                    (equality(l, r), record_output(&result), json!(borrowed))
                }
                "generated" => {
                    let l = GeneratedValues::from(left);
                    let r = GeneratedValues::from(right);
                    let result: Record = l.clone().into();
                    (equality(l, r), record_output(&result), Json::Null)
                }
                "snapshot" => {
                    let l = EntitySnapshot::from(left);
                    let r = EntitySnapshot::from(right);
                    let result: Record = l.clone().into();
                    (equality(l, r), record_output(&result), Json::Null)
                }
                _ => panic!("invalid wrapper fixture"),
            };
            json!({"equal":eq,"roundtrip":roundtrip,"iteration":iteration})
        }
        "command" => command(&input),
        _ => panic!("invalid fixture"),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
