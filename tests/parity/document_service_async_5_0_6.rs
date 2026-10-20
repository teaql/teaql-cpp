use serde_json::{Value as Json, json};
use std::{
    future::Future,
    io::{self, BufRead},
    pin::Pin,
    sync::{
        Arc, Mutex,
        atomic::{AtomicUsize, Ordering},
    },
    task::{Context, Poll, Wake, Waker},
    time::Duration,
};
use teaql_runtime::*;

#[derive(Default)]
struct State {
    ready: bool,
    calls: usize,
    polls: usize,
    cancelled: usize,
    finished: usize,
    waker: Option<Waker>,
    events: Vec<Json>,
}
struct Marker(String);
struct CountWake(AtomicUsize);
impl Wake for CountWake {
    fn wake(self: Arc<Self>) {
        self.0.fetch_add(1, Ordering::SeqCst);
    }
}
struct Guard {
    state: Arc<Mutex<State>>,
    finished: bool,
}
impl Drop for Guard {
    fn drop(&mut self) {
        let mut state = self.state.lock().unwrap();
        if self.finished {
            state.finished += 1;
        } else {
            state.cancelled += 1;
        }
        state.waker = None;
    }
}
struct Service {
    state: Arc<Mutex<State>>,
    label: String,
    error: String,
}
fn error(kind: &str) -> Option<DocumentRoundTripError> {
    match kind {
        "revision" => Some(DocumentRoundTripError::revision_conflict()),
        "validation" => Some(DocumentRoundTripError::validation_failed()),
        "projection" => Some(DocumentRoundTripError::projection_violation()),
        "not_found" => Some(DocumentRoundTripError::not_found()),
        "invalid" => Some(DocumentRoundTripError::invalid()),
        "expired" => Some(DocumentRoundTripError::expired()),
        "scope" => Some(DocumentRoundTripError::scope_mismatch()),
        "authorization" => Some(DocumentRoundTripError::authorization_required()),
        "configuration" => Some(DocumentRoundTripError::configuration()),
        _ => None,
    }
}
impl Service {
    async fn pending(&self) {
        let mut guard = Guard {
            state: self.state.clone(),
            finished: false,
        };
        std::future::poll_fn(|cx| {
            let mut state = self.state.lock().unwrap();
            state.polls += 1;
            if state.ready {
                guard.finished = true;
                Poll::Ready(())
            } else {
                state.waker = Some(cx.waker().clone());
                Poll::Pending
            }
        })
        .await;
    }
    fn record(
        &self,
        user_context: &UserContext,
        principal: &TrustedReferencePrincipal,
        method: &str,
        request: Json,
    ) {
        let marker = user_context.get_resource::<Marker>().map(|v| v.0.clone());
        let mut state = self.state.lock().unwrap();
        state.calls += 1;
        state.events.push(
            json!({"service":self.label,"method":method,"subject":principal.subject,
            "realm":principal.authentication_realm,"root_type":principal.domain_root_type,
            "root_id":principal.domain_root_id,"marker":marker,"request":request}),
        );
    }
}
#[async_trait::async_trait]
impl ContextBoundDocumentService for Service {
    async fn open_document(
        &self,
        user_context: &UserContext,
        principal: &TrustedReferencePrincipal,
        request: DocumentOpenRequest,
    ) -> Result<ContextualDocument, DocumentRoundTripError> {
        self.record(user_context,principal,"open",json!({"business":request.business_id,"purpose":request.purpose,"type":request.aggregate_type,
            "seconds":request.lifetime.as_secs(),"nanos":request.lifetime.subsec_nanos()}));
        self.pending().await;
        if let Some(e) = error(&self.error) {
            return Err(e);
        }
        Ok(ContextualDocument {
            business_id: request.business_id,
            document_token: self.label.clone(),
            body: json!({"service":self.label}),
        })
    }
    async fn accept_document(
        &self,
        user_context: &UserContext,
        principal: &TrustedReferencePrincipal,
        request: DocumentAcceptRequest,
    ) -> Result<AcceptedContextualDocument, DocumentRoundTripError> {
        self.record(user_context,principal,"accept",json!({"business":request.submitted.business_id,"purpose":request.purpose,"type":request.expected_aggregate_type,"command":request.submitted.command_id,"token":request.submitted.document_token,"body":request.submitted.body}));
        self.pending().await;
        if let Some(e) = error(&self.error) {
            return Err(e);
        }
        Ok(AcceptedContextualDocument {
            document: ContextualDocument {
                business_id: request.submitted.business_id,
                document_token: self.label.clone(),
                body: request.submitted.body,
            },
            previous_revision: 4,
            new_revision: 5,
            changed_entities: vec![
                ReferenceIdentity::new("Order", 10, 5).unwrap(),
                ReferenceIdentity::new("Line", 21, 2).unwrap(),
            ],
        })
    }
}
fn phase(user_context: &UserContext, state: &Arc<Mutex<State>>, input: &Json) -> Json {
    state.lock().unwrap().ready = false;
    let counter = Arc::new(CountWake(AtomicUsize::new(0)));
    let waker = Waker::from(counter.clone());
    let mut cx = Context::from_waker(&waker);
    let mut future: Pin<Box<dyn Future<Output = Result<Json, DocumentRoundTripError>> + '_>> =
        if input["method"] == "open" {
            Box::pin(async {
                user_context.open_document(DocumentOpenRequest { aggregate_type:"Order".into(),business_id:"ORD-source".into(),purpose:"edit-source".into(),lifetime:Duration::from_secs(60) }).await.map(|v|json!({"business":v.business_id,"token":v.document_token,"body":v.body}))
            })
        } else {
            Box::pin(async {
                user_context.accept_document(DocumentAcceptRequest { expected_aggregate_type:"Order".into(),purpose:"edit-source".into(),submitted:SubmittedContextualDocument { business_id:"ORD-source".into(),document_token:"prior-token".into(),command_id:"command-source".into(),body:input["body"].clone() } }).await.map(|v|json!({"business":v.document.business_id,"token":v.document.document_token,"body":v.document.body,"previous":v.previous_revision,"next":v.new_revision,"changed":v.changed_entities.iter().map(|i|json!({"type":i.entity_type,"id":i.id,"version":i.version})).collect::<Vec<_>>()}))
            })
        };
    let finish = input["finish"].as_str().unwrap();
    let mut completed = None;
    if finish != "unpolled" {
        for _ in 0..input["pending"].as_u64().unwrap() {
            if let Poll::Ready(value) = future.as_mut().poll(&mut cx) {
                completed = Some(value);
                break;
            }
        }
    }
    let mut woke = false;
    if completed.is_none() && finish == "ready" {
        let old = counter.0.load(Ordering::SeqCst);
        let wake = {
            let mut s = state.lock().unwrap();
            s.ready = true;
            s.waker.take()
        };
        if let Some(w) = wake {
            w.wake();
        }
        woke = counter.0.load(Ordering::SeqCst) != old;
        if let Poll::Ready(value) = future.as_mut().poll(&mut cx) {
            completed = Some(value);
        } else {
            panic!("ready service remains pending");
        }
    }
    drop(future);
    match completed {
        Some(Ok(value)) => json!({"status":"ready","value":value,"woke":woke}),
        Some(Err(error)) => json!({"status":"error","error":error.code(),"woke":woke}),
        None => json!({"status":"cancelled","woke":woke}),
    }
}
fn observe(input: &Json) -> Json {
    let state = Arc::new(Mutex::new(State::default()));
    let mut user_context = UserContext::new();
    if input["principal"].as_bool().unwrap() {
        user_context.set_trusted_reference_principal(
            TrustedReferencePrincipal::new("oidc", "alice", "Merchant", 7).unwrap(),
        );
    }
    if input["marker"].as_bool().unwrap() {
        user_context.insert_resource(Marker("marker-first".into()));
    }
    if input["service"].as_bool().unwrap() {
        user_context.set_context_bound_document_service(Arc::new(Service {
            state: state.clone(),
            label: input["label"].as_str().unwrap().into(),
            error: input["error"].as_str().unwrap().into(),
        }));
    }
    let mut phases = vec![phase(&user_context, &state, input)];
    if input["replace"].as_bool().unwrap() {
        user_context.set_context_bound_document_service(Arc::new(Service {
            state: state.clone(),
            label: "replacement-service".into(),
            error: input["error"].as_str().unwrap().into(),
        }));
        user_context.set_trusted_reference_principal(
            TrustedReferencePrincipal::new("oidc", "bob", "Merchant", 7).unwrap(),
        );
        user_context.insert_resource(Marker("marker-replacement".into()));
        phases.push(phase(&user_context, &state, input));
    }
    drop(user_context);
    let state = state.lock().unwrap();
    json!({"phases":phases,"calls":state.calls,"polls":state.polls,"cancelled":state.cancelled,"finished":state.finished,"has_waker":state.waker.is_some(),"events":state.events})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
