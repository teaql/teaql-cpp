//! Complete original typed JSON registry capability and edge decoder callbacks.
use serde_json::{Value as J,json};
use std::{collections::BTreeMap,io::{self,BufRead},sync::Arc};
use teaql_core::*;
use teaql_runtime::{EntityRuntimeState,EntityGraphBuilder,InMemoryEntityGraphDecoderRegistry};
#[path="../../../rust_values/src/fixture_value.rs"] mod fixture_value;
#[derive(Clone)]struct Model<const K:usize>{row:CompactRow,state:Option<EntityRuntimeState>}
fn descriptor()->EntityDescriptor {
    let mut d=EntityDescriptor::new("Child");
    let mut id=PropertyDescriptor::new("id",DataType::U64);id.is_id=true;d.properties.push(id);
    d.properties.push(PropertyDescriptor::new("label",DataType::Text));
    let mut version=PropertyDescriptor::new("version",DataType::I64);version.is_version=true;d.properties.push(version);
    for (name,many) in [("detail",false),("tags",true)]{let mut r=RelationDescriptor::new(name,"Leaf");r.local_key="id".into();r.foreign_key="owner".into();r.many=many;d.relations.push(r);}d
}
impl<const K:usize> TeaqlEntity for Model<K>{const ENTITY_NAME:&'static str="Child";fn entity_descriptor()->EntityDescriptor{descriptor()}
    fn field_layout()->Result<Option<Arc<FieldLayout>>,EntityError>{
        if K==0{return Ok(None)}if K==3{return Err(EntityError::new("Child","layout fixture failed"))}
        Ok(Some(FieldLayout::from_generated("Child","json_decoder_fixture",&[("id",0),("label",1),("version",2)],&[("id","id","id"),("label","label","label"),("version","version","version")],&["detail","tags"],&["id","label","version"]).unwrap()))
    }}
impl<const K:usize> Entity for Model<K>{
    fn supports_dynamic_property_load()->bool{K>=2}
    fn from_compact_row(row:CompactRow)->Result<Self,EntityError>{if row.get("fail").and_then(Value::try_bool).unwrap_or(false){Err(EntityError::new("Child","model decode failed"))}else{Ok(Self{row,state:None})}}
    fn into_values(self)->MutationValues{self.row.into_map().into()}
    fn on_loaded(&mut self,ctx:&dyn std::any::Any){if let Some(root)=ctx.downcast_ref::<EntityRuntimeState>(){let mut state=root.clone();state.set_original_compact_row("Child",self.row.clone());self.state=Some(state);}}
    fn is_field_loaded(&self,name:&str)->bool{self.row.contains_key(name)}
}
impl<const K:usize> IdentifiableEntity for Model<K>{fn id_value(&self)->Value{self.row.get("id").cloned().unwrap_or(Value::Null)}}
fn modify(mut d:EntityDescriptor,input:&J)->EntityDescriptor {
    match input["metadata"].as_u64().unwrap(){
        1=>d.properties.reverse(),2=>d.table_name="different_table".into(),3=>d.name="Missing".into(),
        4=>{d.properties.pop();},5=>d.properties.push(PropertyDescriptor::new("extra",DataType::Bool)),
        6=>d.properties[0].nullable=false,7=>d.properties[0].column_name="other_id".into(),
        8=>d.relations[0].target_entity="Other".into(),9=>{d.relations.pop();},10=>d.relations.reverse(),
        11=>{d.audit_mask_fields_declared=true;d.audit_mask_fields=vec!["label".into()];},_=>{},
    };d
}
fn fields<const K:usize>(model:&Model<K>)->J{json!({"fields":model.row.clone().into_map().iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>(),"view":model.state.as_ref().map(|s|s.__cpp_json_view_id())})}
fn execute<const K:usize>(input:&J)->J {
    let mut registry=InMemoryEntityGraphDecoderRegistry::default();if input["registered"].as_bool().unwrap(){registry.register::<Model<K>>();}
    let copied=registry.clone();registry.register::<Model<3>>();
    let registry=if input["copy"].as_bool().unwrap(){copied}else{let mut r=InMemoryEntityGraphDecoderRegistry::default();if input["registered"].as_bool().unwrap(){r.register::<Model<K>>();}r};
    let capability=registry.__cpp_json_capability(&modify(descriptor(),input));
    let root=EntityRuntimeState::default().__cpp_json_view(input["parent_view"].as_u64().unwrap());
    let weak=EntityRuntimeState::__cpp_weak_json_view(&root,55);
    let weak_rejected=weak.freeze_graph(EntityGraphBuilder::default()).is_err();
    let mut graph=if input["json"].as_bool().unwrap(){EntityGraphBuilder::__cpp_for_json_read()}else{EntityGraphBuilder::default()};
    let rows=input["rows"].as_array().unwrap().iter().map(|r|(r["view"].as_u64().unwrap(),CompactRow::from_map(r["fields"].as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect()))).collect();
    let result=registry.__cpp_decode_json_edge(input["entity"].as_str().unwrap(),rows,&root,&mut graph,"Owner",1,"items",input["many"].as_bool().unwrap());
    let counts=(graph.entity_count(),graph.relation_list_count());root.freeze_graph(graph).unwrap();
    let list=root.resolve_relation_list::<Model<K>>("Owner",1,"items").map(|l|json!({"rows":l.data.iter().map(fields).collect::<Vec<_>>(),"loaded":l.is_loaded}));
    let option=root.resolve_relation_option::<Model<K>>("Owner",1,"items");
    json!({"capability":capability.as_ref().ok(),"capability_error":capability.as_ref().err().map(|e|e.to_string()),"decode_error":result.err().map(|e|e.to_string()),"counts":counts,
        "list":list,"option":option.and_then(|v|v.as_ref()).map(fields),"option_present":option.is_some(),"identity":root.resolve_entity::<Model<K>>(10).map(fields),"weak_rejected":weak_rejected})
}
fn evaluate(input:&J)->J{match input["kind"].as_u64().unwrap(){0=>execute::<0>(input),1=>execute::<1>(input),2=>execute::<2>(input),_=>execute::<3>(input)}}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(&serde_json::from_str(&line.unwrap()).unwrap()));}}
