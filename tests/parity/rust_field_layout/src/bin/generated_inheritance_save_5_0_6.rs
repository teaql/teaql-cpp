use cpp_inheritance_core::{Academy, ResearchAcademy, School, Q};
use serde_json::{json, Value as Json};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::{Entity, TeaqlEntity};
use teaql_runtime::{JsonFieldNamingProfile, LedgerEntity, RuntimeError};

struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
fn wait<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(result) = future.as_mut().poll(&mut context) {
            return result;
        }
    }
}
fn native<T: Entity>(entity: T) -> Json {
    json!(entity
        .into_values()
        .into_iter()
        .filter(|(name, _)| !name.starts_with('_'))
        .map(|(name, value)| (name, value.to_json_value()))
        .collect::<serde_json::Map<_, _>>())
}
trait OwnFields {
    fn update_own_fields(&mut self);
}
impl OwnFields for School {
    fn update_own_fields(&mut self) {}
}
impl OwnFields for Academy {
    fn update_own_fields(&mut self) {
        self.update_campus_code("updated campus");
    }
}
impl OwnFields for ResearchAcademy {
    fn update_own_fields(&mut self) {
        self.update_campus_code("updated campus")
            .update_budget("-7.50".parse::<teaql_core::Decimal>().unwrap());
    }
}
fn database(connection: &rusqlite::Connection) -> Json {
    let mut output = serde_json::Map::new();
    for table in [
        "platform_data",
        "school_data",
        "academy_data",
        "research_academy_data",
    ] {
        let campus = if table == "school_data" || table == "platform_data" {
            "NULL"
        } else {
            "campus_code"
        };
        let budget = if table == "research_academy_data" {
            "CAST(budget AS TEXT)"
        } else {
            "NULL"
        };
        let (rank, platform) = if table == "platform_data" {
            ("0", "id")
        } else {
            ("rank", "platform")
        };
        let mut statement=connection.prepare(&format!("SELECT id,name,{rank},{platform},version,{campus},{budget} FROM {table} ORDER BY id")).unwrap();
        let rows=statement.query_map([],|row|Ok(json!({"id":row.get::<_,i64>(0)?,"name":row.get::<_,String>(1)?,"rank":row.get::<_,i64>(2)?,"platform":row.get::<_,i64>(3)?,"version":row.get::<_,i64>(4)?,"campus":row.get::<_,Option<String>>(5)?,"budget":row.get::<_,Option<String>>(6)?}))).unwrap();
        output.insert(
            table.into(),
            json!(rows.collect::<Result<Vec<_>, _>>().unwrap()),
        );
    }
    Json::Object(output)
}
fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let user_context = wait(cpp_inheritance_core::service_runtime_from_pool(
        pool.clone(),
    ))
    .unwrap();
    wait(user_context.ensure_schema()).unwrap();
    pool.lock().unwrap().execute_batch("UPDATE platform_data SET name='root',version=3 WHERE id=1;
        INSERT INTO school_data(id,name,rank,platform,version) VALUES(7,'school',1,1,3),(9,'plain',3,1,3);
        INSERT INTO academy_data(id,name,rank,platform,version,campus_code) VALUES(7,'academy',7,1,4,'north');
        INSERT INTO research_academy_data(id,name,rank,platform,version,campus_code,budget) VALUES(7,'research',17,1,5,'lab','12.50');").unwrap();
    if input["operation"] == "create" {
        return create_observe(input, pool, user_context);
    }
    macro_rules! save {
        ($request:expr, $model:ty) => {{
            let mut request=$request;
            if input["related"]==true {request=request.select_platform();}
            let entity=wait(request.return_type::<$model>().with_id_is(input["id"].as_u64().unwrap())
                .comment("Inherited save source view").purpose("Qualify inherited graph save")
                .execute_for_one(&user_context)).unwrap().unwrap();
            let original=entity.original_values().map(|values|values.iter().map(|(name,value)|(name.clone(),value.to_json_value())).collect::<serde_json::Map<_,_>>());
            let root=entity.entity_runtime_state().unwrap();
            let related_view=json!({"loaded":entity.is_loaded("platform"),"present":entity.platform().is_some()});
            let mut entity=entity;
            let deleting=input["operation"]=="delete";
            if deleting { entity.mark_for_deletion(); }
            else {
                entity.update_name(input["name"].as_str().unwrap()).update_rank(23_i64);
                if input["own_fields"] == true { entity.update_own_fields(); }
            }
            if input["related"]==true {
                if let Some(platform)=entity.platform() {
                    let mut platform=platform.clone();
                    platform.attach_runtime_state_recursive(root.clone());
                    platform.update_name("updated root");
                }
            }
            let before=native(entity.clone());
            let table=<$model>::entity_descriptor().table_name;
            {
                let connection=pool.lock().unwrap();
                if input["fault"]=="conflict" {
                    connection.execute(&format!("UPDATE {table} SET version=version+1 WHERE id=?1"),[input["id"].as_i64().unwrap()]).unwrap();
                }
                if input["fault"]=="trigger" {
                    connection.execute_batch(&format!("CREATE TRIGGER fixture_failure BEFORE UPDATE ON {table} BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;")).unwrap();
                }
            }
            let db_before=database(&pool.lock().unwrap());
            let deleted_before=root.is_marked_as_delete(&entity.entity_key());
            let result=wait(teaql_runtime::save_audited_ledger_entity(entity.clone().audit_as("Save inherited returned model"), &user_context));
            let mut error:Option<String>=None;
            let mut kind=None;
            let mut diagnostics=Vec::new();
            let mut saved=None;
            match result {
                Ok(entity)=>saved=Some(native(entity)),
                Err(RuntimeError::Check(results))=>{error=Some("Check".into());kind=Some("Check");diagnostics=results;},
                Err(other)=>{
                    kind=Some(match &other {RuntimeError::OptimisticLockConflict{..}=>"OptimisticLockConflict", RuntimeError::Graph(_)=>"Graph", RuntimeError::Policy(_)=>"Policy", RuntimeError::Transaction(_)=>"Transaction", _=>"Other"});
                    error=Some(match &other {
                        RuntimeError::OptimisticLockConflict{..}=>"OptimisticLockConflict".into(),
                        _ if other.to_string().contains("fixture write failed")=>"FixtureWriteFailure".into(),
                        _=>other.to_string(),
                    });
                }
            }
            let mut output=json!({"type":<$model>::entity_descriptor().name,"original":original,"before":before,"related_view":related_view,
                "saved":saved,"error":error,"kind":kind,
                "diagnostics":diagnostics.iter().map(|v|v.to_wire(JsonFieldNamingProfile::SnakeCase)).collect::<Vec<_>>(),
                "db_before":db_before,"db_after":database(&pool.lock().unwrap()),
                "pending":root.current_change_set().changes().values().flat_map(|fields|fields.keys().cloned()).collect::<std::collections::BTreeSet<_>>()});
            if deleting {
                output["deleted_before"]=json!(deleted_before);
                output["deleted_after"]=json!(root.is_marked_as_delete(&entity.entity_key()));
            }
            output
        }};
    }
    macro_rules! target {
        ($request:expr) => {
            match input["target"].as_str().unwrap() {
                "school" => save!($request, School),
                "academy" => save!($request, Academy),
                _ => save!($request, ResearchAcademy),
            }
        };
    }
    macro_rules! entry {
        ($normal:expr,$minimal:expr,$children:expr) => {
            match input["entry"].as_str().unwrap() {
                "minimal" => $minimal,
                "children" => $children,
                _ => $normal,
            }
        };
    }
    match input["source"].as_str().unwrap() {
        "school" => target!(entry!(
            Q::schools(),
            Q::schools_minimal(),
            Q::schools_with_children()
        )),
        "academy" => target!(entry!(
            Q::academies(),
            Q::academies_minimal(),
            Q::academies_with_children()
        )),
        _ => target!(entry!(
            Q::research_academies(),
            Q::research_academies_minimal(),
            Q::research_academies_with_children()
        )),
    }
}
fn create_observe(
    input: &Json,
    pool: Arc<Mutex<rusqlite::Connection>>,
    mut user_context: teaql_runtime::UserContext,
) -> Json {
    user_context.set_internal_id_generator(teaql_runtime::AtomicCounterIdGenerator::new(1000));
    let existing = input["secondary"] == "existing";
    let mut secondary = if existing {
        let mut secondary = wait(
            Q::schools()
                .with_id_is(9_u64)
                .purpose("Read associated row")
                .comment("Inherited graph creation")
                .execute_for_one(&user_context),
        )
        .unwrap()
        .unwrap();
        secondary.update_name("updated sibling").update_rank(11_i64);
        Some(secondary)
    } else if input["secondary"] == "new" {
        let mut secondary = Q::schools()
            .purpose("Create associated row")
            .comment("Inherited graph creation")
            .new_entity(&user_context);
        secondary.update_name("new sibling").update_rank(11_i64);
        Some(secondary)
    } else {
        None
    };
    macro_rules! create {
        ($request:expr, $model:ty) => {{
            let mut entity = $request.purpose("Create inherited model").comment("Inherited graph creation").new_entity(&user_context);
            entity.update_name(input["name"].as_str().unwrap()).update_rank(23_i64);
            entity.update_own_fields();
            let root = entity.entity_runtime_state().unwrap();
            if let Some(secondary) = &mut secondary { secondary.attach_runtime_state_recursive(root.clone()); }
            let before = native(entity.clone());
            let table = if input["fault"] == "related_trigger" { "school_data".to_string() } else { <$model>::entity_descriptor().table_name };
            if input["fault"] == "root_trigger" || input["fault"] == "related_trigger" {
                let action = if input["fault"] == "related_trigger" && existing { "UPDATE" } else { "INSERT" };
                pool.lock().unwrap().execute_batch(&format!("CREATE TRIGGER fixture_failure BEFORE {action} ON {table} BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;")).unwrap();
            }
            let db_before = database(&pool.lock().unwrap());
            let result = wait(teaql_runtime::save_audited_ledger_entity(entity.audit_as("Create inherited graph"), &user_context));
            let (saved, error, kind, diagnostics) = match result {
                Ok(entity) => (Some(native(entity)), None, None, Vec::new()),
                Err(RuntimeError::Check(results)) => (None, Some("Check".to_string()), Some("Check"), results),
                Err(error) => {
                    let kind = match &error { RuntimeError::Graph(_) => "Graph", RuntimeError::OptimisticLockConflict{..} => "OptimisticLockConflict", RuntimeError::Policy(_) => "Policy", RuntimeError::Transaction(_) => "Transaction", _ => "Other" };
                    let message = if error.to_string().contains("fixture write failed") { "FixtureWriteFailure".to_string() } else { error.to_string() };
                    (None, Some(message), Some(kind), Vec::new())
                }
            };
            json!({"type":<$model>::entity_descriptor().name,"before":before,"saved":saved,"error":error,"kind":kind,
                "diagnostics":diagnostics.iter().map(|v|v.to_wire(JsonFieldNamingProfile::SnakeCase)).collect::<Vec<_>>(),
                "db_before":db_before,"db_after":database(&pool.lock().unwrap()),
                "pending":root.current_change_set().changes().values().flat_map(|fields|fields.keys().cloned()).collect::<std::collections::BTreeSet<_>>()})
        }};
    }
    match input["target"].as_str().unwrap() {
        "school" => create!(Q::schools(), School),
        "academy" => create!(Q::academies(), Academy),
        _ => create!(Q::research_academies(), ResearchAcademy),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
