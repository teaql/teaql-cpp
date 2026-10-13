//! Original public graph facet storage, replacement and independent ledger behavior.
use std::{collections::BTreeMap, io::{self, BufRead}};
use serde_json::{json, Value as J};
use teaql_core::{CompactRow, SmartList, Value};
use teaql_runtime::{EntityGraphBuilder, EntityRuntimeState};

fn facets(input: &J) -> BTreeMap<String, SmartList<CompactRow>> {
    input.as_object().unwrap().iter().map(|(name, values)| {
        let rows = values.as_array().unwrap().iter().map(|value|
            CompactRow::from_map(BTreeMap::from([("label".into(), Value::Text(value.as_str().unwrap().into()))]))
        ).collect();
        (name.clone(), SmartList::new(rows))
    }).collect()
}
fn inspect(state: &EntityRuntimeState, input: &J) -> J {
    let owner=input["owner"].as_str().unwrap();
    let id=input["id"].as_u64().unwrap();
    let relation=input["relation"].as_str().unwrap();
    let facet=input["facet"].as_str().unwrap();
    let result=state.relation_facet(owner,id,relation,facet).map(|list|
        list.data.iter().map(|row|row.get("label").unwrap().try_text().unwrap().to_owned()).collect::<Vec<_>>());
    json!({"facet":result,"list_state":format!("{:?}",state.relation_list::<CompactRow>(owner,id,relation).state()),
        "option_state":format!("{:?}",state.relation_option::<CompactRow>(owner,id,"optional").state())})
}
fn evaluate(input:J)->J {
    let root=EntityRuntimeState::default();
    let mut graph=EntityGraphBuilder::default();
    for action in input["actions"].as_array().unwrap() {
        let owner=action["owner"].as_str().unwrap();
        let id=action["id"].as_u64().unwrap();
        let relation=action["relation"].as_str().unwrap();
        match action["op"].as_str().unwrap() {
            "facets"=>graph.install_relation_facets(owner,id,relation,facets(&action["facets"])),
            "list"=>{let mut list=SmartList::<CompactRow>::new(Vec::new());list.facets=facets(&action["facets"]);graph.install_relation_list(owner,id,relation,list);},
            "null"=>graph.install_relation_option::<CompactRow>(owner,id,relation,None),
            _=>panic!("unknown graph fixture operation"),
        }
    }
    let relation_count=graph.relation_list_count();
    let before=input["lookups"].as_array().unwrap().iter().map(|item|inspect(&root,item)).collect::<Vec<_>>();
    root.freeze_graph(graph).unwrap();
    let fresh=EntityRuntimeState::fresh_with_shared_graph(&root);
    let after=input["lookups"].as_array().unwrap().iter().map(|item|inspect(&root,item)).collect::<Vec<_>>();
    let shared=input["lookups"].as_array().unwrap().iter().map(|item|inspect(&fresh,item)).collect::<Vec<_>>();
    let mut row=CompactRow::from_map(BTreeMap::from([("id".into(),Value::U64(1))]));
    let mut loaded=SmartList::<CompactRow>::new(Vec::new());loaded.facets=facets(&input["snapshot_facets"]);
    row.set_loaded_relation("reference".into(),loaded);
    let mut first=EntityRuntimeState::fresh_with_shared_graph(&root);
    first.set_original_compact_row("Owner",row);
    first.set_comment("independent ledger");
    json!({"before":before,"after":after,"shared":shared,"relation_count":relation_count,
        "first_comment":first.get_comment(),"fresh_comment":fresh.get_comment()})
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
