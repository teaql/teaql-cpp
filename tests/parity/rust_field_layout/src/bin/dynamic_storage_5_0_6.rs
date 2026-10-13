use serde_json::{Value as Json, json};
use std::{
    collections::{BTreeMap, HashMap},
    io::{self, BufRead},
};
use teaql_core::dynamic_fields::{
    DynamicFieldDefinitions, DynamicFieldError, DynamicFieldMutation, DynamicFieldSelection,
};
use teaql_core::{CompactRow, DataType, QueryIntent, Value};
use teaql_data_service::dynamic_fields::{
    DynamicFieldStore, DynamicFieldStreamPlan, DynamicFieldWrite, DynamicStorageFuture,
    DynamicStorageRows,
};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
struct Store {
    key: usize,
    identity: Option<u64>,
}
impl DynamicFieldStore for Store {
    fn is_transaction_bound(&self) -> bool {
        false
    }
    fn binding_key(&self) -> usize {
        self.key
    }
    fn source_identity(&self) -> Option<u64> {
        self.identity
    }
    fn ensure_schema(&self) -> DynamicStorageFuture<'_, ()> {
        panic!("unexpected I/O")
    }
    fn load_values<'a>(
        &'a self,
        _: &'a str,
        _: &'a DynamicFieldDefinitions,
        _: &'a [u64],
        _: &'a DynamicFieldSelection,
        _: &'a QueryIntent,
    ) -> DynamicStorageFuture<'a, DynamicStorageRows> {
        panic!("unexpected I/O")
    }
    fn validate_writes(&self, _: &[DynamicFieldWrite]) -> Result<(), DynamicFieldError> {
        Ok(())
    }
    fn apply_writes<'a>(
        &'a self,
        _: &'a [DynamicFieldWrite],
        _: &'a teaql_core::MutationIntent,
    ) -> DynamicStorageFuture<'a, ()> {
        panic!("unexpected I/O")
    }
}
fn store(v: &Json) -> Store {
    Store {
        key: v["key"].as_u64().unwrap() as usize,
        identity: v["identity"].as_u64(),
    }
}
fn error(e: DynamicFieldError) -> Json {
    json!({"error":e.code,"field":e.field})
}
fn record(v: &Json) -> BTreeMap<String, Value> {
    v.as_object()
        .unwrap()
        .iter()
        .map(|(k, v)| (k.clone(), fixture_value::value(v)))
        .collect()
}
fn rows_state(rows: &[CompactRow]) -> Json {
    json!(rows.iter().map(|row|{
    let fields=row.clone().take_loaded_dynamic_fields();
    json!({"native":row.clone().into_map().iter().map(|(k,v)|(k.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>(),
        "fields":fields.map(|v|v.values().iter().map(|(k,v)|(k.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>())})
}).collect::<Vec<_>>())
}
fn observe(input: &Json) -> Json {
    let definitions = DynamicFieldDefinitions::new(
        "Item",
        "v1",
        vec![
            ("name".into(), DataType::Text),
            ("count".into(), DataType::I64),
        ],
    )
    .unwrap();
    if input["mode"] == "write_debug" {
        let changes = input["changes"]
            .as_object()
            .unwrap()
            .iter()
            .map(|(name, v)| {
                (
                    name.clone(),
                    if v.is_null() {
                        DynamicFieldMutation::Delete
                    } else {
                        DynamicFieldMutation::Set(fixture_value::value(v))
                    },
                )
            })
            .collect();
        let write = DynamicFieldWrite {
            namespace: input["namespace"].as_str().unwrap().into(),
            definitions,
            owner_id: 999,
            changes,
        };
        return json!({"debug":format!("{write:?}"),"pretty":format!("{write:#?}")});
    }
    let selection = if input["selection"].is_null() {
        DynamicFieldSelection::All
    } else {
        match DynamicFieldSelection::fields(input["selection"].as_array().unwrap().iter().map(
            |p| {
                (
                    p[0].as_str().unwrap().into(),
                    if p[1].as_u64().unwrap() == 5 {
                        DataType::Text
                    } else {
                        DataType::I64
                    },
                )
            },
        )) {
            Ok(v) => v,
            Err(e) => return error(e),
        }
    };
    let s = store(&input["store"]);
    let intent = QueryIntent::new("read extensions", "cursor hydration").unwrap();
    let plan = match DynamicFieldStreamPlan::new(
        input["namespace"].as_str().unwrap().into(),
        definitions,
        selection,
        &s,
        intent,
    ) {
        Ok(v) => v,
        Err(e) => return error(e),
    };
    let mut result = json!({"namespace":plan.namespace(),"bound":plan.definitions().storage_binding_matches(plan.namespace(),s.identity.unwrap()),
        "validations":input["validate"].as_array().unwrap().iter().map(|v|match plan.validate_store(&store(v)){Ok(())=>Json::Null,Err(e)=>error(e)}).collect::<Vec<_>>()});
    let mut rows: Vec<_> = input["rows"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| CompactRow::from(record(v)))
        .collect();
    result["ids"] = match plan.owner_ids(&rows) {
        Ok(ids) => json!({"ids":ids}),
        Err(e) => error(e),
    };
    let initial = rows_state(&rows);
    let ids: Vec<_> = input["ids"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| v.as_u64().unwrap())
        .collect();
    let stored: DynamicStorageRows = input["stored"]
        .as_object()
        .unwrap()
        .iter()
        .map(|(id, v)| {
            (
                id.parse().unwrap(),
                record(v).into_iter().collect::<HashMap<_, _>>(),
            )
        })
        .collect();
    result["merge"] = match plan.merge(&mut rows, &ids, stored) {
        Ok(()) => Json::Null,
        Err(e) => error(e),
    };
    result["initial"] = initial;
    result["final"] = rows_state(&rows);
    result
}
fn main() {
    for line in io::stdin().lock().lines() {
        let v: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&v));
    }
}
