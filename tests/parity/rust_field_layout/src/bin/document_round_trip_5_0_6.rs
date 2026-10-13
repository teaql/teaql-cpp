use serde_json::{Value as Json, json};
use std::{
    io::{self, BufRead},
    sync::{
        Arc,
        atomic::{AtomicBool, AtomicUsize, Ordering},
    },
    time::Duration,
};
use teaql_runtime::*;

// Only the stimulus is encrypted here. The actual public frozen decoder remains
// the oracle, including all of its private payload validation.
fn authenticated_fixture(token: &str, input: &Json) -> String {
    use aes_gcm::{
        Aes256Gcm, Nonce,
        aead::{Aead, KeyInit, Payload},
    };
    use base64::{Engine, engine::general_purpose::URL_SAFE_NO_PAD};
    let parts: Vec<_> = token.split('.').collect();
    let id = parts[1];
    let service = input["issue_service"].as_str().unwrap_or("order-service");
    let environment = input["issue_environment"].as_str().unwrap_or("test");
    let mut info = Vec::new();
    for text in ["document-snapshot-encryption", service, environment, id] {
        info.extend_from_slice(&(text.len() as u16).to_be_bytes());
        info.extend_from_slice(text.as_bytes());
    }
    let mut key = [0u8; 32];
    hkdf::Hkdf::<sha2::Sha256>::new(
        Some(b"teaql.round-trip-reference.v1"),
        &[if id == "k1" { 0x42 } else { 0x24 }; 32],
    )
    .expand(&info, &mut key)
    .unwrap();
    let mut aad = Vec::new();
    for text in [
        "tqd1",
        id,
        service,
        environment,
        input["snapshot"]["scope"]["purpose"].as_str().unwrap(),
    ] {
        aad.extend_from_slice(&(text.len() as u32).to_be_bytes());
        aad.extend_from_slice(text.as_bytes());
    }
    let envelope = URL_SAFE_NO_PAD.decode(parts[2]).unwrap();
    let cipher = Aes256Gcm::new_from_slice(&key).unwrap();
    let nonce = Nonce::from_slice(&envelope[..12]);
    let mut plaintext = cipher
        .decrypt(
            nonce,
            Payload {
                msg: &envelope[12..],
                aad: &aad,
            },
        )
        .unwrap();
    let offset = input["wire_offset"].as_u64().unwrap() as usize;
    match input["wire_operation"].as_str().unwrap() {
        "truncate" => plaintext.truncate(offset),
        "append" => plaintext.push(0),
        operation => {
            let width = if operation == "byte" {
                1
            } else if operation == "u32" {
                4
            } else {
                8
            };
            let value = input["wire_value"].as_i64().unwrap() as u64;
            plaintext[offset..offset + width].copy_from_slice(&value.to_be_bytes()[8 - width..]);
        }
    }
    let encrypted = cipher
        .encrypt(
            nonce,
            Payload {
                msg: &plaintext,
                aad: &aad,
            },
        )
        .unwrap();
    let mut changed = envelope[..12].to_vec();
    changed.extend_from_slice(&encrypted);
    format!("tqd1.{id}.{}", URL_SAFE_NO_PAD.encode(changed))
}

