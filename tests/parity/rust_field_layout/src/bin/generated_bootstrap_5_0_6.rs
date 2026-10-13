use serde_json::{json, Value as Json};
use std::{future::Future, io::{self, BufRead}, sync::{Arc, Mutex, atomic::{AtomicUsize, Ordering}}, task::{Context, Poll, Wake, Waker}};

struct NoWake;
impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
fn wait<T>(future: impl Future<Output=T>)->T {
    let mut future=Box::pin(future);let waker=Waker::from(Arc::new(NoWake));let mut context=Context::from_waker(&waker);
    loop {if let Poll::Ready(value)=future.as_mut().poll(&mut context) {return value;}}
}
fn database(connection:&rusqlite::Connection)->Json {
    let mut out=serde_json::Map::new();
    for (table,columns) in [("platform_data","id,name,version"),("order_status_data","id,name,code,platform,version")] {
        let mut statement=connection.prepare(&format!("SELECT {columns} FROM {table} ORDER BY id")).unwrap();
        let count=statement.column_count();
        let rows=statement.query_map([],|row| {
            Ok((0..count).map(|index|match row.get_ref(index).unwrap() {
                rusqlite::types::ValueRef::Null=>Json::Null,
                rusqlite::types::ValueRef::Integer(value)=>json!(value),
                rusqlite::types::ValueRef::Text(value)=>json!(std::str::from_utf8(value).unwrap()),_=>unreachable!(),
            }).collect::<Vec<_>>())
        }).unwrap();
        out.insert(table.into(),json!(rows.map(Result::unwrap).collect::<Vec<_>>()));
    }
    let exists=connection.query_row("SELECT count(*) FROM sqlite_master WHERE type='table' AND name='teaql_id_space'",[],|row|row.get::<_,i64>(0)).unwrap()!=0;
    let levels=if exists {let mut statement=connection.prepare("SELECT type_name,current_level FROM teaql_id_space ORDER BY type_name").unwrap();
        statement.query_map([],|row|Ok(json!([row.get::<_,String>(0)?,row.get::<_,i64>(1)?]))).unwrap().map(Result::unwrap).collect::<Vec<_>>()} else {Vec::new()};
    out.insert("ids".into(),json!(levels));Json::Object(out)
}
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let mut user_context=wait(cpp_candidates_core::service_runtime_from_pool(pool.clone())).unwrap();
    let state=input["state"].as_str().unwrap();
    {
        let connection=pool.lock().unwrap();
        let definition="CREATE TABLE platform_data(id INTEGER PRIMARY KEY,name TEXT NOT NULL,version INTEGER NOT NULL);CREATE TABLE order_status_data(id INTEGER PRIMARY KEY,name TEXT NOT NULL,code TEXT NOT NULL,platform INTEGER NOT NULL,version INTEGER NOT NULL,FOREIGN KEY(platform) REFERENCES platform_data(id));";
        connection.execute_batch(&if state=="null_fields" {definition.replace(" NOT NULL", "")} else {definition.to_string()}).unwrap();
        if state!="empty" {connection.execute_batch("INSERT INTO platform_data VALUES(1,'existing root',7);INSERT INTO order_status_data VALUES(11,'Draft order','DRAFT',1,3),(22,'In progress order','IN_PROGRESS',1,4),(2104194,'Completed order','DONE',1,5);").unwrap();}
        match state {
            "corrupt"=>connection.execute_batch("UPDATE order_status_data SET name='stale',code='OLD' WHERE id=11;").unwrap(),
            "missing"=>connection.execute_batch("DELETE FROM order_status_data WHERE id=22;").unwrap(),
            "deleted"=>connection.execute_batch("UPDATE order_status_data SET version=-3 WHERE id=11;").unwrap(),
            "foreign_root"=>connection.execute_batch("INSERT INTO platform_data VALUES(2,'other',3);UPDATE order_status_data SET platform=2 WHERE id=11;").unwrap(),
            "extra"=>connection.execute_batch("INSERT INTO order_status_data VALUES(999,'Custom','CUSTOM',1,8);").unwrap(),
            "null_fields"=>connection.execute_batch("UPDATE order_status_data SET name=NULL,code=NULL WHERE id=11;").unwrap(),_=>{},
        }
    }
    if state=="root_mismatch" {user_context=user_context.with_active_root("Platform",42);}
    let attempts=Arc::new(AtomicUsize::new(0));let observed=attempts.clone();
    let limit=input["failures"].as_u64().unwrap() as usize;
    pool.lock().unwrap().create_scalar_function("bootstrap_failure",0,rusqlite::functions::FunctionFlags::SQLITE_UTF8,
        move |_|Ok(if observed.fetch_add(1,Ordering::SeqCst)<limit {1_i64} else {0_i64})).unwrap();
    let error_message=match input["fault"].as_str().unwrap() {"primary"=>"UNIQUE constraint failed: order_status_data.id","secondary"=>"UNIQUE constraint failed: order_status_data.code",_=>"fixture bootstrap write failed"};
    if limit!=0 {
        pool.lock().unwrap().execute_batch(&format!("CREATE TRIGGER insert_failure BEFORE INSERT ON order_status_data WHEN NEW.id=11 AND bootstrap_failure()!=0 BEGIN SELECT RAISE(ABORT,'{error_message}'); END;
            CREATE TRIGGER update_failure BEFORE UPDATE ON order_status_data WHEN NEW.id=11 AND bootstrap_failure()!=0 BEGIN SELECT RAISE(ABORT,'{error_message}'); END;")).unwrap();
    }
    let mut rounds=Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        let before=database(&pool.lock().unwrap());let result=wait(user_context.ensure_schema());
        rounds.push(json!({"before":before,"after":database(&pool.lock().unwrap()),"error":result.err().map(|error|error.to_string()),"attempts":attempts.load(Ordering::SeqCst),"root":user_context.require_active_root("Platform").ok().map(|root|root.id)}));
    }
    json!(rounds)
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));
    }
}
