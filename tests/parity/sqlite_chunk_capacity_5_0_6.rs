//! Observe original native cursor capacities, complete rows, cancellation and reuse.
use futures_util::StreamExt;
use serde_json::{json, Value as Json};
use std::{future::Future, io::{self, BufRead}, task::{Context, Poll, Waker}};
use teaql_core::Value;
use teaql_provider_sqlite::SqliteMutationExecutor;
use teaql_sql::{CompiledQuery, SqlTransaction, SqlTransactionTransport, SqlTransport, StreamingSqlTransport};

fn ready<T>(future: impl Future<Output=T>) -> T {
    let mut future=Box::pin(future);
    match future.as_mut().poll(&mut Context::from_waker(Waker::noop())) {
        Poll::Ready(value)=>value,
        Poll::Pending=>panic!("unexpected pending fixture"),
    }
}
fn query() -> CompiledQuery {
    CompiledQuery { log_context:Default::default(), sql:"SELECT id FROM stream_probe ORDER BY id".into(), params:vec![],
        comment:Some("what: observe bounded cursor allocation; why: avoid chunk-buffer growth".into()) }
}
fn observe(input: Json) -> Json {
    let rows=usize::try_from(input["rows"].as_u64().unwrap()).unwrap();
    let size=usize::try_from(input["chunk_size"].as_u64().unwrap()).unwrap();
    let consume=input["consume"].as_u64().map(|n|usize::try_from(n).unwrap());
    let connection=rusqlite::Connection::open_in_memory().unwrap();
    connection.execute_batch(&format!("CREATE TABLE stream_probe(id INTEGER); INSERT INTO stream_probe SELECT x FROM (WITH RECURSIVE seq(x) AS (SELECT 1 WHERE {rows} > 0 UNION ALL SELECT x+1 FROM seq WHERE x < {rows}) SELECT x FROM seq);")).unwrap();
    let provider=SqliteMutationExecutor::from_connection(connection);
    let transaction=if input["transaction"]==true {Some(ready(provider.begin_sql()).unwrap())} else {None};
    let mut stream=if let Some(tx)=transaction.as_ref() {tx.stream_sql(query(),size)} else {provider.stream_sql(query(),size)};
    let mut chunks=Vec::new();
    let mut exhausted=false;
    while consume.is_none_or(|limit| chunks.len()<limit) {
        match ready(stream.next()) {
            Some(value)=>{
                let chunk=value.unwrap();
                let ids=chunk.rows.iter().map(|row|row.get("id").unwrap().to_json_value()).collect::<Vec<_>>();
                chunks.push(json!({"capacity":chunk.rows.capacity(),"ids":ids,"chunk_index":chunk.chunk_index,"is_last":chunk.is_last}));
            },
            None=>{exhausted=true;break;},
        }
    }
    drop(stream);
    if let Some(tx)=transaction {ready(tx.rollback_sql()).unwrap();}
    let again=ready(provider.fetch_all_compact_sql(&query())).unwrap();
    assert_eq!(again.len(),rows);
    for (index,row) in again.iter().enumerate() { assert_eq!(row.get("id"),Some(&Value::I64((index+1) as i64))); }
    json!({"chunks":chunks,"exhausted":exhausted,"reuse_rows":again.len(),"autocommit":provider.connection().lock().unwrap().is_autocommit()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
