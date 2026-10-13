//! Original public reference models, derives, redacted key Debug and Error defaults.
use chrono::{DateTime, Utc};
use serde_json::{json, Value};
use std::io::{self, BufRead};
use teaql_runtime::*;
fn formatted<T: std::fmt::Debug>(value: &T) -> Value {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn snapshot<T: std::fmt::Debug + Clone + PartialEq>(value: &T) -> Value {
    let copy=value.clone();
    json!({"debug":formatted(value),"clone_debug":formatted(&copy),"equal":value==&copy})
}
fn text<'a>(j:&'a Value,field:&str)->&'a str {j[field].as_str().unwrap()}
fn unsigned(j:&Value,field:&str)->u64 {text(j,field).parse().unwrap()}
fn signed(j:&Value,field:&str)->i64 {text(j,field).parse().unwrap()}
fn time(j:&Value)->DateTime<Utc> {DateTime::from_timestamp(j[0].as_str().unwrap().parse().unwrap(),j[1].as_u64().unwrap() as u32).unwrap()}
fn identity(j:&Value)->ReferenceIdentity {ReferenceIdentity{entity_type:text(j,"entity").into(),id:unsigned(j,"id"),version:signed(j,"version")}}
fn mode(j:&Value)->ReferenceMode {if j["raw"]==true {ReferenceMode::Raw}else{ReferenceMode::Governed}}
fn static_text(j:&Value,field:&str)->&'static str {Box::leak(text(j,field).to_owned().into_boxed_str())}
fn evaluate(j:Value)->Value {
    match text(&j,"kind") {
        "claims"=>snapshot(&EntityReferenceClaims{entity_type:text(&j,"entity").into(),id:unsigned(&j,"id"),version:signed(&j,"version"),issued_at:time(&j["issued"]),expires_at:time(&j["expires"]),purpose:text(&j,"purpose").into(),key_version:j["key_version"].as_u64().unwrap() as u32}),
        "principal"=>snapshot(&TrustedReferencePrincipal{authentication_realm:text(&j,"realm").into(),subject:text(&j,"subject").into(),domain_root_type:text(&j,"root").into(),domain_root_id:unsigned(&j,"id")}),
        "document"=>snapshot(&ReferenceDocumentScope{document_id:text(&j,"document").into(),purpose:text(&j,"purpose").into(),aggregate_type:text(&j,"aggregate").into(),aggregate_id:unsigned(&j,"id"),aggregate_revision:signed(&j,"version")}),
        "identity"=>snapshot(&identity(&j)),
        "external"=>snapshot(&if j["raw"]==true {ExternalEntityReference::Raw{id:unsigned(&j,"id"),version:signed(&j,"version")}}else{ExternalEntityReference::Governed(text(&j,"token").into())}),
        "resolved"=>snapshot(&ResolvedEntityReference{identity:identity(&j),mode:mode(&j),key_id:j["key"].as_str().map(str::to_owned),issued_at:(!j["issued"].is_null()).then(||time(&j["issued"])),expires_at:(!j["expires"].is_null()).then(||time(&j["expires"]))}),
        "profile"=>snapshot(&[DeploymentProfile::Development,DeploymentProfile::Test,DeploymentProfile::Production][j["variant"].as_u64().unwrap() as usize]),
        "mode"=>snapshot(&mode(&j)),
        "wire"=>{let value=ReferenceWireCodec::default();json!({"debug":formatted(&value),"clone_debug":formatted(&value.clone())})},
        "key"=>{let value=ReferenceKey::new(text(&j,"key"),[j["byte"].as_u64().unwrap() as u8;32]).unwrap();json!({"debug":formatted(&value),"clone_debug":formatted(&value.clone())})},
        "notice"=>snapshot(&ReferenceStartupNotice{level:static_text(&j,"level"),message:static_text(&j,"message"),deployment_profile:static_text(&j,"profile"),telemetry_key:static_text(&j,"telemetry_key"),telemetry_value:static_text(&j,"telemetry_value"),response_headers:[("TeaQL-Reference-Mode","raw-internal-id"),("Cache-Control","no-store")]}),
        "token_error"=>{let error=if j["variant"]==0{EntityReferenceTokenError::invalid()}else{EntityReferenceTokenError::codec_required()};let mut out=snapshot(&error);out["display"]=json!(error.to_string());out["source_none"]=json!(std::error::Error::source(&error).is_none());out},
        "round_error"=>{let error=[RoundTripReferenceError::invalid(),RoundTripReferenceError::expired(),RoundTripReferenceError::scope_mismatch(),RoundTripReferenceError::authorization_required(),RoundTripReferenceError::configuration()][j["variant"].as_u64().unwrap() as usize].clone();let mut out=snapshot(&error);out["display"]=json!(error.to_string());out["source_none"]=json!(std::error::Error::source(&error).is_none());out},
        _=>panic!("unknown fixture"),
    }
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
