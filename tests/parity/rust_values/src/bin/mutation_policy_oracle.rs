use std::{collections::BTreeMap,io::{self,BufRead},sync::{Arc,Mutex},panic::{catch_unwind,AssertUnwindSafe},time::SystemTime};
use serde_json::{json,Value as J};
use teaql_core::Value;
use teaql_runtime::{UserContext,EntityKey,MutationPlan,MutationOperation,MutationOperationKind,MutationPolicy,MutationPolicyIdentity,MutationDecision,MutationVerdict,MutationPolicyApproval,MutationGovernanceEvent,MutationGovernanceSnapshot,RuntimeError};
struct Policy{input:J,notes:Arc<Mutex<Vec<String>>>}
impl MutationPolicy for Policy {
 fn identity(&self)->MutationPolicyIdentity{self.notes.lock().unwrap().push("identity".into());MutationPolicyIdentity::new(" policy ","5.0.5","fingerprint")}
 fn review(&self,_:&UserContext,_:&MutationPlan)->MutationDecision{
  self.notes.lock().unwrap().push("review".into());
  if self.input["policy"]=="throw"{panic!("policy exception")}
  let mut d=MutationDecision::allow();
  if self.input["policy"]=="deny"{d.verdict=MutationVerdict::Deny;d.code=self.input["code"].as_str().map(String::from);d.message=self.input["message"].as_str().map(String::from);d.field_paths=vec!["unrendered.path".into()];}d
 }
}
fn snapshot(s:&MutationGovernanceSnapshot)->J{json!({"source":format!("{:?}",s.source),"approval":format!("{:?}",s.approval_status),"warnings":s.warning_codes,"operations":s.operations.iter().map(|o|json!({"kind":format!("{:?}",o.kind),"entity":o.entity.entity,"fields":o.changed_fields})).collect::<Vec<_>>()})}
fn evaluate(j:J)->J {
 let notes=Arc::new(Mutex::new(Vec::<String>::new()));let events=Arc::new(Mutex::new(Vec::<J>::new()));
 let p:Arc<dyn MutationPolicy>=Arc::new(Policy{input:j.clone(),notes:notes.clone()});
 let missing=j["policy"]=="none";let registry_throw=j["policy"]=="registry_throw";
 let n=notes.clone();let mut c=UserContext::default().with_mutation_policy_registry(move |_:&str|{n.lock().unwrap().push("resolve".into());if registry_throw{panic!("registry exception")}if missing{None}else{Some(p.clone())}});
 let n=notes.clone();let approval=j["approval"].as_str().unwrap().to_owned();
 c.set_mutation_policy_approval_provider(move |identity:&MutationPolicyIdentity|{n.lock().unwrap().push("approval".into());if approval=="throw"{panic!("approval exception")}if approval=="none"{return None}let mut a=MutationPolicyApproval::new(identity.clone(),"reviewer",SystemTime::UNIX_EPOCH);match approval.as_str(){"id"=>a.policy.id.push('x'),"version"=>a.policy.version.push('x'),"fingerprint"=>a.policy.fingerprint.push('x'),"trim"=>a.policy.id="policy".into(),"blank"=>a.approved_by="\u{2003}".into(),_=>()}Some(a)});
 let n=notes.clone();let e=events.clone();let sink=j["sink"].as_str().unwrap().to_owned();
 c.set_mutation_governance_sink(move |_:&UserContext,event:&MutationGovernanceEvent|{n.lock().unwrap().push("sink".into());e.lock().unwrap().push(json!({"first":event.first_occurrence,"warning":event.warning_code,"snapshot":snapshot(&event.snapshot)}));if sink=="throw"{panic!("sink exception")}if sink=="error"{return Err(RuntimeError::Event("delivery failed".into()))}Ok(())});
 let mut outcomes=Vec::new();
 for _ in 0..j["repeat"].as_u64().unwrap(){
  let plan=MutationPlan{execution_id:"execution".into(),request_key:j["request"].as_str().unwrap().into(),root_entity_type:"Order".into(),audit_reason:Some("not included in summary".into()),operations:[MutationOperationKind::Create,MutationOperationKind::Update,MutationOperationKind::Delete,MutationOperationKind::Recover].into_iter().enumerate().map(|(i,kind)|MutationOperation{kind,entity:EntityKey::new("Order",i as u64),original_version:Some(9),changed_values:BTreeMap::from([("z".into(),Value::Text("private".into())),("a".into(),Value::Null)])}).collect()};
  let result=catch_unwind(AssertUnwindSafe(||c.review_mutation_plan(&plan)));
  outcomes.push(match result{Ok(Ok(s))=>snapshot(&s),Ok(Err(RuntimeError::Policy(s)))=>json!({"error":s}),Ok(Err(_))=>panic!("unexpected error"),Err(_)=>json!({"panic":true})});
 }
 let output=json!({"outcomes":outcomes,"notes":*notes.lock().unwrap(),"events":*events.lock().unwrap()});output
}
fn main(){std::panic::set_hook(Box::new(|_|{}));unsafe{std::env::set_var("TEAQL_LOG_ENDPOINT","off");}for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
