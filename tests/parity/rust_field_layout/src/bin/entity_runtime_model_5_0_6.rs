//! Actual original entity-runtime constructors, derives and public state transitions.
use serde_json::{json, Value as J};
use std::io::{self, BufRead};
use teaql_core::{CompactRow, SmartList, Value};
use teaql_runtime::*;
#[path = "../../../rust_values/src/fixture_value.rs"] mod fixture_value;
fn formatted<T: std::fmt::Debug>(value: &T) -> J {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn key(input:&J)->EntityKey {
    let id=if let Some(bits)=input["id"]["bits"].as_str() {Value::F64(f64::from_bits(u64::from_str_radix(bits,16).unwrap()))}else{fixture_value::value(&input["id"])};
    let mut key=EntityKey::new(input["entity"].as_str().unwrap(),id);
    if let Some(value)=input.get("mutate_id") {key.id=fixture_value::value(value);}
    if let Some(value)=input["mutate_entity"].as_str(){key.entity=value.to_owned().into();}
    key
}
fn changes(steps:&J)->ChangeSetStack {
    let mut stack=ChangeSetStack::default();
    for action in steps.as_array().unwrap() {
        match action["op"].as_str().unwrap(){
            "push"=>stack.push(),"pop"=>{stack.pop();},"clear"=>stack.clear_current(),
            "clear_entity"=>stack.clear_entity(&key(&action["key"])),
            "set"=>stack.set(key(&action["key"]),action["field"].as_str().unwrap(),fixture_value::value(&action["value"])),
            _=>panic!("unknown change-set input"),
        }
    }
    stack
}
fn state(input:&J)->EntityRuntimeState {
    let mut state=EntityRuntimeState::default();
    for action in input["actions"].as_array().unwrap() {
        match action["op"].as_str().unwrap(){
            "set"=>state.set(key(&action["key"]),action["field"].as_str().unwrap(),fixture_value::value(&action["value"])),
            "push"=>state.push_change_set(),"pop"=>{state.pop_change_set();},"clear"=>state.clear_current_change_set(),
            "commit"=>state.clear_committed(),"comment"=>state.set_comment(action["value"].as_str().unwrap()),
            "new"=>state.mark_as_new(key(&action["key"])),"delete"=>state.mark_as_delete(key(&action["key"])),
            "version"=>state.set_original_version(key(&action["key"]),action["value"].as_str().unwrap().parse().unwrap()),
            "snapshot"=>{
                let pairs=action["values"].as_array().unwrap();
                let row=CompactRow::new(pairs.iter().map(|p|p[0].as_str().unwrap().to_owned()).collect::<Vec<_>>().into(),pairs.iter().map(|p|fixture_value::value(&p[1])).collect());
                state.set_original_compact_row(action["entity"].as_str().unwrap(),row);
            },
            _=>panic!("unknown state input"),
        }
    }
    state
}
fn graph(input:&J)->EntityGraphBuilder {
    let mut graph=EntityGraphBuilder::default();
    for item in input["entities"].as_array().unwrap() {
        let id=item["id"].as_str().unwrap().parse().unwrap();
        if item["integer"]==true {graph.install(id,item["value"].as_i64().unwrap());}
        else {graph.install(id,item["value"].as_str().unwrap().to_owned());}
    }
    for relation in input["relations"].as_array().unwrap() {
        let owner=relation["owner"].as_str().unwrap();let id=relation["id"].as_str().unwrap().parse().unwrap();let name=relation["name"].as_str().unwrap();
        if relation["list"]==true {
            let values=relation["values"].as_array().unwrap().iter().map(|v|v.as_str().unwrap().to_owned()).collect::<Vec<_>>();
            graph.install_relation_list(owner,id,name,SmartList::from(values));
        }else{graph.install_relation_option(owner,id,name,relation["value"].as_str().map(str::to_owned));}
    }
    graph
}
fn evaluate(input:J)->J {
    match input["mode"].as_str().unwrap(){
        "key"=>{let value=key(&input["key"]);let copied=value.clone();let other=key(&input["other"]);json!({"debug":formatted(&value),"clone":formatted(&copied),"clone_equal":value==copied,"other_equal":value==other,"ordering":format!("{:?}",value.cmp(&other))})},
        "stack"=>{let value=changes(&input["steps"]);let current=value.current().cloned().unwrap_or_default();json!({"debug":formatted(&value),"clone":formatted(&value.clone()),"clone_equal":value==value.clone(),"current":formatted(&current),"current_clone":formatted(&current.clone()),"current_equal":current==current.clone()})},
        "error"=>{let value=match input["variant"].as_u64().unwrap(){0=>LedgerCompositionError::MissingTargetState,1=>LedgerCompositionError::MissingSourceState,_=>LedgerCompositionError::ConflictingOriginalVersion{entity:input["entity"].as_str().unwrap().into(),id:input["id"].as_str().unwrap().into(),target:input["target"].as_str().unwrap().parse().unwrap(),source:input["source"].as_str().unwrap().parse().unwrap()}};json!({"debug":formatted(&value),"clone":formatted(&value.clone()),"clone_equal":value==value.clone(),"display":value.to_string(),"source_none":std::error::Error::source(&value).is_none()})},
        "graph"=>{
            let graph=graph(&input);let debug=formatted(&graph);let state=state(&input);
            state.freeze_graph(graph).unwrap();
            let owner=input["owner"].as_str().unwrap();let id=input["owner_id"].as_str().unwrap().parse().unwrap();
            let list=state.relation_list::<String>(owner,id,"items");let option=state.relation_option::<String>(owner,id,"optional");
            json!({"builder":debug,"state":formatted(&state),"clone_state":formatted(&state.clone()),"state_equal":state==state.clone(),"list":formatted(&list),"list_clone":formatted(&list.clone()),"list_state":formatted(&list.state()),"list_loaded":list.is_loaded(),"list_empty":list.is_empty(),"option":formatted(&option),"option_clone":formatted(&option.clone()),"option_state":formatted(&option.state()),"option_loaded":option.is_loaded(),"option_empty":option.is_empty()})
        },
        "state"=>{let state=state(&input);json!({"debug":formatted(&state),"clone":formatted(&state.clone()),"fresh":formatted(&EntityRuntimeState::fresh_with_shared_graph(&state)),"equal_clone":state==state.clone(),"equal_fresh":state==EntityRuntimeState::fresh_with_shared_graph(&state)})},
        _=>panic!("unknown entity-runtime model fixture"),
    }
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
