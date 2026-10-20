use cpp_relations_core::{Device, Platform, School};
use serde_json::{json, Value as Json};
use std::{collections::BTreeMap, io::{self, BufRead}, panic::{catch_unwind, AssertUnwindSafe}};
use teaql_core::{CompactRow, Entity, TeaqlEntity, Value, eval::LoadState};

fn observe(input: &Json) -> Json {
    macro_rules! run {
        ($model:ty, $other:ty) => {{
            let mut entity = <$model>::from_compact_row(CompactRow::from_map(BTreeMap::from([
                ("id".into(),Value::U64(7)),("name".into(),Value::Text("stored".into())),
                ("version".into(),Value::I64(3)),("platform_id".into(),Value::U64(42))
            ]))).unwrap();
            entity.set_load_state(if input["initial"] == "full" {LoadState::FullyLoaded}
                else {LoadState::Partial(["id".to_owned()].into_iter().collect())});
            let names = ["id","name","version","platform_id","platform","school_list","profile","device_list","#extra","missing"];
            let before = names.map(|name|entity.is_loaded(name));
            let selected: Vec<String> = input["fields"].as_array().unwrap().iter()
                .map(|v|v.as_str().unwrap().to_owned()).collect();
            let partial = || LoadState::Partial(selected.iter().cloned().collect());
            let state = match input["state"].as_str().unwrap() {
                "none"=>LoadState::NotLoaded,
                "full"=>LoadState::FullyLoaded,
                "compact"=>LoadState::PartialCompact(selected.iter().cloned().map(std::borrow::Cow::Owned).collect()),
                "columns"=>LoadState::SharedColumns(selected.clone().into()),
                "indexed"=>partial().into_indexed(<$model>::field_layout().unwrap().unwrap()).unwrap(),
                "other"=>partial().into_indexed(<$other>::field_layout().unwrap().unwrap()).unwrap(),
                _=>partial(),
            };
            let failed = catch_unwind(AssertUnwindSafe(||entity.set_load_state(state))).is_err();
            let id = input["identifier"].as_str().unwrap().parse::<u64>().unwrap();
            json!({"id":<$model>::with_id(id).to_json_value(),"before":before,
                "failed":failed,"after":names.map(|name|entity.is_loaded(name)),
                "name":entity.name(),"changed":entity.changed_name().map(|v|v.to_json_value()),
                "original":entity.original_values().map(|v|v.get("name").map(|v|v.to_json_value()))})
        }};
    }
    match input["model"].as_str().unwrap() {
        "platform"=>run!(Platform,School),
        "school"=>run!(School,Platform),
        _=>run!(Device,Platform),
    }
}

fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: Json=serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}",observe(&input));
    }
}
