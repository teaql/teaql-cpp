use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
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

// No Clone/Debug is required from an entity consumed into an input graph.
struct Dummy { values: MutationValues }
impl TeaqlEntity for Dummy {
    const ENTITY_NAME: &'static str = "DummyConstant";
    fn entity_descriptor() -> EntityDescriptor {
        EntityDescriptor::new("DescriptorName").property(PropertyDescriptor::new("id",DataType::I64).id())
    }
}
impl Entity for Dummy {
    fn into_values(self) -> MutationValues { self.values }
    fn from_compact_row(row: CompactRow) -> Result<Self,EntityError> {
        Ok(Self {values: row.into_map().into()})
    }
}

fn entity(payload: Value) -> Dummy {
    Dummy {values: MutationValues::from(std::collections::BTreeMap::from([
        ("private\n雪\0".into(),payload)]))}
}

fn evaluate(input: Json) -> Json {
    let flags=input["flags"].as_u64().unwrap();
    let payload=fixture_value::value(&input["value"]);
    let initial=EntityGraph::new(entity(payload.clone())).build();
    let mut builder=EntityGraph::new(entity(payload.clone()));
    if flags & 1 != 0 { builder=builder.comment(""); }
    if flags & 2 != 0 { builder=builder.comment("private parent\n雪").comment("final parent\0"); }
    if flags & 4 != 0 { builder=builder.delete().delete(); }
    if flags & 8 != 0 {
        let child=EntityGraph::new(entity(payload.clone())).comment("Child deletion").delete()
            .child("grandchild",EntityGraph::new(entity(Value::I64(-1))));
        builder=builder.child("duplicate",child)
            .child("duplicate",EntityGraph::new(entity(Value::Null)).comment(""))
            .child("",EntityGraph::new(entity(payload.clone())));
    }
    let graph=builder.build();
    let cloned=graph.root.clone();
    let mut changed=graph.root.clone();
    changed.comment=Some("changed clone".into());
    changed.values.insert("added".into(),Value::U64(u64::MAX));
    if let Some((name,node))=changed.children.first_mut() {
        *name="changed relation".into();node.operation=EntityGraphOperation::Save;
    }
    json!({"initial":formatted(&initial.root),"root":formatted(&graph.root),
        "cloned":formatted(&cloned),"changed":formatted(&changed),
        "operations":[formatted(&EntityGraphOperation::Save),formatted(&EntityGraphOperation::Delete)]})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
