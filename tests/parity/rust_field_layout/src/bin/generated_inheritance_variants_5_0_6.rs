use cpp_inheritance_variants_core::{Q,Document,IntermediateRecord,Invoice,Receipt,SpecialInvoice};
use serde_json::{json,Value as Json};
use std::{future::Future,io::{self,BufRead},sync::{Arc,Mutex},task::{Context,Poll,Wake,Waker}};
use teaql_core::{Entity,TeaqlEntity,IdentifiableEntity,SelectQuery,request::QuerySelection};
use teaql_runtime::{LedgerEntity,RuntimeError,JsonFieldNamingProfile};

struct NoWake;
impl Wake for NoWake { fn wake(self:Arc<Self>) {} }
fn wait<T>(future:impl Future<Output=T>)->T {
    let mut future=Box::pin(future);let waker=Waker::from(Arc::new(NoWake));let mut context=Context::from_waker(&waker);
    loop {if let Poll::Ready(value)=future.as_mut().poll(&mut context) {return value;}}
}
fn query(value:&SelectQuery)->Json {
    json!({"entity":value.entity,"projection":value.projection,"filter":value.filter.as_ref().map(|v|format!("{:?}",v)),"slice":value.slice.as_ref().map(|v|format!("{:?}",v)),"hard_limit":value.hard_limit,"children":value.child_enhancements.iter().map(query).collect::<Vec<_>>()})
}
fn native<T:Entity>(entity:T)->Json {
    json!(entity.into_values().into_iter().filter(|(name,_)|!name.starts_with('_'))
        .map(|(name,value)|{
            let v=if name=="created_at" {json!(value.try_timestamp().unwrap().0!=111)} else if name=="updated_at" {json!(value.try_timestamp().unwrap().0!=222)} else {value.to_json_value()};(name,v)
        }).collect::<serde_json::Map<_,_>>())
}
fn database(connection:&rusqlite::Connection)->Json {
    let mut output=serde_json::Map::new();
    for table in ["document_data","intermediate_record_data","invoice_data","receipt_data","special_invoice_data"] {
        let amount=if table=="invoice_data"||table=="special_invoice_data" {"CAST(amount AS TEXT)"} else {"NULL"};
        let status=if table=="receipt_data" {"status"} else {"NULL"};
        let reference=if table=="special_invoice_data" {"reference"} else {"NULL"};
        let mut statement=connection.prepare(&format!("SELECT id,name,active,note,tenant,version,created_at,updated_at,{amount},{status},{reference} FROM {table} ORDER BY id")).unwrap();
        let rows=statement.query_map([],|r|Ok(json!({"id":r.get::<_,i64>(0)?,"name":r.get::<_,String>(1)?,"active":r.get::<_,bool>(2)?,"note":r.get::<_,Option<String>>(3)?,"tenant":r.get::<_,i64>(4)?,"version":r.get::<_,i64>(5)?,"created_changed":r.get::<_,i64>(6)?!=111,"updated_changed":r.get::<_,i64>(7)?!=222,"amount":r.get::<_,Option<String>>(8)?,"status":r.get::<_,Option<String>>(9)?,"reference":r.get::<_,Option<String>>(10)?}))).unwrap();
        output.insert(table.into(),json!(rows.collect::<Result<Vec<_>,_>>().unwrap()));
    }
    Json::Object(output)
}
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let user_context=wait(cpp_inheritance_variants_core::service_runtime_from_pool(pool.clone())).unwrap();
    wait(user_context.ensure_schema()).unwrap();
    pool.lock().unwrap().execute_batch("UPDATE tenant_data SET name='root',version=3 WHERE id=1;
        INSERT INTO document_data(id,name,active,note,tenant,created_at,updated_at,version) VALUES(7,'document',1,NULL,1,111,222,3),(8,'deleted document',0,'old',1,111,222,-2),(9,'plain',0,NULL,1,111,222,3);
        INSERT INTO intermediate_record_data(id,name,active,note,tenant,created_at,updated_at,version) VALUES(7,'intermediate',0,'mid',1,111,222,4);
        INSERT INTO invoice_data(id,name,active,note,tenant,created_at,updated_at,version,amount) VALUES(7,'invoice',1,NULL,1,111,222,5,'12.50'),(10,'orphan',1,NULL,1,111,222,5,'2.50');
        INSERT INTO receipt_data(id,name,active,note,tenant,created_at,updated_at,version,status) VALUES(7,'receipt',0,'branch',1,111,222,6,'open');
        INSERT INTO special_invoice_data(id,name,active,note,tenant,created_at,updated_at,version,amount,reference) VALUES(7,'special',1,NULL,1,111,222,7,'20.50','ref');").unwrap();
    macro_rules! run {
        ($request:expr,$model:ty)=>{{
            let mut request=$request;
            if input["automatic"]==true {request=request.enhance_children_if_needed();}
            if input["manual"]==true {request=request.enhance_child(Q::receipts_minimal().select_status());}
            if input["subtype"]==true {request=request.match_types(["Invoice","SpecialInvoice"]);}
            if input["mode"]=="construct" {return query(&QuerySelection::from(request).into_query());}
            let values=match wait(request.order_by_id_asc().purpose("Qualify branched inheritance").comment("Empty intermediate and inherited fields").execute_for_list(&user_context)) {
                Ok(v)=>v,Err(e)=>return json!({"error":e.to_string()}),
            };
            if input["mode"]=="read" {return json!({"rows":values.data.into_iter().map(native).collect::<Vec<_>>()});}
            let mut entity=values.data.into_iter().find(|v|v.id_value().try_u64()==Some(7)).unwrap();
            let root=entity.entity_runtime_state().unwrap();
            entity.update_name(input["name"].as_str().unwrap()).update_active(false).update_note(teaql_core::Value::Null);
            let table=<$model>::entity_descriptor().table_name;
            if input["fault"]=="conflict" {pool.lock().unwrap().execute(&format!("UPDATE {table} SET version=version+1 WHERE id=7"),[]).unwrap();}
            if input["fault"]=="trigger" {pool.lock().unwrap().execute_batch(&format!("CREATE TRIGGER fixture_failure BEFORE UPDATE ON {table} BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;")).unwrap();}
            let before=database(&pool.lock().unwrap());let mut saved=None;let mut error=None;let mut kind=None;let mut diagnostics=Vec::new();
            match wait(teaql_runtime::save_audited_ledger_entity(entity.audit_as("Save inherited optional clock fields"),&user_context)) {
                Ok(v)=>saved=Some(native(v)),
                Err(RuntimeError::Check(v))=>{kind=Some("Check");error=Some("Check".to_string());diagnostics=v;},
                Err(e)=>{kind=Some(match e {RuntimeError::OptimisticLockConflict{..}=>"OptimisticLockConflict",RuntimeError::Graph(_)=>"Graph",_=>"Other"});error=Some(if kind==Some("OptimisticLockConflict") {"OptimisticLockConflict".to_string()} else if e.to_string().contains("fixture write failed") {"FixtureWriteFailure".to_string()} else {e.to_string()});},
            }
            json!({"saved":saved,"kind":kind,"error":error,"diagnostics":diagnostics.iter().map(|v|v.to_wire(JsonFieldNamingProfile::SnakeCase)).collect::<Vec<_>>(),"before":before,"after":database(&pool.lock().unwrap()),"pending":root.current_change_set().changes().values().flat_map(|v|v.keys().cloned()).collect::<std::collections::BTreeSet<_>>(),"fix_fields":user_context.last_fix_evidence().iter().map(|v|v.model_path.clone()).collect::<Vec<_>>()})
        }};
    }
    macro_rules! entry {($normal:expr,$minimal:expr,$children:expr)=>{match input["entry"].as_str().unwrap() {"minimal"=>$minimal,"children"=>$children,_=>$normal}};}
    match input["model"].as_str().unwrap() {
        "document"=>run!(entry!(Q::documents(),Q::documents_minimal(),Q::documents_with_children()),Document),
        "intermediate_record"=>run!(entry!(Q::intermediate_records(),Q::intermediate_records_minimal(),Q::intermediate_records_with_children()),IntermediateRecord),
        "invoice"=>run!(entry!(Q::invoices(),Q::invoices_minimal(),Q::invoices_with_children()),Invoice),
        "receipt"=>run!(entry!(Q::receipts(),Q::receipts_minimal(),Q::receipts_with_children()),Receipt),
        _=>run!(entry!(Q::special_invoices(),Q::special_invoices_minimal(),Q::special_invoices_with_children()),SpecialInvoice),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));
    }
}
