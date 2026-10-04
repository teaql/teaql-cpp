use std::io::{self,BufRead};
use std::sync::{Arc,Mutex};
use serde_json::{Value as J,json};
use teaql_core::business_id::*;
use teaql_runtime::{UserContext,BusinessClock,BusinessDate,FixedBusinessClock,BusinessIdEncodingKey,BusinessIdKeyProvider,BusinessIdSchemaContributor,InMemoryBusinessIdAllocator};
type Log=Arc<Mutex<Vec<String>>>;
fn error(e:BusinessIdError)->J{json!({"code":format!("{:?}",e.code),"message":e.message,"display":e.to_string()})}
fn definition(j:&J)->BusinessIdDefinition{BusinessIdDefinition{field_name:j["field"].as_str().unwrap().into(),profile:j["profile"].as_str().unwrap().into(),prefix:j["prefix"].as_str().unwrap().into(),namespace:j["namespace"].as_str().unwrap().into(),separator:j["separator"].as_str().unwrap().into(),split_by_date:j["split"].as_bool().unwrap(),preserve_digits:j["digits"].as_u64().unwrap() as u8}}
fn scope(j:&J)->BusinessIdScope{let a=j.as_array().unwrap();BusinessIdScope{domain_root_key:a[0].as_str().unwrap().into(),aggregate_type:a[1].as_str().unwrap().into(),namespace:a[2].as_str().unwrap().into(),period_key:a[3].as_str().unwrap().into()}}
fn plan(j:&J)->BusinessIdPlan{BusinessIdPlan{scope:scope(&j["scope"]),prefix:"P".into(),date_text:None,preserve_digits:6,separator:"-".into(),initial_sequence:j["initial"].as_str().unwrap().parse().unwrap(),max_sequence:j["maximum"].as_str().unwrap().parse().unwrap()}}
struct Clock{time:chrono::DateTime<chrono::Utc>,date:Option<chrono::NaiveDate>,log:Log}
impl BusinessClock for Clock{fn business_time(&self)->chrono::DateTime<chrono::Utc>{self.log.lock().unwrap().push("time".into());self.time}fn business_date(&self)->chrono::NaiveDate{self.log.lock().unwrap().push("date".into());self.date.unwrap_or_else(||self.business_time().date_naive())}}
struct Key{mode:String,log:Log}
impl BusinessIdKeyProvider for Key{fn current_key(&self,_:&UserContext,_:&BusinessIdDefinition)->Result<BusinessIdEncodingKey,BusinessIdError>{self.log.lock().unwrap().push("key".into());if self.mode=="error"{Err(BusinessIdError::new(BusinessIdErrorCode::KeyNotFound,"key unavailable"))}else{BusinessIdEncodingKey::new(if self.mode=="rotated"{2}else{1},[7;32])}}}
struct Allocator{inner:InMemoryBusinessIdAllocator,mode:String,log:Log}
impl BusinessIdAllocator for Allocator{fn allocate(&self,p:&BusinessIdPlan)->Result<BusinessIdAllocation,BusinessIdError>{self.log.lock().unwrap().push("allocate".into());if self.mode=="error"{return Err(BusinessIdError::new(BusinessIdErrorCode::Allocation,"allocator unavailable"));}let mut a=self.inner.allocate(p)?;if self.mode=="mismatch"{a.scope.period_key.push('!');}if self.mode=="outside"{a.sequence=u64::MAX;}Ok(a)}}
impl BusinessIdSchemaContributor for Allocator{fn ensure_schema(&self,_:&UserContext)->Result<(),BusinessIdError>{self.log.lock().unwrap().push("schema".into());Ok(())}}
struct Slot{value:Option<String>,new:bool,log:Log}
impl BusinessIdSlot for Slot{fn current_business_id(&self)->Option<&str>{self.log.lock().unwrap().push("current".into());self.value.as_deref()}fn is_new_aggregate(&self)->bool{self.log.lock().unwrap().push("fresh".into());self.new}fn assign_business_id(&mut self,v:BusinessIdValue){self.log.lock().unwrap().push("assign".into());self.value=Some(v.0)}}
fn date(j:&J)->Option<chrono::NaiveDate>{j.as_str().map(|s|chrono::NaiveDate::parse_from_str(s,"%Y-%m-%d").unwrap())}
fn evaluate(j:J)->J{
    let op=j["op"].as_str().unwrap();
    if op=="allocate"{let allocator=InMemoryBusinessIdAllocator::default();return json!(j["plans"].as_array().unwrap().iter().map(|p|match allocator.allocate(&plan(p)){Ok(a)=>json!({"sequence":a.sequence.to_string(),"scope":a.scope.canonical_key()}),Err(e)=>error(e)}).collect::<Vec<_>>());}
    let seconds=j["seconds"].as_str().unwrap().parse().unwrap();let nanos=j["nanos"].as_u64().unwrap() as u32;let time=match chrono::DateTime::from_timestamp(seconds,nanos){Some(t)=>t,None=>return json!({"valid":false})};
    let mut context=UserContext::new();
    if op=="time"{context.set_business_clock(FixedBusinessClock::new(time));if let Some(d)=date(&j["override"]){context.insert_resource(BusinessDate(d));}let observed=context.business_time();return json!({"valid":true,"seconds":observed.timestamp().to_string(),"nanos":observed.timestamp_subsec_nanos(),"date":observed.date_naive().to_string(),"businessDate":context.business_date().to_string()});}
    let log=Arc::new(Mutex::new(vec![]));context.set_business_clock(Clock{time,date:date(&j["clock_date"]),log:log.clone()});if let Some(d)=date(&j["override"]){context.insert_resource(BusinessDate(d));}
    let allocator=Allocator{inner:InMemoryBusinessIdAllocator::default(),mode:j["allocator"].as_str().unwrap().into(),log:log.clone()};context.set_business_id_infrastructure(allocator);
    let mut results=vec![];
    for step in j["steps"].as_array().unwrap(){if let Some(mode)=step["key"].as_str(){if mode!="missing"{context.set_business_id_key_provider(Key{mode:mode.into(),log:log.clone()}).unwrap();}}
        let mut slot=Slot{value:step["existing"].as_str().map(str::to_owned),new:step["new"].as_bool().unwrap(),log:log.clone()};let d=definition(&step["definition"]);
        let result=context.business_ids().unwrap().ensure(&context,&d,step["root"].as_str().unwrap(),step["aggregate"].as_str().unwrap(),&mut slot);
        let observed=match result{Ok(v)=>json!({"ok":v.0}),Err(e)=>error(e)};let logs=std::mem::take(&mut *log.lock().unwrap());results.push(json!({"result":observed,"slot":slot.value,"calls":logs}));
    }json!(results)
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
