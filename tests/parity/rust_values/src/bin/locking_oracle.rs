//! Direct unchanged UserContext lock calls; provider fixture only.
use std::{collections::BTreeMap,io::{self,BufRead},sync::{Arc,Mutex},time::Duration};
use serde_json::{json,Value as J};
use teaql_runtime::{UserContext,RemoteLockProvider};
struct Provider { index:usize, owners:Mutex<BTreeMap<String,String>>, calls:Arc<Mutex<Vec<J>>>, owner_ids:Arc<Mutex<BTreeMap<String,usize>>> }
impl Provider {
 fn note(&self,key:&str,owner:&str,timeout:u64,expiry:u64,unlock:bool){
  let mut ids=self.owner_ids.lock().unwrap();let n=ids.len();let id=*ids.entry(owner.into()).or_insert(n);
  self.calls.lock().unwrap().push(json!({"provider":self.index,"key":key,"owner":id,"timeout":timeout,"expiry":expiry,"unlock":unlock}));
 }
}
#[async_trait::async_trait]
impl RemoteLockProvider for Provider {
 async fn try_remote_lock(&self,key:&str,owner:&str,timeout:u64,expiry:u64)->bool {
  self.note(key,owner,timeout,expiry,false);let mut owners=self.owners.lock().unwrap();
  if owners.contains_key(key){return false}owners.insert(key.into(),owner.into());true
 }
 async fn unlock_remote(&self,key:&str,owner:&str)->bool {
  self.note(key,owner,0,0,true);let mut owners=self.owners.lock().unwrap();
  if owners.get(key).is_some_and(|saved|saved==owner){owners.remove(key);return true}false
 }
}
async fn evaluate(input:J,sequence:usize)->J {
 let calls=Arc::new(Mutex::new(vec![]));let ids=Arc::new(Mutex::new(BTreeMap::new()));
 let providers:Vec<Arc<dyn RemoteLockProvider>>=(0..2).map(|index|Arc::new(Provider{index,owners:Mutex::new(BTreeMap::new()),calls:calls.clone(),owner_ids:ids.clone()})as Arc<dyn RemoteLockProvider>).collect();
 let mut users:Vec<Option<UserContext>>=(0..3).map(|_|Some(UserContext::new())).collect();let mut outcomes=vec![];
 for step in input["steps"].as_array().unwrap(){
  let u=step["user"].as_u64().unwrap_or(0)as usize;let key=step["key"].as_str().unwrap_or("");let timeout=step["timeout"].as_u64().unwrap_or(0);let expiry=step["expiry"].as_u64().unwrap_or(0);
  let local=format!("locking-fixture:{sequence}:{key}");
  let outcome=match step["op"].as_str().unwrap(){
   "local"=>json!(users[u].as_ref().unwrap().try_local_lock(&local,timeout,expiry)),
   "unlock_local"=>{users[u].as_ref().unwrap().unlock_local(&local);J::Null},
   "remote"=>json!(users[u].as_ref().unwrap().try_remote_lock(key,timeout,expiry).await),
   "unlock_remote"=>json!(users[u].as_ref().unwrap().unlock_remote(key).await),
   "provider"=>{users[u].as_mut().unwrap().insert_resource(providers[step["provider"].as_u64().unwrap()as usize].clone());J::Null},
   "close"=>{users[u]=None;J::Null},"new"=>{users[u]=Some(UserContext::new());J::Null},
   "identifiers"=>{let user=users[u].as_mut().unwrap();user.set_user_identifier("changed");user.set_trace_id("changed");J::Null},
   "sleep"=>{std::thread::sleep(Duration::from_millis(step["millis"].as_u64().unwrap()));J::Null},
   _=>panic!("unknown script action"),
  };outcomes.push(outcome);
 }
 let result=json!({"results":outcomes,"calls":*calls.lock().unwrap()});result
}
fn main(){unsafe{std::env::set_var("TEAQL_LOG_ENDPOINT","off");}let r=tokio::runtime::Builder::new_current_thread().enable_all().build().unwrap();for(i,line)in io::stdin().lock().lines().enumerate(){println!("{}",r.block_on(evaluate(serde_json::from_str(&line.unwrap()).unwrap(),i)));}}