fn names(value: &Json) -> Vec<String> {
    value
        .as_array()
        .unwrap()
        .iter()
        .map(|v| v.as_str().unwrap().to_owned())
        .collect()
}
fn mutations(value: &Json) -> Vec<DocumentMutationKind> {
    value
        .as_array()
        .unwrap()
        .iter()
        .map(|v| match v.as_str().unwrap() {
            "update" => DocumentMutationKind::Update,
            "create" => DocumentMutationKind::CreateChild,
            _ => DocumentMutationKind::RemoveChild,
        })
        .collect()
}
fn identity(value: &Json) -> ReferenceIdentity {
    ReferenceIdentity {
        entity_type: value["type"].as_str().unwrap().into(),
        id: value["id"].as_u64().unwrap(),
        version: value["version"].as_i64().unwrap(),
    }
}
fn snapshot(input: &Json) -> Result<DocumentSnapshot, DocumentRoundTripError> {
    let v = &input["snapshot"];
    let s = &v["scope"];
    let scope = ReferenceDocumentScope {
        document_id: s["document"].as_str().unwrap().into(),
        purpose: s["purpose"].as_str().unwrap().into(),
        aggregate_type: s["type"].as_str().unwrap().into(),
        aggregate_id: s["id"].as_u64().unwrap(),
        aggregate_revision: s["version"].as_i64().unwrap(),
    };
    let entities = v["entities"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| {
            DocumentEntitySnapshot::new(
                v["key"].as_str().unwrap(),
                identity(v),
                names(&v["loaded"]),
                names(&v["writable"]),
                mutations(&v["mutations"]),
            )
        })
        .collect::<Result<Vec<_>, _>>()?;
    let relations = v["relations"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| {
            DocumentRelationSnapshot::new(
                v["name"].as_str().unwrap(),
                names(&v["rows"]),
                if v["completeness"] == "complete" {
                    DocumentRelationCompleteness::Complete
                } else {
                    DocumentRelationCompleteness::Partial
                },
                v["writable"].as_bool().unwrap(),
                mutations(&v["mutations"]),
            )
        })
        .collect::<Result<Vec<_>, _>>()?;
    DocumentSnapshot::new(
        v["model"].as_str().unwrap(),
        v["business"].as_str().unwrap(),
        scope,
        identity(&v["aggregate"]),
        entities,
        relations,
    )
}
struct Policy {
    deny: Arc<AtomicBool>,
    calls: Arc<AtomicUsize>,
}
impl ReferenceAuthorizationPolicy for Policy {
    fn authorize(
        &self,
        _: &TrustedReferencePrincipal,
        _: &ReferenceDocumentScope,
        _: &ReferenceIdentity,
    ) -> Result<(), RoundTripReferenceError> {
        self.calls.fetch_add(1, Ordering::SeqCst);
        if self.deny.load(Ordering::SeqCst) {
            Err(RoundTripReferenceError::authorization_required())
        } else {
            Ok(())
        }
    }
}
fn actor(subject: &str) -> TrustedReferencePrincipal {
    TrustedReferencePrincipal::new("oidc", subject, "Merchant", 7).unwrap()
}

