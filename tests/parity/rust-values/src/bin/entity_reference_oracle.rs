//! Unchanged original public codec and Context calls; forged payloads are data.
use aes_gcm::{
    Aes256Gcm, Nonce,
    aead::{Aead, KeyInit, Payload},
};
use base64::{Engine, engine::general_purpose::URL_SAFE_NO_PAD};
use chrono::{DateTime, Utc};
use serde_json::{Value as J, json};
use std::{
    io::{self, BufRead},
    sync::Arc,
    time::Duration,
};
use teaql_runtime::{
    AeadEntityReferenceCodec, ENTITY_REFERENCE_AAD, EntityReferenceClaims, EntityReferenceCodec,
    UserContext,
};

fn hex(s: &str) -> Vec<u8> {
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).unwrap())
        .collect()
}
fn now(j: &J) -> DateTime<Utc> {
    DateTime::from_timestamp(
        j[0].as_str().unwrap().parse().unwrap(),
        j[1].as_u64().unwrap() as u32,
    )
    .unwrap()
}
fn claims(c: EntityReferenceClaims) -> J {
    json!({"entity":c.entity_type,"id":c.id.to_string(),"version":c.version.to_string(),
        "issued":c.issued_at.timestamp().to_string(),"expires":c.expires_at.timestamp().to_string(),
        "purpose":c.purpose,"key_version":c.key_version})
}
fn evaluate(j: &J) -> J {
    let active = j["active"].as_u64().unwrap() as u32;
    let keys = j["keys"]
        .as_array()
        .unwrap()
        .iter()
        .map(|k| (k[0].as_u64().unwrap() as u32, hex(k[1].as_str().unwrap())));
    let codec = match AeadEntityReferenceCodec::new(active, keys) {
        Ok(c) => c,
        Err(e) => return json!({"error":e.code()}),
    };
    if j["op"] == "new" {
        return json!({"ok":true});
    }
    let time = now(&j["now"]);
    let nonce: [u8; 12] = hex(j["nonce"].as_str().unwrap()).try_into().unwrap();
    let codec = codec
        .with_clock(move || time)
        .with_nonce_source(move || nonce);
    let token = if j["op"] == "decode" {
        j["token"].as_str().unwrap().to_owned()
    } else if j["op"] == "forged" {
        let key = hex(j["encryption_key"].as_str().unwrap());
        let plaintext = hex(j["plaintext"].as_str().unwrap());
        let cipher = Aes256Gcm::new_from_slice(&key).unwrap();
        let encrypted = cipher
            .encrypt(
                Nonce::from_slice(&nonce),
                Payload {
                    msg: &plaintext,
                    aad: ENTITY_REFERENCE_AAD.as_bytes(),
                },
            )
            .unwrap();
        let mut envelope = (j["header_version"].as_u64().unwrap() as u32)
            .to_be_bytes()
            .to_vec();
        envelope.extend_from_slice(&nonce);
        envelope.extend_from_slice(&encrypted);
        format!("tqr1.{}", URL_SAFE_NO_PAD.encode(envelope))
    } else {
        let lifetime = Duration::new(
            j["lifetime"][0].as_str().unwrap().parse().unwrap(),
            j["lifetime"][1].as_u64().unwrap() as u32,
        );
        let entity = j["entity"].as_str().unwrap();
        let id = j["id"].as_str().unwrap().parse().unwrap();
        let version = j["version"].as_str().unwrap().parse().unwrap();
        let purpose = j["purpose"].as_str().unwrap();
        let encoded = if j["context"] == true {
            let user = UserContext::new();
            if j["installed"] == true {
                user.with_entity_reference_codec(Arc::new(codec))
                    .encode_entity_reference(entity, id, version, purpose, lifetime)
            } else {
                user.encode_entity_reference(entity, id, version, purpose, lifetime)
            }
        } else {
            codec.encode(entity, id, version, purpose, lifetime)
        };
        match encoded {
            Ok(t) => t,
            Err(e) => return json!({"error":e.code()}),
        }
    };
    let keys = j["decode_keys"]
        .as_array()
        .unwrap()
        .iter()
        .map(|k| (k[0].as_u64().unwrap() as u32, hex(k[1].as_str().unwrap())));
    let consume_now = now(&j["consume_now"]);
    let decode_codec =
        AeadEntityReferenceCodec::new(j["decode_active"].as_u64().unwrap() as u32, keys)
            .unwrap()
            .with_clock(move || consume_now);
    let result = if j["context"] == true {
        let user = UserContext::new();
        if j["installed"] == true {
            user.with_entity_reference_codec(Arc::new(decode_codec))
                .decode_entity_reference(
                    &token,
                    j["expected_entity"].as_str().unwrap(),
                    j["expected_purpose"].as_str().unwrap(),
                )
        } else {
            user.decode_entity_reference(
                &token,
                j["expected_entity"].as_str().unwrap(),
                j["expected_purpose"].as_str().unwrap(),
            )
        }
    } else {
        decode_codec.decode(
            &token,
            j["expected_entity"].as_str().unwrap(),
            j["expected_purpose"].as_str().unwrap(),
        )
    };
    match result {
        Ok(c) => json!({"token":token,"claims":claims(c)}),
        Err(e) => json!({"token":token,"decode_error":e.code()}),
    }
}
fn main() {
    unsafe {
        std::env::set_var("TEAQL_LOG_ENDPOINT", "off");
    }
    for line in io::stdin().lock().lines() {
        let input = serde_json::from_str::<J>(&line.unwrap()).unwrap();
        let result =
            std::panic::catch_unwind(|| evaluate(&input)).unwrap_or_else(|_| json!({"panic":true}));
        println!("{result}");
    }
}
