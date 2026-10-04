use std::io::{self,BufRead};
use std::collections::BTreeMap;
use std::sync::{Arc,Mutex};
use serde_json::{Value as J,json};
use teaql_runtime::{InMemoryMetadataStore,InMemoryEntityRegistry,MetadataStore,EntityRegistry,UserContext};
use teaql_core::{EntityDescriptor,PropertyDescriptor,DataType,EntityDescriptorStore};
use teaql_data_service::{SchemaProvider,ExecutionMetadata};
fn descriptor(j:&J)->EntityDescriptor{let mut e=EntityDescriptor::new(j["name"].as_str().unwrap()).table_name(j["table"].as_str().unwrap());for name in j["fields"].as_array().unwrap(){e=e.property(PropertyDescriptor::new(name.as_str().unwrap(),DataType::Text));}e}
fn describe(e:Option<&EntityDescriptor>)->J{e.map(|e|json!({"name":e.name,"table":e.table_name,"fields":e.properties.iter().map(|p|&p.name).collect::<Vec<_>>()})).unwrap_or(J::Null)}
struct Spy{capture:bool,debug:bool,seen:Mutex<Vec<usize>>}
impl MetadataStore for Spy{fn entity(&self,_:&str)->Option<&EntityDescriptor>{None}fn all_entities(&self)->Vec<&EntityDescriptor>{vec![]}fn capture_execution_metadata(&self)->bool{self.capture}fn capture_query_debug(&self)->bool{self.debug}fn record_metadata_log(&self,m:&ExecutionMetadata){self.seen.lock().unwrap().push(m.result_count.unwrap_or(0));}}
fn evaluate(input:J)->J{
    if input.get("capture").is_some(){let store=Spy{capture:input["capture"].as_bool().unwrap(),debug:input["debug"].as_bool().unwrap(),seen:Mutex::new(vec![])};let q=store.query_diagnostic_observer();let m=store.mutation_diagnostic_observer();if let Some(ref cb)=q{cb(ExecutionMetadata::unrecorded_query(11));}if let Some(ref cb)=m{cb(ExecutionMetadata::unrecorded_query(17));}return json!({"query":q.is_some(),"mutation":m.is_some(),"debug":store.capture_query_debug(),"seen":*store.seen.lock().unwrap()});}
    let mut metadata=InMemoryMetadataStore::new();let mut registry=InMemoryEntityRegistry::new();let mut context=UserContext::new();let mut snapshots:BTreeMap<String,Option<Arc<EntityDescriptor>>>=BTreeMap::new();let mut out=vec![];
    for action in input["actions"].as_array().unwrap(){let name=action["name"].as_str().unwrap_or("");out.push(match action["op"].as_str().unwrap(){
        "register"=>{metadata.register(descriptor(action));json!(true)},"register_descriptor"=>{metadata.register_descriptor(descriptor(action));json!(true)},
        "registry"=>{registry.register(name);json!(true)},"contains"=>json!(registry.contains(name)),"entity"=>describe(metadata.entity(name)),
        "all"=>json!(metadata.all_entities().iter().map(|e|describe(Some(e))).collect::<Vec<_>>()),
        "snapshot"=>{snapshots.insert(action["key"].as_str().unwrap().into(),metadata.get_entity(name));json!(true)},
        "snapshot_get"=>describe(snapshots.get(action["key"].as_str().unwrap()).and_then(|e|e.as_deref())),
        "install_metadata"=>{context.set_metadata(metadata.clone());json!(true)},"install_registry"=>{context.set_entity_registry(registry.clone());json!(true)},
        "context_entity"=>describe(context.entity(name)),"context_all"=>json!(context.all_entities().iter().map(|e|describe(Some(e))).collect::<Vec<_>>()),
        "has"=>json!(context.has_entity_data_service(name)),"require"=>match context.require_entity(name){Ok(e)=>json!({"value":describe(Some(e))}),Err(e)=>json!({"error":e.to_string()})},
        "defaults"=>json!({"capture":metadata.capture_execution_metadata(),"debug":metadata.capture_query_debug(),"query":metadata.query_diagnostic_observer().is_some(),"mutation":metadata.mutation_diagnostic_observer().is_some()}),
        _=>panic!("unknown op"),
    });}json!(out)
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
