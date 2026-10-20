//! Observe root lease ordering and independent native-lock error precedence.
use futures_util::StreamExt;
use serde_json::{json, Value as Json};
use std::{future::Future, io::{self, BufRead}, panic::{catch_unwind, AssertUnwindSafe}, task::{Context, Poll, Waker}};
use teaql_core::{CompactRow, Value};
use teaql_provider_sqlite::{MutationExecutorError, SqliteMutationExecutor};
use teaql_sql::{CompiledQuery, SqlTransaction, SqlTransactionTransport, SqlTransport, StreamingSqlTransport};

fn query(sql: &str) -> CompiledQuery {
    CompiledQuery { log_context:Default::default(), sql:sql.into(), params:vec![], comment:None }
}
fn ready<T>(future: impl Future<Output=T>) -> T {
    let mut future=Box::pin(future);
    match future.as_mut().poll(&mut Context::from_waker(Waker::noop())) {
        Poll::Ready(value)=>value,
        Poll::Pending=>panic!("unexpected pending fixture"),
    }
}
fn rows(values: Vec<CompactRow>) -> Json {
    json!(values.into_iter().map(|row| row.values().map(Value::to_json_value).collect::<Vec<_>>()).collect::<Vec<_>>())
}
fn outcome<T>(value: Result<T, MutationExecutorError>, convert: impl FnOnce(T)->Json) -> Json {
    match value {
        Ok(value)=>convert(value),
        Err(error)=>json!({"error":error.to_string()}),
    }
}
fn observe(input: Json) -> Json {
    let provider=SqliteMutationExecutor::from_connection(rusqlite::Connection::open_in_memory().unwrap());
    let held=input["held"].as_bool().unwrap();
    let mode=input["mode"].as_str().unwrap();
    assert!(!mode.starts_with("sync_") || !held, "Synchronous controls require an idle connection");
    let poison=input["poison"].as_bool().unwrap_or(false);
    assert!(!poison || !held, "Poison controls use an idle connection");
    if poison {
        provider.execute(&query("CREATE TABLE poison_fixture(ts TIMESTAMP)")).unwrap();
        provider.execute(&query("INSERT INTO poison_fixture VALUES ('2024-01-01 00:00:/0')")).unwrap();
    }
    let transaction=if held { Some(ready(provider.begin_sql()).unwrap()) } else { None };
    if poison {
        let caught=catch_unwind(AssertUnwindSafe(|| {
            ready(provider.fetch_all_compact_sql(&query("SELECT ts FROM poison_fixture"))).unwrap();
        }));
        assert!(caught.is_err(), "Original checked timestamp must panic inside the native lock");
    }
    let binding=match input["binding"].as_str().unwrap_or("list") {
        "list"=>Value::List(vec![Value::I64(1)]),
        "object"=>Value::Object(Default::default()),
        "u64"=>Value::U64(u64::MAX),
        "valid"=>Value::I64(1),
        _=>panic!("unknown fixture binding"),
    };
    let mut request=query("SELECT ?"); request.params.push(binding);
    let repeated=request.params.clone();
    let mut work=Box::pin(async {
        match mode {
            "execute"=>outcome(provider.execute_sql(&request).await, |count|json!({"affected":count})),
            "fetch"=>outcome(provider.fetch_all_compact_sql(&request).await, |value|json!({"rows":rows(value)})),
            "repeated"=>outcome(provider.fetch_repeated_compact_sql(&request,0,&repeated).await, |value|json!({"rows":rows(value)})),
            "stream"=>{
                let mut stream=provider.stream_sql(request.clone(),1);
                outcome(stream.next().await.unwrap(), |chunk|json!({"rows":rows(chunk.rows),"chunk_index":chunk.chunk_index,"is_last":chunk.is_last}))
            },
            "sync_execute"=>outcome(provider.execute(&request), |count|json!({"affected":count})),
            "sync_fetch"=>outcome(provider.fetch_all_compact(&request), |value|json!({"rows":rows(value)})),
            _=>panic!("unknown fixture mode"),
        }
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
    let connection=provider.connection();
    let autocommit=connection.lock().unwrap_or_else(|error|error.into_inner()).is_autocommit();
    json!({"pending":pending,"cancelled":cancelled,"result":result,"autocommit":autocommit})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
