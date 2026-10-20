fn schema_rows(rows:&[CompactRow])->J {
    json!(rows.iter().map(|row|json!({
        "equals_plain":row==&CompactRow::from_map(row.clone().into_map()),
        "validation":row.validate_dynamic_properties().err(),
    })).collect::<Vec<_>>())
}

fn provider_rows(mock:&Mock)->Vec<CompactRow> {
    if mock.schema_probe && mock.empty {return vec![];}
    let mut values=std::collections::BTreeMap::from([("id".into(),Value::U64(1))]);
    if mock.schema_probe {
        values.insert("_result".into(),match mock.result_kind.as_str(){
            "u64"=>Value::U64(7),"text"=>Value::Text("result".into()),
            "typed_null"=>Value::TypedNull(DataType::U64),"null"=>Value::Null,
            _=>panic!("fixture result kind"),
        });
    }
    vec![CompactRow::from_map(values)]
}

fn schema_query(mut query:SelectQuery,input:&J)->SelectQuery {
    if input["initial_dynamic"]==true {
        query.dynamic_field_selection=Some(Arc::new(teaql_core::dynamic_fields::DynamicFieldSelection::All));
    }
    if input["schema_probe"]!=true || input["schema_kind"]=="none" {return query;}
    query.with_dynamic_property_definitions(teaql_core::dynamic_properties::DynamicPropertyDefinitions::new(
        [("_result".into(),if input["schema_kind"]=="text" {DataType::Text}else{DataType::U64})]).unwrap())
}

struct RecordingCache {
    inner:InMemoryAggregationCache,
    events:Mutex<Vec<J>>,
}
impl crate::AggregationCacheBackend for RecordingCache {
    fn namespace(&self)->&str {self.inner.namespace()}
    fn get(&self,key:&str,age:u64)->Option<Vec<CompactRow>> {
        let rows=self.inner.get(key,age);
        self.events.lock().unwrap().push(json!({"op":"get","age":age,"hit":rows.is_some(),
            "schema":rows.as_ref().map(|rows|schema_rows(rows))}));
        rows
    }
    fn put(&self,key:String,rows:Vec<CompactRow>) {
        self.events.lock().unwrap().push(json!({"op":"put","schema":schema_rows(&rows)}));
        self.inner.put(key,rows)
    }
    fn invalidate_namespace(&self,namespace:&str) {self.inner.invalidate_namespace(namespace)}
}

#[derive(Clone)]
struct SchemaItem<const DYNAMIC_GUARD:bool=false> { row:CompactRow }
impl<const DYNAMIC_GUARD:bool> teaql_core::TeaqlEntity for SchemaItem<DYNAMIC_GUARD> {
    const ENTITY_NAME:&'static str="Item";
    fn entity_descriptor()->EntityDescriptor {EntityDescriptor::new("Item").table_name("items")}
}
impl<const DYNAMIC_GUARD:bool> teaql_core::Entity for SchemaItem<DYNAMIC_GUARD> {
    fn supports_dynamic_field_load()->bool {DYNAMIC_GUARD}
    fn from_compact_row(row:CompactRow)->Result<Self,teaql_core::EntityError> {Ok(Self{row})}
    fn into_values(self)->teaql_core::MutationValues {self.row.into_map().into()}
}

fn select_dynamic_fields(state:&State,query:&mut SelectQuery,stage:&str,action:&str)->Result<(),crate::RuntimeError> {
    let before=query.dynamic_field_selection.is_some();
    match action {
        "add"=>query.dynamic_field_selection=Some(Arc::new(teaql_core::dynamic_fields::DynamicFieldSelection::All)),
        "remove"=>query.dynamic_field_selection=None,"keep"=>{},_=>panic!("selection fixture action"),
    }
    state.selection_calls.lock().unwrap().push(json!({"stage":stage,"before":before,"after":query.dynamic_field_selection.is_some()}));
    Ok(())
}
struct SelectionBehavior(Arc<State>,String);
impl crate::EntityDataServiceBehavior for SelectionBehavior {
    fn before_select(&self,_:&crate::UserContext,query:&mut SelectQuery)->Result<(),crate::RuntimeError> {
        select_dynamic_fields(&self.0,query,"behavior",&self.1)
    }
}
struct SelectionPolicy(Arc<State>,String,bool);
impl crate::RequestPolicy for SelectionPolicy {
    fn enforce_select(&self,_:&crate::UserContext,query:&mut SelectQuery)->Result<(),crate::RuntimeError> {
        if self.2 {query.comment=Some("rewritten query comment".into());query.purpose=Some("rewritten query purpose".into());}
        select_dynamic_fields(&self.0,query,"policy",&self.1)
    }
}

