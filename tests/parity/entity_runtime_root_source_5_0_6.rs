//! The complete locked module is included unchanged so the probe can observe
//! crate-private weak graph views, lazy ledgers and deferred composition errors.
use std::{io::{self,BufRead},collections::BTreeMap,panic::{catch_unwind,AssertUnwindSafe}};
use serde_json::{Value as J,json};
use teaql_core::{Value,CompactRow,SmartList};
pub use teaql_runtime::{canonical_id_space_entity, TraceScopeToken};
#[path="rust_values/src/fixture_value.rs"]mod fixture_value;
// Original observer body below is unchanged; access delegates to the crate-root runtime.
mod pinned {
    pub use teaql_runtime::*;
    pub fn materialized(s: &EntityRuntimeState) -> bool { s.__cpp_source_materialized() }
    pub fn error(s: &EntityRuntimeState) -> Option<LedgerCompositionError> { s.__cpp_source_error() }
    pub fn compose(a: &EntityRuntimeState, b: &EntityRuntimeState) -> Result<(), LedgerCompositionError> {
        a.__cpp_source_compose(b)
    }
    pub fn weak(s: &EntityRuntimeState) -> EntityRuntimeState { EntityRuntimeState::__cpp_source_weak(s) }
}
use pinned::*;
fn items<'a>(j:&'a J,key:&str)->&'a [J]{j[key].as_array().map(Vec::as_slice).unwrap_or(&[])}
fn key(j:&J)->EntityKey{EntityKey::new(j["entity"].as_str().unwrap(),fixture_value::value(&j["id"]))}
fn record(j:&J)->BTreeMap<String,Value>{j.as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect()}
fn record_out(values:&BTreeMap<String,Value>)->J{json!(values.iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>())}
fn changes(set:&EntityChangeSet)->J{json!(set.changes().iter().map(|(k,v)|json!([k.entity,format!("{:?}",k.id),record_out(v)])).collect::<Vec<_>>())}
fn snapshot(state:&EntityRuntimeState,keys:&[EntityKey])->J{
 let queried=keys.iter().map(|key|json!({"new":state.is_new(key),"delete":state.is_marked_as_delete(key),"version":state.get_original_version(key).map(|v|v.to_string()),"fields":state.changed_field_names(key),"values":(["a","b","name","id","version"]).iter().map(|f|state.get(key,f).map(|v|format!("{v:?}"))).collect::<Vec<_>>(),"trace":state.get_trace_chain(key).len()})).collect::<Vec<_>>();
 let keys_out=|keys:std::collections::BTreeSet<EntityKey>|keys.iter().map(|k|json!([k.entity,format!("{:?}",k.id)])).collect::<Vec<_>>();
 json!({"materialized":pinned::materialized(state),"current":changes(&state.current_change_set()),"comment":state.get_comment(),"original":state.original_snapshot().as_ref().map(|v|record_out(v)),"new":keys_out(state.new_keys()),"deleted":keys_out(state.deleted_keys()),"error":pinned::error(state).map(|e|e.to_string()),"queried":queried})
}
fn ledger(j:&J)->J{
 let keys=items(j,"keys").iter().map(key).collect::<Vec<_>>();let mut states=(0..3).map(|_|EntityRuntimeState::default()).collect::<Vec<_>>();let mut out=vec![];
 for step in items(j,"steps"){
  let n=step["state"].as_u64().unwrap_or(0) as usize;let other=step["other"].as_u64().unwrap_or(1) as usize;let key=&keys[step["key"].as_u64().unwrap_or(0) as usize];let mut result=J::Null;
  match step["op"].as_str().unwrap(){
   "clone"=>states[n]=states[other].clone(),"fresh"=>states[n]=EntityRuntimeState::fresh_with_shared_graph(&states[other]),"share"=>states[n]=states[n].with_shared_graph(&states[other]),"weak"=>states[n]=pinned::weak(&states[other]),"reset"=>states[n]=EntityRuntimeState::default(),
   "load"=>states[n].set_original_compact_row(step["entity"].as_str().unwrap(),CompactRow::from_map(record(&step["values"]))),
   "snapshot"=>states[n].set_original_snapshot(record(&step["values"]).into()),
   "set"=>states[n].set(key.clone(),step["field"].as_str().unwrap(),fixture_value::value(&step["value"])),
   "push"=>states[n].push_change_set(),"pop"=>result=states[n].pop_change_set().as_ref().map(changes).unwrap_or(J::Null),"clear"=>states[n].clear_current_change_set(),"commit"=>states[n].clear_committed(),
   "new"=>states[n].mark_as_new(key.clone()),"delete"=>states[n].mark_as_delete(key.clone()),"version"=>states[n].set_original_version(key.clone(),step["value"].as_str().unwrap().parse().unwrap()),"comment"=>states[n].set_comment(step["value"].as_str().unwrap()),
   "adopt"=>states[n].adopt_mutations_from(&states[other]),"compose"=>result=match pinned::compose(&states[n],&states[other]){Ok(())=>json!({"ok":true}),Err(e)=>json!({"error":e.to_string()})},_=>panic!("step")
  }
  out.push(json!({"result":result,"states":states.iter().map(|s|snapshot(s,&keys)).collect::<Vec<_>>(),"equal":states.iter().flat_map(|a|states.iter().map(move|b|a==b)).collect::<Vec<_>>()}));
 }
 json!(out)
}
fn graph(j:&J)->J{
 let root=EntityRuntimeState::default();let mut builder=EntityGraphBuilder::default();
 for entry in items(j,"entries"){
  let id=entry["id"].as_u64().unwrap();let value=entry["value"].as_str().unwrap();if entry["type"]=="text"{builder.install(id,value.to_owned());}else{builder.install(id,value.parse::<u64>().unwrap());}
 }
 for entry in items(j,"relations"){
  let owner=entry["owner"].as_str().unwrap();let id=entry["id"].as_u64().unwrap();let name=entry["name"].as_str().unwrap();
  match entry["type"].as_str().unwrap(){
   "list"=>builder.install_relation_list(owner,id,name,SmartList::from(items(entry,"values").iter().map(|v|v.as_u64().unwrap()).collect::<Vec<_>>())),
   "option"=>builder.install_relation_option(owner,id,name,entry["value"].as_u64()),
   "text"=>builder.install_relation_option(owner,id,name,entry["value"].as_str().map(str::to_owned)),_=>panic!("relation")
  }
 }
 let counts=json!([builder.entity_count(),builder.relation_list_count()]);let view=if j["weak"]==true{pinned::weak(&root)}else{root.clone()};let view=if j["clone"]==true{view.clone()}else{view};let freeze=view.freeze_graph(builder);let accepted=freeze.is_ok();let returned=freeze.err().map(|b|json!([b.entity_count(),b.relation_list_count()]));
 let entities=items(j,"lookups").iter().map(|id|json!([root.resolve_entity::<u64>(id.as_u64().unwrap()).map(|v|v.to_string()),root.resolve_entity::<String>(id.as_u64().unwrap())])).collect::<Vec<_>>();
 let relations=items(j,"queries").iter().map(|q|{
  let owner=q["owner"].as_str().unwrap();let id=q["id"].as_u64().unwrap();let name=q["name"].as_str().unwrap();let raw=if q["type"]=="list"{root.resolve_relation_list::<u64>(owner,id,name).map(|v|json!(v.as_slice()))}else{root.resolve_relation_option::<u64>(owner,id,name).map(|v|json!(v))};
  let handle=catch_unwind(AssertUnwindSafe(||if q["type"]=="list"{let h=root.relation_list::<u64>(owner,id,name);json!({"state":format!("{:?}",h.state()),"loaded":h.is_loaded(),"empty":h.is_empty(),"value":h.value().map(|v|json!(v.as_slice()))})}else{let h=root.relation_option::<u64>(owner,id,name);json!({"state":format!("{:?}",h.state()),"loaded":h.is_loaded(),"empty":h.is_empty(),"value":h.value()})})).unwrap_or(json!({"panic":true}));
  json!({"present":root.has_relation_view(owner,id,name),"raw":raw,"handle":handle})
 }).collect::<Vec<_>>();
 let mut second=EntityGraphBuilder::default();second.install(99,42_u64);let second=root.freeze_graph(second);json!({"counts":counts,"accepted":accepted,"returned":returned,"entities":entities,"relations":relations,"second":second.err().map(|b|b.entity_count())})
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){let j:J=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",if j["mode"]=="graph"{graph(&j)}else{ledger(&j)});}}
