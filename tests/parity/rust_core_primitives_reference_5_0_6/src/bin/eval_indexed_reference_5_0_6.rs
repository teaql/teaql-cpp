//! Exercise the complete original six-variant load-state mutation surface.
use serde_json::{Value as Json, json};
use std::{io::{self, BufRead}, sync::Arc};
use teaql_core::{FieldLayout, LoadedSnapshot, eval::LoadState};

fn layout(entity: &str) -> Arc<FieldLayout> {
    let names: Vec<_> = (0..130).map(|i| match i {
        0 => "id".to_owned(), 1 => "version".to_owned(), _ => format!("field_{i}"),
    }).collect();
    let aliases: Vec<_> = (0..130).map(|i| format!("sql_{i}")).collect();
    let indexes: Vec<_> = names.iter().enumerate().map(|(i,n)| (n.as_str(),i)).collect();
    let mappings: Vec<_> = names.iter().zip(&aliases).map(|(n,a)| (n.as_str(),n.as_str(),a.as_str())).collect();
    let expected: Vec<_> = names.iter().map(String::as_str).collect();
    FieldLayout::from_generated(entity,"eval-v1",&indexes,&mappings,&["lines"],&expected).unwrap()
}
#[derive(Debug)]
enum SnapshotWrapper<'a> { Indexed(&'a LoadedSnapshot) }
fn diagnostics(state: &LoadState) -> Json {
    let actual = vec![format!("{state:?}"),format!("{state:#?}"),format!("{state:x?}"),format!("{state:#X?}")];
    let expected = if let LoadState::Indexed(snapshot) = state {
        let wrapper=SnapshotWrapper::Indexed(snapshot);
        vec![format!("{wrapper:?}"),format!("{wrapper:#?}"),format!("{wrapper:x?}"),format!("{wrapper:#X?}")]
    } else {vec![]};
    json!({"complete": expected.is_empty() || actual==expected,
           "actual":actual,"expected_snapshot_wrapper":expected})
}
fn summary(state: &LoadState) -> Json {
    let kind=match state {
        LoadState::NotLoaded=>"NotLoaded",LoadState::FullyLoaded=>"FullyLoaded",LoadState::Partial(_)=>"Partial",
        LoadState::PartialCompact(_)=>"PartialCompact",LoadState::SharedColumns(_)=>"SharedColumns",LoadState::Indexed(_)=>"Indexed",
    };
    let wire_rejected=matches!(state,LoadState::Indexed(_)) &&
        serde_json::to_value(state).unwrap_err().to_string()=="the enum variant LoadState::Indexed cannot be serialized";
    json!({"kind":kind,"loaded":(["id","version","field_2","sql_2","field_64","field_65","field_129","lines","#memo","unknown"]
        .iter().map(|name|state.is_loaded(name)).collect::<Vec<_>>()),"clone_equal":state==&state.clone(),"indexed_wire_rejected":wire_rejected})
}
fn observe(input: Json) -> Json {
    let metadata=layout("EvalRow");let foreign=layout("OtherEvalRow");
    let kind=input["kind"].as_str().unwrap();
    let fields=input["fields"].clone();
    let mut state=match kind {
        "NotLoaded"=>LoadState::NotLoaded,"FullyLoaded"=>LoadState::FullyLoaded,
        "Indexed"=>serde_json::from_value::<LoadState>(json!({"Partial":fields})).unwrap().into_indexed(metadata.clone()).unwrap(),
        _=>serde_json::from_value::<LoadState>(json!({kind:fields})).unwrap(),
    };
    let original=state.clone();let initial=summary(&state);let before=diagnostics(&state);
    let mut steps=Vec::new();
    for action in input["actions"].as_array().unwrap() {
        let field=action["field"].as_str().unwrap();
        let result=if action["action"]=="load" {state.mark_loaded(field)} else {state.mark_unloaded(field)};
        steps.push(json!({"error":result.err(),"state":summary(&state)}));
    }
    let converted=state.clone().into_indexed(metadata).unwrap();
    let other=state.into_indexed(foreign);let after=diagnostics(&converted);
    json!({"common":{"initial":initial,"initial_wrapper_complete":before["complete"],"steps":steps,
        "original":summary(&original),"original_unchanged":summary(&original)==initial,
        "converted":summary(&converted),"foreign":match other {Ok(value)=>summary(&value),Err(error)=>json!(error)},
        "final_wrapper_complete":after["complete"]},"initial_diagnostics":before,"final_diagnostics":after})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
