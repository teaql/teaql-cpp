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
fn observe(input: Json) -> Json {
    let provider=SqliteMutationExecutor::from_connection(rusqlite::Connection::open_in_memory().unwrap());
    let held=input["held"].as_bool().unwrap();
    let transaction=if held { Some(ready(provider.begin_sql()).unwrap()) } else { None };
    let query=CompiledQuery { log_context:Default::default(),sql:"SELECT ?".into(),
        params:vec![Value::List(vec![Value::I64(1)])],comment:None };
    let mut work=Box::pin(async {
        let error=match input["mode"].as_str().unwrap() {
            "execute"=>provider.execute_sql(&query).await.unwrap_err().to_string(),
            "fetch"=>provider.fetch_all_compact_sql(&query).await.unwrap_err().to_string(),
            "stream"=>{
                let mut stream=provider.stream_sql(query.clone(),1);
                stream.next().await.unwrap().unwrap_err().to_string()
            },
            _=>panic!("unknown fixture mode"),
        };
        json!({"error":error})
    });
    let mut cx=Context::from_waker(Waker::noop());
    let mut result=None;
    let mut pending=Vec::new();
    for _ in 0..2 {
        match work.as_mut().poll(&mut cx) {
            Poll::Pending=>pending.push(true),
            Poll::Ready(value)=>{pending.push(false);result=Some(value);break;},
        }
    }
    let cancelled=input["end"]=="cancel";
    if cancelled {
        drop(work);
        drop(transaction);
    } else {
        if let Some(tx)=transaction {
            match input["end"].as_str().unwrap() {
                "commit"=>ready(tx.commit_sql()).unwrap(),
                "rollback"=>ready(tx.rollback_sql()).unwrap(),
                "drop"=>drop(tx),
                _=>panic!("unknown fixture end"),
            }
        }
        if result.is_none() {
            result=Some(match work.as_mut().poll(&mut cx) {
                Poll::Ready(value)=>value,
                Poll::Pending=>panic!("lease failed to resume"),
            });
        }
    }
    json!({"pending":pending,"cancelled":cancelled,"result":result,
        "autocommit":provider.connection().lock().unwrap().is_autocommit()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
