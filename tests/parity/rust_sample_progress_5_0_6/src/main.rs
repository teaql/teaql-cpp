use serde_json::{Value as Json,json};
use std::{io::{self,BufRead},sync::{Mutex,atomic::{AtomicBool,Ordering}}};
mod observed {
    include!("observed.rs");
    pub fn run(input:&serde_json::Value)->serde_json::Value {observe(input)}
}
struct Collector {enabled:AtomicBool,records:Mutex<Vec<Json>>}
static LOGGER:Collector=Collector {enabled:AtomicBool::new(true),records:Mutex::new(Vec::new())};
impl log::Log for Collector {
    fn enabled(&self,metadata:&log::Metadata)->bool {
        self.enabled.load(Ordering::Relaxed)&&metadata.level()==log::Level::Info&&metadata.target().ends_with("::sample_data")
    }
    fn log(&self,record:&log::Record) {
        if self.enabled(record.metadata()) {
            self.records.lock().unwrap().push(json!({"level":record.level().to_string(),"target":record.target(),"message":record.args().to_string()}));
        }
    }
    fn flush(&self) {}
}
fn main() {
    log::set_logger(&LOGGER).unwrap();log::set_max_level(log::LevelFilter::Info);
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
        LOGGER.records.lock().unwrap().clear();LOGGER.enabled.store(input["enabled"].as_bool().unwrap(),Ordering::Relaxed);
        let outcome=observed::run(&input);let records=LOGGER.records.lock().unwrap().clone();
        println!("{}",json!({"outcome":outcome,"records":records}));
    }
}
