//! Public locked registry APIs, exercised with handwritten models.
use std::{io::{self,BufRead},collections::BTreeMap};
use serde_json::{Value as J,json};
use teaql_core::{Entity,TeaqlEntity,IdentifiableEntity,EntityDescriptor,EntityError,CompactRow,Value,MutationValues};
use teaql_runtime::{InMemoryEntityGraphDecoderRegistry,EntityRuntimeState,EntityGraphBuilder,RuntimeModule,UserContext,EntityDataServiceBehavior};
#[path="../../../rust_values/src/fixture_value.rs"]mod fixture_value;
#[derive(Clone)]struct Item<const K:usize>{row:CompactRow,state:Option<EntityRuntimeState>}
impl<const K:usize> TeaqlEntity for Item<K>{const ENTITY_NAME:&'static str="Item";fn entity_descriptor()->EntityDescriptor{EntityDescriptor::new(if K==2{"Alias"}else{"Item"}).table_name(if K==1{"alternate_items"}else{"items"})}}
impl<const K:usize> Entity for Item<K>{
 fn from_compact_row(row:CompactRow)->Result<Self,EntityError>{if row.get("fail").and_then(Value::try_bool).unwrap_or(false){return Err(EntityError::new("Item","handwritten model decode failed"))}Ok(Self{row,state:None})}
 fn into_values(self)->MutationValues{self.row.into_map().into()}
 fn on_loaded(&mut self,context:&dyn std::any::Any){if let Some(root)=context.downcast_ref::<EntityRuntimeState>(){let mut state=root.clone();state.set_original_compact_row("Item",self.row.clone());self.state=Some(state);}}
}
impl<const K:usize> IdentifiableEntity for Item<K>{fn id_value(&self)->Value{self.row.get("id").cloned().unwrap_or(Value::Null)}}
struct NamedBehavior(String);
impl EntityDataServiceBehavior for NamedBehavior{fn relation_loads(&self,_:&UserContext)->Vec<String>{vec![self.0.clone()]}}
fn items<'a>(j:&'a J,key:&str)->&'a [J]{j[key].as_array().map(Vec::as_slice).unwrap_or(&[])}
fn row(j:&J)->CompactRow{CompactRow::from_map(j.as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect::<BTreeMap<_,_>>())}
fn describe<const K:usize>(v:Option<&Item<K>>,states:&mut Vec<EntityRuntimeState>)->J{
 let Some(v)=v else{return J::Null};let mut out=json!({"id":format!("{:?}",v.id_value()),"label":v.row.get("label").map(|v|format!("{v:?}")),"kind":K,"context":v.state.is_some()});
 if let Some(s)=&v.state{states.push(s.clone());out["sees_first"]=json!(s.resolve_entity::<Item<0>>(1).is_some());}out
}
fn inspect<const K:usize>(root:&EntityRuntimeState,name:&str,states:&mut Vec<EntityRuntimeState>)->J{
 let list=root.resolve_relation_list::<Item<K>>("purchase_order",9,name).map(|l|json!({"loaded":l.is_loaded,"data":l.as_slice().iter().map(|v|describe(Some(v),states)).collect::<Vec<_>>()}));
 let option=root.resolve_relation_option::<Item<K>>("purchase_order",9,name);let value=describe(option.and_then(|v|v.as_ref()),states);json!({"list":list,"option_present":option.is_some(),"option":value})
}
fn decode(input:&J)->J{
 let root=EntityRuntimeState::default();let mut registry=InMemoryEntityGraphDecoderRegistry::default();let mut graph=EntityGraphBuilder::default();let mut steps=vec![];
 for step in items(input,"steps"){
  let error=match step["op"].as_str().unwrap(){
   "register"=>{if step["kind"]==1{registry.register::<Item<1>>()}else{registry.register::<Item<0>>()}None},
   "copy"=>{let copy=registry.clone();registry.register::<Item<1>>();copy.decode_compact_batch("Item",items(step,"rows").iter().map(row).collect(),&root,&mut graph).err()},
   op=>{let entity=step["entity"].as_str().unwrap_or("Item");match op{
    "compact"=>registry.decode_compact(entity,row(&step["row"]),&root,&mut graph),
    "batch"=>registry.decode_compact_batch(entity,items(step,"rows").iter().map(row).collect(),&root,&mut graph),
    "list"=>registry.decode_compact_list(entity,items(step,"rows").iter().map(row).collect(),&root,&mut graph,"PurchaseOrder",9,step["relation"].as_str().unwrap()),
    "option"=>registry.decode_compact_option(entity,items(step,"rows").iter().map(row).collect(),&root,&mut graph,"PurchaseOrder",9,step["relation"].as_str().unwrap()),_=>panic!("fixture op")}.err()}
  };
  steps.push(json!({"error":error.map(|e|e.to_string()),"entities":graph.entity_count(),"relations":graph.relation_list_count(),"contains":registry.contains("Item")}));
 }
 assert!(root.freeze_graph(graph).is_ok());let mut states=vec![];let mut entities=vec![];for id in [1,2,3]{entities.push(describe(root.resolve_entity::<Item<0>>(id),&mut states));entities.push(describe(root.resolve_entity::<Item<1>>(id),&mut states));}
 let relations=["rows","optional","empty"].into_iter().map(|name|json!({"present":root.has_relation_view("purchase_order",9,name),"types":[inspect::<0>(&root,name,&mut states),inspect::<1>(&root,name,&mut states)]})).collect::<Vec<_>>();
 json!({"steps":steps,"entities":entities,"relations":relations,"shared_ledgers":states.iter().flat_map(|a|states.iter().map(move|b|a==b)).collect::<Vec<_>>(),"root_ledger":states.iter().map(|a|a==&root).collect::<Vec<_>>()})
}
fn module(input:&J)->J{
 let mut m=RuntimeModule::new();let mut c=UserContext::new();let mut out=vec![];
 for step in items(input,"steps"){
  let name=step["name"].as_str().unwrap_or("");let value=step["value"].as_str().unwrap_or("").to_owned();m=match step["op"].as_str().unwrap(){
   "entity"=>match step["kind"].as_u64().unwrap_or(0){2=>m.entity::<Item<2>>(),1=>m.entity::<Item<1>>(),_=>m.entity::<Item<0>>()},
   "descriptor"=>m.descriptor(EntityDescriptor::new(name)),"behavior"=>m.behavior(name,NamedBehavior(value)),
   "entity_behavior"=>if step["kind"]==2{m.entity_with_behavior::<Item<2>,_>(NamedBehavior(value))}else{m.entity_with_behavior::<Item<0>,_>(NamedBehavior(value))},_=>panic!("module fixture op")};
  m.clone().apply_to(&mut c);out.push(["Item","Alias","OnlyBehavior"].iter().map(|name|{let e=c.entity(name);json!({"entity":e.map(|e|&e.name),"table":e.map(|e|&e.table_name),"has":c.has_entity_data_service(name),"behavior":c.entity_data_service_behavior(name).map(|b|b.relation_loads(&c))})}).collect::<Vec<_>>());
 }json!(out)
}
fn module_snapshot(context: &UserContext) -> J {
    json!(["Item", "Alias", "OnlyBehavior"].iter().map(|name| {
        let descriptor = context.entity(name);
        json!({
            "entity": descriptor.map(|value| &value.name),
            "table": descriptor.map(|value| &value.table_name),
            "has": context.has_entity_data_service(name),
            "behavior": context.entity_data_service_behavior(name)
                .map(|value| value.relation_loads(context)),
        })
    }).collect::<Vec<_>>())
}

