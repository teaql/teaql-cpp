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
struct SchemaItem { row:CompactRow }
impl teaql_core::TeaqlEntity for SchemaItem {
    const ENTITY_NAME:&'static str="Item";
    fn entity_descriptor()->EntityDescriptor {EntityDescriptor::new("Item").table_name("items")}
}
impl teaql_core::Entity for SchemaItem {
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
struct SelectionPolicy(Arc<State>,String);
impl crate::RequestPolicy for SelectionPolicy {
    fn enforce_select(&self,_:&crate::UserContext,query:&mut SelectQuery)->Result<(),crate::RuntimeError> {
        select_dynamic_fields(&self.0,query,"policy",&self.1)
    }
}
