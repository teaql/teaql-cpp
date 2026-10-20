//! Caller fixtures for the unchanged original public raw-event sink registry.
fn main() {}

#[cfg(test)]
mod tests {
    use std::collections::BTreeMap;
    use std::panic::{AssertUnwindSafe, catch_unwind};
    use std::sync::{Arc, Mutex};
    use teaql_runtime::{
        InMemoryRawAuditEventSink, RawAuditEvent, RawAuditEventSink, RuntimeError, UserContext,
    };

    struct Sink {
        calls: Arc<Mutex<Vec<u8>>>,
        id: u8,
        fail: bool,
        panic: bool,
    }

    impl RawAuditEventSink for Sink {
        fn on_event(&self, _: &UserContext, event: &RawAuditEvent) -> Result<(), RuntimeError> {
            assert_eq!(event.entity, "Item");
            self.calls.lock().unwrap().push(self.id);
            if self.panic {
                panic!("controlled sink panic");
            }
            if self.fail {
                return Err(RuntimeError::Event("controlled sink failure".into()));
            }
            Ok(())
        }
    }

    fn event() -> RawAuditEvent {
        RawAuditEvent::created("Item", BTreeMap::new())
    }

    fn sink(calls: &Arc<Mutex<Vec<u8>>>, id: u8) -> Sink {
        Sink {
            calls: calls.clone(),
            id,
            fail: false,
            panic: false,
        }
    }

    #[test]
    fn default_and_new_empty_registries_succeed() {
        let context = UserContext::new();
        InMemoryRawAuditEventSink::default()
            .on_event(&context, &event())
            .unwrap();
        InMemoryRawAuditEventSink::new()
            .on_event(&context, &event())
            .unwrap();
    }

    #[test]
    fn registration_order_and_duplicates_are_preserved() {
        let calls = Arc::new(Mutex::new(Vec::new()));
        let mut registry = InMemoryRawAuditEventSink::new();
        for id in [1, 3, 1] {
            registry.register(sink(&calls, id));
        }
        registry.on_event(&UserContext::new(), &event()).unwrap();
        assert_eq!(*calls.lock().unwrap(), [1, 3, 1]);
    }

    #[test]
    fn first_result_error_stops_later_sinks() {
        let calls = Arc::new(Mutex::new(Vec::new()));
        let mut failure = sink(&calls, 2);
        failure.fail = true;
        let registry = InMemoryRawAuditEventSink::new()
            .with_sink(sink(&calls, 1))
            .with_sink(failure)
            .with_sink(sink(&calls, 3));
        let error = registry
            .on_event(&UserContext::new(), &event())
            .unwrap_err();
        assert!(
            matches!(error, RuntimeError::Event(message) if message == "controlled sink failure")
        );
        assert_eq!(*calls.lock().unwrap(), [1, 2]);
    }

    #[test]
    fn registry_clone_shares_sinks_but_not_the_registration_vector() {
        let calls = Arc::new(Mutex::new(Vec::new()));
        let original = InMemoryRawAuditEventSink::new().with_sink(sink(&calls, 1));
        let clone = original.clone().with_sink(sink(&calls, 2));
        original.on_event(&UserContext::new(), &event()).unwrap();
        clone.on_event(&UserContext::new(), &event()).unwrap();
        assert_eq!(*calls.lock().unwrap(), [1, 1, 2]);
    }

    #[test]
    fn callback_panic_propagates_before_later_sinks() {
        let calls = Arc::new(Mutex::new(Vec::new()));
        let mut failure = sink(&calls, 1);
        failure.panic = true;
        let registry = InMemoryRawAuditEventSink::new()
            .with_sink(failure)
            .with_sink(sink(&calls, 2));
        assert!(
            catch_unwind(AssertUnwindSafe(|| {
                registry.on_event(&UserContext::new(), &event()).unwrap();
            }))
            .is_err()
        );
        assert_eq!(*calls.lock().unwrap(), [1]);
    }
}
