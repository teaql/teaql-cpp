//! Observe actual independent transaction lease futures and drop rollback.
use serde_json::{Value as Json, json};
use std::{
    future::Future,
    io::{self, BufRead},
    pin::Pin,
    sync::{
        Arc,
        atomic::{AtomicUsize, Ordering},
    },
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::{CompactRow, Value};
use teaql_provider_sqlite::SqliteMutationExecutor;
use teaql_sql::{CompiledQuery, SqlTransaction, SqlTransactionTransport, SqlTransport};

fn query(sql: &str) -> CompiledQuery {
    CompiledQuery {
        log_context: Default::default(),
        sql: sql.into(),
        params: Vec::new(),
        comment: None,
    }
}
fn ready<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    match future
        .as_mut()
        .poll(&mut Context::from_waker(Waker::noop()))
    {
        Poll::Ready(value) => value,
        Poll::Pending => panic!("unexpected pending fixture"),
    }
}
fn rows(input: Vec<CompactRow>) -> Json {
    json!(
        input
            .into_iter()
            .map(|row| row.values().map(Value::to_json_value).collect::<Vec<_>>())
            .collect::<Vec<_>>()
    )
}
struct Signal(AtomicUsize);
impl Wake for Signal {
    fn wake(self: Arc<Self>) {
        self.0.fetch_add(1, Ordering::Relaxed);
    }
}

fn observe(input: &Json) -> Json {
    let provider =
        SqliteMutationExecutor::from_connection(rusqlite::Connection::open_in_memory().unwrap());
    provider
        .execute(&query(
            "CREATE TABLE records(id INTEGER PRIMARY KEY,name TEXT)",
        ))
        .unwrap();
    for id in 1..=input["count"].as_u64().unwrap() {
        provider
            .execute(&query(&format!(
                "INSERT INTO records VALUES({id},'before')"
            )))
            .unwrap();
    }
    let transaction = ready(provider.begin_sql()).unwrap();
    let changed = if input["write"] == true {
        ready(transaction.execute_sql(&query("UPDATE records SET name='tx' WHERE id=1"))).unwrap()
    } else {
        0
    };
    let select = query("SELECT id,name FROM records ORDER BY id");
    let inside = rows(ready(transaction.fetch_all_compact_sql(&select)).unwrap());
    let outside = query("UPDATE records SET name='outside' WHERE id=1");
    let parent = input["parent"].as_str().unwrap();
    let mut work: Option<Pin<Box<dyn Future<Output = Json> + '_>>> = Some(match parent {
        "read" => Box::pin(async {
            json!({"rows":rows(provider.fetch_all_compact_sql(&select).await.unwrap())})
        }),
        "begin" => Box::pin(async {
            let next = provider.begin_sql().await.unwrap();
            next.rollback_sql().await.unwrap();
            json!({"began":true})
        }),
        _ => Box::pin(async { json!({"affected":provider.execute_sql(&outside).await.unwrap()}) }),
    });
    let signal = Arc::new(Signal(AtomicUsize::new(0)));
    let waker = Waker::from(signal.clone());
    let mut context = Context::from_waker(&waker);
    let mut pending = Vec::new();
    for _ in 0..input["polls"].as_u64().unwrap() {
        pending.push(
            work.as_mut()
                .unwrap()
                .as_mut()
                .poll(&mut context)
                .is_pending(),
        );
    }
    let cancelled = parent == "cancel";
    if cancelled {
        drop(work.take());
    }
    match input["end"].as_str().unwrap() {
        "commit" => ready(transaction.commit_sql()).unwrap(),
        "rollback" => ready(transaction.rollback_sql()).unwrap(),
        _ => drop(transaction),
    }
    let woke = signal.0.load(Ordering::Relaxed) > 0;
    let result = if let Some(mut future) = work.take() {
        match future.as_mut().poll(&mut context) {
            Poll::Ready(value) => value,
            Poll::Pending => panic!("lease did not resume"),
        }
    } else {
        Json::Null
    };
    let after = rows(ready(provider.fetch_all_compact_sql(&select)).unwrap());
    json!({"changed":changed,"inside":inside,"pending":pending,"cancelled":cancelled,"woke":woke,"result":result,"after":after,"autocommit":provider.connection().lock().unwrap().is_autocommit()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
