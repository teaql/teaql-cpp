#[cfg(test)]
mod source_context_contracts_5_0_6 {
    use super::*;
    use serde_json::{Value as Json, json};
    use std::time::Duration;

    #[test]
    fn actual_context_observation_vectors() {
        let inputs: Vec<Json> = serde_json::from_str(
            &std::fs::read_to_string(std::env::var("TEAQL_STORE_CONTEXT_INPUTS").unwrap()).unwrap(),
        ).unwrap();
        let mut outputs = Vec::new();
        for input in inputs {
            assert_eq!(input["mode"], "observations");
            let user_context = UserContext::new();
            let mut observations = Vec::new();
            for step in input["steps"].as_array().unwrap() {
                let key = step["key"].as_str().unwrap_or("");
                match step["op"].as_str().unwrap() {
                    "continuous" => user_context.observe_continuous_page(
                        key, step["cursor"].as_str().map(str::to_owned)),
                    "ids" => user_context.observe_id_set(key, step["count"].as_u64()),
                    "read" => {},
                    _ => panic!("unknown retained observation operation"),
                }
                observations.push(json!({
                    "continuous": user_context.continuous_page_plan(),
                    "cursor": user_context.continuous_page_cursor_id(),
                    "id_plan": user_context.id_set_plan(),
                    "count": user_context.id_set_count(),
                }));
            }
            outputs.push(observations);
        }
        std::fs::write(
            std::env::var("TEAQL_STORE_CONTEXT_OUTPUTS").unwrap(),
            serde_json::to_vec(&outputs).unwrap(),
        ).unwrap();
    }

    #[tokio::test]
    async fn actual_context_store_registration_and_lock_identity() {
        let mut user_context = UserContext::new();
        assert_eq!(user_context.continuous_page_plan().as_deref(), Some("DISABLED"));
        assert_eq!(user_context.id_set_plan().as_deref(), Some("ID_SET_DISABLED"));
        assert!(user_context.continuous_page_cursor_id().is_none());
        assert!(user_context.id_set_count().is_none());
        let cursor_defaults = InMemoryContinuousPageCursorStore::default();
        let id_defaults = InMemoryIdSetStore::default();
        assert_eq!(cursor_defaults.max_entries, 4096);
        assert_eq!(id_defaults.max_entries, 64);
        assert_eq!(id_defaults.max_bytes, 256 * 1024 * 1024);
        let old_cursors: Arc<dyn ContinuousPageCursorStore> = Arc::new(cursor_defaults);
        let old_ids: Arc<dyn IdSetStore> = Arc::new(id_defaults);
        user_context.set_continuous_page_cursor_store(old_cursors.clone());
        user_context.set_id_set_store(old_ids.clone());
        let expiry = SystemTime::now() + Duration::from_secs(3600);
        user_context.continuous_page_cursor_store().put(ContinuousPageCursor {
            cursor_id: "actual-cursor".into(), query_key: "source-key".into(),
            entity: "Item".into(), direction: teaql_core::SortDirection::Asc,
            boundary: Value::Null, page_size: 10, next_offset: 10, expires_at: expiry,
        }).await.unwrap();
        user_context.id_set_store().put(RetainedIdSet {
            query_key: "source-key".into(), ids: Arc::new(vec![0, 7, u64::MAX]),
            expires_at: expiry,
        }).await.unwrap();
        assert_eq!(old_cursors.get("source-key", 10).await.unwrap().unwrap().cursor_id, "actual-cursor");
        assert_eq!(*old_ids.get("source-key").await.unwrap().unwrap().ids, [0, 7, u64::MAX]);
        user_context.set_continuous_page_cursor_store(Arc::new(InMemoryContinuousPageCursorStore::default()));
        user_context.set_id_set_store(Arc::new(InMemoryIdSetStore::default()));
        assert!(user_context.continuous_page_cursor_store().get("source-key", 10).await.unwrap().is_none());
        assert!(user_context.id_set_store().get("source-key").await.unwrap().is_none());
        assert!(old_cursors.get("source-key", 10).await.unwrap().is_some());
        assert!(old_ids.get("source-key").await.unwrap().is_some());
        let lock = user_context.id_set_build_lock("actual-source-lock-key");
        let other_user_context = UserContext::new();
        assert!(Arc::ptr_eq(&lock, &other_user_context.id_set_build_lock("actual-source-lock-key")));
        assert!(!Arc::ptr_eq(&lock, &user_context.id_set_build_lock("actual-source-other-key")));
    }
}
