//! Complete original reconstruction of loaded compact rows into a frozen graph.
use serde_json::{Value as J,json};
use std::{collections::BTreeMap,io::{self,BufRead}};
use teaql_core::*;
use teaql_data_service::*;
use teaql_runtime::{EntityRuntimeState,EntityDataService,RuntimeModule,InMemoryEntityGraphDecoderRegistry};
#[path="../../../rust_values/src/fixture_value.rs"] mod fixture_value;
#[derive(Clone)]struct Model<const K:usize>{row:CompactRow,state:Option<EntityRuntimeState>}
impl<const K:usize> TeaqlEntity for Model<K>{const ENTITY_NAME:&'static str=if K==0{"Parent"}else if K==1{"Child"}else{"Leaf"};fn entity_descriptor()->EntityDescriptor{EntityDescriptor::new(Self::ENTITY_NAME)}}
impl<const K:usize> Entity for Model<K>{fn from_compact_row(row:CompactRow)->Result<Self,EntityError>{if row.get("fail").and_then(Value::try_bool).unwrap_or(false){Err(EntityError::new(Self::ENTITY_NAME,"model decode failed"))}else{Ok(Self{row,state:None})}}fn into_values(self)->MutationValues{self.row.into_map().into()}fn on_loaded(&mut self,context:&dyn std::any::Any){if let Some(root)=context.downcast_ref::<EntityRuntimeState>(){let mut state=root.clone();state.set_original_compact_row(Self::ENTITY_NAME,self.row.clone());self.state=Some(state);}}}
impl<const K:usize> IdentifiableEntity for Model<K>{fn id_value(&self)->Value{self.row.get("id").cloned().unwrap_or(Value::Null)}}


struct Never;
impl DataServiceExecutor for Never {type Error=std::io::Error;fn capabilities(&self)->DataServiceCapabilities{panic!("Unexpected executor access")}}
impl QueryExecutor for Never {async fn query(&self,_:QueryRequest)->Result<QueryResult,Self::Error>{panic!("Reconstruction must not query")}}
impl MutationExecutor for Never {async fn mutate(&self,_:MutationRequest)->Result<MutationResult,Self::Error>{panic!("Reconstruction must not mutate")}}
fn row(input:&J)->CompactRow {
    let mut row=CompactRow::from_map(input["fields"].as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect());
    for (name,value) in input["loaded"].as_object().unwrap(){row.set_loaded_relation(name.clone(),list(value));}
    row
}
fn list(input:&J)->SmartList<CompactRow>{
    let mut result=SmartList::new(input["data"].as_array().unwrap().iter().map(row).collect());
    result.total_count=input["total_count"].as_u64();result.is_loaded=input["is_loaded"].as_bool().unwrap();
    result.summary=input["summary"].as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect();
    result.aggregations=input["aggregations"].as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect();
    result.facets=input["facets"].as_object().unwrap().iter().map(|(k,v)|(k.clone(),list(v))).collect();result
}
fn record(row:&CompactRow)->J{json!(row.clone().into_map().iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>())}
fn list_out<T>(value:&SmartList<T>,rows:Vec<J>)->J{json!({"data":rows,"total_count":value.total_count,"is_loaded":value.is_loaded,
    "summary":value.summary.iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>(),
    "aggregations":value.aggregations.iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>(),
    "facets":value.facets.iter().map(|(k,v)|(k.clone(),list_out(v,v.data.iter().map(record).collect()))).collect::<BTreeMap<_,_>>()})}
fn inspect<const K:usize>(root:&EntityRuntimeState,owner:&str,id:u64,relation:&str)->J{
    let list=root.resolve_relation_list::<Model<K>>(owner,id,relation).map(|v|list_out(v,v.data.iter().map(|m|record(&m.row)).collect()));
    let option=root.resolve_relation_option::<Model<K>>(owner,id,relation).map(|v|v.as_ref().map(|m|record(&m.row)));
    let facet=root.relation_facet(owner,id,relation,"choices").map(|v|list_out(v,v.data.iter().map(record).collect()));
    json!({"list":list,"option":option,"option_present":root.resolve_relation_option::<Model<K>>(owner,id,relation).is_some(),"facet":facet,"present":root.has_relation_view(owner,id,relation)})
}
fn relation(name:&str,target:&str,local:&str,foreign:&str,many:bool)->RelationDescriptor{let mut r=RelationDescriptor::new(name,target);r.local_key=local.into();r.foreign_key=foreign.into();r.many=many;r}
fn evaluate(input:&J)->J{
    let mut parent=EntityDescriptor::new("Parent");parent.relations.push(relation("children","Child",input["local"].as_str().unwrap(),input["foreign"].as_str().unwrap(),input["many"].as_bool().unwrap()));
    let mut child=EntityDescriptor::new("Child");child.relations.push(relation("leaves","Leaf","id","owner",true));
    let mut context=RuntimeModule::new().descriptor(parent).descriptor(child).descriptor(EntityDescriptor::new("Leaf")).into_context();
    let mut registry=InMemoryEntityGraphDecoderRegistry::default();
    if input["registered"].as_bool().unwrap(){registry.register::<Model<1>>();registry.register::<Model<2>>();}
    context.set_entity_graph_decoder_registry(registry);context.disable_sql_log();
    let executor=Never;let service=EntityDataService::for_executor(&context,"Parent",&executor);
    let mut rows=input["rows"].as_array().unwrap().iter().map(row).collect::<Vec<_>>();
    let result=service.__cpp_reconstruct_loaded_graph("Parent",&mut rows);
    let graph=result.as_ref().ok().map(|root|json!({"owners":[inspect::<1>(root,"Parent",1,"children"),inspect::<1>(root,"Parent",2,"children")],
        "nested":inspect::<2>(root,"Child",10,"leaves"),"table":root.resolve_entity::<Model<1>>(10).map(|m|record(&m.row)),
        "table_marker":root.resolve_entity::<Model<1>>(10).map(|m|m.row.is_loaded_relation("leaves"))}));
    json!({"error":result.as_ref().err().map(|e|e.to_string()),"rows":rows.iter().map(record).collect::<Vec<_>>(),
        "markers":rows.iter().map(|r|r.is_loaded_relation("children")).collect::<Vec<_>>(),"loaded_remaining":rows.iter().map(|r|r.loaded_relation("children").is_some()).collect::<Vec<_>>(),"graph":graph})
}
fn main(){for line in std::io::stdin().lock().lines(){println!("{}",evaluate(&serde_json::from_str(&line.unwrap()).unwrap()));}}
