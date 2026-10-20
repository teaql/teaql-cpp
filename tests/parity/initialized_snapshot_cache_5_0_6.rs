//! Public original APIs; cache identities are observed, never reconstructed.
use serde_json::{Value as Json, json};
use std::{io::{self, BufRead}, sync::Arc};
use teaql_core::{CompactRow, CompactRowLayout, DataType, FieldLayout, LoadedSnapshot,
                 RelationShapeCache, Value, eval::LoadState};
use teaql_core::dynamic_properties::DynamicPropertyDefinitions;

fn layout(count: usize, entity: &str) -> Arc<FieldLayout> {
    let names: Vec<_> = (0..count).map(|i| match i {
        0 => "id".to_owned(), 1 => "version".to_owned(), _ => format!("field_{i}"),
    }).collect();
    let aliases: Vec<_> = (0..count).map(|i| format!("sql_{i}")).collect();
    let indexes: Vec<_> = names.iter().enumerate().map(|(i, n)| (n.as_str(), i)).collect();
    let mappings: Vec<_> = names.iter().zip(&aliases)
        .map(|(n, a)| (n.as_str(), n.as_str(), a.as_str())).collect();
    let expected: Vec<_> = names.iter().map(String::as_str).collect();
    FieldLayout::from_generated(entity, "v1", &indexes, &mappings, &["lines"], &expected).unwrap()
}

fn snapshot(state: LoadState) -> Arc<LoadedSnapshot> {
    match state { LoadState::Indexed(state) => state, _ => panic!("expected indexed state") }
}

fn geometry(state: &LoadedSnapshot, names: &[String]) -> Json {
    let mut overflow: Vec<_> = state.overflow().into_iter().flatten().copied().collect();
    overflow.sort();
    let mut selected: Vec<_> = state.selected_names().into_iter().flatten().cloned().collect();
    selected.sort();
    let schema = state.dynamic_property_definitions().map(|s| format!("{s:?}"));
    json!({"bits":state.bits().to_string(),"overflow":overflow,"selected":selected,
           "loaded":names.iter().map(|n| state.is_loaded(n)).collect::<Vec<_>>(),
           "schema":schema})
}

fn layout_case(input: &Json) -> Json {
    let count = input["count"].as_u64().unwrap() as usize;
    let names: Vec<String> = input["selected"].as_array().unwrap().iter()
        .map(|n| n.as_str().unwrap().to_owned()).collect();
    let mut columns = CompactRowLayout::new(names.clone().into());
    let schema = match input["schema"].as_u64().unwrap() {
        0 => None,
        1 => Some(DynamicPropertyDefinitions::new(Vec::new()).unwrap()),
        _ => Some(DynamicPropertyDefinitions::new(vec![
            ("_rank".to_owned(), DataType::U64), ("_label".to_owned(), DataType::Text)]).unwrap()),
    };
    if let Some(schema) = &schema { columns = columns.with_dynamic_property_definitions(schema.clone()); }
    let mut first = CompactRow::with_layout(columns, vec![Value::I64(11); names.len()]);
    let mut second = first.clone();
    if input["relations"].as_bool().unwrap() {
        let mut shapes = RelationShapeCache::default();
        first.mark_relation_loaded("lines", &mut shapes);
        second.mark_relation_loaded("lines", &mut shapes);
    }
    let a = layout(count, "Item");
    let b = layout(count, if input["other_entity"].as_bool().unwrap() { "Other" } else { "Item" });
    let primary = snapshot(first.indexed_load_state(a.clone()));
    let primary_again = snapshot(second.indexed_load_state(a.clone()));
    let secondary = snapshot(first.indexed_load_state(b.clone()));
    let secondary_again = snapshot(second.indexed_load_state(b.clone()));
    let lookups: Vec<String> = input["lookups"].as_array().unwrap().iter()
        .map(|n| n.as_str().unwrap().to_owned()).collect();
    let changed = LoadedSnapshot::with_loaded(&primary, "id", !primary.is_loaded("id")).unwrap();
    let restored = LoadedSnapshot::with_loaded(&changed, "id", primary.is_loaded("id")).unwrap();
    let before = format!("{:?}", first.shared_layout());
    let replacement = DynamicPropertyDefinitions::new(vec![("_rank".to_owned(), DataType::Bool)]).unwrap();
    let old_columns = first.shared_layout();
    let equal_schema_same = schema.as_ref().map(|s|
        Arc::ptr_eq(&old_columns, &old_columns.with_dynamic_property_definitions(s.clone())));
    let rebound_columns = old_columns.with_dynamic_property_definitions(replacement);
    let rebound = CompactRow::with_layout(rebound_columns.clone(), vec![Value::I64(33); names.len()]);
    let rebound_state = snapshot(rebound.indexed_load_state(a.clone()));
    if let Some(name) = names.first() { *first.get_mut(name).unwrap() = Value::I64(99); }
    let isolated = names.first().is_none_or(|n| second.get(n) == Some(&Value::I64(11)));
    json!({"common":{
        "shared_columns":Arc::ptr_eq(&first.shared_layout(), &second.shared_layout()),
        "primary_hit":Arc::ptr_eq(&primary, &primary_again),
        "secondary_hit":Arc::ptr_eq(&secondary, &secondary_again),
        "distinct_layout_state":!Arc::ptr_eq(&primary, &secondary),
        "primary":geometry(&primary, &lookups),"secondary":geometry(&secondary, &lookups),
        "changed":geometry(&changed, &lookups),"unchanged":geometry(&primary_again, &lookups),
        "restored_hit":Arc::ptr_eq(&primary, &restored),"equal_schema_same":equal_schema_same,
        "schema_rebound":!Arc::ptr_eq(&old_columns, &rebound_columns),
        "rebound":geometry(&rebound_state, &lookups),"value_isolated":isolated},
        "diagnostics":{"initialized_layout":before,"after_value_change":format!("{first:?}")}})
}

