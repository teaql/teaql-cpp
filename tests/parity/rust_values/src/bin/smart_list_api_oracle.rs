//! Direct original SmartList public calls with complete recursive metadata.
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use std::{
    cell::RefCell,
    collections::{BTreeMap, BTreeSet},
    rc::Rc,
};
use teaql_core::{
    CompactRow, DataType, Entity, EntityDescriptor, EntityError, IdentifiableEntity,
    MutationValues, SmartList, TeaqlEntity, Value, VersionedEntity,
};

#[derive(Clone)]
struct Item {
    fields: BTreeMap<String, Value>,
    dirty: Option<BTreeSet<String>>,
    new: bool,
    deleted: bool,
    version: i64,
    index: usize,
    calls: Rc<RefCell<Vec<String>>>,
}
impl TeaqlEntity for Item {
    const ENTITY_NAME: &'static str = "ListItem";
    fn entity_descriptor() -> EntityDescriptor {
        EntityDescriptor::new(Self::ENTITY_NAME)
    }
}
impl Entity for Item {
    fn from_compact_row(_: CompactRow) -> Result<Self, EntityError> {
        unreachable!()
    }
    fn into_values(self) -> MutationValues {
        self.calls
            .borrow_mut()
            .push(format!("values:{}", self.index));
        self.fields.into()
    }
    fn is_new(&self) -> bool {
        self.calls.borrow_mut().push(format!("new:{}", self.index));
        self.new
    }
    fn dirty_fields(&self) -> Option<BTreeSet<String>> {
        self.calls
            .borrow_mut()
            .push(format!("dirty:{}", self.index));
        self.dirty.clone()
    }
    fn is_marked_as_delete(&self) -> bool {
        self.calls
            .borrow_mut()
            .push(format!("deleted:{}", self.index));
        self.deleted
    }
}
impl IdentifiableEntity for Item {
    fn id_value(&self) -> Value {
        self.fields["id"].clone()
    }
}
impl VersionedEntity for Item {
    fn version(&self) -> i64 {
        self.version
    }
}

fn record_output(fields: &BTreeMap<String, Value>) -> Json {
    json!(
        fields
            .iter()
            .map(|(k, v)| (k, format!("{v:?}")))
            .collect::<Vec<_>>()
    )
}

fn entities(input: &Json) -> Json {
    let calls = Rc::new(RefCell::new(Vec::new()));
    let mut list = SmartList::new(
        input["entities"]
            .as_array()
            .unwrap()
            .iter()
            .enumerate()
            .map(|(index, x)| Item {
                fields: x["fields"]
                    .as_object()
                    .unwrap()
                    .iter()
                    .map(|(k, v)| (k.clone(), value(v)))
                    .collect(),
                dirty: x["dirty"]
                    .as_array()
                    .map(|v| v.iter().map(|x| x.as_str().unwrap().to_owned()).collect()),
                new: x["new"].as_bool().unwrap(),
                deleted: x["deleted"].as_bool().unwrap(),
                version: x["version"].as_str().unwrap().parse().unwrap(),
                index,
                calls: calls.clone(),
            })
            .collect(),
    );
    let meta = facet(&input["metadata"]);
    list.is_loaded = meta.is_loaded;
    list.total_count = meta.total_count;
    list.aggregations = meta.aggregations;
    list.summary = meta.summary;
    list.facets = meta.facets;
    let ids = list
        .ids()
        .iter()
        .map(|v| format!("{v:?}"))
        .collect::<Vec<_>>();
    let versions = list
        .versions()
        .iter()
        .map(|v| v.to_string())
        .collect::<Vec<_>>();
    let by_id = list
        .map_by_id()
        .iter()
        .map(|(k, v)| (k.clone(), record_output(&v.fields)))
        .collect::<Vec<_>>();
    let values = list.clone().into_values();
    let values_output = json!({"metadata":metadata(&values),"data":values.iter().map(|v|record_output(v)).collect::<Vec<_>>(),"calls":calls.borrow().clone()});
    calls.borrow_mut().clear();
    let converted = list.into_value();
    json!({"ids":ids,"versions":versions,"by_id":by_id,"into_values":values_output,
           "into_value":{"present":converted.is_some(),"value":converted.map(|v|format!("{v:?}")),"calls":calls.borrow().clone()}})
}

