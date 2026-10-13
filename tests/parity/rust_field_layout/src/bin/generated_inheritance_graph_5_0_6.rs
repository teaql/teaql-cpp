use cpp_inheritance_graph_core::{Academy, Q, ResearchAcademy, School};
use serde_json::{Value as Json, json};
use std::{future::Future, io::{self, BufRead}, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
use teaql_core::{Entity, TeaqlEntity};
use teaql_runtime::{JsonFieldNamingProfile, LedgerEntity, RuntimeError};

struct NoWake;
impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
fn wait<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop { if let Poll::Ready(value) = future.as_mut().poll(&mut context) { return value; } }
}
fn native<T: Entity>(entity: T) -> Json {
    json!(entity.into_values().into_iter().filter(|(name, _)| !name.starts_with('_'))
        .map(|(name, value)| (name, value.to_json_value())).collect::<serde_json::Map<_, _>>())
}
fn original<T: Entity>(entity: &T) -> Json {
    json!(entity.original_values().map(|values| values.iter()
        .map(|(name, value)| (name.clone(), value.to_json_value())).collect::<serde_json::Map<_, _>>()))
}
trait ReverseSummary { fn reverse_summary(&self) -> Json; }
impl ReverseSummary for School {
    fn reverse_summary(&self) -> Json {
        let view = self.enrollment_list();
        json!({"state":format!("{:?}",view.state()),"members":view.value().map(|v|v.data.iter().cloned().map(native).collect::<Vec<_>>())})
    }
}
impl ReverseSummary for Academy { fn reverse_summary(&self) -> Json { Json::Null } }
impl ReverseSummary for ResearchAcademy { fn reverse_summary(&self) -> Json { Json::Null } }
fn database(connection: &rusqlite::Connection) -> Json {
    let mut output = serde_json::Map::new();
    for table in ["platform_data", "specialist_data", "school_data", "academy_data", "research_academy_data", "enrollment_data"] {
        let name = if table == "enrollment_data" { "note" } else { "name" };
        let rank = if ["school_data", "academy_data", "research_academy_data"].contains(&table) { "rank" } else { "0" };
        let advisor = if rank == "rank" { "advisor" } else { "NULL" };
        let platform = if table == "platform_data" { "id" } else if table == "enrollment_data" { "0" } else { "platform" };
        let school = if table == "enrollment_data" { "school" } else { "NULL" };
        let active = if table == "specialist_data" { "active" } else { "NULL" };
        let campus = if ["academy_data", "research_academy_data"].contains(&table) { "campus_code" } else { "NULL" };
        let budget = if table == "research_academy_data" { "CAST(budget AS TEXT)" } else { "NULL" };
        let mut statement = connection.prepare(&format!("SELECT id,{name},version,{rank},{advisor},{platform},{school},{active},{campus},{budget} FROM {table} ORDER BY id")).unwrap();
        let rows = statement.query_map([], |row| Ok(json!({"id":row.get::<_, i64>(0)?, "name":row.get::<_, String>(1)?, "version":row.get::<_, i64>(2)?, "rank":row.get::<_, i64>(3)?, "advisor":row.get::<_, Option<i64>>(4)?, "platform":row.get::<_, i64>(5)?, "school":row.get::<_, Option<i64>>(6)?, "active":row.get::<_, Option<i64>>(7)?, "campus":row.get::<_, Option<String>>(8)?, "budget":row.get::<_, Option<String>>(9)?}))).unwrap();
        output.insert(table.into(), json!(rows.collect::<Result<Vec<_>, _>>().unwrap()));
    }
    Json::Object(output)
}
fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let user_context = wait(cpp_inheritance_graph_core::service_runtime_from_pool(pool.clone())).unwrap();
    wait(user_context.ensure_schema()).unwrap();
    pool.lock().unwrap().execute_batch("PRAGMA foreign_keys=OFF;
        UPDATE platform_data SET name='root',version=3 WHERE id=1;
        INSERT INTO specialist_data(id,name,platform,active,version) VALUES(2,'advisor',1,1,3);
        INSERT INTO school_data(id,name,rank,advisor,platform,version) VALUES(7,'school',1,2,1,3),(8,'nullable',2,NULL,1,3),(9,'missing',3,99,1,3);
        INSERT INTO academy_data(id,name,rank,advisor,platform,version,campus_code) VALUES(7,'academy',7,2,1,4,'north'),(8,'nullable',8,NULL,1,4,'south'),(9,'missing',9,99,1,4,'east');
        INSERT INTO research_academy_data(id,name,rank,advisor,platform,version,campus_code,budget) VALUES(7,'research',17,2,1,5,'lab','12.50'),(8,'nullable',18,NULL,1,5,'lab','10.50'),(9,'missing',19,99,1,5,'lab','8.50');
        INSERT INTO enrollment_data(id,school,note,version) VALUES(21,7,'student',3);
        PRAGMA foreign_keys=ON;").unwrap();
    if input["direction"] == "reverse" {
        pool.lock().unwrap().execute("INSERT INTO enrollment_data(id,school,note,version) VALUES(22,7,'second',3)", []).unwrap();
        return observe_reverse(input, pool, user_context);
    }
    macro_rules! run {
        ($request:expr, $model:ty) => {{
            let mut request = $request.select_advisor_with(Q::specialists().select_platform());
            if let Some(shortcut) = input["shortcut"].as_str() {
                request = match shortcut { "self_without_parent" => request.select_self_without_parent(), "all" => request.select_all(), "children" => request.select_children(), _ => request.select_any() };
            }
            let entity = wait(request
                .with_id_is(input["id"].as_u64().unwrap()).purpose("Qualify inherited relation graph")
                .comment("Inherited optional forward relation").execute_for_one(&user_context)).unwrap().unwrap();
            let mut entity = entity;
            let root = entity.entity_runtime_state().unwrap();
            let mut advisor = entity.advisor().cloned();
            let evaluation = match entity.eval_advisor() { teaql_core::eval::EvalResult::Value(_) => "Value", teaql_core::eval::EvalResult::Null => "Null", _ => "NotLoaded" };
            let mut before = json!({"root":native(entity.clone()),"original":original(&entity),"advisor":advisor.clone().map(native),"advisor_original":advisor.as_ref().map(original),"advisor_loaded":entity.is_loaded("advisor"),"evaluation":evaluation,"nested_platform":advisor.as_ref().and_then(|v|v.platform()).cloned().map(native)});
            if input["shortcut"].is_string() { before["shortcut_views"] = json!({"platform":entity.platform().cloned().map(native),"reverse":entity.reverse_summary()}); }
            let operation = input["operation"].as_str().unwrap();
            if operation == "delete" { entity.mark_for_deletion(); }
            if operation == "update" || operation == "related_update" || operation == "related_before" { entity.update_name("updated root"); }
            if operation == "clear" { entity.update_advisor_id(teaql_core::Value::Null); }
            if operation == "related_update" || operation == "related_before" {
                if let Some(advisor) = &mut advisor {
                    if operation == "related_before" { advisor.update_name("updated advisor").update_active(false); }
                    advisor.attach_runtime_state_recursive(root.clone());
                    if operation == "related_update" { advisor.update_name("updated advisor").update_active(false); }
                }
            }
            let fault = input["fault"].as_str().unwrap();
            let table = if fault.starts_with("advisor") { "specialist_data".to_string() } else { <$model>::entity_descriptor().table_name };
            let fault_id = if fault.starts_with("advisor") { 2 } else { input["id"].as_i64().unwrap() };
            if fault.ends_with("conflict") { pool.lock().unwrap().execute(&format!("UPDATE {table} SET version=version+1 WHERE id=?1"), [fault_id]).unwrap(); }
            if fault.ends_with("trigger") { pool.lock().unwrap().execute_batch(&format!("CREATE TRIGGER fixture_failure BEFORE UPDATE ON {table} BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;")).unwrap(); }
            let db_before = database(&pool.lock().unwrap());
            let mut saved = None;
            let mut error = None;
            let mut kind = None;
            let mut diagnostics = Vec::new();
            if operation != "read" {
                match wait(teaql_runtime::save_audited_ledger_entity(entity.clone().audit_as("Save inherited relation graph"), &user_context)) {
                    Ok(value) => saved = Some(native(value)),
                    Err(RuntimeError::Check(values)) => { error = Some("Check".to_string()); kind = Some("Check"); diagnostics = values; },
                    Err(value) => {
                        kind = Some(match &value { RuntimeError::OptimisticLockConflict{..} => "OptimisticLockConflict", RuntimeError::Graph(_) => "Graph", RuntimeError::Policy(_) => "Policy", RuntimeError::Transaction(_) => "Transaction", _ => "Other" });
                        error = Some(if kind == Some("OptimisticLockConflict") { "OptimisticLockConflict".to_string() } else if value.to_string().contains("fixture write failed") { "FixtureWriteFailure".to_string() } else { value.to_string() });
                    }
                }
            }
            json!({"before":before,"saved":saved,"error":error,"kind":kind,"diagnostics":diagnostics.iter().map(|v|v.to_wire(JsonFieldNamingProfile::SnakeCase)).collect::<Vec<_>>(),"db_before":db_before,"db_after":database(&pool.lock().unwrap()),"pending":root.current_change_set().changes().values().flat_map(|fields|fields.keys().cloned()).collect::<std::collections::BTreeSet<_>>(),"deleted":root.is_marked_as_delete(&entity.entity_key())})
        }};
    }
    macro_rules! entry { ($normal:expr,$minimal:expr,$children:expr) => { match input["entry"].as_str().unwrap() { "minimal"=>$minimal,"children"=>$children,_=>$normal } }; }
    match input["model"].as_str().unwrap() {
        "school" => run!(entry!(Q::schools(),Q::schools_minimal(),Q::schools_with_children()),School),
        "academy" => run!(entry!(Q::academies(),Q::academies_minimal(),Q::academies_with_children()),Academy),
        _ => run!(entry!(Q::research_academies(),Q::research_academies_minimal(),Q::research_academies_with_children()),ResearchAcademy),
    }
}
fn observe_reverse(input: &Json, pool: Arc<Mutex<rusqlite::Connection>>, user_context: teaql_runtime::UserContext) -> Json {
    let mut request = match input["entry"].as_str().unwrap() {
        "minimal" => Q::schools_minimal(), "children" => Q::schools_with_children(), _ => Q::schools()
    };
    request = match input["selection"].as_str().unwrap() {
        "none" => request,
        "filtered" => request.select_enrollment_list_with(Q::enrollments().with_id_is(999_u64)),
        "minimal" => request.select_enrollment_list_with(Q::enrollments_minimal()),
        "nested" => request.select_enrollment_list_with(Q::enrollments().select_school_with(Q::schools().select_advisor())),
        _ => request.select_enrollment_list(),
    };
    let mut entity = wait(request.with_id_is(input["id"].as_u64().unwrap())
        .purpose("Qualify reverse relation ledger").comment("Inherited School enrollment view")
        .execute_for_one(&user_context)).unwrap().unwrap();
    let root = entity.entity_runtime_state().unwrap();
    let view = entity.enrollment_list();
    let children = view.value().cloned();
    let before = json!({"root":native(entity.clone()), "original":original(&entity),
        "state":format!("{:?}",view.state()),
        "evaluation":match entity.eval_enrollment_list() { teaql_core::eval::EvalResult::Value(_) => "Value", _ => "NotLoaded" },
        "children":children.as_ref().map(|v|v.data.iter().map(|e|json!({"native":native(e.clone()),"original":original(e),"school":e.school().cloned().map(native)})).collect::<Vec<_>>())});
    let operation = input["operation"].as_str().unwrap();
    if operation != "read" { entity.update_name("updated owner"); }
    if let Some(mut children) = children {
        for child in &mut children.data {
            if operation == "children_before" || operation == "children_no_attach" { child.update_note("updated member"); }
            if operation == "children_delete" { child.mark_for_deletion(); }
            if operation == "children_before" || operation == "children_after" || operation == "children_delete" {
                child.attach_runtime_state_recursive(root.clone());
            }
            if operation == "children_after" { child.update_note("updated member"); }
        }
    }
    let fault = input["fault"].as_str().unwrap();
    let table = if fault.starts_with("root") { "school_data" } else { "enrollment_data" };
    let identity = if fault.starts_with("root") { input["id"].as_i64().unwrap() } else { 21_i64 };
    if fault.ends_with("conflict") { pool.lock().unwrap().execute(&format!("UPDATE {table} SET version=version+1 WHERE id=?1"), [identity]).unwrap(); }
    if fault.ends_with("trigger") {
        let condition = if fault == "second_trigger" { " WHEN OLD.id=22" } else { "" };
        pool.lock().unwrap().execute_batch(&format!("CREATE TRIGGER fixture_failure BEFORE UPDATE ON {table}{condition} BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;")).unwrap();
    }
    let db_before = database(&pool.lock().unwrap());
    let mut saved = None;
    let mut error = None;
    let mut kind = None;
    let mut diagnostics = Vec::new();
    if operation != "read" {
        match wait(teaql_runtime::save_audited_ledger_entity(entity.clone().audit_as("Save reverse relation ledger"), &user_context)) {
            Ok(value) => saved = Some(native(value)),
            Err(RuntimeError::Check(values)) => { error = Some("Check".to_string()); kind = Some("Check"); diagnostics = values; },
            Err(value) => {
                kind = Some(match &value { RuntimeError::OptimisticLockConflict{..} => "OptimisticLockConflict", RuntimeError::Graph(_) => "Graph", RuntimeError::Policy(_) => "Policy", RuntimeError::Transaction(_) => "Transaction", _ => "Other" });
                error = Some(if kind == Some("OptimisticLockConflict") { "OptimisticLockConflict".to_string() } else if value.to_string().contains("fixture write failed") { "FixtureWriteFailure".to_string() } else { value.to_string() });
            }
        }
    }
    json!({"before":before,"saved":saved,"error":error,"kind":kind,"diagnostics":diagnostics.iter().map(|v|v.to_wire(JsonFieldNamingProfile::SnakeCase)).collect::<Vec<_>>(),"db_before":db_before,"db_after":database(&pool.lock().unwrap()),"pending":root.current_change_set().changes().values().flat_map(|fields|fields.keys().cloned()).collect::<std::collections::BTreeSet<_>>(),"deleted_keys":root.deleted_keys().iter().map(|v|format!("{}:{}",v.entity,v.id.try_u64().unwrap())).collect::<Vec<_>>()})
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
