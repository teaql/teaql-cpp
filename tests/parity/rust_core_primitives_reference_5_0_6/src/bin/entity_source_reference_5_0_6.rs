//! Actual core traits: audit intent, default metadata and boxed presentation delegation.
use serde_json::{Value as Json,json};
use std::{collections::BTreeMap,io::{self,BufRead},panic::{catch_unwind,AssertUnwindSafe}};
use teaql_core::{CompactRow,DataType,Entity,EntityDescriptor,EntityError,MutationValues,
    PropertyDescriptor,TeaqlBoxedRelations,TeaqlEntity,Value,eval::LoadState};
struct Model {payload:String,comment:String}
impl TeaqlEntity for Model {
    const ENTITY_NAME:&'static str="SourceModel";
    fn entity_descriptor()->EntityDescriptor {EntityDescriptor::new(Self::ENTITY_NAME)}
}
impl Entity for Model {
    fn from_compact_row(row:CompactRow)->Result<Self,EntityError> {
        row.get("payload").and_then(Value::try_text).map(|text|Self{payload:text.into(),comment:String::new()})
            .ok_or_else(||EntityError::new(Self::ENTITY_NAME,"missing payload"))
    }
    fn into_values(self)->MutationValues {BTreeMap::from([("payload".into(),Value::Text(self.payload)),("comment".into(),Value::Text(self.comment))]).into()}
    fn set_comment(&mut self,text:String) {self.comment=text;}
}
struct Plain(String);
struct Special(String);
macro_rules! common {
    () => {
        fn extend_descriptor(descriptor:&mut EntityDescriptor) {
            *descriptor=descriptor.clone().property(PropertyDescriptor::new("payload",DataType::Text));
        }
        fn extract_from_values(row:&CompactRow)->Result<Self,EntityError> {
            row.get("payload").and_then(Value::try_text).map(|text|Self(text.into()))
                .ok_or_else(||EntityError::new("Relation","missing payload"))
        }
        fn inject_into_values(self,values:&mut BTreeMap<String,Value>) {values.insert("mutation_payload".into(),Value::Text(self.0));}
    }
}
impl TeaqlBoxedRelations for Plain {common!();}
impl TeaqlBoxedRelations for Special {
    common!();
    fn inject_into_json_values(self,values:&mut BTreeMap<String,Value>,loaded:Option<&LoadState>) {
        values.insert("presentation_payload".into(),Value::Text(self.0));
        values.insert("has_state".into(),Value::Bool(loaded.is_some()));
        values.insert("payload_loaded".into(),Value::Bool(loaded.is_some_and(|s|s.is_loaded("payload"))));
    }
}
fn seed()->BTreeMap<String,Value> {BTreeMap::from([
    ("unchanged".into(),Value::from("keep")),("mutation_payload".into(),Value::from("old-mutation")),
    ("presentation_payload".into(),Value::from("old-presentation"))])}
fn boxed<T:TeaqlBoxedRelations>(input:&Json)->Json {
    let mut descriptor=EntityDescriptor::new("RelationOwner");T::extend_descriptor(&mut descriptor);
    let row=CompactRow::from_map(if input["valid_row"].as_bool().unwrap() {
        BTreeMap::from([("payload".into(),Value::Text(input["payload"].as_str().unwrap().into()))])
    } else {BTreeMap::new()});
    let relation=match T::extract_from_values(&row) {Ok(value)=>value,Err(error)=>return json!({"error":error.to_string()})};
    let mut mutation=seed();T::extract_from_values(&row).unwrap().inject_into_values(&mut mutation);
    let state=if input["state"].is_null() {None} else {Some(serde_json::from_value::<LoadState>(input["state"].clone()).unwrap())};
    let mut presentation=seed();relation.inject_into_json_values(&mut presentation,state.as_ref());
    json!({"available":true,"properties":descriptor.properties.iter().map(|p|&p.name).collect::<Vec<_>>(),
        "mutation":teaql_core::record_to_json_value(&mutation),"presentation":teaql_core::record_to_json_value(&presentation)})
}
fn observe(input:Json)->Json {
    match input["kind"].as_str().unwrap() {
        "metadata"=>json!({"available":true,"default_none":Model::field_layout().unwrap().is_none()}),
        "defaults"=>{
            let mut model=Model{payload:"private-default-payload".into(),comment:String::new()};
            let code=input["code"].as_str().unwrap();
            let update=model.update_dynamic_field(code,Value::Text("private-canary".into())).unwrap_err();
            let remove=model.delete_dynamic_field(code).unwrap_err();
            let layout=teaql_core::FieldLayout::from_generated(Model::ENTITY_NAME,"v1",&[("id",0),("version",1)],&[("id","id","id"),("version","version","version")],&[],&["id","version"]).unwrap();
            let snapshot=teaql_core::LoadedSnapshot::fully_loaded(layout).into_shared();
            let definitions=teaql_core::dynamic_fields::DynamicFieldDefinitions::new(Model::ENTITY_NAME,"v1",std::iter::empty()).unwrap();
            let values=teaql_core::dynamic_fields::DynamicFieldValues::from_values(definitions,std::collections::HashMap::new()).unwrap();
            let install=model.install_loaded_dynamic_fields(values,snapshot).unwrap_err();
            json!({"readonly_absent":model.dynamic_property(code).is_none(),"readonly_present":model.has_dynamic_property(code),
                "readonly_type_none":model.dynamic_property_type(code).is_none(),"readonly_supported":Model::supports_dynamic_property_load(),
                "dynamic_supported":Model::supports_dynamic_field_load(),"dynamic_none":model.dynamic_field_values().is_none(),
                "snapshot_none":model.loaded_state_snapshot().is_none(),"runtime_carrier_none":model.__teaql_runtime_state_any().is_none(),
                "update":{"code":update.code,"field":update.field},"delete":{"code":remove.code,"field":remove.field},
                "install":install.to_string(),"pending":model.has_pending_dynamic_mutations()})
        },
        "audit"=>{
            let model=Model{payload:input["payload"].as_str().unwrap().into(),comment:"initial".into()};
            match catch_unwind(AssertUnwindSafe(||{
                let mut audit=model.audit_as(input["comment"].as_str().unwrap());
                let comment=audit.get_comment().to_owned();let before=audit.entity().comment.clone();
                audit.entity_mut().payload.push_str("-updated");
                json!({"ok":true,"comment":comment,"before":before,"entity":audit.into_entity().into_json()})
            })) {
                Ok(result)=>result,Err(error)=>json!({"ok":false,"message":error.downcast_ref::<String>().cloned().or_else(||error.downcast_ref::<&str>().map(|s|s.to_string())).unwrap()})
            }
        },
        _=>match (input["override"].as_bool().unwrap(),input["depth"].as_u64().unwrap()) {
            (false,1)=>boxed::<Box<Plain>>(&input),(false,_)=>boxed::<Box<Box<Plain>>>(&input),
            (true,1)=>boxed::<Box<Special>>(&input),(true,_)=>boxed::<Box<Box<Special>>>(&input),
        }
    }
}
fn main() {
    std::panic::set_hook(Box::new(|_|{}));
    for line in io::stdin().lock().lines() {println!("{}",observe(serde_json::from_str(&line.unwrap()).unwrap()));}
}
