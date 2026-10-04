use std::{io::{self,BufRead},future::Future,task::{Context,Poll,Waker}};
use serde_json::{Value as J,json};
use teaql_core::{Value,CompactRow};
use teaql_core::business_id::{BusinessIdAllocator,BusinessIdScope,BusinessIdPlan,BusinessIdError};
use teaql_provider_sqlite::{SqliteMutationExecutor,SqliteIdSpaceGenerator,SqliteBusinessIdAllocator,MutationExecutorError};
use teaql_sql::{SqlTransport,StreamingSqlTransport,SqlTransactionTransport,SqlTransaction,CompiledQuery};
use teaql_data_service::StreamChunk;
#[path="../fixture_value.rs"]mod fixture_value;
fn ready<F:Future>(future:F)->F::Output{let mut f=std::pin::pin!(future);match f.as_mut().poll(&mut Context::from_waker(Waker::noop())){Poll::Ready(v)=>v,Poll::Pending=>panic!("unexpected pending fixture")}}
fn error(e:MutationExecutorError)->J{let kind=match &e{MutationExecutorError::Sqlite(_)=>"Sqlite",MutationExecutorError::SqlCompile(_)=>"SqlCompile",MutationExecutorError::UnsupportedValue(_)=>"UnsupportedValue",MutationExecutorError::UnsupportedColumnType(_)=>"UnsupportedColumnType",MutationExecutorError::Bind(_)=>"Bind",MutationExecutorError::Lock(_)=>"Lock"};let code=match &e{MutationExecutorError::Sqlite(e)=>e.sqlite_error().map(|e|e.extended_code).unwrap_or(0),_=>0};json!({"error":e.to_string(),"kind":kind,"code":code})}
fn rows(v:&[CompactRow])->J{json!(v.iter().map(|r|json!({"keys":r.keys().collect::<Vec<_>>(),"values":r.values().map(Value::to_json_value).collect::<Vec<_>>()})).collect::<Vec<_>>())}
fn chunk(v:&StreamChunk)->J{json!({"rows":rows(&v.rows),"index":v.chunk_index,"last":v.is_last})}
fn transaction_steps(provider:&SqliteMutationExecutor,j:&J)->J{
    let mut tx:Option<SqliteMutationExecutor>=None;let asynchronous=j["async"].as_bool().unwrap();let mut results=vec![];
    fn unit(value:Result<(),MutationExecutorError>)->J{match value{Ok(())=>json!({"ok":true}),Err(e)=>error(e)}}
    for action in j["actions"].as_array().unwrap(){
        let op=action["op"].as_str().unwrap();
        let mut result=match op{
            "begin"=>if asynchronous{match ready(provider.begin_sql()){Ok(value)=>{tx=Some(value);json!({"ok":true})},Err(e)=>error(e)}}else{unit(provider.begin_transaction())},
            "commit"|"rollback"=>if asynchronous && tx.is_some(){let previous=tx.take().unwrap();unit(if op=="commit"{ready(previous.commit_sql())}else{ready(previous.rollback_sql())})}else{unit(if op=="commit"{provider.commit_transaction()}else{provider.rollback_transaction()})},
            _=>{
                let query=CompiledQuery{log_context:Default::default(),sql:action["sql"].as_str().unwrap().into(),params:action["params"].as_array().unwrap().iter().map(fixture_value::value).collect(),comment:None};
                let transport=if asynchronous && op!="outside_execute"{tx.as_ref().unwrap_or(provider)}else{provider};
                if op=="fetch"{match ready(transport.fetch_all_compact_sql(&query)){Ok(v)=>json!({"rows":rows(&v)}),Err(e)=>error(e)}}
                else{match ready(transport.execute_sql(&query)){Ok(v)=>json!({"affected":v}),Err(e)=>error(e)}}
            }
        };
        result["autocommit"]=json!(provider.connection().lock().unwrap().is_autocommit());results.push(result);
    }
    json!({"steps":results})
}
fn id_steps(provider:&SqliteMutationExecutor,j:&J)->J{
    let table=j["table"].as_str().unwrap();let generator=SqliteIdSpaceGenerator::from_executor(provider.clone()).with_table_name(table);let mut results=vec![];
    for action in j["actions"].as_array().unwrap(){
        let op=action["op"].as_str().unwrap();results.push(match op{
            "sql"=>match provider.execute(&CompiledQuery{log_context:Default::default(),sql:action["sql"].as_str().unwrap().into(),params:vec![],comment:None}){Ok(value)=>json!({"affected":value}),Err(e)=>error(e)},
            "next"=>match generator.next_id(action["entity"].as_str().unwrap()){Ok(value)=>json!({"id":value}),Err(e)=>error(e)},
            _=>match if op=="ensure"{generator.ensure_table()}else{generator.ensure_floor(action["entity"].as_str().unwrap(),action["floor"].as_u64().unwrap())}{Ok(())=>json!({"ok":true}),Err(e)=>error(e)},
        });
    }
    let query=CompiledQuery{log_context:Default::default(),sql:format!("SELECT type_name,current_level,typeof(current_level) AS storage_type FROM {} ORDER BY type_name",teaql_sql::quote_identifier_if_needed(table,'"')),params:vec![],comment:None};
    json!({"steps":results,"space":match provider.fetch_all_compact(&query){Ok(value)=>rows(&value),Err(e)=>error(e)}})
}
fn business_error_json(e:BusinessIdError)->J{json!({"code":format!("{:?}",e.code),"message":e.message,"display":e.to_string()})}
fn business_steps(provider:&SqliteMutationExecutor,j:&J)->J{
    let table=j["table"].as_str().unwrap();let allocator=SqliteBusinessIdAllocator::from_executor(provider.clone()).with_table_name(table);let mut results=vec![];
    for action in j["actions"].as_array().unwrap(){
        let mut result=match action["op"].as_str().unwrap(){
            "sql"=>match provider.execute(&CompiledQuery{log_context:Default::default(),sql:action["sql"].as_str().unwrap().into(),params:vec![],comment:None}){Ok(value)=>json!({"affected":value}),Err(e)=>error(e)},
            "ensure"=>match allocator.ensure_schema(&Default::default()){Ok(())=>json!({"ok":true}),Err(e)=>business_error_json(e)},
            _=>{
                let parts=action["scope"].as_array().unwrap();let scope=BusinessIdScope{domain_root_key:parts[0].as_str().unwrap().into(),aggregate_type:parts[1].as_str().unwrap().into(),namespace:parts[2].as_str().unwrap().into(),period_key:parts[3].as_str().unwrap().into()};
                let plan=BusinessIdPlan{scope,prefix:"ignored".into(),date_text:None,preserve_digits:0,separator:"".into(),initial_sequence:action["initial"].as_u64().unwrap(),max_sequence:action["maximum"].as_u64().unwrap()};
                match allocator.allocate(&plan){Ok(value)=>json!({"sequence":value.sequence,"scope":value.scope.canonical_key()}),Err(e)=>business_error_json(e)}
            }
        };
        result["autocommit"]=json!(provider.connection().lock().unwrap().is_autocommit());results.push(result);
    }
    let query=CompiledQuery{log_context:Default::default(),sql:format!("SELECT scope_key,current_level,version,typeof(current_level) AS level_type,typeof(version) AS version_type FROM {} ORDER BY scope_key",teaql_sql::quote_identifier_if_needed(table,'"')),params:vec![],comment:None};
    json!({"steps":results,"space":match provider.fetch_all_compact(&query){Ok(value)=>rows(&value),Err(e)=>error(e)}})
}
fn evaluate(j:J)->J{
    let connection=rusqlite::Connection::open_in_memory().unwrap();connection.execute_batch("CREATE TABLE records(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)").unwrap();
    for i in 1..=j["count"].as_u64().unwrap(){connection.execute("INSERT INTO records VALUES(?,1,?)",rusqlite::params![i as i64,format!("name-{i}")]).unwrap();}
    let provider=SqliteMutationExecutor::from_connection(connection);
    let query=CompiledQuery{log_context:Default::default(),sql:j["sql"].as_str().unwrap().into(),params:j["params"].as_array().unwrap().iter().map(fixture_value::value).collect(),comment:j["comment"].as_str().map(str::to_owned)};
    let outcome=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||match j["mode"].as_str().unwrap(){
        "transaction"=>transaction_steps(&provider,&j),
        "id_space"=>id_steps(&provider,&j),
        "business_id"=>business_steps(&provider,&j),
        "fetch"=>match ready(provider.fetch_all_compact_sql(&query)){Ok(v)=>json!({"rows":rows(&v)}),Err(e)=>error(e)},
        "execute"=>match ready(provider.execute_sql(&query)){Ok(v)=>json!({"affected":v}),Err(e)=>error(e)},
        "repeated"=>{let values=j["values"].as_array().unwrap().iter().map(fixture_value::value).collect::<Vec<_>>();match ready(provider.fetch_repeated_compact_sql(&query,j["index"].as_u64().unwrap() as usize,&values)){Ok(v)=>json!({"rows":rows(&v)}),Err(e)=>error(e)}},
        "buffered"=>match provider.fetch_stream(&query,j["size"].as_u64().unwrap() as usize){Ok(v)=>json!({"chunks":v.iter().map(chunk).collect::<Vec<_>>()}),Err(e)=>error(e)},
        "stream"=>{let mut stream=provider.stream_sql(query,j["size"].as_u64().unwrap() as usize);let mut polls=vec![];for _ in 0..j["polls"].as_u64().unwrap(){match stream.as_mut().poll_next(&mut Context::from_waker(Waker::noop())){Poll::Pending=>polls.push(json!({"pending":true})),Poll::Ready(None)=>{polls.push(json!({"end":true}));break},Poll::Ready(Some(Ok(v)))=>polls.push(chunk(&v)),Poll::Ready(Some(Err(e)))=>polls.push(error(e))}}drop(stream);json!({"polls":polls})},
        _=>panic!("unknown fixture mode"),
    }));
    let outcome=outcome.unwrap_or_else(|_|json!({"panic":true}));
    let saved=provider.fetch_all_compact(&CompiledQuery{log_context:Default::default(),sql:"SELECT id,version,name FROM records ORDER BY id".into(),params:vec![],comment:None});
    json!({"outcome":outcome,"saved":match saved{Ok(v)=>rows(&v),Err(e)=>error(e)}})
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
