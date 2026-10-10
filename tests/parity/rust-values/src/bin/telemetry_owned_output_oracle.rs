//! Compile and observe the original run methods with a non-Clone output.
use std::{future::Future,io::{self,BufRead},sync::Arc,task::{Context,Poll,Waker}};
use serde_json::{json,Value};
use teaql_runtime::{NoopRuntimeTelemetry,RuntimeOperation,RuntimeTelemetry,start_runtime_operation,extract_runtime_context};
struct OwnedOutput(String);
fn ready<F:Future>(future:F)->F::Output {
    let mut future=std::pin::pin!(future);
    match future.as_mut().poll(&mut Context::from_waker(Waker::noop())) {
        Poll::Ready(value)=>value,Poll::Pending=>panic!("ready owned fixture"),
    }
}
fn evaluate(input:Value)->Value {
    let telemetry:Arc<dyn RuntimeTelemetry>=Arc::new(NoopRuntimeTelemetry);
    let output=OwnedOutput(input["text"].as_str().unwrap().to_owned());
    let result=if input["propagation"]==true {
        let scope=extract_runtime_context(&telemetry,&Default::default());
        ready(scope.run(async move {output}))
    } else {
        let scope=start_runtime_operation(&telemetry,RuntimeOperation::new("query","owned"));
        ready(scope.run(async move {output}))
    };
    json!({"text":result.0})
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