struct IndexedSchemaItem {
    row:CompactRow,
    state:Option<crate::EntityRuntimeState>,
    load_state:teaql_core::eval::LoadState,
}
impl teaql_core::TeaqlEntity for IndexedSchemaItem {
    const ENTITY_NAME:&'static str="Item";
    fn entity_descriptor()->EntityDescriptor {EntityDescriptor::new("Item").table_name("items")}
}
impl teaql_core::Entity for IndexedSchemaItem {
    fn from_compact_row(row:CompactRow)->Result<Self,teaql_core::EntityError> {
        let layout=teaql_core::FieldLayout::from_generated("Item","indexed-query-v1",
            &[("id",0),("version",1)],&[("id","id","id"),("version","version","version")],&[],&["id","version"]).unwrap();
        let load_state=row.indexed_load_state(layout);
        Ok(Self{row,state:None,load_state})
    }
    fn into_values(self)->teaql_core::MutationValues {self.row.into_map().into()}
    fn on_loaded(&mut self,context:&dyn std::any::Any) {
        self.state=context.downcast_ref::<crate::EntityRuntimeState>().cloned();
        if let Some(root)=self.state.as_mut() {root.set_original_compact_row("Item",self.row.clone());}
    }
    fn supports_dynamic_field_load()->bool {true}
    fn loaded_state_snapshot(&self)->Option<Arc<teaql_core::LoadedSnapshot>> {
        match &self.load_state {teaql_core::eval::LoadState::Indexed(snapshot)=>Some(snapshot.clone()),_=>None}
    }
    fn install_loaded_dynamic_fields(&mut self,fields:teaql_core::dynamic_fields::DynamicFieldValues,snapshot:Arc<teaql_core::LoadedSnapshot>)->Result<(),teaql_core::EntityError> {
        self.state.as_mut().unwrap().install_loaded_dynamic_fields(fields);
        self.load_state=teaql_core::eval::LoadState::Indexed(snapshot);Ok(())
    }
}
fn indexed_schema(items:Vec<IndexedSchemaItem>)->J {
    let hydration=items.iter().map(|item|json!({
        "has_root":item.state.is_some(),"id_loaded":item.load_state.is_loaded("id"),
        "result_loaded":item.load_state.is_loaded("_result"),"a_loaded":item.load_state.is_loaded("#a"),"b_loaded":item.load_state.is_loaded("#b"),
        "values":item.state.as_ref().and_then(|root|root.loaded_dynamic_fields()).map(|fields|teaql_core::record_to_json_value(&fields.values().iter().map(|(k,v)|(k.clone(),v.clone())).collect()))
    })).collect::<Vec<_>>();
    json!({"rows":schema_rows(&items.into_iter().map(|item|item.row).collect::<Vec<_>>()),"hydration":hydration})
}
struct IndexedProvider(Arc<State>,String);
#[async_trait::async_trait]
impl crate::dynamic_fields::DynamicFieldsProvider for IndexedProvider {
    async fn load_values(&self,_:&crate::UserContext,owner:&str,ids:&[u64],_:&teaql_core::dynamic_fields::DynamicFieldSelection,intent:&teaql_core::QueryIntent)->Result<crate::dynamic_fields::DynamicFieldBatch,teaql_core::dynamic_fields::DynamicFieldError> {
        self.0.dynamic_calls.lock().unwrap().push(json!({"owner":owner,"ids":ids,"comment":intent.comment(),"purpose":intent.purpose()}));
        if self.1=="error" {return Err(teaql_core::dynamic_fields::DynamicFieldError{code:"FIXTURE_DYNAMIC_FAILED",field:owner.into()});}
        let definitions=teaql_core::dynamic_fields::DynamicFieldDefinitions::new(owner,"v1",[("a".into(),DataType::Text),("b".into(),DataType::I64)]).unwrap();
        let rows=ids.iter().map(|id|(*id,if self.1=="empty" {std::collections::HashMap::new()}else{std::collections::HashMap::from([("a".into(),if self.1=="null" {Value::Null}else{Value::Text("loaded".into())})])})).collect();
        Ok(crate::dynamic_fields::DynamicFieldBatch{definitions,rows})
    }
}