fn macro_module(input: &J) -> J {
    let first = input["first"].as_str().unwrap();
    let last = input["last"].as_str().unwrap();
    let alias = input["alias"].as_str().unwrap();
    // Invoke the unchanged original macro, including recursive and terminal
    // branches, rather than reconstructing its builder algorithm in the oracle.
    let mut module = match input["case"].as_u64().unwrap() {
        0 => teaql_runtime::module!(Item<0>, Item<2>,),
        1 => teaql_runtime::module!(Item<2>, Item<0>),
        2 => teaql_runtime::module!(Item<0>, Item<1>, Item<2>),
        3 => teaql_runtime::module!(Item<2>, Item<1>, Item<0>,),
        4 => teaql_runtime::module!(
            Item<0> => NamedBehavior(first.into()),
            Item<2> => NamedBehavior(alias.into()),
            Item<1> => NamedBehavior(last.into()),
        ),
        5 => teaql_runtime::module!(
            Item<2> => NamedBehavior(alias.into()),
            Item<0> => NamedBehavior(first.into()),
            Item<0> => NamedBehavior(last.into()),
        ),
        6 => teaql_runtime::module!(
            Item<0> => NamedBehavior(first.into()), Item<1>, Item<2>,
        ),
        7 => teaql_runtime::module!(
            Item<0>, Item<2> => NamedBehavior(alias.into()),
            Item<0> => NamedBehavior(last.into()),
        ),
        _ => panic!("macro fixture case"),
    };
    let cloned = module.clone();
    let mut original_context = UserContext::new();
    module.clone().apply_to(&mut original_context);
    let before = module_snapshot(&original_context);
    let cloned_context = cloned.into_context();
    let cloned_snapshot = module_snapshot(&cloned_context);
    let built = module.clone().into_context();
    let built_snapshot = module_snapshot(&built);
    module = module.entity::<Item<1>>().behavior("Item", NamedBehavior(last.into()));
    let mut updated = UserContext::new();
    module.apply_to(&mut updated);
    json!({
        "before": before,
        "cloned": cloned_snapshot,
        "built": built_snapshot,
        "updated": module_snapshot(&updated),
        "original_after": module_snapshot(&original_context),
        "clone_after": module_snapshot(&cloned_context),
    })
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: J = serde_json::from_str(&line.unwrap()).unwrap();
        let output = match input["mode"].as_str().unwrap() {
            "module" => module(&input),
            "macro_module" => macro_module(&input),
            _ => decode(&input),
        };
        println!("{output}");
    }
}
