//! Complete original JSON graph view coordinates and option identity index.
use serde_json::{Value as J,json};
use std::{collections::BTreeMap,io::{self,BufRead}};
use teaql_core::*;
use teaql_runtime::{EntityGraphBuilder,EntityRuntimeState};
#[derive(Clone)]struct Model<const K:usize>{row:CompactRow,state:Option<EntityRuntimeState>}
impl<const K:usize> TeaqlEntity for Model<K>{const ENTITY_NAME:&'static str=if K==0{"Parent"}else if K==1{"Child"}else{"Leaf"};fn entity_descriptor()->EntityDescriptor{EntityDescriptor::new(Self::ENTITY_NAME)}}
impl<const K:usize> Entity for Model<K>{fn from_compact_row(row:CompactRow)->Result<Self,EntityError>{if row.get("fail").and_then(Value::try_bool).unwrap_or(false){Err(EntityError::new(Self::ENTITY_NAME,"model decode failed"))}else{Ok(Self{row,state:None})}}fn is_field_loaded(&self,field:&str)->bool{field!="id" || self.row.get("loaded").and_then(Value::try_bool).unwrap_or(true)}fn into_values(self)->MutationValues{self.row.into_map().into()}fn on_loaded(&mut self,context:&dyn std::any::Any){if let Some(root)=context.downcast_ref::<EntityRuntimeState>(){let mut state=root.clone();state.set_original_compact_row(Self::ENTITY_NAME,self.row.clone());self.state=Some(state);}}}
impl<const K:usize> IdentifiableEntity for Model<K>{fn id_value(&self)->Value{self.row.get("id").cloned().unwrap_or(Value::Null)}}



fn model(input:&J)->Model<1>{Model{row:CompactRow::from_map(BTreeMap::from([
    ("id".into(),if input["bad_id"].as_bool().unwrap_or(false){Value::Text("invalid identity".into())}else{Value::U64(input["id"].as_u64().unwrap())}),
    ("label".into(),Value::Text(input["label"].as_str().unwrap().into())),
    ("loaded".into(),Value::Bool(input["loaded"].as_bool().unwrap()))])),state:None}}
fn fields(value:&Model<1>)->J{json!({"id":format!("{:?}",value.id_value()),"label":value.row.get("label").unwrap().try_text(),"loaded":value.is_field_loaded("id")})}
fn evaluate(input:&J)->J {
    let mut graph=if input["json"].as_bool().unwrap(){EntityGraphBuilder::__cpp_for_json_read()}else{EntityGraphBuilder::default()};
    for action in input["actions"].as_array().unwrap(){
        let owner=action["owner"].as_str().unwrap_or("Owner");let id=action["owner_id"].as_u64().unwrap_or(1);let name=action["relation"].as_str().unwrap_or("items");let view=action["view"].as_u64().unwrap_or(0);
        match action["op"].as_str().unwrap(){
            "option"=>graph.__cpp_json_option(view,owner,id,name,if action["value"].is_null(){None}else{Some(model(&action["value"]))}),
            "plain_option"=>graph.install_relation_option(owner,id,name,if action["value"].is_null(){None}else{Some(model(&action["value"]))}),
            "list"|"plain_list"=>{let mut list=SmartList::new(action["values"].as_array().unwrap().iter().map(model).collect());
                if action["facets"].as_bool().unwrap_or(false){list.facets.insert("choices".into(),SmartList::new(Vec::<CompactRow>::new()));}
                if action["op"]=="list"{graph.__cpp_json_list(view,owner,id,name,list)}else{graph.install_relation_list(owner,id,name,list)}},
            "table"=>graph.install(action["value"]["id"].as_u64().unwrap(),model(&action["value"])),
            _=>panic!("unknown graph action"),
        }
    }
    let counts=json!({"entities":graph.entity_count(),"relations":graph.relation_list_count()});
    let root=EntityRuntimeState::default();root.freeze_graph(graph).unwrap();
    let mut lookups=Vec::new();
    for lookup in input["lookups"].as_array().unwrap(){
        let scoped=root.clone().__cpp_json_view(lookup["view"].as_u64().unwrap());
        let owner=lookup["owner"].as_str().unwrap();let id=lookup["owner_id"].as_u64().unwrap();let name=lookup["relation"].as_str().unwrap();
        let list=scoped.resolve_relation_list::<Model<1>>(owner,id,name).map(|v|json!({"data":v.data.iter().map(fields).collect::<Vec<_>>(),"facet_count":v.facets.len()}));
        let option=scoped.resolve_relation_option::<Model<1>>(owner,id,name);
        lookups.push(json!({"view":scoped.__cpp_json_view_id(),"present":scoped.has_relation_view(owner,id,name),"option_present":option.is_some(),"option":option.and_then(|o|o.as_ref()).map(fields),"list":list,
            "entity":scoped.resolve_entity::<Model<1>>(lookup["id"].as_u64().unwrap()).map(fields),"facet":scoped.relation_facet(owner,id,name,"choices").is_some(),"debug":format!("{scoped:?}")}));
    }
    let source=root.clone().__cpp_json_view(11);let target=if input["shared_ledger"].as_bool().unwrap(){root.clone().__cpp_json_view(22)}else{EntityRuntimeState::default().__cpp_json_view(22)};
    if !input["shared_ledger"].as_bool().unwrap(){target.set_comment("target ledger");}source.set_comment("source ledger");
    let fresh=EntityRuntimeState::fresh_with_shared_graph(&source);let copied=source.clone();
    let shared=target.with_shared_graph(&source);let loaded=target.clone().with_loaded_view_from(&source);
    json!({"counts":counts,"lookups":lookups,"state_views":[fresh.__cpp_json_view_id(),copied.__cpp_json_view_id(),shared.__cpp_json_view_id(),loaded.__cpp_json_view_id()],
        "fresh_comment":fresh.get_comment(),"shared_comment":shared.get_comment(),"loaded_comment":loaded.get_comment(),"same_ledger":source==target})
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(&serde_json::from_str(&line.unwrap()).unwrap()));}}
