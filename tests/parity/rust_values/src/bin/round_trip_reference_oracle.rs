//! Unchanged public context-bound reference calls; instrument only SPI boundaries.
use aes_gcm::{
    Aes256Gcm, Nonce,
    aead::{Aead, KeyInit, Payload},
};
use base64::{Engine, engine::general_purpose::URL_SAFE_NO_PAD};
use chrono::{DateTime, Utc};
use hkdf::Hkdf;
use serde_json::{Value as J, json};
use sha2::Sha256;
use std::{
    io::{self, BufRead},
    sync::{
        Arc, Mutex,
        atomic::{AtomicBool, Ordering},
    },
    time::Duration,
};
use teaql_runtime::*;
fn fixture_crypto(j: &J) -> J {
    let mut info = vec![];
    for text in [
        "entity-reference-encryption",
        "order-service",
        "test-a",
        "k2",
    ] {
        info.extend_from_slice(&(text.len() as u16).to_be_bytes());
        info.extend_from_slice(text.as_bytes());
    }
    let mut key = [0; 32];
    Hkdf::<Sha256>::new(Some(b"teaql.round-trip-reference.v1"), &[0x22; 32])
        .expand(&info, &mut key)
        .unwrap();
    let mut aad = vec![];
    for text in ["tqr1", "k2", "order-service", "test-a", "edit-order"] {
        aad.extend_from_slice(&(text.len() as u16).to_be_bytes());
        aad.extend_from_slice(text.as_bytes());
    }
    let cipher = Aes256Gcm::new_from_slice(&key).unwrap();
    if j["op"] == "fixture_decrypt" {
        let token = j["token"].as_str().unwrap();
        let input = URL_SAFE_NO_PAD
            .decode(token.split('.').nth(2).unwrap())
            .unwrap();
        let plain = cipher
            .decrypt(
                Nonce::from_slice(&input[..12]),
                Payload {
                    msg: &input[12..],
                    aad: &aad,
                },
            )
            .unwrap();
        return json!({"plaintext":plain.iter().map(|b|format!("{b:02x}")).collect::<String>()});
    }
    let plain = hex(j["plaintext"].as_str().unwrap());
    let nonce = [0x33; 12];
    let encrypted = cipher
        .encrypt(
            Nonce::from_slice(&nonce),
            Payload {
                msg: &plain,
                aad: &aad,
            },
        )
        .unwrap();
    let mut bytes = nonce.to_vec();
    bytes.extend_from_slice(&encrypted);
    json!({"reference":{"token":format!("tqr1.k2.{}",URL_SAFE_NO_PAD.encode(bytes))}})
}
fn hex(s: &str) -> Vec<u8> {
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).unwrap())
        .collect()
}
fn p(j: &J) -> TrustedReferencePrincipal {
    TrustedReferencePrincipal {
        authentication_realm: j[0].as_str().unwrap().into(),
        subject: j[1].as_str().unwrap().into(),
        domain_root_type: j[2].as_str().unwrap().into(),
        domain_root_id: j[3].as_str().unwrap().parse().unwrap(),
    }
}
fn s(j: &J) -> ReferenceDocumentScope {
    ReferenceDocumentScope {
        document_id: j[0].as_str().unwrap().into(),
        purpose: j[1].as_str().unwrap().into(),
        aggregate_type: j[2].as_str().unwrap().into(),
        aggregate_id: j[3].as_str().unwrap().parse().unwrap(),
        aggregate_revision: j[4].as_str().unwrap().parse().unwrap(),
    }
}
fn i(j: &J) -> ReferenceIdentity {
    ReferenceIdentity {
        entity_type: j[0].as_str().unwrap().into(),
        id: j[1].as_str().unwrap().parse().unwrap(),
        version: j[2].as_str().unwrap().parse().unwrap(),
    }
}
fn reference(j: &J) -> ExternalEntityReference {
    if let Some(t) = j.get("token") {
        ExternalEntityReference::Governed(t.as_str().unwrap().into())
    } else {
        ExternalEntityReference::Raw {
            id: j["id"].as_str().unwrap().parse().unwrap(),
            version: j["version"].as_str().unwrap().parse().unwrap(),
        }
    }
}
fn describe(r: &ExternalEntityReference) -> J {
    match r {
        ExternalEntityReference::Governed(t) => json!({"token":t}),
        ExternalEntityReference::Raw { id, version } => {
            json!({"id":id.to_string(),"version":version.to_string()})
        }
    }
}
fn resolved(r: ResolvedEntityReference) -> J {
    json!({"identity":[r.identity.entity_type,r.identity.id.to_string(),r.identity.version.to_string()],"mode":format!("{:?}",r.mode),"key":r.key_id,"issued":r.issued_at.map(|v|v.timestamp().to_string()),"expires":r.expires_at.map(|v|v.timestamp().to_string())})
}
fn time(j: &J) -> DateTime<Utc> {
    DateTime::from_timestamp(
        j[0].as_str().unwrap().parse().unwrap(),
        j[1].as_u64().unwrap() as u32,
    )
    .unwrap()
}
fn notice(n: Option<ReferenceStartupNotice>) -> J {
    match n {
        None => J::Null,
        Some(n) => {
            json!({"level":n.level,"message":n.message,"profile":n.deployment_profile,"telemetry":[n.telemetry_key,n.telemetry_value],"headers":n.response_headers})
        }
    }
}
fn error(code: &str) -> RoundTripReferenceError {
    match code {
        "expired" => RoundTripReferenceError::expired(),
        "scope" => RoundTripReferenceError::scope_mismatch(),
        "auth" => RoundTripReferenceError::authorization_required(),
        "invalid" => RoundTripReferenceError::invalid(),
        _ => RoundTripReferenceError::configuration(),
    }
}
struct Keys {
    keys: StaticReferenceKeyProvider,
    calls: Arc<Mutex<Vec<J>>>,
    config_error: String,
    issue_error: String,
    lookup_error: String,
    current_count: Mutex<usize>,
}
impl ReferenceKeyProvider for Keys {
    fn current_key(&self) -> Result<ReferenceKey, RoundTripReferenceError> {
        self.calls.lock().unwrap().push(json!("current"));
        let mut count = self.current_count.lock().unwrap();
        let failure = if *count == 0 {
            &self.config_error
        } else {
            &self.issue_error
        };
        *count += 1;
        if failure != "none" {
            return Err(error(failure));
        }
        self.keys.current_key()
    }
    fn key_by_id(&self, id: &str) -> Result<Option<ReferenceKey>, RoundTripReferenceError> {
        self.calls
            .lock()
            .unwrap()
            .push(json!(format!("lookup:{id}")));
        if self.lookup_error != "none" {
            return Err(error(&self.lookup_error));
        }
        self.keys.key_by_id(id)
    }
}
struct Policy {
    allowed: Arc<AtomicBool>,
    calls: Arc<Mutex<Vec<J>>>,
}
impl ReferenceAuthorizationPolicy for Policy {
    fn authorize(
        &self,
        p: &TrustedReferencePrincipal,
        s: &ReferenceDocumentScope,
        i: &ReferenceIdentity,
    ) -> Result<(), RoundTripReferenceError> {
        self.calls.lock().unwrap().push(json!({"authorize":{"principal":[p.authentication_realm,p.subject,p.domain_root_type,p.domain_root_id.to_string()],"scope":[s.document_id,s.purpose,s.aggregate_type,s.aggregate_id.to_string(),s.aggregate_revision.to_string()],"identity":[i.entity_type,i.id.to_string(),i.version.to_string()]}}));
        if self.allowed.load(Ordering::SeqCst)
            && p.subject == "alice"
            && i.entity_type == "OrderItem"
            && i.id == 42
        {
            Ok(())
        } else {
            Err(RoundTripReferenceError::authorization_required())
        }
    }
}
fn evaluate(j: &J, calls: Arc<Mutex<Vec<J>>>) -> J {
    let op = j["op"].as_str().unwrap();
    if op.starts_with("fixture_") {
        return fixture_crypto(j);
    }
    if op == "model" {
        return match j["model"].as_str().unwrap() {
            "principal" => {
                let p = p(&j["principal"]);
                match TrustedReferencePrincipal::new(
                    p.authentication_realm,
                    p.subject,
                    p.domain_root_type,
                    p.domain_root_id,
                ) {
                    Ok(_) => json!({"ok":true}),
                    Err(e) => json!({"error":e.code()}),
                }
            }
            "scope" => {
                let s = s(&j["scope"]);
                match ReferenceDocumentScope::new(
                    s.document_id,
                    s.purpose,
                    s.aggregate_type,
                    s.aggregate_id,
                    s.aggregate_revision,
                ) {
                    Ok(_) => json!({"ok":true}),
                    Err(e) => json!({"error":e.code()}),
                }
            }
            "identity" => {
                let i = i(&j["identity"]);
                match ReferenceIdentity::new(i.entity_type, i.id, i.version) {
                    Ok(_) => json!({"ok":true}),
                    Err(e) => json!({"error":e.code()}),
                }
            }
            "key" => match ReferenceKey::new(
                j["current"].as_str().unwrap(),
                hex(j["key"].as_str().unwrap()),
            ) {
                Ok(k) => json!({"debug":format!("{k:?}")}),
                Err(e) => json!({"error":e.code()}),
            },
            _ => panic!("unknown model"),
        };
    }
    if op == "wire" {
        return match ReferenceWireCodec::deserialize(
            &j["wire"],
            if j["raw"] == true {
                ReferenceMode::Raw
            } else {
                ReferenceMode::Governed
            },
        ) {
            Ok(r) => json!({"reference":describe(&r),"wire":ReferenceWireCodec::serialize(&r)}),
            Err(e) => json!({"error":e.code()}),
        };
    }
    let mut material = vec![];
    for k in j["keys"].as_array().unwrap() {
        match ReferenceKey::new(k[0].as_str().unwrap(), hex(k[1].as_str().unwrap())) {
            Ok(k) => material.push(k),
            Err(e) => return json!({"error":e.code()}),
        }
    }
    let static_keys =
        match StaticReferenceKeyProvider::new(j["current"].as_str().unwrap(), material) {
            Ok(k) => k,
            Err(e) => return json!({"error":e.code()}),
        };
    if op == "provider" {
        return json!({"current":format!("{:?}",static_keys.current_key().unwrap()),"lookup":static_keys.key_by_id(j["lookup"].as_str().unwrap()).unwrap().map(|k|format!("{k:?}"))});
    }
    let keys = Arc::new(Keys {
        keys: static_keys,
        calls: calls.clone(),
        config_error: j["config_error"].as_str().unwrap().into(),
        issue_error: j["issue_error"].as_str().unwrap().into(),
        lookup_error: j["lookup_error"].as_str().unwrap().into(),
        current_count: Mutex::new(0),
    });
    let allowed = Arc::new(AtomicBool::new(j["issue_allowed"].as_bool().unwrap()));
    let policy = Arc::new(Policy {
        allowed: allowed.clone(),
        calls: calls.clone(),
    });
    let profile = [
        DeploymentProfile::Development,
        DeploymentProfile::Test,
        DeploymentProfile::Production,
    ][j["profile"].as_u64().unwrap() as usize];
    unsafe {
        if let Some(a) = j["ack"].as_str() {
            std::env::set_var(UNSAFE_EXPOSE_RAW_ENTITY_IDS_ENVIRONMENT, a);
        } else {
            std::env::remove_var(UNSAFE_EXPOSE_RAW_ENTITY_IDS_ENVIRONMENT);
        }
    }
    let make = if j["governed"] == true {
        ContextBoundReferenceRuntime::governed(
            profile,
            j["service"].as_str().unwrap(),
            j["environment"].as_str().unwrap(),
            keys,
            policy,
        )
    } else {
        ContextBoundReferenceRuntime::from_process_environment(
            profile,
            j["service"].as_str().unwrap(),
            j["environment"].as_str().unwrap(),
            keys,
            policy,
        )
    };
    let mut runtime = match make {
        Ok(r) => r,
        Err(e) => return json!({"error":e.code()}),
    };
    let now = time(&j["time"]);
    let clock = Arc::new(Mutex::new(now));
    let clock_state = clock.clone();
    let recorder = calls.clone();
    runtime = runtime.with_clock(move || {
        recorder.lock().unwrap().push(json!("clock"));
        *clock_state.lock().unwrap()
    });
    let nonce: [u8; 12] = hex(j["nonce"].as_str().unwrap()).try_into().unwrap();
    let recorder = calls.clone();
    runtime = runtime.with_nonce_source(move || {
        recorder.lock().unwrap().push(json!("nonce"));
        nonce
    });
    let runtime = Arc::new(runtime);
    let mut user = UserContext::new();
    if j["installed"] == true {
        user.set_round_trip_reference_runtime(runtime.clone());
    }
    if j["principal_installed"] == true {
        user.set_trusted_reference_principal(p(&j["principal"]));
    }
    let context = j["context"] == true;
    let mut out = json!({"mode":format!("{:?}",runtime.mode()),"profile":format!("{:?}",runtime.deployment_profile()),"notice":notice(runtime.startup_notice())});
    if op == "config" {
        return out;
    }
    if op == "mode" {
        match user.reference_mode() {
            Ok(m) => out["result"] = json!(format!("{m:?}")),
            Err(e) => out["error"] = json!(e.code()),
        };
        return out;
    }
    if op == "notice" {
        match user.reference_startup_notice() {
            Ok(n) => out["result"] = notice(n),
            Err(e) => out["error"] = json!(e.code()),
        };
        return out;
    }
    if op == "serialize" {
        let reference = reference(&j["reference"]);
        match user.serialize_reference(&reference) {
            Ok(v) => out["wire"] = v,
            Err(e) => out["error"] = json!(e.code()),
        };
        return out;
    }
    if op == "deserialize" {
        match user.deserialize_reference(&j["wire"]) {
            Ok(r) => out["reference"] = describe(&r),
            Err(e) => out["error"] = json!(e.code()),
        };
        return out;
    }
    let reference = if op == "consume" {
        reference(&j["reference"])
    } else {
        let duration = Duration::new(
            j["lifetime"][0].as_str().unwrap().parse().unwrap(),
            j["lifetime"][1].as_u64().unwrap() as u32,
        );
        let issued = if context {
            user.reference_for(i(&j["identity"]), &s(&j["scope"]), duration)
        } else {
            runtime.issue(
                &p(&j["principal"]),
                &s(&j["scope"]),
                i(&j["identity"]),
                duration,
            )
        };
        match issued {
            Ok(r) => r,
            Err(e) => {
                out["error"] = json!(e.code());
                return out;
            }
        }
    };
    out["reference"] = describe(&reference);
    if op == "issue" {
        return out;
    }
    allowed.store(j["consume_allowed"].as_bool().unwrap(), Ordering::SeqCst);
    *clock.lock().unwrap() = time(&j["consume_time"]);
    let cp = p(&j["consume_principal"]);
    let cs = s(&j["consume_scope"]);
    if context {
        if j["installed"] == true {
            user.set_round_trip_reference_runtime(runtime.clone());
        }
        if j["principal_installed"] == true {
            user.set_trusted_reference_principal(cp.clone());
        }
    }
    let result = if context {
        user.resolve_reference(&reference, j["expected_entity"].as_str().unwrap(), &cs)
    } else {
        runtime.consume(&cp, &cs, &reference, j["expected_entity"].as_str().unwrap())
    };
    match result {
        Ok(r) => out["resolved"] = resolved(r),
        Err(e) => out["consume_error"] = json!(e.code()),
    };
    out
}
fn main() {
    unsafe {
        std::env::set_var("TEAQL_LOG_ENDPOINT", "off");
    }
    for line in io::stdin().lock().lines() {
        let input = serde_json::from_str::<J>(&line.unwrap()).unwrap();
        let calls = Arc::new(Mutex::new(vec![]));
        let mut output = std::panic::catch_unwind(|| evaluate(&input, calls.clone()))
            .unwrap_or_else(|_| json!({"panic":true}));
        output["calls"] = json!(*calls.lock().unwrap());
        println!("{output}");
    }
}
