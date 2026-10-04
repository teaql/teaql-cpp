use std::{collections::BTreeMap,future::Future,io::{self,BufRead},pin::Pin,sync::{Arc,atomic::{AtomicU64,Ordering}},task::{Context,Poll,Wake,Waker}};
use serde_json::{Value as J,json};
use futures_util::lock::{Mutex,OwnedMutexGuard,OwnedMutexLockFuture};
mod locks {include!(concat!(env!("OUT_DIR"),"/id_set_build_lock.rs"));}
#[derive(Default)]struct Count(AtomicU64);
impl Wake for Count{fn wake(self:Arc<Self>){self.0.fetch_add(1,Ordering::Relaxed);}fn wake_by_ref(self:&Arc<Self>){self.0.fetch_add(1,Ordering::Relaxed);}}
struct Task{future:Option<Pin<Box<OwnedMutexLockFuture<()>>>>,guard:Option<OwnedMutexGuard<()>>,wake:Arc<Count>,state:&'static str}
impl Task{
 fn new(lock:Arc<Mutex<()>>)->Self{Self{future:Some(Box::pin(lock.lock_owned())),guard:None,wake:Arc::default(),state:"pending"}}
 fn poll(&mut self)->bool{if self.state=="ready"{return true}assert_eq!(self.state,"pending");let waker=Waker::from(self.wake.clone());let mut context=Context::from_waker(&waker);match self.future.as_mut().unwrap().as_mut().poll(&mut context){Poll::Ready(guard)=>{self.guard=Some(guard);self.future=None;self.state="ready";true},Poll::Pending=>false}}
 fn cancel(&mut self){if self.state=="pending"{self.future=None;self.state="cancelled";}}
}
fn evaluate(j:&J)->J{
 let mut handles=BTreeMap::<u64,Arc<Mutex<()>>>::new();let mut tasks=BTreeMap::<u64,Task>::new();let mut out=vec![];
 for step in j["steps"].as_array().unwrap(){let mut row=json!({});let h=step["handle"].as_u64().unwrap_or(0);let task=step["task"].as_u64().unwrap_or(0);
  match step["op"].as_str().unwrap(){
   "borrow"=>{handles.insert(h,locks::id_set_build_lock(step["key"].as_str().unwrap()));},
   "same"=>row["same"]=json!(Arc::ptr_eq(&handles[&h],&handles[&step["other"].as_u64().unwrap()])),
   "lock"=>{tasks.insert(task,Task::new(handles[&h].clone()));},
   "poll"=>row["ready"]=json!(tasks.get_mut(&task).unwrap().poll()),
   "cancel"=>tasks.get_mut(&task).unwrap().cancel(),
   "release"=>{tasks.get_mut(&task).unwrap().guard=None;},
   "reset"=>{handles.remove(&h);},
   _=>panic!("unknown lock operation")
  }
  row["tasks"]=json!(tasks.iter().map(|(id,t)|(id.to_string(),json!({"state":t.state,"held":t.guard.is_some(),"wakes":t.wake.0.load(Ordering::Relaxed)}))).collect::<BTreeMap<_,_>>());out.push(row);
 }json!(out)
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(&serde_json::from_str::<J>(&line.unwrap()).unwrap()));}}
