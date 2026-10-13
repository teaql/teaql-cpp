//! Replay historical async calls with explicit pending/resume/cancel scheduling.
use serde_json::{Value as Json, json};
use std::{
    future::Future,
    io::{self, BufRead},
    pin::Pin,
    task::{Context, Poll, Waker},
};
use teaql_core::{CompactRow, Value};
use teaql_provider_sqlite::{MutationExecutorError, SqliteMutationExecutor, SqliteTransaction};
use teaql_sql::{CompiledQuery, SqlTransaction, SqlTransactionTransport, SqlTransport};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn ready<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    match future
        .as_mut()
        .poll(&mut Context::from_waker(Waker::noop()))
    {
        Poll::Ready(value) => value,
        Poll::Pending => panic!("transaction-local call unexpectedly pending"),
    }
}
fn compiled(sql: &str) -> CompiledQuery {
    CompiledQuery {
        log_context: Default::default(),
        sql: sql.into(),
        params: Vec::new(),
        comment: None,
    }
}
fn error(e: MutationExecutorError) -> Json {
    let kind = match &e {
        MutationExecutorError::Sqlite(_) => "Sqlite",
        MutationExecutorError::SqlCompile(_) => "SqlCompile",
        MutationExecutorError::UnsupportedValue(_) => "UnsupportedValue",
        MutationExecutorError::UnsupportedColumnType(_) => "UnsupportedColumnType",
        MutationExecutorError::Bind(_) => "Bind",
        MutationExecutorError::Lock(_) => "Lock",
    };
    let code = if let MutationExecutorError::Sqlite(ref e) = e {
        e.sqlite_error().map(|e| e.extended_code).unwrap_or(0)
    } else {
        0
    };
    json!({"error":e.to_string(),"kind":kind,"code":code})
}
fn unit(result: Result<(), MutationExecutorError>) -> Json {
    match result {
        Ok(()) => json!({"ok":true}),
        Err(e) => error(e),
    }
}
fn write(result: Result<u64, MutationExecutorError>) -> Json {
    match result {
        Ok(n) => json!({"affected":n}),
        Err(e) => error(e),
    }
}
fn rows(result: Result<Vec<CompactRow>, MutationExecutorError>) -> Json {
    match result {
        Ok(rows) => {
            json!({"rows":rows.into_iter().map(|row|json!({"keys":row.keys().collect::<Vec<_>>(),"values":row.values().map(Value::to_json_value).collect::<Vec<_>>()})).collect::<Vec<_>>()})
        }
        Err(e) => error(e),
    }
}
enum Work<'a> {
    Begin(Pin<Box<dyn Future<Output = Result<SqliteTransaction, MutationExecutorError>> + 'a>>),
    Write(Pin<Box<dyn Future<Output = Result<u64, MutationExecutorError>> + 'a>>),
}
fn poll(work: &mut Work<'_>, active: &mut Option<SqliteTransaction>) -> Option<Json> {
    let mut context = Context::from_waker(Waker::noop());
    match work {
        Work::Begin(future) => match future.as_mut().poll(&mut context) {
            Poll::Pending => None,
            Poll::Ready(result) => Some(match result {
                Ok(tx) => {
                    assert!(active.is_none());
                    *active = Some(tx);
                    json!({"ok":true})
                }
                Err(e) => error(e),
            }),
        },
        Work::Write(future) => match future.as_mut().poll(&mut context) {
            Poll::Pending => None,
            Poll::Ready(value) => Some(write(value)),
        },
    }
}
fn observe(input: &Json) -> Json {
    let provider =
        SqliteMutationExecutor::from_connection(rusqlite::Connection::open_in_memory().unwrap());
    provider
        .execute(&compiled(
            "CREATE TABLE records(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)",
        ))
        .unwrap();
    for i in 1..=input["count"].as_u64().unwrap() {
        provider
            .execute(&compiled(&format!(
                "INSERT INTO records VALUES({i},1,'name-{i}')"
            )))
            .unwrap();
    }
    let autocommit = || provider.connection().lock().unwrap().is_autocommit();
    let mut active = None;
    let mut pending: Vec<(usize, Work<'_>)> = Vec::new();
    let mut steps = Vec::new();
    let mut resumed = Vec::new();
    for (index, action) in input["actions"].as_array().unwrap().iter().enumerate() {
        let op = action["op"].as_str().unwrap();
        let mut result = match op {
            "begin" => {
                let mut work = Work::Begin(Box::pin(provider.begin_sql()));
                if let Some(result) = poll(&mut work, &mut active) {
                    result
                } else {
                    pending.push((index, work));
                    json!({"pending":true})
                }
            }
            "commit" | "rollback" => {
                if let Some(tx) = active.take() {
                    unit(if op == "commit" {
                        ready(tx.commit_sql())
                    } else {
                        ready(tx.rollback_sql())
                    })
                } else {
                    unit(if op == "commit" {
                        provider.commit_transaction()
                    } else {
                        provider.rollback_transaction()
                    })
                }
            }
            _ => {
                let mut query = compiled(action["sql"].as_str().unwrap());
                query.params = action["params"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(fixture_value::value)
                    .collect();
                if op == "fetch" {
                    rows(if let Some(tx) = active.as_ref() {
                        ready(tx.fetch_all_compact_sql(&query))
                    } else {
                        ready(provider.fetch_all_compact_sql(&query))
                    })
                } else if op != "outside_execute" && active.is_some() {
                    write(ready(active.as_ref().unwrap().execute_sql(&query)))
                } else {
                    let executor = provider.clone();
                    let mut work =
                        Work::Write(Box::pin(async move { executor.execute_sql(&query).await }));
                    if let Some(result) = poll(&mut work, &mut active) {
                        result
                    } else {
                        pending.push((index, work));
                        json!({"pending":true})
                    }
                }
            }
        };
        result["autocommit"] = json!(autocommit());
        steps.push(result);
        // One explicit sweep after each action. Submitted waiters retain order.
        let mut i = 0;
        while i < pending.len() {
            if let Some(mut result) = poll(&mut pending[i].1, &mut active) {
                result["autocommit"] = json!(autocommit());
                let (origin, _) = pending.remove(i);
                resumed.push(json!({"action":origin,"result":result}));
            } else {
                i += 1;
            }
        }
    }
    let cancelled = pending.iter().map(|(i, _)| *i).collect::<Vec<_>>();
    pending.clear();
    let dropped = active.is_some();
    drop(active.take());
    let final_rows = rows(ready(provider.fetch_all_compact_sql(&compiled(
        "SELECT id,version,name FROM records ORDER BY id",
    ))));
    json!({"steps":steps,"resumed":resumed,"cancelled":cancelled,"dropped_active":dropped,"final":final_rows,"autocommit":autocommit()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