fn value(x: &Json) -> Value {
    let v = &x["value"];
    match x["kind"].as_str().unwrap() {
        "null" => Value::Null,
        "typed_null" => Value::TypedNull(DataType::Text),
        "bool" => Value::Bool(v.as_bool().unwrap()),
        "text" => Value::Text(v.as_str().unwrap().into()),
        "i64" => Value::I64(v.as_str().unwrap().parse().unwrap()),
        "u64" => Value::U64(v.as_str().unwrap().parse().unwrap()),
        "f64" => Value::F64(v.as_str().unwrap().parse().unwrap()),
        "decimal" => Value::Decimal(v.as_str().unwrap().parse().unwrap()),
        "json" => Value::Json(v.clone()),
        "date" => Value::Date(Value::from(v.as_str().unwrap()).try_date().unwrap()),
        "timestamp" => Value::Timestamp(teaql_core::time::Timestamp::from(
            v.as_str().unwrap().parse::<i64>().unwrap(),
        )),
        "list" => Value::List(v.as_array().unwrap().iter().map(value).collect()),
        "object" => Value::Object(
            v.as_object()
                .unwrap()
                .iter()
                .map(|(k, v)| (k.clone(), value(v)))
                .collect(),
        ),
        _ => panic!("invalid fixture"),
    }
}
fn values(x: &Json) -> Vec<Value> {
    x.as_array().unwrap().iter().map(value).collect()
}
fn row(x: &Json) -> CompactRow {
    let names: Vec<String> = x["names"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| v.as_str().unwrap().into())
        .collect();
    CompactRow::new(names.into(), values(&x["values"]))
}
fn facet(x: &Json) -> SmartList<CompactRow> {
    let mut list = SmartList::new(x["data"].as_array().unwrap().iter().map(row).collect());
    list.is_loaded = x["loaded"].as_bool().unwrap();
    list.total_count = x["total"].as_str().map(|v| v.parse().unwrap());
    for (k, v) in x["summary"].as_object().unwrap() {
        list.summary.insert(k.clone(), value(v));
    }
    for (k, v) in x["aggregations"].as_object().unwrap() {
        list.aggregations.insert(k.clone(), value(v));
    }
    for (k, v) in x["facets"].as_object().unwrap() {
        list.add_facet(k, facet(v));
    }
    list
}
fn metadata<T>(list: &SmartList<T>) -> Json {
    json!({"loaded":list.is_loaded,"total":list.total_count.map(|v|v.to_string()),
           "aggregations":list.aggregations.iter().map(|(k,v)|(k,format!("{v:?}"))).collect::<Vec<_>>(),
           "summary":list.summary.iter().map(|(k,v)|(k,format!("{v:?}"))).collect::<Vec<_>>(),
           "facets":list.facets.iter().map(|(k,v)|(k,facet_output(v))).collect::<Vec<_>>()})
}
fn facet_output(list: &SmartList<CompactRow>) -> Json {
    json!({"metadata":metadata(list),"rows":list.data.iter().map(|r|r.iter().map(|(k,v)|(k,format!("{v:?}"))).collect::<Vec<_>>()).collect::<Vec<_>>()})
}
fn output(list: &SmartList<Value>) -> Json {
    json!({"metadata":metadata(list),"data":list.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),
           "count":list.total_count_or_len().to_string(),"length":list.len(),"empty":list.is_empty(),
           "first":list.first().map(|v|format!("{v:?}")),"last":list.last().map(|v|format!("{v:?}")),
           "equal_clone":list==&list.clone(),"aggregation_json":list.aggregation_json(),"summary_json":list.summary_json()})
}
fn observe(input: Json) -> Json {
    if let Some(fail_at) = input["drop_entity"].as_i64() {
        struct DropEntity { value:i32, fail_at:i32, events:Rc<RefCell<Vec<i32>>> }
        impl Drop for DropEntity {
            fn drop(&mut self) { self.events.borrow_mut().push(-self.value); }
        }
        impl TeaqlEntity for DropEntity {
            const ENTITY_NAME:&'static str="DropEntity";
            fn entity_descriptor()->EntityDescriptor { EntityDescriptor::new(Self::ENTITY_NAME) }
        }
        impl Entity for DropEntity {
            fn from_compact_row(_:CompactRow)->Result<Self,EntityError> { unreachable!() }
            fn into_values(self)->MutationValues {
                self.events.borrow_mut().push(self.value);
                if self.value==self.fail_at { panic!("entity conversion failed"); }
                BTreeMap::from([("id".to_owned(),Value::U64(self.value as u64))]).into()
            }
        }
        let events=Rc::new(RefCell::new(Vec::new()));
        let mut list:SmartList<_>=(1..=5).map(|value|DropEntity{value,fail_at:fail_at as i32,events:events.clone()}).collect();
        list.is_loaded=input["loaded"].as_bool().unwrap();
        let mut present=false;
        let failed=std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            if input["projection"]=="values" {
                let result=list.into_values();
                present=!result.is_empty();
            } else {
                present=list.into_value().is_some();
            }
        })).is_err();
        let observed=events.borrow().clone();
        return json!({"events":observed,"present":present,"failed":failed});
    }
    if let Some(bits) = input["float32"].as_u64() {
        let list = SmartList::new(vec![f32::from_bits(bits as u32)]);
        return json!({"compact":format!("{list:?}"),"pretty":format!("{list:#?}"),
            "lower_hex":format!("{list:x?}"),"upper_hex":format!("{list:X?}"),
            "pretty_lower_hex":format!("{list:#x?}"),"pretty_upper_hex":format!("{list:#X?}"),
            "width_precision":format!("{list:>40.2?}"),"fancy":format!("{list:🧊^+18.3?}"),
            "zero_pad":format!("{list:+012?}")});
    }
    if let Some(fail_at) = input["drop_map"].as_i64() {
        struct DropItem { value:i32, events:Rc<RefCell<Vec<i32>>> }
        impl Drop for DropItem {
            fn drop(&mut self) { self.events.borrow_mut().push(-self.value); }
        }
        let events=Rc::new(RefCell::new(Vec::new()));
        let list:SmartList<_>=(1..=5).map(|value|DropItem{value,events:events.clone()}).collect();
        let mut result=None;
        let failed=std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            result=Some(list.map(|item| {
                events.borrow_mut().push(item.value);
                if item.value==fail_at as i32 { panic!("map callback failed"); }
                item.value
            }));
        })).is_err();
        let data=result.map(|list|list.into_vec()).unwrap_or_default();
        let observed=events.borrow().clone();
        return json!({"events":observed,"data":data,"failed":failed});
    }
    if let Some(seed) = input["boolean"].as_u64() {
        let mut list = if seed & 1 == 0 { SmartList::<bool>::empty() } else {
            SmartList::new((0..6).map(|bit|seed & (1 << bit) != 0).collect())
        };
        list = list.with_total_count(17).with_summary("flag", seed & 2 != 0);
        let state = |list: &SmartList<bool>|json!({"data":list.as_slice(),
            "loaded":list.is_loaded,"total":list.total_count,"debug":format!("{list:?}")});
        let mut states = vec![state(&list)];
        list.push(seed & 4 != 0);
        list.extend([true, false]);
        states.push(state(&list));
        let replaced = list.set(0, seed & 8 != 0);
        *list.get_mut(1).unwrap() = seed & 16 != 0;
        for item in list.iter_mut() { *item = !*item; }
        list.as_mut_slice()[0] = seed & 32 != 0;
        states.push(state(&list));
        let mapped = list.to_list(|item|!*item);
        let unique = list.to_set(|item|*item).into_iter().collect::<Vec<_>>();
        let identities = list.identity_map(|item|*item).into_iter().collect::<Vec<_>>();
        let groups = list.group_by(|item|*item).into_iter().collect::<Vec<_>>();
        list.retain(|item| *item || seed & 64 != 0);
        states.push(state(&list));
        list.merge_by([false, true, false], |item|*item);
        states.push(state(&list));
        list = list.map(|item|!item);
        states.push(state(&list));
        let loaded = list.is_loaded;
        let data = list.into_vec();
        let reconstructed = SmartList::new(data);
        return json!({"states":states,"replaced":replaced,"mapped":mapped,
            "unique":unique,"identities":identities,"groups":groups,
            "loaded_before_extract":loaded,"reconstructed":state(&reconstructed)});
    }
    if let Some(fail_at) = input["drop_trace"].as_i64() {
        struct DropItem {
            value: i32,
            events: Rc<RefCell<Vec<i32>>>,
        }
        impl Drop for DropItem {
            fn drop(&mut self) { self.events.borrow_mut().push(-self.value); }
        }
        let events = Rc::new(RefCell::new(Vec::new()));
        let mut list: SmartList<_> = (1..=5).map(|value|DropItem {value,events:events.clone()}).collect();
        let failed = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            list.retain(|item| {
                events.borrow_mut().push(item.value);
                if item.value == fail_at as i32 { panic!("list callback failed"); }
                item.value % 2 != 0
            });
        })).is_err();
        let during = events.borrow().clone();
        let data: Vec<_> = list.iter().map(|item|item.value).collect();
        drop(list);
        let after = events.borrow().clone();
        return json!({"during":during,"after":after,"data":data,"failed":failed});
    }
    if let Some(field) = input["equality"].as_str() {
        let row = CompactRow::new(vec!["v".to_owned()].into(), vec![Value::U64(3)]);
        let mut before = SmartList::new(vec![Value::U64(1), Value::U64(2)])
            .with_aggregation("a", 1_u64).with_summary("s", 2_u64)
            .with_facet("rows", SmartList::new(vec![row]));
        match field {
            "nan_data" => before.data[0] = Value::F64(f64::NAN),
            "nan_aggregation" => { before.aggregations.insert("a".into(), Value::F64(f64::NAN)); },
            "nan_summary" => { before.summary.insert("s".into(), Value::F64(f64::NAN)); },
            "nan_facet" => { before.facets.get_mut("rows").unwrap().data[0].insert("v".into(), Value::F64(f64::NAN)); },
            _ => (),
        }
        let mut after = before.clone();
        match field {
            "same" | "nan_data" | "nan_aggregation" | "nan_summary" | "nan_facet" => (),
            "data" => after.data[0] = Value::U64(3),
            "data_order" => after.data.swap(0, 1),
            "total_count" => after.total_count = Some(0),
            "aggregation" => { after.aggregations.insert("a".into(), Value::U64(2)); },
            "summary" => { after.summary.insert("s".into(), Value::U64(3)); },
            "facets" => after.facets.clear(),
            "is_loaded" => after.is_loaded = false,
            "facet_count" => after.facets.get_mut("rows").unwrap().total_count = Some(0),
            "facet_data" => { after.facets.get_mut("rows").unwrap().data[0].insert("v".into(), Value::U64(4)); },
            "facet_loaded" => after.facets.get_mut("rows").unwrap().is_loaded = false,
            "facet_summary" => { after.facets.get_mut("rows").unwrap().summary.insert("caption".into(), Value::U64(9)); },
            _ => panic!("Unknown equality field"),
        }
        return json!({"before":output(&before),"after":output(&after),
            "self_equal":before==before,"equal":before==after});
    }
    if let Some(method) = input["callback_trace"].as_str() {
        let trace = RefCell::new(Vec::new());
        let mut list = SmartList::new(vec![1, 2, 3, 4, 5]);
        let fail_at = input["fail_at"].as_i64().unwrap() as i32;
        let callback = |item: &i32| {
            trace.borrow_mut().push(*item);
            if *item == fail_at {
                panic!("list callback failed");
            }
            item % 3
        };
        let mut returned = Json::Null;
        let failed = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            returned = match method {
                "retain" => {
                    list.retain(|item| {
                        callback(item);
                        item % 2 != 0
                    });
                    Json::Null
                },
                "map" => json!(list.clone().map(|item|callback(&item)).into_vec()),
                "to_list" => json!(list.to_list(callback)),
                "to_set" => json!(list.to_set(callback).into_iter().collect::<Vec<_>>()),
                "identity" => json!(list.identity_map(callback).into_iter().collect::<Vec<_>>()),
                "group" => json!(list.group_by(callback).into_iter().collect::<Vec<_>>()),
                _ => panic!("Unknown callback operation"),
            };
        })).is_err();
        return json!({"trace":trace.into_inner(),"data":list.into_vec(),"failed":failed,"returned":returned});
    }
    if input.get("iterator_trace").is_some() {
        let trace = std::cell::RefCell::new(Vec::new());
        let mut list = SmartList::new(vec![1, 2]);
        let fail = input["iterator_trace"].as_bool().unwrap();
        let method = input["method"].as_str().unwrap_or("merge");
        let stage = input["fail_stage"].as_str().unwrap_or("yield");
        let fail_at = input["fail_at"].as_i64().unwrap_or(3) as i32;
        let incoming = [12, 3, 14].into_iter().map(|item| {
            trace.borrow_mut().push(item);
            if fail && stage == "yield" && item == fail_at {
                panic!("incoming iterator failed");
            }
            item
        });
        let failed = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            match method {
                "merge" => list.merge_by(incoming, |item| {
                    trace.borrow_mut().push(-*item);
                    if fail && stage == "key" && *item == fail_at {
                        panic!("merge key failed");
                    }
                    item % 10
                }),
                "extend" => list.extend(incoming),
                "collect" => list = incoming.collect(),
                _ => panic!("Unknown iterator operation"),
            }
        })).is_err();
        return json!({"trace":trace.into_inner(),"data":list.into_vec(),"failed":failed});
    }
    if input.get("entities").is_some() {
        return entities(&input);
    }
    let mut list = if input["construct"] == "default" {
        SmartList::empty()
    } else {
        SmartList::new(values(&input["data"]))
    };
    let mut states = vec![json!({"list":output(&list),"returned":Json::Null})];
    for op in input["ops"].as_array().unwrap() {
        let key = op["key"].as_str().unwrap_or("");
        let returned = match op["op"].as_str().unwrap() {
            "push" => {
                list.push(value(&op["value"]));
                Json::Null
            }
            "extend" => {
                list.extend(values(&op["values"]));
                Json::Null
            }
            "self_extend" => {
                list.extend(list.clone().into_vec());
                Json::Null
            }
            "set" => json!(
                list.set(op["index"].as_u64().unwrap() as usize, value(&op["value"]))
                    .map(|v| format!("{v:?}"))
            ),
            "get" => json!(
                list.get(op["index"].as_u64().unwrap() as usize)
                    .map(|v| format!("{v:?}"))
            ),
            "get_mut" => {
                let target = list.get_mut(op["index"].as_u64().unwrap() as usize);
                if let Some(target) = target {
                    *target = value(&op["value"]);
                    json!(format!("{target:?}"))
                } else {
                    Json::Null
                }
            }
            "index" => json!(format!(
                "{:?}",
                list[op["index"].as_u64().unwrap() as usize]
            )),
            "index_mut" => {
                list[op["index"].as_u64().unwrap() as usize] = value(&op["value"]);
                Json::Null
            }
            "loaded" => {
                list.is_loaded = op["value"].as_bool().unwrap();
                Json::Null
            }
            "total" => {
                list = list.with_total_count(op["value"].as_str().unwrap().parse().unwrap());
                Json::Null
            }
            "aggregation" => {
                list = list.with_aggregation(key, value(&op["value"]));
                Json::Null
            }
            "summary" => {
                list = list.with_summary(key, value(&op["value"]));
                Json::Null
            }
            "facet" => {
                list.add_facet(key, facet(&op["value"]));
                Json::Null
            }
            "with_facet" => {
                list = list.with_facet(key, facet(&op["value"]));
                Json::Null
            }
            "facets_mut" => {
                list.facets_mut()
                    .insert(key.to_owned(), facet(&op["value"]));
                Json::Null
            }
            "facet_lookup" => list
                .facets()
                .get(key)
                .map(facet_output)
                .unwrap_or(Json::Null),
            "aggregation_lookup" => json!(list.aggregation(key).map(|v| format!("{v:?}"))),
            "summary_lookup" => json!(list.summary(key).map(|v| format!("{v:?}"))),
            "facet_mut" => {
                if let Some(f) = list.facet_mut(key) {
                    f.is_loaded = op["value"].as_bool().unwrap();
                }
                Json::Null
            }
            "remove_facet" => list
                .remove_facet(key)
                .map(|v| facet_output(&v))
                .unwrap_or(Json::Null),
            "take_facets" => json!(
                list.take_facets()
                    .iter()
                    .map(|(k, v)| (k, facet_output(v)))
                    .collect::<Vec<_>>()
            ),
            "collect" => {
                list = SmartList::from_iter(list.into_vec());
                Json::Null
            }
            "map" => {
                list = list.map(|v| v);
                Json::Null
            }
            "iter_mut" => {
                for item in list.iter_mut() {
                    *item = value(&op["value"]);
                }
                Json::Null
            }
            "slice_mut" => {
                for item in list.as_mut_slice() {
                    *item = value(&op["value"]);
                }
                Json::Null
            }
            "retain" => {
                list.retain(|v| v != &value(&op["value"]));
                Json::Null
            }
            "to_list" => json!(list.to_list(|v| format!("{v:?}"))),
            "to_set" => json!(list.to_set(|v| format!("{v:?}"))),
            "identity" => json!(
                list.identity_map(|v| format!("{v:?}"))
                    .iter()
                    .map(|(k, v)| (k, format!("{v:?}")))
                    .collect::<Vec<_>>()
            ),
            "group" => json!(
                list.group_by(|v| format!("{v:?}"))
                    .iter()
                    .map(|(k, v)| (k, v.iter().map(|v| format!("{v:?}")).collect::<Vec<_>>()))
                    .collect::<Vec<_>>()
            ),
            "merge" => {
                list.merge_by(values(&op["values"]), |v| format!("{v:?}"));
                Json::Null
            }
            "self_merge" => {
                list.merge_by(list.clone().into_vec(), |v| format!("{v:?}"));
                Json::Null
            }
            _ => panic!("invalid operation"),
        };
        states.push(json!({"list":output(&list),"returned":returned}));
    }
    json!(states)
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
