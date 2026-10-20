//! Stateful metadata provider fixture around unchanged original public execution.
#[path = "rust_values/src/fixture_value.rs"]
mod fixture_value;
mod original {
    include!(concat!(env!("CARGO_MANIFEST_DIR"), "/../rust_field_layout/src/bin/sql_execution_5_0_6.rs"));
    use teaql_data_service::QueryExecutor;

    struct ChangingSchema {
        first: Arc<EntityDescriptor>,
        next: Arc<EntityDescriptor>,
        switched: std::sync::atomic::AtomicBool,
        lookups: Mutex<Vec<String>>,
    }
    impl SchemaProvider for ChangingSchema {
        fn get_entity(&self, name: &str) -> Option<Arc<EntityDescriptor>> {
            self.lookups.lock().unwrap().push(name.into());
            let entity = if self.switched.load(std::sync::atomic::Ordering::Relaxed) {
                &self.next
            } else {
                &self.first
            };
            (name == entity.name).then(|| entity.clone())
        }
    }

    pub fn observe(input: J) -> J {
        let mut first = fixture_sql::entity(&input["entity"]);
        first.audit_mask_fields = if input["masked"] == true { vec!["name".into()] } else { vec![] };
        let mut next = first.clone();
        next.audit_mask_fields = if input["masked"] == true { vec![] } else { vec!["name".into()] };
        let executor = SqlDataServiceExecutor::new(
            fixture_sql::FixtureDialect { numbered: true },
            Transport { steps: vec![], calls: Mutex::new(vec![]) },
            ChangingSchema { first: Arc::new(first), next: Arc::new(next),
                switched: Default::default(), lookups: Mutex::new(vec![]) });
        if input["prewarm"] == true {
            let query = SelectQuery::new("Account").filter(Expr::eq("name", "BEFORE")).limit(10);
            executor.query_log_intent(&query);
        }
        executor.schema_provider.switched.store(true, std::sync::atomic::Ordering::Relaxed);
        let request = mutation(&input["request"], "Account");
        let mut out = if input["transaction"] == true {
            let tx = ready(executor.begin()).unwrap();
            poll(tx.mutate(request))
        } else {
            poll(executor.mutate(request))
        };
        out["lookups"] = json!(*executor.schema_provider.lookups.lock().unwrap());
        out["calls"] = json!(*executor.transport.calls.lock().unwrap());
        out
    }
}

fn main() {
    use std::io::{self, BufRead};
    for line in io::stdin().lock().lines() {
        println!("{}", original::observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
