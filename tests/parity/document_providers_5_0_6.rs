#[allow(dead_code)]
mod retained {
    include!("rust_field_layout/src/bin/document_round_trip_5_0_6.rs");
    pub fn source_snapshot(input: &Json) -> DocumentSnapshot {
        snapshot(input).unwrap()
    }
}

use serde_json::{Value as Json, json};
use std::{
    io::{self, BufRead},
    sync::{Arc, Mutex},
    time::Duration,
};
use teaql_runtime::*;

struct State {
    input: Json,
    stage: &'static str,
    rotated: bool,
    authorizations: usize,
    nonces: u8,
    events: Vec<Json>,
}
impl State {
    fn stage(&mut self, stage: &'static str) {
        self.stage = stage;
        self.authorizations = 0;
    }
    fn event(&mut self, mut event: Json) {
        event["stage"] = self.stage.into();
        self.events.push(event);
    }
    fn current_id(&self) -> &'static str {
        if self.rotated { "k2" } else { "k1" }
    }
}
fn failure(code: &str) -> Option<RoundTripReferenceError> {
    match code {
        "invalid" => Some(RoundTripReferenceError::invalid()),
        "expired" => Some(RoundTripReferenceError::expired()),
        "scope" => Some(RoundTripReferenceError::scope_mismatch()),
        "authorization" => Some(RoundTripReferenceError::authorization_required()),
        "configuration" => Some(RoundTripReferenceError::configuration()),
        _ => None,
    }
}
struct Keys(Arc<Mutex<State>>);
impl ReferenceKeyProvider for Keys {
    fn current_key(&self) -> Result<ReferenceKey, RoundTripReferenceError> {
        let mut state = self.0.lock().unwrap();
        let id = state.current_id();
        state.event(json!({"kind":"current","key":id}));
        let error = if state.stage == "startup" {
            "startup_error"
        } else {
            "current_error"
        };
        if let Some(error) = failure(state.input[error].as_str().unwrap()) {
            return Err(error);
        }
        ReferenceKey::new(id, [if id == "k1" { 0x42 } else { 0x24 }; 32])
    }
    fn key_by_id(&self, id: &str) -> Result<Option<ReferenceKey>, RoundTripReferenceError> {
        let mut state = self.0.lock().unwrap();
        state.event(json!({"kind":"lookup","key":id}));
        if let Some(error) = failure(state.input["lookup_error"].as_str().unwrap()) {
            return Err(error);
        }
        let mode = state.input["lookup_mode"].as_str().unwrap();
        if mode == "missing" {
            return Ok(None);
        }
        let returned = if mode == "mismatch" { "k2" } else { id };
        Ok(Some(ReferenceKey::new(
            returned,
            [if mode == "wrong_master" {
                0x17
            } else if returned == "k1" {
                0x42
            } else {
                0x24
            }; 32],
        )?))
    }
}
struct Policy(Arc<Mutex<State>>);
impl ReferenceAuthorizationPolicy for Policy {
    fn authorize(
        &self,
        principal: &TrustedReferencePrincipal,
        scope: &ReferenceDocumentScope,
        identity: &ReferenceIdentity,
    ) -> Result<(), RoundTripReferenceError> {
        let mut state = self.0.lock().unwrap();
        state.authorizations += 1;
        state.event(json!({"kind":"authorize",
            "principal":{"realm":principal.authentication_realm,"subject":principal.subject,"root_type":principal.domain_root_type,"root_id":principal.domain_root_id},
            "scope":{"document":scope.document_id,"purpose":scope.purpose,"type":scope.aggregate_type,"id":scope.aggregate_id,"revision":scope.aggregate_revision},
            "identity":{"type":identity.entity_type,"id":identity.id,"version":identity.version}}));
        let prefix = if state.stage == "consume" {
            "consume"
        } else {
            "issue"
        };
        if state.input[format!("deny_{prefix}_at")].as_u64().unwrap() as usize
            == state.authorizations
        {
            if let Some(error) = failure(
                state.input[format!("deny_{prefix}_error")]
                    .as_str()
                    .unwrap(),
            ) {
                return Err(error);
            }
        }
        Ok(())
    }
}
fn runtime(
    state: &Arc<Mutex<State>>,
    service: &str,
) -> Result<ContextBoundReferenceRuntime, RoundTripReferenceError> {
    let clock = state.clone();
    let nonce = state.clone();
    Ok(ContextBoundReferenceRuntime::governed(
        DeploymentProfile::Test,
        service,
        "test",
        Arc::new(Keys(state.clone())),
        Arc::new(Policy(state.clone())),
    )?
    .with_clock(move || {
        let mut state = clock.lock().unwrap();
        let now = 1800000000
            + if state.stage == "consume" {
                state.input["delta"].as_i64().unwrap()
            } else if state.stage == "reissue" {
                1
            } else {
                0
            };
        let nanos = state.input["nanos"].as_u64().unwrap() as u32;
        state.event(json!({"kind":"clock","time":now,"nanos":nanos}));
        chrono::DateTime::from_timestamp(now, nanos).unwrap()
    })
    .with_nonce_source(move || {
        let mut state = nonce.lock().unwrap();
        let seed = (state.input["seed"].as_u64().unwrap() as u8).wrapping_add(state.nonces);
        state.nonces = state.nonces.wrapping_add(1);
        state.event(json!({"kind":"nonce","seed":seed}));
        [seed; 12]
    }))
}
fn issued(value: Result<String, DocumentRoundTripError>) -> Json {
    match value {
        Ok(token) => json!({"token":token}),
        Err(error) => json!({"error":error.code()}),
    }
}
fn observe(input: &Json) -> Json {
    let state = Arc::new(Mutex::new(State {
        input: input.clone(),
        stage: "startup",
        rotated: false,
        authorizations: 0,
        nonces: 0,
        events: vec![],
    }));
    let result = (|| {
        let mut runtime = match runtime(&state, "order-service") {
            Ok(value) => Arc::new(value),
            Err(error) => return json!({"startup_error":error.code()}),
        };
        let mut user_context = UserContext::new();
        let mut principal = TrustedReferencePrincipal::new("oidc", "alice", "Merchant", 7).unwrap();
        if input["invalid"] == "principal" {
            principal.subject.clear();
        }
        if input["has_runtime"].as_bool().unwrap() {
            if input["builders"].as_bool().unwrap() {
                user_context = user_context.with_round_trip_reference_runtime(runtime.clone());
            } else {
                user_context.set_round_trip_reference_runtime(runtime.clone());
            }
        }
        if input["has_principal"].as_bool().unwrap() {
            if input["builders"].as_bool().unwrap() {
                user_context = user_context.with_trusted_reference_principal(principal.clone());
            } else {
                user_context.set_trusted_reference_principal(principal.clone());
            }
        }
        let mut snapshot = retained::source_snapshot(input);
        if input["invalid"] == "snapshot" {
            snapshot.model_fingerprint.clear();
        }
        let lifetime = Duration::from_secs(if input["invalid"] == "lifetime" {
            0
        } else {
            60
        });
        let context = input["entry"] == "context";
        state.lock().unwrap().stage("issue");
        let value = if context {
            user_context.issue_document_snapshot(snapshot.clone(), lifetime)
        } else {
            runtime.issue_document_snapshot(&principal, snapshot.clone(), lifetime)
        };
        let mut result = json!({"startup_error":null});
        let token = match value {
            Ok(token) => token,
            Err(error) => {
                result["issued"] = json!({"error":error.code()});
                return result;
            }
        };
        result["issued"] = json!({"token":token});
        state.lock().unwrap().rotated = input["rotation"].as_bool().unwrap();
        if input["reissue"].as_bool().unwrap() {
            state.lock().unwrap().stage("reissue");
            result["reissued"] = issued(if context {
                user_context.issue_document_snapshot(snapshot.clone(), lifetime)
            } else {
                runtime.issue_document_snapshot(&principal, snapshot.clone(), lifetime)
            });
        }
        if input["replace"].as_bool().unwrap() {
            state.lock().unwrap().stage("replace");
            runtime = Arc::new(
                self::runtime(
                    &state,
                    if input["namespace"] == "different" {
                        "other-service"
                    } else {
                        "order-service"
                    },
                )
                .unwrap(),
            );
            principal = TrustedReferencePrincipal::new(
                "oidc",
                if input["actor"] == "different" {
                    "bob"
                } else {
                    "alice"
                },
                "Merchant",
                7,
            )
            .unwrap();
            if input["builders"].as_bool().unwrap() {
                user_context = user_context
                    .with_round_trip_reference_runtime(runtime.clone())
                    .with_trusted_reference_principal(principal.clone());
            } else {
                user_context.set_round_trip_reference_runtime(runtime.clone());
                user_context.set_trusted_reference_principal(principal.clone());
            }
        }
        state.lock().unwrap().stage("consume");
        let value = if context {
            user_context.consume_document_snapshot(&token, "Order", "edit-order")
        } else {
            runtime.consume_document_snapshot(&principal, &token, "Order", "edit-order")
        };
        result["consumed"] = match value {
            Err(error) => json!({"error":error.code()}),
            Ok(value) => {
                json!({"snapshot":format!("{:?}",value.snapshot),"equal":value.snapshot==snapshot,
                "key":value.key_id,"issued":value.issued_at.timestamp(),"expires":value.expires_at.timestamp()})
            }
        };
        result
    })();
    let mut result = result;
    result["events"] = json!(state.lock().unwrap().events);
    result
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
