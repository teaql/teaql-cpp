#[cfg(test)]
mod continuation_source_observer {
    use super::*;
    use serde_json::{Value as J, json};
    use std::sync::Arc;
    use teaql_core::{Record, Value};

    fn fact(input: &J, entity: &str, marker: &str) -> RawAuditEvent {
        let mut values = Record::from([("name".into(), Value::Text(marker.into()))]);
        match input["id_kind"].as_str().unwrap() {
            "u64" => { values.insert("id".into(), Value::U64(7)); },
            "negative" => { values.insert("id".into(), Value::I64(-7)); },
            "missing" => {},
            _ => unreachable!(),
        }
        let mut event = RawAuditEvent::created(entity, values);
        if input["new_values"] == false { event.new_values = None; }
        event
    }

    fn changes(input: &J) -> Vec<crate::EntityPropertyChange> {
        let change = |value| crate::EntityPropertyChange::new("#title", Some(Value::Null), value);
        match input["change"].as_str().unwrap() {
            "empty" => vec![],
            "remove" => vec![change(None)],
            "null" => vec![change(Some(Value::Null))],
            "text" => vec![change(Some(Value::Text("雪\0retained".into())))],
            "repeated" => vec![change(Some(Value::Text("first".into()))), change(None), change(Some(Value::Text("last".into())))],
            _ => unreachable!(),
        }
    }

    fn outcome(result: Result<(), RuntimeError>) -> J {
        match result {
            Ok(()) => json!({"ok":true}),
            Err(error) => json!({"ok":false,"debug":format!("{error:?}"),"display":error.to_string()}),
        }
    }

    async fn observe_enrich(input: &J) -> J {
        let owner = UserContext::default();
        let other = UserContext::default();
        let inspect = || {
            let scope_owner = has_scope(&owner);
            let scope_other = has_scope(&other);
            let returned = [fact(input, "Item", "first"), fact(input, "Item", "latest"), fact(input, "Other", "other entity")]
                .into_iter().map(|event| try_enqueue(&owner, event).is_some()).collect::<Vec<_>>();
            let target = if input["scope"] == "other" { &other } else { &owner };
            json!({"scope_owner":scope_owner,"scope_other":scope_other,"returned":returned,
                   "outcome":outcome(enrich_dynamic(target,"Item",7,changes(input)))})
        };
        let (observation, events) = if input["scope"] == "none" { (inspect(), vec![]) }
            else { collect(&owner, async { inspect() }).await };
        json!({"observation":observation,"queued":events.iter().map(|v|json!({"debug":format!("{v:?}"),"pretty":format!("{v:#?}")})).collect::<Vec<_>>(),
               "scope_after":has_scope(&owner)})
    }

    struct Sink { mask: u64, seen: Arc<Mutex<Vec<u64>>> }
    impl crate::RawAuditEventSink for Sink {
        fn on_event(&self, _: &UserContext, event: &RawAuditEvent) -> Result<(),RuntimeError> {
            let id = event.values["id"].try_u64().unwrap();
            self.seen.lock().unwrap().push(id);
            if self.mask & (1 << (id-1)) != 0 { Err(RuntimeError::Event(format!("sink failure {id}"))) }
            else { Ok(()) }
        }
    }

    async fn observe_deliver(input: &J) -> J {
        let mut owner = UserContext::default();
        let seen = Arc::new(Mutex::new(vec![]));
        owner.set_event_sink(Sink { mask:input["mask"].as_u64().unwrap(), seen:seen.clone() });
        let mut events = vec![];
        for id in 1..=input["count"].as_u64().unwrap() {
            events.push(RawAuditEvent::created("Item",Record::from([("id".into(),Value::U64(id))])));
        }
        let (result, queued) = collect(&owner, async { deliver(&owner,events) }).await;
        json!({"outcome":outcome(result),"seen":*seen.lock().unwrap(),"queued":queued.len(),"scope_after":has_scope(&owner)})
    }

    #[tokio::test]
    async fn complete_source_observations() {
        let inputs: Vec<J> = serde_json::from_str(&std::fs::read_to_string(std::env::var("TEAQL_COMMIT_AUDIT_INPUTS").unwrap()).unwrap()).unwrap();
        let mut outputs = vec![];
        for input in inputs {
            outputs.push(if input["kind"] == "enrich" { observe_enrich(&input).await } else { observe_deliver(&input).await });
        }
        std::fs::write(std::env::var("TEAQL_COMMIT_AUDIT_OUTPUTS").unwrap(),serde_json::to_vec(&outputs).unwrap()).unwrap();
    }
}
