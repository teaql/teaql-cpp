#[allow(dead_code)]
mod retained {
    include!("rust_field_layout/src/bin/document_round_trip_5_0_6.rs");
    use std::fmt::Debug;
    fn model_debug<T: Debug + Clone + PartialEq>(value: &T) -> Json {
        json!({"debug":format!("{value:?}"), "pretty":format!("{value:#?}"), "clone_equal": value.clone() == *value})
    }
    fn changed_equal<T: Clone + PartialEq>(value: &T, change: impl FnOnce(&mut T)) -> bool {
        let mut copy = value.clone();
        change(&mut copy);
        *value == copy
    }
    pub fn model_observe(input: &Json) -> Json {
        let document_snapshot = snapshot(input).unwrap();
        let entity = document_snapshot.entities[0].clone();
        let relation = document_snapshot.relations[0].clone();
        let document = ContextualDocument { business_id:document_snapshot.business_id.clone(),
            document_token:input["token"].as_str().unwrap().into(), body:input["body"].clone() };
        let submitted = SubmittedContextualDocument { business_id:document_snapshot.business_id.clone(),
            document_token:input["token"].as_str().unwrap().into(), command_id:input["command"].as_str().unwrap().into(), body:input["body"].clone() };
        let open_request = DocumentOpenRequest { aggregate_type:document_snapshot.scope.aggregate_type.clone(), business_id:document_snapshot.business_id.clone(),
            purpose:document_snapshot.scope.purpose.clone(), lifetime:Duration::new(input["seconds"].as_u64().unwrap(),input["nanos"].as_u64().unwrap() as u32) };
        let accept_request = DocumentAcceptRequest { expected_aggregate_type:document_snapshot.scope.aggregate_type.clone(),
            purpose:document_snapshot.scope.purpose.clone(), submitted:submitted.clone() };
        let accepted = AcceptedContextualDocument { document:document.clone(), previous_revision:document_snapshot.aggregate.version,
            new_revision:document_snapshot.aggregate.version+1, changed_entities:document_snapshot.entities.iter().map(|e|e.identity.clone()).collect() };
        let verified = VerifiedDocumentSnapshot { snapshot:document_snapshot.clone(), key_id:input["key_id"].as_str().unwrap().into(),
            issued_at:chrono::DateTime::from_timestamp(input["issued"].as_i64().unwrap(),input["timestamp_nanos"].as_u64().unwrap() as u32).unwrap(),
            expires_at:chrono::DateTime::from_timestamp(input["now"].as_i64().unwrap(),input["timestamp_nanos"].as_u64().unwrap() as u32).unwrap() };
        let mut output=serde_json::Map::new();
        output.insert("entity".into(),model_debug(&entity));
        output.insert("entity_equality".into(),json!([
            changed_equal(&entity,|v|{ v.row_key.push_str("!"); }),
            changed_equal(&entity,|v|{ v.identity.entity_type.push_str("!"); }),
            changed_equal(&entity,|v|{ v.identity.id ^= 1; }),
            changed_equal(&entity,|v|{ v.identity.version ^= 1; }),
            changed_equal(&entity,|v|{ v.loaded_fields.clear(); }),
            changed_equal(&entity,|v|{ v.writable_fields.clear(); }),
            changed_equal(&entity,|v|{ v.allowed_mutations.clear(); }),
        ]));
        output.insert("relation".into(),model_debug(&relation));
        output.insert("relation_equality".into(),json!([
            changed_equal(&relation,|v|{ v.relation_name.push_str("!"); }),
            changed_equal(&relation,|v|{ v.row_keys.reverse(); }),
            changed_equal(&relation,|v|{ v.writable = !v.writable; }),
            changed_equal(&relation,|v|{ v.completeness = if v.completeness == DocumentRelationCompleteness::Complete { DocumentRelationCompleteness::Partial } else { DocumentRelationCompleteness::Complete }; }),
            changed_equal(&relation,|v|{ v.allowed_mutations.clear(); }),
        ]));
        output.insert("document_snapshot".into(),model_debug(&document_snapshot));
        output.insert("document_snapshot_equality".into(),json!([
            changed_equal(&document_snapshot,|v|{ v.model_fingerprint.push_str("!"); }),
            changed_equal(&document_snapshot,|v|{ v.business_id.push_str("!"); }),
            changed_equal(&document_snapshot,|v|{ v.scope.document_id.push_str("!"); }),
            changed_equal(&document_snapshot,|v|{ v.scope.purpose.push_str("!"); }),
            changed_equal(&document_snapshot,|v|{ v.scope.aggregate_type.push_str("!"); }),
            changed_equal(&document_snapshot,|v|{ v.scope.aggregate_id ^= 1; }),
            changed_equal(&document_snapshot,|v|{ v.scope.aggregate_revision ^= 1; }),
            changed_equal(&document_snapshot,|v|{ v.aggregate.entity_type.push_str("!"); }),
            changed_equal(&document_snapshot,|v|{ v.aggregate.id ^= 1; }),
            changed_equal(&document_snapshot,|v|{ v.aggregate.version ^= 1; }),
            changed_equal(&document_snapshot,|v|{ v.entities.pop(); }),
            changed_equal(&document_snapshot,|v|{ v.relations.pop(); }),
        ]));
        output.insert("document".into(),model_debug(&document));
        output.insert("document_equality".into(),json!([
            changed_equal(&document,|v|{ v.business_id.push_str("!"); }),
            changed_equal(&document,|v|{ v.document_token.push_str("!"); }),
            changed_equal(&document,|v|{ v.body = Json::Null; }),
        ]));
        output.insert("submitted".into(),model_debug(&submitted));
        output.insert("submitted_equality".into(),json!([
            changed_equal(&submitted,|v|{ v.business_id.push_str("!"); }),
            changed_equal(&submitted,|v|{ v.document_token.push_str("!"); }),
            changed_equal(&submitted,|v|{ v.command_id.push_str("!"); }),
            changed_equal(&submitted,|v|{ v.body = Json::Null; }),
        ]));
        output.insert("open_request".into(),model_debug(&open_request));
        output.insert("open_request_equality".into(),json!([
            changed_equal(&open_request,|v|{ v.aggregate_type.push_str("!"); }),
            changed_equal(&open_request,|v|{ v.business_id.push_str("!"); }),
            changed_equal(&open_request,|v|{ v.purpose.push_str("!"); }),
            changed_equal(&open_request,|v|{ v.lifetime = Duration::new(v.lifetime.as_secs() ^ 1, v.lifetime.subsec_nanos()); }),
            changed_equal(&open_request,|v|{ v.lifetime = Duration::new(v.lifetime.as_secs(), v.lifetime.subsec_nanos() ^ 1); }),
        ]));
        output.insert("accept_request".into(),model_debug(&accept_request));
        output.insert("accept_request_equality".into(),json!([
            changed_equal(&accept_request,|v|{ v.expected_aggregate_type.push_str("!"); }),
            changed_equal(&accept_request,|v|{ v.purpose.push_str("!"); }),
            changed_equal(&accept_request,|v|{ v.submitted.command_id.push_str("!"); }),
        ]));
        output.insert("accepted".into(),model_debug(&accepted));
        output.insert("accepted_equality".into(),json!([
            changed_equal(&accepted,|v|{ v.document.document_token.push_str("!"); }),
            changed_equal(&accepted,|v|{ v.previous_revision ^= 1; }),
            changed_equal(&accepted,|v|{ v.new_revision ^= 1; }),
            changed_equal(&accepted,|v|{ v.changed_entities.clear(); }),
        ]));
        output.insert("verified".into(),model_debug(&verified));
        output.insert("verified_equality".into(),json!([
            changed_equal(&verified,|v|{ v.snapshot.business_id.push_str("!"); }),
            changed_equal(&verified,|v|{ v.key_id.push_str("!"); }),
            changed_equal(&verified,|v|{ v.issued_at = chrono::DateTime::from_timestamp(v.issued_at.timestamp() ^ 1, v.issued_at.timestamp_subsec_nanos()).unwrap(); }),
            changed_equal(&verified,|v|{ v.expires_at = chrono::DateTime::from_timestamp(v.expires_at.timestamp() ^ 1, v.expires_at.timestamp_subsec_nanos()).unwrap(); }),
        ]));
        let errors=[DocumentRoundTripError::invalid(),DocumentRoundTripError::expired(),DocumentRoundTripError::scope_mismatch(),
            DocumentRoundTripError::authorization_required(),DocumentRoundTripError::configuration(),DocumentRoundTripError::revision_conflict(),
            DocumentRoundTripError::projection_violation(),DocumentRoundTripError::validation_failed(),DocumentRoundTripError::not_found()];
        output.insert("errors".into(),Json::Array(errors.iter().map(|e| { let mut v=model_debug(e);v["display"]=json!(e.to_string());v["source_none"]=json!(std::error::Error::source(e).is_none());v }).collect()));
        output.insert("variants".into(),json!([model_debug(&DocumentMutationKind::Update),model_debug(&DocumentMutationKind::CreateChild),model_debug(&DocumentMutationKind::RemoveChild),
            model_debug(&DocumentRelationCompleteness::Complete),model_debug(&DocumentRelationCompleteness::Partial)]));
        Json::Object(output)
    }
}
fn main() {
    use std::io::{self,BufRead};
    for line in io::stdin().lock().lines() {
        let input=serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}",retained::model_observe(&input));
    }
}
