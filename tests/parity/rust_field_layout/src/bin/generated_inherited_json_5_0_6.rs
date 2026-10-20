//! Decode and read back actual unchanged generated entities through Context.
use cpp_inheritance_graph_core::*;
use serde_json::{Value as J,json};
use std::{io::{self,BufRead},collections::BTreeMap};
use teaql_core::{Entity,TeaqlEntity};
fn state<T:Entity+TeaqlEntity+Clone>(entity:&T)->J {
    let d=T::entity_descriptor();
    let loaded:BTreeMap<_,_>=d.properties.iter().map(|p|p.name.clone()).chain(d.relations.iter().map(|r|r.name.clone()))
        .map(|name|{let value=entity.is_field_loaded(&name);(name,value)}).collect();
    json!({"json":entity.clone().into_json(),"loaded":loaded,
        "dirty":entity.dirty_fields(),"is_new":entity.is_new(),"deleted":entity.is_marked_as_delete()})
}
fn execute<T:Entity+TeaqlEntity+Clone>(context:&teaql_runtime::UserContext,input:&J)->J {
    if input["array"].as_bool().unwrap() {
        match context.decode_json_entities::<T>(&input["value"]) {
            Err(e)=>json!({"error":e.to_string()}),Ok(rows)=>json!({"rows":rows.iter().map(state).collect::<Vec<_>>()})
        }
    } else {
        match context.decode_json_entity::<T>(&input["value"]) {
            Err(e)=>json!({"error":e.to_string()}),Ok(row)=>json!({"row":state(&row)})
        }
    }
}
fn main(){for line in io::stdin().lock().lines(){let input:J=serde_json::from_str(&line.unwrap()).unwrap();
    let context=module().into_context();let out=match input["entity"].as_str().unwrap(){
        "School"=>execute::<School>(&context,&input),"Academy"=>execute::<Academy>(&context,&input),
        "ResearchAcademy"=>execute::<ResearchAcademy>(&context,&input),"Specialist"=>execute::<Specialist>(&context,&input),
        "Enrollment"=>execute::<Enrollment>(&context,&input),_=>execute::<Platform>(&context,&input)};
        println!("{}",out);}}