struct ServiceFixture {
    failure: String,
    calls: Arc<AtomicUsize>,
}
fn service_error(code: &str) -> Option<DocumentRoundTripError> {
    match code {
        "revision" => Some(DocumentRoundTripError::revision_conflict()),
        "projection" => Some(DocumentRoundTripError::projection_violation()),
        "validation" => Some(DocumentRoundTripError::validation_failed()),
        "not_found" => Some(DocumentRoundTripError::not_found()),
        _ => None,
    }
}
#[async_trait::async_trait]
impl ContextBoundDocumentService for ServiceFixture {
    async fn open_document(
        &self,
        _: &UserContext,
        principal: &TrustedReferencePrincipal,
        request: DocumentOpenRequest,
    ) -> Result<ContextualDocument, DocumentRoundTripError> {
        self.calls.fetch_add(1, Ordering::SeqCst);
        if let Some(error) = service_error(&self.failure) {
            return Err(error);
        }
        Ok(ContextualDocument {
            business_id: request.business_id,
            document_token: "issued-token".into(),
            body: json!({"subject":principal.subject,"type":request.aggregate_type,"purpose":request.purpose,
                "seconds":request.lifetime.as_secs(),"nanos":request.lifetime.subsec_nanos()}),
        })
    }
    async fn accept_document(
        &self,
        _: &UserContext,
        principal: &TrustedReferencePrincipal,
        request: DocumentAcceptRequest,
    ) -> Result<AcceptedContextualDocument, DocumentRoundTripError> {
        self.calls.fetch_add(1, Ordering::SeqCst);
        if let Some(error) = service_error(&self.failure) {
            return Err(error);
        }
        Ok(AcceptedContextualDocument {
            document: ContextualDocument {
                business_id: request.submitted.business_id,
                document_token: "accepted-token".into(),
                body: json!({"subject":principal.subject,"type":request.expected_aggregate_type,"purpose":request.purpose,
                "command":request.submitted.command_id,"submitted_token":request.submitted.document_token,
                "submitted_body":request.submitted.body}),
            },
            previous_revision: 4,
            new_revision: 5,
            changed_entities: vec![
                ReferenceIdentity::new("Order", 10, 5).unwrap(),
                ReferenceIdentity::new("OrderItem", 20, 3).unwrap(),
            ],
        })
    }
}
fn wait<T>(future: impl std::future::Future<Output = T>) -> T {
    struct NoWake;
    impl std::task::Wake for NoWake {
        fn wake(self: Arc<Self>) {}
    }
    let mut future = Box::pin(future);
    let waker = std::task::Waker::from(Arc::new(NoWake));
    let mut context = std::task::Context::from_waker(&waker);
    loop {
        if let std::task::Poll::Ready(value) = future.as_mut().poll(&mut context) {
            return value;
        }
    }
}
fn service_observe(input: &Json) -> Json {
    let calls = Arc::new(AtomicUsize::new(0));
    let mut user_context = UserContext::new();
    if input["service"].as_bool().unwrap() {
        user_context.set_context_bound_document_service(Arc::new(ServiceFixture {
            failure: input["failure"].as_str().unwrap().into(),
            calls: calls.clone(),
        }));
    }
    if input["principal"].as_bool().unwrap() {
        user_context.set_trusted_reference_principal(actor("alice"));
    }
    let value = (|| -> Result<Json, DocumentRoundTripError> {
        let kind = input["type"].as_str().unwrap();
        let purpose = input["purpose"].as_str().unwrap();
        let business = input["business"].as_str().unwrap();
        let validated = input["validate"].as_bool().unwrap();
        if input["method"] == "open" {
            let lifetime = Duration::new(
                input["lifetime"].as_u64().unwrap(),
                input["nanos"].as_u64().unwrap() as u32,
            );
            let request = if validated {
                DocumentOpenRequest::new(kind, business, purpose, lifetime)?
            } else {
                DocumentOpenRequest {
                    aggregate_type: kind.into(),
                    business_id: business.into(),
                    purpose: purpose.into(),
                    lifetime,
                }
            };
            let value = wait(user_context.open_document(request))?;
            Ok(
                json!({"business":value.business_id,"token":value.document_token,"body":value.body,"error":null}),
            )
        } else {
            let submitted = SubmittedContextualDocument {
                business_id: business.into(),
                document_token: input["token"].as_str().unwrap().into(),
                command_id: input["command"].as_str().unwrap().into(),
                body: input["body"].clone(),
            };
            let request = if validated {
                DocumentAcceptRequest::new(kind, purpose, submitted)?
            } else {
                DocumentAcceptRequest {
                    expected_aggregate_type: kind.into(),
                    purpose: purpose.into(),
                    submitted,
                }
            };
            let value = wait(user_context.accept_document(request))?;
            Ok(
                json!({"business":value.document.business_id,"token":value.document.document_token,"body":value.document.body,"error":null,
                "previous":value.previous_revision,"next":value.new_revision,"changed":value.changed_entities.into_iter().map(|v|json!([v.entity_type,v.id,v.version])).collect::<Vec<_>>()}),
            )
        }
    })();
    let mut out = value.unwrap_or_else(|error| json!({"error":error.code()}));
    out["calls"] = json!(calls.load(Ordering::SeqCst));
    out
}
fn observe(input: &Json) -> Json {
    if input["kind"] == "service" {
        return service_observe(input);
    }
    let snapshot = match snapshot(input) {
        Ok(v) => v,
        Err(e) => return json!({"error": e.code()}),
    };
    let calls = Arc::new(AtomicUsize::new(0));
    let deny = Arc::new(AtomicBool::new(input["deny_issue"].as_bool().unwrap()));
    let runtime = ContextBoundReferenceRuntime::governed(
        DeploymentProfile::Test,
        input["issue_service"].as_str().unwrap_or("order-service"),
        input["issue_environment"].as_str().unwrap_or("test"),
        Arc::new(
            StaticReferenceKeyProvider::new(
                input["issue_key"].as_str().unwrap_or("k1"),
                [
                    ReferenceKey::new("k1", [0x42; 32]).unwrap(),
                    ReferenceKey::new("k2", [0x24; 32]).unwrap(),
                ],
            )
            .unwrap(),
        ),
        Arc::new(Policy {
            deny: deny.clone(),
            calls: calls.clone(),
        }),
    )
    .unwrap();
    let issued = input["issued"].as_i64().unwrap();
    let now = input["now"].as_i64().unwrap();
    let runtime = runtime
        .with_clock(move || chrono::DateTime::from_timestamp(issued, 0).unwrap())
        .with_nonce_source(|| [0x33; 12]);
    let lifetime = Duration::new(
        input["lifetime"].as_u64().unwrap(),
        input["nanos"].as_u64().unwrap() as u32,
    );
    let token = runtime.issue_document_snapshot(&actor("alice"), snapshot.clone(), lifetime);
    let issue_calls = calls.load(Ordering::SeqCst);
    let token = match token {
        Ok(v) => v,
        Err(e) => return json!({"issue_calls":issue_calls,"error":e.code()}),
    };
    let mut submitted = token.clone();
    match input["modification"].as_str().unwrap() {
        "tag" => {
            let last = submitted.pop().unwrap();
            submitted.push(if last == 'A' { 'B' } else { 'A' });
        }
        "prefix" => submitted.replace_range(0..1, "x"),
        "extra" => submitted.push_str(".extra"),
        "missing_key" => submitted.replace_range(5..7, "k9"),
        _ => {}
    }
    let authenticated = input
        .get("wire_operation")
        .map(|_| authenticated_fixture(&token, input));
    if let Some(fixture) = &authenticated {
        submitted = fixture.clone();
    }
    let remove_old = input["old_key"] == "remove";
    let mut keys = vec![ReferenceKey::new("k2", [0x24; 32]).unwrap()];
    if !remove_old {
        keys.push(ReferenceKey::new("k1", [0x42; 32]).unwrap());
    }
    let runtime = ContextBoundReferenceRuntime::governed(
        DeploymentProfile::Test,
        input["decode_service"]
            .as_str()
            .or(input["issue_service"].as_str())
            .unwrap_or("order-service"),
        input["decode_environment"]
            .as_str()
            .or(input["issue_environment"].as_str())
            .unwrap_or("test"),
        Arc::new(
            StaticReferenceKeyProvider::new(
                input["decode_key"]
                    .as_str()
                    .unwrap_or(if remove_old { "k2" } else { "k1" }),
                keys,
            )
            .unwrap(),
        ),
        Arc::new(Policy {
            deny: deny.clone(),
            calls: calls.clone(),
        }),
    )
    .unwrap()
    .with_clock(move || chrono::DateTime::from_timestamp(now, 0).unwrap());
    deny.store(input["deny_consume"].as_bool().unwrap(), Ordering::SeqCst);
    let result = runtime.consume_document_snapshot(
        &actor(input["subject"].as_str().unwrap()),
        &submitted,
        input["expected_type"].as_str().unwrap(),
        input["expected_purpose"].as_str().unwrap(),
    );
    let mut out = json!({"token":token,"issue_calls":issue_calls,"calls":calls.load(Ordering::SeqCst),"error":result.as_ref().err().map(|e|e.code())});
    if let Some(fixture) = authenticated {
        out["authenticated_fixture"] = json!(fixture);
    }
    if let Ok(v) = result {
        out["equal"] = json!(v.snapshot == snapshot);
        out["key"] = json!(v.key_id);
        out["issued"] = json!(v.issued_at.timestamp());
        out["expires"] = json!(v.expires_at.timestamp());
        out["entity_found"] = json!(v.snapshot.entity("item:0").is_some());
        out["relation_found"] = json!(v.snapshot.relation("items").is_some());
    }
    out
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
