use cpp_inheritance_core::{Q, School, Academy, ResearchAcademy};
use serde_json::{json, Value as Json};
use teaql_core::Entity;
use std::{future::Future, io::{self, BufRead}, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
struct NoWake;
impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
fn wait<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop { if let Poll::Ready(result) = future.as_mut().poll(&mut context) { return result; } }
}
fn describe_return<T: Entity + teaql_core::TeaqlEntity>(entity: T) -> Json {
    let loaded = ["id", "name", "rank", "version", "platform_id", "campus_code", "budget"]
        .into_iter().map(|name|(name.to_owned(), Json::Bool(entity.is_field_loaded(name)))).collect::<serde_json::Map<_,_>>();
    let original = entity.original_values().map(|values|values.iter().map(|(name,value)|(name.clone(),value.to_json_value())).collect::<serde_json::Map<_,_>>());
    let native = entity.into_values().into_iter().filter(|(name,_)|!name.starts_with('_'))
        .map(|(name,value)|(name,value.to_json_value())).collect::<serde_json::Map<_,_>>();
    json!({"type":T::entity_descriptor().name,"loaded":loaded,"original":original,"native":native})
}
fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let user_context = wait(cpp_inheritance_core::service_runtime_from_pool(pool.clone())).unwrap();
    wait(user_context.ensure_schema()).unwrap();
    pool.lock().unwrap().execute_batch("INSERT OR IGNORE INTO platform_data(id,name,version) VALUES(1,'root',3);
        INSERT INTO school_data(id,name,rank,platform,version) VALUES(7,'school',1,1,3),(8,'deleted school',2,1,-1),(9,'plain',3,1,3);
        INSERT INTO academy_data(id,name,rank,platform,version,campus_code) VALUES(7,'academy',7,1,4,'north'),(8,'deleted academy',8,1,-2,'south'),(10,'orphan academy',10,1,5,'orphan');
        INSERT INTO research_academy_data(id,name,rank,platform,version,campus_code,budget) VALUES(7,'research',17,1,5,'lab','12.50'),(8,'deleted research',18,1,-3,'old lab','0'),(11,'orphan research',11,1,5,'orphan lab','2.50');").unwrap();
    macro_rules! run {
        ($request:expr) => {{
            let mut request = $request.order_by_id_asc();
            if input["automatic"] == true { request = request.enhance_children_if_needed(); }
            if input["manual"] == true {
                request = request.enhance_child(Q::research_academies_minimal().select_budget().with_rank_greater_than(10_i64));
            }
            if input["limit"] == true { request = request.limit(1); }
            if input["failure"] == "type_filter" { request = request.match_types(["Academy", "ResearchAcademy"]); }
            if input["failure"] == "type_group" { request = request.with_type_group(); }
            if input["result"] == "stream" { request = request.stream(1); }
            let executable = request.comment("Inherited SQLite qualification").purpose("Compare actual descendant enhancement");
            if input["result"] == "one" {
                let entity = match wait(executable.execute_for_one(&user_context)) {
                    Ok(entity)=>entity,
                    Err(error)=>return json!({"error":error.to_string()}),
                };
                return json!({"rows":entity.into_iter().map(describe_return).collect::<Vec<_>>()});
            }
            if input["result"] == "stream" {
                let mut stream = match wait(executable.execute_for_stream(&user_context)) {
                    Ok(stream)=>stream,
                    Err(error)=>return json!({"error":error.to_string()}),
                };
                let mut rows = Vec::new();
                while let Some(entity) = wait(std::future::poll_fn(|context|stream.as_mut().poll_next(context))) {
                    match entity {
                        Ok(entity)=>rows.push(describe_return(entity)),
                        Err(error)=>return json!({"error":error.to_string()}),
                    }
                }
                return json!({"rows":rows});
            }
            let values = if input["result"] == "rows" {
                let rows = wait(executable.execute_for_rows(&user_context)).unwrap();
                rows.data.into_iter().map(|row|row.into_map().into_iter().map(|(name,value)|(name,value.to_json_value())).collect::<serde_json::Map<_,_>>()).collect::<Vec<_>>()
            } else {
                let entities = match wait(executable.execute_for_list(&user_context)) {
                    Ok(entities)=>entities,
                    Err(error)=>return json!({"error":error.to_string()}),
                };
                if input["return_type"].is_string() {
                    return json!({"rows":entities.data.into_iter().map(describe_return).collect::<Vec<_>>()});
                }
                entities.data.into_iter().map(|entity|entity.into_values().into_iter().filter(|(name,_)|!name.starts_with('_')).map(|(name,value)|(name,value.to_json_value())).collect::<serde_json::Map<_,_>>()).collect::<Vec<_>>()
            };
            json!({"rows":values})
        }};
    }
    macro_rules! adapt {
        ($request:expr) => {
            match input["return_type"].as_str() {
                Some("school")=>run!($request.return_type::<School>()),
                Some("academy")=>run!($request.return_type::<Academy>()),
                Some("research_academy")=>run!($request.return_type::<ResearchAcademy>()),
                _=>run!($request),
            }
        };
    }
    macro_rules! entry {
        ($normal:expr, $minimal:expr, $children:expr) => {
            match input["entry"].as_str().unwrap() { "minimal"=>$minimal, "children"=>$children, _=>$normal }
        };
    }
    match input["entity"].as_str().unwrap() {
        "school"=>adapt!(entry!(Q::schools(), Q::schools_minimal(), Q::schools_with_children())),
        "academy"=>adapt!(entry!(Q::academies(), Q::academies_minimal(), Q::academies_with_children())),
        _=>adapt!(entry!(Q::research_academies(), Q::research_academies_minimal(), Q::research_academies_with_children())),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
