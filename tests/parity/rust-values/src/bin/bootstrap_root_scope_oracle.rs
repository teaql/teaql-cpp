//! Observe active-root installation through the unchanged pinned schema pipeline.
use std::{future::Future, pin::Pin, sync::Arc, task::{Context, Poll, Wake, Waker}};
use teaql_runtime::{GeneratedSchemaBootstrapFuture, RuntimeError, RuntimeModule, SchemaInvocation, SchemaProvider, UserContext};

struct Provider;
impl SchemaProvider for Provider {
    fn ensure_schema<'a>(&'a self, _: &'a UserContext, _: &'a SchemaInvocation) -> Pin<Box<dyn Future<Output = Result<(), RuntimeError>> + Send + 'a>> {
        Box::pin(async { Ok(()) })
    }
}
fn install(context: &UserContext) -> GeneratedSchemaBootstrapFuture<'_> {
    Box::pin(async move {
        context.set_generated_bootstrap_active_root("Tenant", 42)?;
        context.set_generated_bootstrap_active_root("Tenant", 42)?;
        assert!(matches!(context.set_generated_bootstrap_active_root("Tenant", 43), Err(RuntimeError::Schema(message)) if message == "active root already set to tenant:42"));
        assert!(matches!(context.set_generated_bootstrap_active_root("", 0), Err(RuntimeError::Schema(message)) if message == "invalid generated active root"));
        assert_eq!(context.require_active_root("Tenant").unwrap().id, 42);
        Ok(())
    })
}
struct Noop;
impl Wake for Noop { fn wake(self: Arc<Self>) {} }
fn main() {
    let mut context = RuntimeModule::new().generated_schema_bootstrap(install).into_context();
    context.set_schema_provider(Provider);
    assert!(context.require_active_root("Tenant").is_err());
    let mut future = Box::pin(context.ensure_schema());
    let waker = Waker::from(Arc::new(Noop));
    let mut task = Context::from_waker(&waker);
    assert!(matches!(future.as_mut().poll(&mut task), Poll::Ready(Ok(()))));
    drop(future);
    assert_eq!(context.require_active_root("Tenant").unwrap().id, 42);
    println!("bootstrap root accepted; same assignment idempotent; conflict and invalid root reject");
}
