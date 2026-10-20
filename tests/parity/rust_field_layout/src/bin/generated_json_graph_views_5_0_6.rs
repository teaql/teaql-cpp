//! Actual generated borrowed presentation over explicit runtime graph boundaries.
use cpp_dynamic_json_core::{Order,Person};
use serde_json::{Value as J,json};
use std::{collections::BTreeMap,io::{self,BufRead}};
use teaql_core::{Entity,CompactRow,RelationShapeCache,SmartList};
use teaql_runtime::{EntityRuntimeState,EntityGraphBuilder};
#[path="../../../rust_values/src/fixture_value.rs"]mod fixture_value;
fn row(value:&J,shapes:&mut RelationShapeCache,relation:&str,loaded:bool)->CompactRow {
    let mut row=CompactRow::from_map(value.as_object().unwrap().iter()
        .map(|(k,v)|(k.clone(),fixture_value::value(v))).collect::<BTreeMap<_,_>>());
    if loaded{row.mark_relation_loaded(relation,shapes);}row
}
fn observe(input:&J)->J {
    let root=EntityRuntimeState::default();let mut shapes=RelationShapeCache::default();
    let order=Order::from_compact_row_with_context(row(&input["order"],&mut shapes,"customer",input["forward_loaded"].as_bool().unwrap()),&root);
    let person=Person::from_compact_row_with_context(row(&input["person"],&mut shapes,"orders",input["reverse_loaded"].as_bool().unwrap()),&root);
    let (order,person)=match (order,person){(Ok(a),Ok(b))=>(a,b),(Err(e),_)|(_,Err(e))=>return json!({"error":e.to_string()})};
    let mut graph=EntityGraphBuilder::default();
    if input["table"].as_bool().unwrap(){graph.install(2,person.clone());}
    if input["list"].as_bool().unwrap(){let mut list=SmartList::new(if input["empty"].as_bool().unwrap(){vec![]}else{vec![order.clone()]});list.is_loaded=true;graph.install_relation_list("Person",2,"orders",list);}
    match input["edge"].as_str().unwrap(){"target"=>graph.install_relation_option("Order",3,"customer",Some(person.clone())),"null"=>graph.install_relation_option::<Person>("Order",3,"customer",None),_=>{}}
    root.freeze_graph(graph).unwrap();json!({"order":order.into_json(),"person":person.into_json()})
}
fn main(){for line in io::stdin().lock().lines(){let input:J=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