fn saturation_case(input: &Json) -> Json {
    let count = input["shapes"].as_u64().unwrap() as usize;
    let layout = layout(2, "Item");
    let mut states: Vec<_> = (0..count).map(|i|
        Some(LoadedSnapshot::projection(layout.clone(), [&*format!("#shape_{i}")]).into_shared()))
        .collect();
    let first = states[0].as_ref().unwrap().clone();
    let last = states[count-1].as_ref().unwrap().clone();
    let first_hit = LoadedSnapshot::projection(layout.clone(), ["#shape_0"]).into_shared();
    let last_hit = LoadedSnapshot::projection(layout.clone(), [&*format!("#shape_{}", count-1)]).into_shared();
    let hits = [Arc::ptr_eq(&first, &first_hit), Arc::ptr_eq(&last, &last_hit)];
    drop(first); drop(last); drop(first_hit); drop(last_hit);
    let weak = Arc::downgrade(states[0].as_ref().unwrap());
    let before = format!("{layout:?}");
    if input["release"].as_bool().unwrap() {
        for state in states.iter_mut().take(count / 2) { *state = None; }
    }
    let new_state = LoadedSnapshot::projection(layout.clone(), ["#new"]).into_shared();
    let repeated = LoadedSnapshot::projection(layout.clone(), ["#new"]).into_shared();
    let after = format!("{layout:?}");
    json!({"common":{"hits":hits,"released":weak.upgrade().is_none(),
        "new_hit":Arc::ptr_eq(&new_state,&repeated),
        "hint_count_before":before.matches("(Weak)").count(),
        "hint_count_after":after.matches("(Weak)").count(),
        "surviving_last":states[count-1].as_ref().unwrap().is_loaded(&format!("#shape_{}",count-1)),
        "new_loaded":new_state.is_loaded("#new")},
        "diagnostics":{"before":before,"after":after}})
}

fn ownership_case(_: &Json) -> Json {
    let a=layout(2,"Item");
    let b=layout(2,"Other");
    let (primary, secondary, alive) = {
        let row=CompactRow::new(vec!["id".to_owned()].into(),vec![Value::I64(1)]);
        let state=snapshot(row.indexed_load_state(a.clone()));
        let primary=Arc::downgrade(&state);
        drop(state);
        let other=snapshot(row.indexed_load_state(b.clone()));
        let secondary=Arc::downgrade(&other);
        drop(other);
        let alive=[primary.upgrade().is_some(),secondary.upgrade().is_some()];
        (primary,secondary,alive)
    };
    json!({"ownership":{"model":"weak_arc","within_row":alive,
        "after_row":[primary.upgrade().is_some(),secondary.upgrade().is_some()]}})
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
        let result=match input["kind"].as_str().unwrap() {
            "layout"=>layout_case(&input),"saturation"=>saturation_case(&input),
            "ownership"=>ownership_case(&input),_=>panic!("unknown kind"),
        };
        println!("{result}");
    }
}
