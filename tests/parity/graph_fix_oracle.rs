//! Original methods and scope keys; public audited save publishes observations.
use serde_json::{Value as J, json};
use std::{
    collections::BTreeMap,
    future::Future,
    io::{self, BufRead},
    pin::Pin,
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::{
    CompactRow, DataType, Entity, EntityDescriptor, EntityError, MutationValues,
    PropertyDescriptor, TeaqlEntity, Value, time::Timestamp,
};
use teaql_runtime::{
    __teaql_cpp_fix_scope, __teaql_cpp_install_save_hook, EntityValues, FixEvidence,
    FixEvidenceSource, GraphNode, JsonFieldNamingProfile, LedgerEntity, ObjectLocation,
    RuntimeError, RuntimeModule, UserContext, save_audited_ledger_entity,
};
mod fixture_value;
fn evidence(label: &str) -> FixEvidence {
    FixEvidence::new("Item", "userName", FixEvidenceSource::Clock, label)
}
fn describe(values: &[FixEvidence]) -> J {
    json!(
        values
            .iter()
            .map(|v| json!({"entity":v.entity_type,"path":v.model_path,
       "source":format!("{:?}",v.source),"label":v.source_label}))
            .collect::<Vec<_>>()
    )
}
struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
#[derive(Clone)]
struct Item;
impl TeaqlEntity for Item {
    const ENTITY_NAME: &'static str = "Item";
    fn entity_descriptor() -> EntityDescriptor {
        EntityDescriptor::new("Item")
            .property(PropertyDescriptor::new("id", DataType::U64).id().not_null())
    }
}
impl Entity for Item {
    fn from_compact_row(_: CompactRow) -> Result<Self, EntityError> {
        Ok(Self)
    }
    fn into_values(self) -> MutationValues {
        BTreeMap::from([("id".into(), Value::U64(7))]).into()
    }
}
impl LedgerEntity for Item {
    fn entity_runtime_state(&self) -> Option<teaql_runtime::EntityRuntimeState> {
        None
    }
}
#[derive(Clone)]
struct Settings {
    pending: usize,
    outcome: String,
    notes: Arc<Mutex<Vec<J>>>,
    first: Arc<Mutex<Option<i64>>>,
}
struct SaveBody<'a> {
    context: &'a UserContext,
    settings: Settings,
    node: Option<GraphNode>,
    polls: usize,
}
impl SaveBody<'_> {
    fn note(&self, label: &str) {
        let time = self.context.fix_time().0;
        let mut first = self.settings.first.lock().unwrap();
        let expected = *first.get_or_insert(time);
        self.settings
            .notes
            .lock()
            .unwrap()
            .push(json!({"label":label,"stable":time==expected}));
        self.context.record_fix_evidence(evidence(label));
    }
}
impl Future for SaveBody<'_> {
    type Output = Result<GraphNode, RuntimeError>;
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        let label = format!("poll:{}", self.polls);
        self.note(&label);
        self.polls += 1;
        if self.polls <= self.settings.pending {
            cx.waker().wake_by_ref();
            return Poll::Pending;
        }
        if self.settings.outcome == "panic" {
            panic!("fixture panic");
        }
        if self.settings.outcome == "error" {
            return Poll::Ready(Err(RuntimeError::Graph("fixture error".into())));
        }
        Poll::Ready(Ok(self.node.take().unwrap()))
    }
}
impl Drop for SaveBody<'_> {
    fn drop(&mut self) {
        if self.polls > 0 {
            self.note("drop");
        }
    }
}
fn save_hook(
    context: &UserContext,
    node: GraphNode,
) -> Pin<Box<dyn Future<Output = Result<GraphNode, RuntimeError>> + Send + '_>> {
    let settings = context.require_resource::<Settings>().unwrap().clone();
    Box::pin(SaveBody {
        context,
        settings,
        node: Some(node),
        polls: 0,
    })
}
fn nested<'a>(
    context: &'a UserContext,
    times: &'a [J],
    index: usize,
    trace: Arc<Mutex<Vec<J>>>,
) -> Pin<Box<dyn Future<Output = ()> + 'a>> {
    Box::pin(async move {
        let evidence = Arc::new(Mutex::new(Vec::new()));
        __teaql_cpp_fix_scope(
            Timestamp(times[index].as_i64().unwrap()),
            evidence.clone(),
            async {
                trace
                    .lock()
                    .unwrap()
                    .push(json!({"time":context.fix_time().0,"level":index,"phase":"before"}));
                context.record_fix_evidence(FixEvidence::new(
                    "Item",
                    "userName",
                    FixEvidenceSource::Context,
                    &format!("level:{index}"),
                ));
                if index + 1 < times.len() {
                    nested(context, times, index + 1, trace.clone()).await;
                }
                trace
                    .lock()
                    .unwrap()
                    .push(json!({"time":context.fix_time().0,"level":index,"phase":"after"}));
            },
        )
        .await;
        trace.lock().unwrap().push(json!({"evidence":describe(&evidence.lock().unwrap()),"level":index,"phase":"evidence"}));
    })
}
fn location(input: &J) -> ObjectLocation {
    let mut out = ObjectLocation::root();
    for part in input["path"].as_array().unwrap() {
        out = if let Some(name) = part.as_str() {
            out.member(name)
        } else {
            out.element(part.as_u64().unwrap() as usize)
        }
    }
    out
}
fn evaluate(input: J) -> J {
    let op = input["op"].as_str().unwrap();
    if op == "construct" {
        let result = std::panic::catch_unwind(|| {
            FixEvidence::new(
                input["entity"].as_str().unwrap(),
                input["path"].as_str().unwrap(),
                if input["source"] == 0 {
                    FixEvidenceSource::Clock
                } else {
                    FixEvidenceSource::Context
                },
                input["label"].as_str().unwrap(),
            )
        });
        return match result {
            Ok(value) => json!({"value":describe(&[value])}),
            Err(error) => {
                let message = error
                    .downcast_ref::<String>()
                    .cloned()
                    .or_else(|| error.downcast_ref::<&str>().map(|v| v.to_string()))
                    .unwrap();
                json!({"error":message})
            }
        };
    }
    let mut context = UserContext::new();
    context.__teaql_cpp_replace_fix_evidence(vec![evidence("seed")]);
    context.record_fix_evidence(evidence("outside"));
    let waker = Waker::from(Arc::new(NoWake));
    let mut cx = Context::from_waker(&waker);
    if op == "scope" {
        let trace = Arc::new(Mutex::new(Vec::new()));
        let mut future = Box::pin(nested(
            &context,
            input["times"].as_array().unwrap(),
            0,
            trace.clone(),
        ));
        assert!(future.as_mut().poll(&mut cx).is_ready());
        drop(future);
        return json!({"trace":*trace.lock().unwrap(),"last":describe(&context.last_fix_evidence())});
    }
    if op == "payload" {
        let mut module = RuntimeModule::new();
        let mut descriptor = EntityDescriptor::new("Item");
        for property in input["properties"].as_array().unwrap() {
            let mut p = PropertyDescriptor::new(property["name"].as_str().unwrap(), DataType::Text);
            if property["required"].as_bool().unwrap() {
                p = p.not_null();
            }
            if property["id"].as_bool().unwrap() {
                p = p.id();
            }
            if property["version"].as_bool().unwrap() {
                p = p.version();
            }
            descriptor = descriptor.property(p);
        }
        module = module.descriptor(descriptor);
        module.apply_to(&mut context);
        context
            .set_language_code(input["language"].as_str().unwrap())
            .unwrap();
        let values: EntityValues = input["values"]
            .as_object()
            .unwrap()
            .iter()
            .map(|(k, v)| (k.clone(), fixture_value::value(v)))
            .collect::<BTreeMap<_, _>>()
            .into();
        let outcome = context.__teaql_cpp_required_create(
            input["entity"].as_str().unwrap(),
            &values,
            &location(&input),
        );
        return match outcome {
            Ok(()) => json!({"error":null,"checks":[]}),
            Err(error) => {
                let checks = if let RuntimeError::Check(ref checks) = error {
                    json!(
                        checks
                            .iter()
                            .map(|v| v.to_wire(JsonFieldNamingProfile::CamelCase))
                            .collect::<Vec<_>>()
                    )
                } else {
                    J::Null
                };
                json!({"error":error.to_string(),"checks":checks})
            }
        };
    }
    let notes = Arc::new(Mutex::new(Vec::new()));
    let first = Arc::new(Mutex::new(None));
    context.insert_resource(Settings {
        pending: input["pending"].as_u64().unwrap() as usize,
        outcome: input["outcome"].as_str().unwrap().into(),
        notes: notes.clone(),
        first,
    });
    __teaql_cpp_install_save_hook(&mut context, save_hook);
    RuntimeModule::new().descriptor(Item::entity_descriptor()).apply_to(&mut context);
    let mut future = Some(Box::pin(save_audited_ledger_entity(
        Item.audit_as("fixture"),
        &context,
    )));
    let mut steps = Vec::new();
    for _ in 0..input["polls"].as_u64().unwrap() {
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            future.as_mut().unwrap().as_mut().poll(&mut cx)
        }));
        let status = match result {
            Ok(Poll::Pending) => "pending",
            Ok(Poll::Ready(Ok(_))) => "ok",
            Ok(Poll::Ready(Err(_))) => "error",
            Err(_) => "panic",
        };
        if status == "panic" {
            drop(future.take());
        }
        steps.push(json!({"status":status,"last":describe(&context.last_fix_evidence())}));
        if status != "pending" {
            break;
        }
    }
    drop(future);
    json!({"steps":steps,"notes":*notes.lock().unwrap(),"last":describe(&context.last_fix_evidence())})
}
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        println!(
            "{}",
            evaluate(serde_json::from_str(&line.unwrap()).unwrap())
        );
    }
}
