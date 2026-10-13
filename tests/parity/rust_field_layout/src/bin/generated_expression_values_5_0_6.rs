use cpp_dynamic_json_core::{E, Order, Person, Profile, ValueExpression};
use serde_json::{Value, json};
use std::{
    collections::BTreeMap,
    io::{self, BufRead},
    panic::{AssertUnwindSafe, catch_unwind},
};
use teaql_core::{CompactRow, Entity, SmartList, Value as TeaqlValue};
use teaql_runtime::{EntityGraphBuilder, EntityRuntimeState};
fn row(id: u64) -> BTreeMap<String, TeaqlValue> {
    BTreeMap::from([
        ("id".into(), TeaqlValue::U64(id)),
        ("version".into(), TeaqlValue::I64(1)),
    ])
}
fn profile(id: u64, note: &Value) -> Profile {
    let mut fields = row(id);
    fields.insert(
        "note".into(),
        match note.as_str() {
            Some(text) => TeaqlValue::Text(text.into()),
            None => TeaqlValue::Null,
        },
    );
    Profile::from_compact_row(CompactRow::from_map(fields)).unwrap()
}
fn note_summary(expression: ValueExpression<'_, Option<String>>) -> Value {
    let evaluation = expression.clone().eval();
    let mut callbacks = 0;
    let fallback = expression.clone().or_else_if_null(|| {
        callbacks += 1;
        Some("fallback".into())
    });
    json!({"eval_present":evaluation.is_some(),"eval_value":evaluation,
        "unwrap_rejected":catch_unwind(AssertUnwindSafe(||expression.clone().unwrap())).is_err(),
        "fallback":expression.clone().or_if_null(Some("fallback".into())),
        "callback_result":fallback,"callbacks":callbacks,"default":expression.or_default_if_null()})
}
fn observe(input: &Value) -> Value {
    let id = input["id"].as_u64().unwrap();
    if input["mode"] == "note" {
        let mut entity = profile(id, &input["note"]);
        let snapshot = E::profile(&entity).get_note();
        // The scalar expression owns its value snapshot rather than the entity borrow.
        let before = note_summary(snapshot.clone());
        drop(snapshot);
        if input["change"] == true {
            entity.update_note(match input["next"].as_str() {
                Some(v) => TeaqlValue::Text(v.into()),
                None => TeaqlValue::Null,
            });
        }
        return json!({"before":before,"after":note_summary(E::profile(&entity).get_note())});
    }
    let mut person = Person::from_compact_row(CompactRow::from_map(row(id))).unwrap();
    let root = EntityRuntimeState::default();
    person.attach_runtime_state_recursive(root.clone());
    let mut graph = EntityGraphBuilder::default();
    if input["mode"] == "profile" {
        let child = if input["empty"] == true {
            None
        } else {
            Some(profile(2, &input["note"]))
        };
        graph.install_relation_option("Person", id, "profile", child);
        root.freeze_graph(graph).unwrap();
        let expression = E::person(&person).get_profile();
        return json!({"present":expression.eval().is_some(),"note":note_summary(expression.get_note())});
    }
    let count = input["count"].as_u64().unwrap();
    let mut orders = Vec::new();
    for n in 0..count {
        let mut fields = row(n + 1);
        fields.insert("code".into(), TeaqlValue::Text(format!("code-{n}")));
        orders.push(Order::from_compact_row(CompactRow::from_map(fields)).unwrap());
    }
    let mut list = SmartList::from(orders);
    list.is_loaded = input["loaded_flag"].as_bool().unwrap();
    graph.install_relation_list("Person", id, "orders", list);
    root.freeze_graph(graph).unwrap();
    let expression = E::person(&person).get_orders();
    let item = expression
        .get(input["index"].as_u64().unwrap() as usize)
        .get_code();
    let mut callbacks = 0;
    let fallback = item.clone().or_else_if_null(|| {
        callbacks += 1;
        "fallback".into()
    });
    json!({"present":expression.eval().is_some(),"size":expression.size().unwrap(),
        "first":expression.first().get_code().eval(),"item":item.clone().eval(),
        "fallback":fallback,"callbacks":callbacks,"default":item.or_default_if_null()})
}
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
