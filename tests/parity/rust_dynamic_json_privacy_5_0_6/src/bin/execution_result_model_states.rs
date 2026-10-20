//! Composed public query/Policy/execution calls; no replacement runtime algorithms.
use cpp_inheritance_graph_core::{Q,School,Academy,ResearchAcademy,Enrollment};
use serde_json::{json,Value as Json};
use std::{io::{self,BufRead},sync::{Arc,Mutex,atomic::{AtomicU64,Ordering}}};
use teaql_core::{Entity,TeaqlEntity,Expr,SelectQuery};
use teaql_runtime::{RequestPolicy,RuntimeError,UserContext};
#[path="../../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;
#[derive(Default)] struct Observation {calls:Mutex<Vec<Json>>,selects:AtomicU64}
struct Policy {mode:String,observation:Arc<Observation>}
impl RequestPolicy for Policy {
    fn enforce_select(&self,_:&UserContext,query:&mut SelectQuery)->Result<(),RuntimeError> {
        let before=format!("{query:?}");
        if self.mode=="restrict" {
            let id=match query.entity.as_str(){"Enrollment"=>21_u64,"Specialist"=>2,"Platform"=>1,_=>7};
            *query=query.clone().and_filter(Expr::eq("id",id));
        }
        let raw=query.raw_sql.is_some() || !query.raw_sql_search_criteria.is_empty() ||
            !query.raw_projections.is_empty() || !query.dynamic_properties.is_empty();
        self.observation.calls.lock().unwrap().push(json!({"before":before,"after":format!("{query:?}"),"raw":raw}));
        if self.mode=="deny" || (self.mode=="reject_raw" && raw) {
            return Err(RuntimeError::Policy("COMPOSED_QUERY_DENIED".into()));
        }
        Ok(())
    }
}
unsafe extern "C" fn database(data:*mut std::ffi::c_void,action:i32,_:*const std::ffi::c_char,_:*const std::ffi::c_char,_:*const std::ffi::c_char,_:*const std::ffi::c_char)->i32 {
    if action==rusqlite::ffi::SQLITE_SELECT {unsafe{&*(data as *const Observation)}.selects.fetch_add(1,Ordering::Relaxed);}
    rusqlite::ffi::SQLITE_OK
}
fn snapshot<T:Entity+TeaqlEntity>(entity:T)->Json {
    let mut loaded=serde_json::Map::new();
    let descriptor=T::entity_descriptor();
    for field in &descriptor.properties {loaded.insert(field.name.clone(),json!(entity.is_field_loaded(&field.name)));}
    for relation in &descriptor.relations {loaded.insert(relation.name.clone(),json!(entity.is_field_loaded(&relation.name)));}
    let computed=entity.dynamic_property("_computed").map(|value|format!("{value:?}"));
    json!({"loaded":loaded,"json":entity.into_json(),"computed":computed})
}
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_inherited_rows.sql")).unwrap();
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_privacy_values.sql")).unwrap();
    let observation=Arc::new(Observation::default());
    let mut user_context=driver::block_on(cpp_inheritance_graph_core::service_runtime_from_pool(pool.clone())).unwrap().with_active_root("Platform",1);
    user_context.set_request_policy(Policy{mode:input["policy"].as_str().unwrap().into(),observation:observation.clone()});
    if input["logging"]==false {user_context.disable_sql_log();}
    {let connection=pool.lock().unwrap();assert_eq!(unsafe{rusqlite::ffi::sqlite3_set_authorizer(connection.handle(),Some(database),Arc::as_ptr(&observation) as *mut std::ffi::c_void)},rusqlite::ffi::SQLITE_OK);}
    macro_rules! run {($query:expr,$facet:ident,$child:expr,$raw:ident,$field:expr,$table:expr)=>{{
        let mut query=$query.filter_with_json(if input["entity"]=="enrollment" {r#"{"school.name":"PRIVATE-SCHOOL-CANARY"}"#}else{r#"{"name":"PRIVATE-SCHOOL-CANARY"}"#});
        query=match input["shape"].as_str().unwrap() {
            "readonly_null"=>query.create_property_as("_computed","NULL"),
            "scalar_null"=>query.$raw("NULL"),
            "readonly"=>query.create_property_as("_computed","id + 7"),
            "raw_projection"=>query.$raw(format!("substr({},1,7)",$field)),
            "raw_filter"=>query.raw_sql_filter("id >= 7"),
            "raw_outer"=>query.raw_sql(format!("SELECT id,version FROM {} WHERE id=8",$table)),
            "facet"=>query.$facet("_composed",$child.select_name().comment("Inspect facet PRIVATE-SCHOOL-CANARY"),true),
            "grouped"=>query.group_by("id").aggregate_count("_metric"),
            _=>query,
        };
        query=match input["optimization"].as_str().unwrap() {
            "id_set"=>query.optimize_pagination_with_id_set_config("composed_query",60,100),
            "continuous"=>query.optimize_for_continuous_page_fetch_with("composed_query",60),
            _=>query,
        };
        if input["mode"]=="stream" {query=query.stream(1);}
        let core:SelectQuery=query.clone().into();
        let mut out=json!({"query":format!("{core:?}"),"rows":[],"scalar":null,"total":null,"error":null,"panic":null});
        if input["shape"]=="missing_table" {
            pool.lock().unwrap().execute_batch(&format!("ALTER TABLE {} RENAME TO hidden_{}",$table,$table)).unwrap();
        }
        // Setup statements are excluded from query execution observations.
        observation.selects.store(0,Ordering::Relaxed);
        let comment=if input["intent"]=="blank_comment" {""}else{"Inspect composed PRIVATE-SCHOOL-CANARY"};
        let purpose=if input["intent"]=="blank_purpose" {""}else{"Compare scope PRIVATE-SCHOOL-CANARY"};
        let execution=std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        let request=query.comment(comment).purpose(purpose);
        match input["mode"].as_str().unwrap() {
            "count"=>match driver::block_on(request.execute_for_count(&user_context)){Ok(value)=>out["scalar"]=json!(value),Err(error)=>out["error"]=json!(error.to_string())},
            "exists"=>match driver::block_on(request.execute_for_exists(&user_context)){Ok(value)=>out["scalar"]=json!(value),Err(error)=>out["error"]=json!(error.to_string())},
            "rows"=>match driver::block_on(request.execute_for_rows(&user_context)) {
                Ok(rows)=>out["rows"]=json!(rows.iter().map(|row|row.iter().map(|(key,value)|(key.to_string(),format!("{value:?}"))).collect::<std::collections::BTreeMap<_,_>>()).collect::<Vec<_>>()),
                Err(error)=>out["error"]=json!(error.to_string()),
            },
            "first"|"one"=> {
                let result=if input["mode"]=="first" {driver::block_on(request.execute_for_first(&user_context))} else {driver::block_on(request.execute_for_one(&user_context))};
                match result{Ok(value)=>out["rows"]=json!(value.map(snapshot).into_iter().collect::<Vec<_>>()),Err(error)=>out["error"]=json!(error.to_string())}
            },
            "stream"=>{
                let mut rows=Vec::new();
                match driver::block_on(request.execute_for_stream(&user_context)) {
                    Err(error)=>out["error"]=json!(error.to_string()),
                    Ok(mut stream)=> {
                        for _ in 0..2 {
                            match driver::block_on(std::future::poll_fn(|task|stream.as_mut().poll_next(task))) {
                                Some(Ok(value))=>rows.push(snapshot(value)),
                                Some(Err(error))=>{out["error"]=json!(error.to_string());break;},
                                None=>break,
                            }
                        }
                        drop(stream);
                    },
                }
                out["rows"]=json!(rows);
            },
            mode=> {
                let result=if mode=="page" {driver::block_on(request.execute_for_page(&user_context,0,1))} else {driver::block_on(request.execute_for_list(&user_context))};
                match result{Ok(rows)=>{out["total"]=json!(rows.total_count);out["rows"]=json!(rows.into_iter().map(snapshot).collect::<Vec<_>>());},Err(error)=>out["error"]=json!(error.to_string())}
            },
        }
        }));
        if let Err(panic)=execution {
            out["panic"]=json!(panic.downcast_ref::<String>().cloned().or_else(||panic.downcast_ref::<&str>().map(|value|value.to_string())).unwrap());
        }
        out["policy"]=json!(*observation.calls.lock().unwrap());
        out["selects"]=json!(observation.selects.load(Ordering::Relaxed));
        out["released"]=json!(pool.try_lock().is_ok());
        out["logs"]=json!(user_context.sql_logs().iter().map(|entry|json!({"sql":entry.sql,"params":entry.params.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>(),"comment":entry.comment,"purpose":entry.purpose,"debug_sql":entry.debug_sql,"pretty_sql":entry.pretty_sql,"context":format!("{:?}",entry.log_context),"trace":entry.trace_path.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>(),"result_count":entry.result_count,"result_type":entry.result_type,"affected_rows":entry.affected_rows,"summary":entry.result_summary})).collect::<Vec<_>>());
        {let connection=pool.lock().unwrap();unsafe{rusqlite::ffi::sqlite3_set_authorizer(connection.handle(),None,std::ptr::null_mut());}}
        out
    }};}
    macro_rules! target {($source:expr,$facet:ident,$child:expr,$raw:ident,$field:expr,$table:expr)=>{{
        let source=$source;
        match input["target"].as_str().unwrap() {
            "school"=>run!(source.return_type::<School>(),$facet,$child,$raw,$field,$table),
            "academy"=>run!(source.return_type::<Academy>(),$facet,$child,$raw,$field,$table),
            "research_academy"=>run!(source.return_type::<ResearchAcademy>(),$facet,$child,$raw,$field,$table),
            _=>run!(source.return_type::<Enrollment>(),$facet,$child,$raw,$field,$table),
        }
    }};}
    let minimal=input["minimal"]==true;
    match input["entity"].as_str().unwrap() {
        "school"=>target!(if minimal {Q::schools_minimal()} else {Q::schools()},facet_by_advisor_as_with_options,Q::specialists_minimal(),select_name_raw,"name","school_data"),
        "academy"=>target!(if minimal {Q::academies_minimal()} else {Q::academies()},facet_by_advisor_as_with_options,Q::specialists_minimal(),select_name_raw,"name","academy_data"),
        "research_academy"=>target!(if minimal {Q::research_academies_minimal()} else {Q::research_academies()},facet_by_advisor_as_with_options,Q::specialists_minimal(),select_name_raw,"name","research_academy_data"),
        _=>target!(if minimal {Q::enrollments_minimal()} else {Q::enrollments()},facet_by_school_as_with_options,Q::schools_minimal(),select_note_raw,"note","enrollment_data"),
    }
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){let input=serde_json::from_str(&line.unwrap()).unwrap();let out=std::panic::catch_unwind(||observe(&input)).unwrap_or_else(|panic|json!({"panic":panic.downcast_ref::<String>().cloned().or_else(||panic.downcast_ref::<&str>().map(|value|value.to_string())).unwrap()}));println!("{out}");}}
