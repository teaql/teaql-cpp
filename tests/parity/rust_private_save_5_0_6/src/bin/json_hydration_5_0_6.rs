//! Complete original Context typed JSON scalar and graph hydration entry points.
use serde_json::{Value as J,json};
use std::{collections::BTreeMap,io::{self,BufRead},sync::Arc};
use teaql_core::*;
use teaql_runtime::{EntityRuntimeState,EntityGraphBuilder,InMemoryEntityGraphDecoderRegistry};
#[path="../../../rust_values/src/fixture_value.rs"] mod fixture_value;
#[derive(Clone)]struct Model<const K:usize>{row:CompactRow,state:Option<EntityRuntimeState>}
fn descriptor()->EntityDescriptor {
    let mut d=EntityDescriptor::new("JsonNode");
    for (name,kind) in [("id",DataType::U64),("label",DataType::Text),("version",DataType::I64),
        ("count",DataType::I64),("active",DataType::Bool),("ratio",DataType::F64),
        ("price",DataType::Decimal),("birthday",DataType::Date),("happened_at",DataType::Timestamp),
        ("raw",DataType::Json),("parent_id",DataType::U64),("nonnull",DataType::Text),("fail",DataType::Bool)] {
        let mut p=PropertyDescriptor::new(name,kind);p.is_id=name=="id";p.is_version=name=="version";
        if name=="label"{p.column_name="stored_label".into();}if name=="nonnull"{p.nullable=false;}d.properties.push(p);
    }
    for (name,many) in [("parent",false),("alternate",false),("children",true)] {
        let mut r=RelationDescriptor::new(name,"JsonNode");r.local_key=if many{"id"}else{"parent_id"}.into();
        r.foreign_key=if many{"parent_id"}else{"id"}.into();r.many=many;d.relations.push(r);
    }d
}
impl<const K:usize> TeaqlEntity for Model<K>{const ENTITY_NAME:&'static str="JsonNode";fn entity_descriptor()->EntityDescriptor{descriptor()}
    fn field_layout()->Result<Option<Arc<FieldLayout>>,EntityError>{
        if K==0{return Ok(None)}if K==3{return Err(EntityError::new("JsonNode","layout fixture failed"))}
        let names=["id","label","version","count","active","ratio","price","birthday","happened_at","raw","parent_id","nonnull","fail"];
        let indexes:Vec<_>=names.iter().enumerate().map(|(i,n)|(*n,i)).collect();
        let mappings:Vec<_>=names.iter().map(|n|(*n,*n,if *n=="label"{"stored_label"}else{*n})).collect();
        Ok(Some(FieldLayout::from_generated("JsonNode","json_hydration_fixture",&indexes,&mappings,&["parent","alternate","children"],&names).unwrap()))
    }}
impl<const K:usize> Entity for Model<K>{
    fn supports_dynamic_property_load()->bool{K>=2}
    fn from_compact_row(row:CompactRow)->Result<Self,EntityError>{if row.get("fail").and_then(Value::try_bool).unwrap_or(false){Err(EntityError::new("JsonNode","model decode failed"))}else{Ok(Self{row,state:None})}}
    fn into_values(self)->MutationValues{self.row.into_map().into()}
    fn on_loaded(&mut self,ctx:&dyn std::any::Any){if let Some(root)=ctx.downcast_ref::<EntityRuntimeState>(){let mut state=root.clone();state.set_original_compact_row("JsonNode",self.row.clone());self.state=Some(state);}}
    fn is_field_loaded(&self,name:&str)->bool{self.row.contains_key(name)}
}
impl<const K:usize> IdentifiableEntity for Model<K>{fn id_value(&self)->Value{self.row.get("id").cloned().unwrap_or(Value::Null)}}
fn projection<const K:usize>(model:&Model<K>,depth:usize)->J {
    let mut edges=serde_json::Map::new();
    if let Some(state)=&model.state {if depth<6 {if let Some(id)=model.id_value().try_u64() {
        for name in ["parent","alternate"] {
            if let Some(edge)=state.resolve_relation_option::<Model<K>>("JsonNode",id,name) {
                edges.insert(name.into(),edge.as_ref().map(|v|projection(v,depth+1)).unwrap_or(J::Null));
            }
        }
        if let Some(list)=state.resolve_relation_list::<Model<K>>("JsonNode",id,"children") {
            edges.insert("children".into(),json!(list.data.iter().map(|m|projection(m,depth+1)).collect::<Vec<_>>()));
        }
    }}}
    json!({"fields":model.row.clone().into_map().iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>(),
        "view":model.state.as_ref().map(|s|s.__cpp_json_view_id()),
        "loaded":{"parent":model.row.is_loaded_relation("parent"),"alternate":model.row.is_loaded_relation("alternate"),"children":model.row.is_loaded_relation("children")},"edges":edges})
}
fn execute<const K:usize>(input:&J)->J {
    let mut d=descriptor();let mode=input["metadata"].as_u64().unwrap();
    if mode==2{d.properties[0].nullable=false;}if mode==3{d.table_name="other_table".into();}
    let mut metadata=teaql_runtime::InMemoryMetadataStore::new();if mode!=1{metadata=metadata.with_entity(d);}
    let mut context=if input["registered"].as_bool().unwrap() {
        teaql_runtime::RuntimeModule::new().entity::<Model<K>>().into_context()
    } else {teaql_runtime::UserContext::new()};
    context=context.with_metadata(metadata);
    if input["array"].as_bool().unwrap() {
        match context.decode_json_entities::<Model<K>>(&input["value"]) {
            Err(e)=>json!({"error":e.to_string()}),
            Ok(rows)=>json!({"rows":rows.iter().map(|m|projection(m,0)).collect::<Vec<_>>(),
                "sharing":rows.iter().map(|a|rows.iter().map(|b|Arc::ptr_eq(&a.row.shared_layout(),&b.row.shared_layout())).collect::<Vec<_>>()).collect::<Vec<_>>()})
        }
    } else {
        match context.decode_json_entity::<Model<K>>(&input["value"]) {
            Err(e)=>json!({"error":e.to_string()}),Ok(row)=>json!({"row":projection(&row,0)})
        }
    }
}
fn main(){for line in io::stdin().lock().lines(){let input:J=serde_json::from_str(&line.unwrap()).unwrap();let out=match input["kind"].as_u64().unwrap(){0=>execute::<0>(&input),1=>execute::<1>(&input),2=>execute::<2>(&input),_=>execute::<3>(&input)};println!("{}",out);}}
