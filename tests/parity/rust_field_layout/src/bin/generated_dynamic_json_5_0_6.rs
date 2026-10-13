use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json, json};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::SelectQuery;
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
    let user_context = if execute {
        pool.lock()
            .unwrap()
            .execute_batch(include_str!("../../../../support/dynamic_json_schema.sql"))
            .unwrap();
        let context = wait(cpp_dynamic_json_core::service_runtime_from_pool(
            pool.clone(),
        ))
        .unwrap();
        pool.lock()
            .unwrap()
            .execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql"))
            .unwrap();
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
                    let result=wait(request.comment("what: inspect dynamic JSON query").purpose("why: qualify generated translation").execute_for_list(user_context.as_ref().unwrap()));
                    match result {Ok(list)=>{out["rows"]=json!(list.into_iter().map(|v|json!([v.id(),v.version()])).collect::<Vec<_>>());out["error"]=Json::Null;},Err(error)=>{out["rows"]=json!([]);out["error"]=json!(error.to_string());}}
                }
                out
            }
        }};
    }
    let minimal = input["minimal"].as_bool().unwrap();
    match input["entity"].as_str().unwrap() {
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
    }
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
