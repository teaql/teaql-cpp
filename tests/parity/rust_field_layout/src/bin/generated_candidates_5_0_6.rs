use cpp_candidates_core::{E, Person, Q, Record};
use serde_json::{Value as Json, json};
use std::{
    collections::BTreeMap,
    future::Future,
    io::{self, BufRead},
    panic::{AssertUnwindSafe, catch_unwind},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::{CompactRow, Entity, TeaqlEntity, Value};
use teaql_runtime::{EntityRuntimeState, JsonFieldNamingProfile, LedgerEntity, RuntimeError};

struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
fn wait<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(value) = future.as_mut().poll(&mut context) {
            return value;
        }
    }
}
fn expression(call: impl FnOnce() -> Option<bool>) -> Json {
    match catch_unwind(AssertUnwindSafe(call)) {
        Ok(Some(value)) => json!({"state":"Value","value":value}),
        Ok(None) => json!({"state":"Null"}),
        Err(error) => {
            let message = error
                .downcast_ref::<String>()
                .map(String::as_str)
                .or_else(|| error.downcast_ref::<&str>().copied())
                .unwrap();
            let property = |prefix: &str| {
                message
                    .lines()
                    .find_map(|line| line.strip_prefix(prefix))
                    .unwrap()
            };
            let node: String = serde_json::from_str(property("break_point: ")).unwrap();
            let path: Vec<String> = serde_json::from_str(property("access_path: ")).unwrap();
            json!({"state":"NotLoaded","node":node,"path":path.join(".")})
        }
    }
}
macro_rules! view {
    ($entity:expr,$entry:ident)=>{{
        let entity=$entity;
        json!({"id":entity.status_id(), "loaded":entity.is_loaded("status_id"),
            "flags":[entity.status_is_draft(),entity.status_is_in_progress(),entity.status_is_done()],
            "expressions":[expression(||E::$entry(entity).status_is_draft().eval()),
                expression(||E::$entry(entity).status_is_in_progress().eval()),
                expression(||E::$entry(entity).status_is_done().eval())],
            "changed":entity.changed_status_id().map(|value|value.to_json_value())})
    }};
}
fn model(input: &Json) -> Json {
    macro_rules! run {
        ($model:ty,$entry:ident)=>{{
            let mut row=BTreeMap::from([("id".into(),Value::U64(7)),("version".into(),Value::I64(3)),
                ("name".into(),Value::Text("original".into())),("platform_id".into(),Value::U64(1)),
                ("phase".into(),Value::Text("Draft".into()))]);
            if input["loaded"]==true {row.insert("status_id".into(),Value::U64(input["id"].as_u64().unwrap()));}
            let mut entity=<$model>::from_compact_row(CompactRow::from_map(row)).unwrap();
            entity.attach_runtime_state_recursive(EntityRuntimeState::default());
            let copy=entity.clone(); let before=view!(&entity,$entry);
            for _ in 0..if input["repeat"]==true {2} else {1} {
                match input["candidate"].as_str().unwrap() {
                    "draft"=>{entity.update_status_to_draft();},
                    "in_progress"=>{entity.update_status_to_in_progress();},
                    "done"=>{entity.update_status_to_done();}, _=>{},
                }
            }
            json!({"before":before,"after":view!(&entity,$entry),"copy":view!(&copy,$entry)})
        }};
    }
    if input["model"] == "person" {
        run!(Person, person)
    } else {
        run!(Record, record)
    }
}
macro_rules! candidate_filter {
    ($request:expr,$input:expr; $s1:ident,$n1:ident,$s2:ident,$n2:ident,$s3:ident,$n3:ident;
        $p1:ident,$q1:ident,$p2:ident,$q2:ident,$p3:ident,$q3:ident) => {{
        let input = $input;
        let request = $request;
        match (
            input["field"].as_str().unwrap(),
            input["candidate"].as_str().unwrap(),
            input["negative"] == true,
        ) {
            ("status", "draft", false) => request.$s1(),
            ("status", "draft", true) => request.$n1(),
            ("status", "in_progress", false) => request.$s2(),
            ("status", "in_progress", true) => request.$n2(),
            ("status", "done", false) => request.$s3(),
            ("status", "done", true) => request.$n3(),
            ("phase", "draft", false) => request.$p1(),
            ("phase", "draft", true) => request.$q1(),
            ("phase", "in_progress", false) => request.$p2(),
            ("phase", "in_progress", true) => request.$q2(),
            ("phase", "done", false) => request.$p3(),
            ("phase", "done", true) => request.$q3(),
            _ => unreachable!(),
        }
    }};
}
macro_rules! record_filter {
    ($request:expr,$input:expr)=>{{
        if $input["field"]=="tag" {
            match($input["candidate"].as_str().unwrap(),$input["negative"]==true) {
                ("red",false)=>$request.with_tag_is_red(),("red",true)=>$request.with_tag_is_not_red(),
                ("blue",false)=>$request.with_tag_is_blue(),("blue",true)=>$request.with_tag_is_not_blue(),
                ("null",false)=>$request.with_tag_is_null(),("null",true)=>$request.with_tag_is_not_null(),_=>unreachable!(),
            }
        } else if $input["field"] == "tag_nullability" {
            if $input["candidate"] == "unknown" { $request.with_tag_is_unknown() } else { $request.with_tag_is_known() }
        } else {
            candidate_filter!($request,$input;
                with_status_is_draft,with_status_is_not_draft,with_status_is_in_progress,with_status_is_not_in_progress,with_status_is_done,with_status_is_not_done;
                with_phase_is_draft,with_phase_is_not_draft,with_phase_is_in_progress,with_phase_is_not_in_progress,with_phase_is_done,with_phase_is_not_done)
        }
    }};
}
macro_rules! person_filter {
    ($request:expr,$input:expr)=>{
        candidate_filter!($request,$input;
            whose_status_is_draft,whose_status_is_not_draft,whose_status_is_in_progress,whose_status_is_not_in_progress,whose_status_is_done,whose_status_is_not_done;
            whose_phase_is_draft,whose_phase_is_not_draft,whose_phase_is_in_progress,whose_phase_is_not_in_progress,whose_phase_is_done,whose_phase_is_not_done)
    };
}
fn query(input: &Json) -> Json {
    macro_rules! run {
        ($request:expr,$apply:ident)=>{{
            let mut request=$request;
            for _ in 0..if input["repeat"]==true {2} else {1} {request=$apply!(request,input);}
            let query=request.into_query();
            json!({"filter":query.filter.map(|value|format!("{value:?}")), "projection":query.projection})
        }};
    }
    if input["model"] == "person" {
        run!(
            if input["entry"] == "minimal" {
                Q::people_minimal()
            } else {
                Q::people()
            },
            person_filter
        )
    } else {
        run!(
            if input["entry"] == "minimal" {
                Q::records_minimal()
            } else {
                Q::records()
            },
            record_filter
        )
    }
}
fn sqlite(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let user_context = wait(cpp_candidates_core::service_runtime_from_pool(pool.clone())).unwrap();
    wait(user_context.ensure_schema()).unwrap();
    pool.lock().unwrap().execute_batch("INSERT INTO order_status_data(id,name,code,platform,version) VALUES(999,'Unknown fixture','OTHER',1,1);
        INSERT INTO record_data(id,name,phase,tag,platform,status,version) VALUES(7,'draft','Draft','red',1,11,3),(8,'next','InProgress','blue',1,22,4),(9,'done','done',NULL,1,2104194,5),(10,'unknown','other',NULL,1,999,6),(11,'literal','other','null',1,11,7);
        INSERT INTO person_data(id,name,phase,platform,status,version) VALUES(7,'draft','Draft',1,11,3),(8,'next','InProgress',1,22,4),(9,'done','done',1,2104194,5),(10,'unknown','other',1,999,6);").unwrap();
    if input["op"] == "save" {
        macro_rules! save {
            ($request:expr,$model:ty,$id:ident)=>{{
                let mut entity=wait($request.$id(7_u64).comment("Read candidate mutation target").purpose("Prepare candidate update").execute_for_one(&user_context)).unwrap().unwrap();
                let root=entity.entity_runtime_state().unwrap();let key=entity.entity_key();
                match input["candidate"].as_str().unwrap() {"draft"=>{entity.update_status_to_draft();},"in_progress"=>{entity.update_status_to_in_progress();},_=>{entity.update_status_to_done();},}
                let table=<$model>::entity_descriptor().table_name;
                if input["fault"]=="conflict" {pool.lock().unwrap().execute(&format!("UPDATE {table} SET version=version+1 WHERE id=7"),[]).unwrap();}
                if input["fault"]=="trigger" {pool.lock().unwrap().execute_batch(&format!("CREATE TRIGGER fixture_failure BEFORE UPDATE ON {table} BEGIN SELECT RAISE(ABORT,'candidate write failed'); END;")).unwrap();}
                let database=|| {let connection=pool.lock().unwrap();let mut statement=connection.prepare(&format!("SELECT id,status,version FROM {table} ORDER BY id")).unwrap();
                    let rows=statement.query_map([],|row|Ok([row.get::<_,i64>(0)?,row.get::<_,i64>(1)?,row.get::<_,i64>(2)?])).unwrap();json!(rows.map(Result::unwrap).collect::<Vec<_>>())};
                let before=database();let mut saved=None;let mut error=None;let mut diagnostics=Vec::new();
                match wait(teaql_runtime::save_audited_ledger_entity(entity.audit_as("Set known model candidate"),&user_context)) {
                    Ok(entity)=>saved=Some(json!(entity.into_values().into_iter().filter(|(name,_)|!name.starts_with('_')).map(|(name,value)|(name,value.to_json_value())).collect::<serde_json::Map<_,_>>())),
                    Err(value)=>{error=Some(value.to_string());if let RuntimeError::Check(values)=value {diagnostics=values.iter().map(|v|v.to_wire(JsonFieldNamingProfile::SnakeCase)).collect();}},
                }
                return json!({"saved":saved,"error":error,"diagnostics":diagnostics,"before":before,"after":database(),"pending":root.get(&key,"status_id").is_some()});
            }};
        }
        if input["model"] == "person" {
            save!(
                if input["entry"] == "minimal" {
                    Q::people_minimal()
                } else {
                    Q::people()
                },
                Person,
                whose_id_is
            )
        } else {
            save!(
                if input["entry"] == "minimal" {
                    Q::records_minimal()
                } else {
                    Q::records()
                },
                Record,
                with_id_is
            )
        }
    }
    macro_rules! run {
        ($request:expr,$apply:ident)=>{{
            let request=$apply!($request,input);
            let result=wait(request.order_by_id_asc().comment("Candidate equality and inequality").purpose("Read candidate rows").execute_for_list(&user_context));
            match result {Ok(rows)=>json!({"ids":rows.data.iter().map(|entity|entity.id()).collect::<Vec<_>>(),"error":null}),Err(error)=>json!({"ids":[],"error":error.to_string()})}
        }};
    }
    if input["model"] == "person" {
        run!(
            if input["entry"] == "minimal" {
                Q::people_minimal()
            } else {
                Q::people()
            },
            person_filter
        )
    } else {
        run!(
            if input["entry"] == "minimal" {
                Q::records_minimal()
            } else {
                Q::records()
            },
            record_filter
        )
    }
}

fn main() {
    std::panic::set_hook(Box::new(|info| {
        if !info.to_string().contains("Coding Logic Bug") {
            eprintln!("{info}");
        }
    }));
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let output = match input["op"].as_str().unwrap() {
            "model" => model(&input),
            "query" => query(&input),
            _ => sqlite(&input),
        };
        println!("{output}");
    }
}
