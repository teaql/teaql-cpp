use std::{collections::BTreeMap,io::{self,BufRead},future::Future,pin::pin,sync::Arc,task::{Context,Poll,Wake,Waker},time::{Duration,SystemTime,UNIX_EPOCH}};
use serde_json::{Value as J,json};use teaql_core::*;use teaql_runtime::UserContext;
#[path="../fixture_value.rs"]mod fixture_value;
#[allow(dead_code)]mod cache {
 include!(concat!(env!("OUT_DIR"),"/aggregation_cache.rs"));
 pub fn age(cache:&InMemoryAggregationCache,key:&str,millis:i64){if let Some(entry)=cache.entries.lock().unwrap().get_mut(key){let now=Instant::now();entry.stored_at=if millis>=0{now-Duration::from_millis(millis as u64)}else{now+Duration::from_millis(millis.unsigned_abs())}}}
 pub fn keys(cache:&InMemoryAggregationCache)->Vec<String>{let mut out=cache.entries.lock().unwrap().keys().cloned().collect::<Vec<_>>();out.sort();out}
}
#[allow(unused_imports)]mod pagination {
 include!(concat!(env!("OUT_DIR"),"/query_stores.rs"));
 pub fn cursor_store(max_entries:usize)->InMemoryContinuousPageCursorStore{InMemoryContinuousPageCursorStore{max_entries,..Default::default()}}
 pub fn id_store(max_entries:usize,max_bytes:usize)->InMemoryIdSetStore{InMemoryIdSetStore{max_entries,max_bytes,..Default::default()}}
 pub fn cursor_keys(store:&InMemoryContinuousPageCursorStore)->Vec<String>{let mut keys=store.cursors.lock().unwrap().keys().cloned().collect::<Vec<_>>();keys.sort();keys}
 pub fn id_keys(store:&InMemoryIdSetStore)->Vec<String>{let mut keys=store.sets.lock().unwrap().keys().cloned().collect::<Vec<_>>();keys.sort();keys}
 pub fn bytes(store:&InMemoryIdSetStore)->usize{InMemoryIdSetStore::retained_bytes(&store.sets.lock().unwrap())}
}
mod observations {
 use std::sync::Mutex;
 pub struct UserContext{continuous_page_observation:Mutex<(String,Option<String>)>,id_set_observation:Mutex<(String,Option<u64>)>}
 impl Default for UserContext{fn default()->Self{let real=teaql_runtime::UserContext::new();Self{continuous_page_observation:Mutex::new((real.continuous_page_plan().unwrap(),real.continuous_page_cursor_id())),id_set_observation:Mutex::new((real.id_set_plan().unwrap(),real.id_set_count()))}}}
 include!(concat!(env!("OUT_DIR"),"/pagination_observations.rs"));
}
use pagination::{ContinuousPageCursorStore,IdSetStore};
struct Noop;impl Wake for Noop{fn wake(self:Arc<Self>){}}
fn ready<F:Future>(f:F)->F::Output{let mut f=pin!(f);let waker=Waker::from(Arc::new(Noop));let mut cx=Context::from_waker(&waker);match f.as_mut().poll(&mut cx){Poll::Ready(value)=>value,Poll::Pending=>panic!("memory store suspended")}}
fn items<'a>(j:&'a J,key:&str)->&'a [J]{j[key].as_array().map(Vec::as_slice).unwrap_or(&[])}
fn rows(j:&J)->Vec<CompactRow>{j.as_array().unwrap().iter().map(|r|CompactRow::from_map(r.as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect())).collect()}
fn rows_out(rows:&[CompactRow])->J{json!(rows.iter().map(|r|r.clone().into_map().iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>()).collect::<Vec<_>>())}
fn expiry(j:&J)->SystemTime{let sec=j["expires"].as_i64().unwrap();let nanos=j["nanos"].as_u64().unwrap() as u32;if sec>=0{UNIX_EPOCH+Duration::new(sec as u64,nanos)}else{UNIX_EPOCH-Duration::from_secs(sec.unsigned_abs())+Duration::new(0,nanos)}}
fn time_out(t:SystemTime)->(i64,u32){match t.duration_since(UNIX_EPOCH){Ok(d)=>(d.as_secs() as i64,d.subsec_nanos()),Err(e)=>{let d=e.duration();if d.subsec_nanos()==0{(-(d.as_secs() as i128) as i64,0)}else{(-(d.as_secs() as i128)-1) as i64; ( (-(d.as_secs() as i128)-1) as i64,1_000_000_000-d.subsec_nanos())}}}}
fn cursor(j:&J)->pagination::ContinuousPageCursor{pagination::ContinuousPageCursor{cursor_id:j["cursor"].as_str().unwrap().into(),query_key:j["key"].as_str().unwrap().into(),entity:j["entity"].as_str().unwrap().into(),direction:if j["direction"]==0{SortDirection::Asc}else{SortDirection::Desc},boundary:fixture_value::value(&j["boundary"]),page_size:j["page_size"].as_u64().unwrap(),next_offset:j["offset"].as_u64().unwrap(),expires_at:expiry(j)}}
fn cursor_out(c:&pagination::ContinuousPageCursor)->J{let(s,n)=time_out(c.expires_at);json!({"cursor":c.cursor_id,"key":c.query_key,"entity":c.entity,"direction":c.direction as u8,"boundary":format!("{:?}",c.boundary),"page_size":c.page_size,"offset":c.next_offset,"expires":s,"nanos":n})}
fn ids(j:&J)->pagination::RetainedIdSet{pagination::RetainedIdSet{query_key:j["key"].as_str().unwrap().into(),ids:Arc::new(items(j,"ids").iter().map(|n|n.as_u64().unwrap()).collect()),expires_at:expiry(j)}}
fn ids_out(ids:&pagination::RetainedIdSet)->J{let(s,n)=time_out(ids.expires_at);json!({"key":ids.query_key,"ids":*ids.ids,"expires":s,"nanos":n})}
fn evaluate(j:&J)->J{
 let cache=cache::InMemoryAggregationCache::with_namespace(j["namespace"].as_str().unwrap());let cursors=pagination::cursor_store(j["max_entries"].as_u64().unwrap() as usize);let sets=pagination::id_store(j["max_entries"].as_u64().unwrap() as usize,j["max_bytes"].as_u64().unwrap() as usize);let context=observations::UserContext::default();let mut out=vec![];
 for step in items(j,"steps"){let key=step["key"].as_str().unwrap_or("");let op=step["op"].as_str().unwrap();let mut result=json!({});match j["mode"].as_str().unwrap(){
  "cache"=>{match op{"put"=>cache.put(key.into(),rows(&step["rows"])),"get"=>result["value"]=cache.get(key,step["age"].as_u64().unwrap()).as_deref().map(rows_out).unwrap_or(J::Null),"age"=>cache::age(&cache,key,step["millis"].as_i64().unwrap()),"clear"=>cache.clear(),_=>cache.invalidate_namespace(key)}result["keys"]=json!(cache::keys(&cache));result["namespace"]=json!(cache.namespace());},
  "cursor"=>{if op=="get"{result["value"]=ready(cursors.get(key,step["offset"].as_u64().unwrap())).unwrap().as_ref().map(cursor_out).unwrap_or(J::Null)}else{let future=if op=="put"{cursors.put(cursor(step))}else{cursors.invalidate(key)};if step["cancel"]==true{drop(future);result["cancelled"]=json!(true)}else if let Err(e)=ready(future){result["error"]=json!(e)}}result["keys"]=json!(pagination::cursor_keys(&cursors));},
  "ids"=>{if op=="get"{result["value"]=ready(sets.get(key)).unwrap().as_ref().map(ids_out).unwrap_or(J::Null)}else{let future=if op=="put"{sets.put(ids(step))}else{sets.invalidate(key)};if step["cancel"]==true{drop(future);result["cancelled"]=json!(true)}else if let Err(e)=ready(future){result["error"]=json!(e)}}result["keys"]=json!(pagination::id_keys(&sets));result["bytes"]=json!(pagination::bytes(&sets));},
  _=>{if op=="continuous"{context.observe_continuous_page(key,step["cursor"].as_str().map(str::to_owned))}else if op=="ids"{context.observe_id_set(key,step["count"].as_u64())}result=json!({"continuous":context.continuous_page_plan(),"cursor":context.continuous_page_cursor_id(),"id_plan":context.id_set_plan(),"count":context.id_set_count()});}
 }out.push(result)}json!(out)
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(&serde_json::from_str::<J>(&line.unwrap()).unwrap()));}}
