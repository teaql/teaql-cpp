//! Calls retained generated Checker and audited-save bodies in the derived fixture.
use cpp_inheritance_business_ids_core::*;
use serde_json::{json, Value as Json};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::{Entity, TeaqlEntity};
use teaql_runtime::{JsonFieldNamingProfile, RuntimeError};

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

trait OwnFields {
    fn update_own_fields(&mut self);
}
impl OwnFields for Document {
    fn update_own_fields(&mut self) {}
}
impl OwnFields for IntermediateRecord {
    fn update_own_fields(&mut self) {}
}
impl OwnFields for Invoice {
    fn update_own_fields(&mut self) {
        self.update_amount("12.50".parse::<teaql_core::Decimal>().unwrap());
    }
}
impl OwnFields for Receipt {
    fn update_own_fields(&mut self) {
        self.update_status("open");
    }
}
impl OwnFields for SpecialInvoice {
    fn update_own_fields(&mut self) {
        self.update_amount("12.50".parse::<teaql_core::Decimal>().unwrap())
            .update_reference("reference");
    }
}

fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let mut user_context = wait(service_runtime_from_pool(pool.clone())).unwrap();
    user_context.disable_sql_log();
    user_context.set_business_clock(teaql_runtime::FixedBusinessClock::new(
        chrono::DateTime::from_timestamp_millis(1_700_000_000_000).unwrap(),
    ));
    wait(user_context.ensure_schema()).unwrap();
    macro_rules! save {
        ($request:expr, $model:ty) => {{
            let mut model = $request
                .comment("Create inherited numbered model")
                .purpose("Qualify inherited Business ID governance")
                .new_entity(&user_context);
            model.update_name(input["name"].as_str().unwrap())
                .update_active(false).update_note(teaql_core::Value::Null);
            model.update_own_fields();
            if let Some(number) = input["number"].as_str() {
                model.update_document_number(DocumentNumber::new(number).unwrap());
            }
            let table = <$model>::entity_descriptor().table_name;
            if input["fault"] == "trigger" {
                pool.lock().unwrap().execute_batch(&format!(
                    "CREATE TRIGGER fixture_failure BEFORE INSERT ON {table} BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;"
                )).unwrap();
            }
            let result = wait(teaql_runtime::save_audited_ledger_entity(
                model.audit_as("Save inherited numbered model"), &user_context));
            let (saved, error, diagnostics) = match result {
                Ok(model) => (Some(json!({
                    "number": model.document_number().as_str(),
                    "tenant": model.tenant_id(),
                    "clock_fixed": model.created_at() == model.updated_at(),
                    "version": model.version(),
                })), None, vec![]),
                Err(RuntimeError::Check(results)) => (None, Some("Check".to_owned()),
                    results.iter().map(|v|v.to_wire(JsonFieldNamingProfile::SnakeCase))
                        .collect::<Vec<_>>()),
                Err(error) => (None, Some(error.to_string()), vec![]),
            };
            let connection = pool.lock().unwrap();
            let count: i64 = connection.query_row(&format!("SELECT COUNT(*) FROM {table}"), [],
                |row| row.get(0)).unwrap();
            let mut statement = connection.prepare(&format!(
                "SELECT document_number,tenant,version FROM {table} ORDER BY id")).unwrap();
            let rows = statement.query_map([], |row| Ok(json!({
                "number": row.get::<_, String>(0)?,
                "tenant": row.get::<_, i64>(1)?,
                "version": row.get::<_, i64>(2)?,
            }))).unwrap().collect::<Result<Vec<_>, _>>().unwrap();
            json!({"model": stringify!($model), "saved": saved, "error": error,
                "diagnostics": diagnostics, "row_count": count, "rows": rows,
                "fixes": user_context.last_fix_evidence().iter().map(|fix| json!({
                    "entity": fix.entity_type, "field": fix.model_path,
                    "source": format!("{:?}", fix.source), "label": fix.source_label,
                })).collect::<Vec<_>>()})
        }};
    }
    match input["model"].as_str().unwrap() {
        "document" => save!(Q::documents(), Document),
        "intermediate_record" => save!(Q::intermediate_records(), IntermediateRecord),
        "invoice" => save!(Q::invoices(), Invoice),
        "receipt" => save!(Q::receipts(), Receipt),
        "special_invoice" => save!(Q::special_invoices(), SpecialInvoice),
        _ => panic!("Unknown inherited Business ID model"),
    }
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
