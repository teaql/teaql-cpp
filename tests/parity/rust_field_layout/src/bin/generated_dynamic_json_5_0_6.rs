use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json, json};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::{Expr, SelectQuery};
use teaql_runtime::{RequestPolicy, RuntimeError, UserContext};

#[derive(Default)]
struct PolicyObservation {
    calls: Mutex<Vec<Json>>,
    selects: std::sync::atomic::AtomicU64,
}
struct JsonRequestPolicy {
    mode: String,
    observation: Arc<PolicyObservation>,
}
impl RequestPolicy for JsonRequestPolicy {
    fn enforce_select(&self, _: &UserContext, query: &mut SelectQuery) -> Result<(), RuntimeError> {
        let before = describe(query);
        if self.mode == "restrict" || self.mode == "slice" {
            let id = match query.entity.as_str() { "Person" => 2_u64, "Order" => 11, "Product" => 6, _ => 21 };
            *query = query.clone().and_filter(Expr::eq("id", id));
        }
        if self.mode == "slice" {
            *query = query.clone().offset(0).limit(1);
        }
        self.observation.calls.lock().unwrap().push(json!({
            "before":before, "after":describe(query), "comment":query.comment, "purpose":query.purpose,
            "aggregates":query.aggregates.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>(),
            "trace":query.trace_chain.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>()
        }));
        if self.mode == "deny" {
            return Err(RuntimeError::Policy("DYNAMIC_JSON_POLICY_DENIED".into()));
        }
        Ok(())
    }
}
unsafe extern "C" fn observe_database(data: *mut std::ffi::c_void, action: i32,
    _: *const std::ffi::c_char, _: *const std::ffi::c_char,
    _: *const std::ffi::c_char, _: *const std::ffi::c_char) -> i32 {
    if action == rusqlite::ffi::SQLITE_SELECT {
        let observation = unsafe { &*(data as *const PolicyObservation) };
        observation.selects.fetch_add(1, std::sync::atomic::Ordering::Relaxed);
    }
    rusqlite::ffi::SQLITE_OK
}
fn wait<T>(future: impl Future<Output = T>) -> T {
    struct NoWake;
    impl Wake for NoWake {
        fn wake(self: Arc<Self>) {}
    }
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(value) = future.as_mut().poll(&mut context) {
            return value;
        }
    }
}
fn describe(query: &SelectQuery) -> Json {
    json!({"entity":query.entity,"projection":query.projection,"filter":query.filter.as_ref().map(|v|format!("{v:?}")),
        "orders":query.order_by.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),"slice":query.slice.as_ref().map(|v|format!("{v:?}")),"hard_limit":query.hard_limit})
}
fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    if input["kind"] == "schema" {
        let context = wait(cpp_dynamic_json_core::service_runtime_from_pool(
            pool.clone(),
        ))
        .unwrap();
        return json!({"error": wait(context.ensure_schema()).err().map(|error|error.to_string())});
    }
    let execute = input["execute"].as_bool().unwrap();
    let observation = Arc::new(PolicyObservation::default());
    let policy = input.get("policy").and_then(Json::as_str);
    let user_context = if execute {
        pool.lock()
            .unwrap()
            .execute_batch(include_str!("../../../../support/dynamic_json_schema.sql"))
            .unwrap();
        let mut context = wait(cpp_dynamic_json_core::service_runtime_from_pool(
            pool.clone(),
        ))
        .unwrap();
        pool.lock()
            .unwrap()
            .execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql"))
            .unwrap();
        if let Some(mode) = policy {
            context.disable_sql_log();
            if input["logging"] == true { context.enable_select_sql_log(); }
            context.set_request_policy(JsonRequestPolicy { mode: mode.into(), observation: observation.clone() });
            let connection = pool.lock().unwrap();
            let status = unsafe { rusqlite::ffi::sqlite3_set_authorizer(connection.handle(), Some(observe_database),
                Arc::as_ptr(&observation) as *mut std::ffi::c_void) };
            assert_eq!(status, rusqlite::ffi::SQLITE_OK);
        }
        Some(context.with_active_root("Platform", 1))
    } else {
        None
    };
    macro_rules! apply {
        ($request:expr)=>{{
            let mut request=$request;
            if input["pre_slice"]==true {request=request.offset(3,7);}
            if input["unlimited"]==true {request=request.unlimited();}
            let mut failure=None;
            for value in input["filters"].as_array().unwrap() {
                let next=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||request.clone().filter_with_json(value.as_str().unwrap())));
                match next {Ok(value)=>request=value,Err(error)=>{failure=Some(if let Some(v)=error.downcast_ref::<String>() {v.clone()} else {error.downcast_ref::<&str>().unwrap().to_string()});break;}}
            }
            if let Some(failure)=failure {json!({"panic":failure})} else {
                let query:SelectQuery=request.clone().into();let mut out=json!({"query":describe(&query)});
                if execute {
                    let request=if input["execution"]=="stream" {request.stream(input["chunk"].as_u64().unwrap() as usize)} else {request};
                    let request=request.comment("what: inspect dynamic JSON query").purpose("why: qualify generated translation");
                    match input.get("execution").and_then(Json::as_str).unwrap_or("list") {
                        "count"=>{
                            let result=wait(request.execute_for_count(user_context.as_ref().unwrap()));
                            out["count"]=json!(result.as_ref().ok());
                            out["error"]=json!(result.err().map(|error|error.to_string()));out["rows"]=json!([]);
                        },
                        "exists"=>{
                            let result=wait(request.execute_for_exists(user_context.as_ref().unwrap()));
                            out["exists"]=json!(result.as_ref().ok());
                            out["error"]=json!(result.err().map(|error|error.to_string()));out["rows"]=json!([]);
                        },
                        "page"=>{
                            match wait(request.execute_for_page(user_context.as_ref().unwrap(),1,1)) {
                                Ok(list)=>{out["total"]=json!(list.total_count);out["rows"]=json!(list.into_iter().map(|v|json!([v.id(),v.version()])).collect::<Vec<_>>());out["error"]=Json::Null;},
                                Err(error)=>{out["total"]=Json::Null;out["rows"]=json!([]);out["error"]=json!(error.to_string());}
                            }
                        },
                        "one"=>{
                            let result=wait(request.execute_for_one(user_context.as_ref().unwrap()));
                            match result {
                                Ok(entity)=>{out["rows"]=json!(entity.map(|v|vec![json!([v.id(),v.version()])]).unwrap_or_default());out["error"]=Json::Null;},
                                Err(error)=>{out["rows"]=json!([]);out["error"]=json!(error.to_string());}
                            }
                        },
                        "stream"=>{
                            let mut rows=Vec::new();let mut error=None;
                            match wait(request.execute_for_stream(user_context.as_ref().unwrap())) {
                                Ok(mut stream)=>{
                                    for _ in 0..input["stop"].as_u64().unwrap() {
                                        match wait(std::future::poll_fn(|context|stream.as_mut().poll_next(context))) {
                                            Some(Ok(entity))=>rows.push(json!([entity.id(),entity.version()])),
                                            Some(Err(value))=>{error=Some(value.to_string());break;},
                                            None=>break,
                                        }
                                    }
                                    drop(stream);
                                },
                                Err(value)=>error=Some(value.to_string()),
                            }
                            out["rows"]=json!(rows);out["error"]=json!(error);
                            let available=pool.try_lock().is_ok();
                            let reused=available && wait(Q::people().whose_id_is(2_u64)
                                .comment("what: reuse executor after JSON stream").purpose("why: verify cursor lease release")
                                .execute_for_one(user_context.as_ref().unwrap())).is_ok_and(|value|value.is_some());
                            out["reused"]=json!(reused);
                        },
                        _=>{
                            let result=wait(request.execute_for_list(user_context.as_ref().unwrap()));
                            match result {Ok(list)=>{out["rows"]=json!(list.into_iter().map(|v|json!([v.id(),v.version()])).collect::<Vec<_>>());out["error"]=Json::Null;},Err(error)=>{out["rows"]=json!([]);out["error"]=json!(error.to_string());}}
                        },
                    }
                }
                out
            }
        }};
    }
    let minimal = input["minimal"].as_bool().unwrap();
    let mut output = match input["entity"].as_str().unwrap() {
        "person" => apply!(if minimal {
            Q::people_minimal()
        } else {
            Q::people()
        }),
        "product" => apply!(if minimal {
            Q::products_minimal()
        } else {
            Q::products()
        }),
        "line" => apply!(if minimal {
            Q::lines_minimal()
        } else {
            Q::lines()
        }),
        _ => apply!(if minimal {
            Q::orders_minimal()
        } else {
            Q::orders()
        }),
    };
    if policy.is_some() {
        let connection = pool.lock().unwrap();
        unsafe { rusqlite::ffi::sqlite3_set_authorizer(connection.handle(), None, std::ptr::null_mut()); }
        output["policy_calls"] = json!(*observation.calls.lock().unwrap());
        output["database_selects"] = json!(observation.selects.load(std::sync::atomic::Ordering::Relaxed));
        output["logs"] = json!(user_context.as_ref().unwrap().sql_logs().iter().map(|entry|json!({
            "sql":entry.sql,"comment":entry.comment,"purpose":entry.purpose,
            "params":entry.params.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>(),
            "trace":entry.trace_path.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>()
        })).collect::<Vec<_>>());
    }
    output
}
fn main() {
    if std::env::var_os("TEAQL_DYNAMIC_JSON_DEBUG").is_none() {
        std::panic::set_hook(Box::new(|_| {}));
    }
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
