use std::{future::Future, pin::Pin, rc::Rc};
use teaql_data_service::TransactionExecutor;
use teaql_runtime::{RuntimeError, TransactionScope, UserContext};

fn assert_send(_: impl Send) {}

#[allow(dead_code)]
fn context_contract<E>(user_context: &UserContext)
where
    E: TransactionExecutor + Send + Sync + 'static,
    for<'a> E::Tx<'a>: Send + Sync,
{
    assert_send(user_context.execute_in_send_transaction::<E, (), _>(|_| {
        Box::pin(async { Ok(()) })
    }));
    let _ordinary = user_context.execute_in_transaction::<E, (), _>(|_| {
        Box::pin(async {
            let local = Rc::new(17);
            std::future::ready(()).await;
            assert_eq!(*local, 17);
            Ok(())
        })
    });
    #[cfg(feature = "bad_context")]
    let _invalid = user_context.execute_in_send_transaction::<E, (), _>(|_| {
        Box::pin(async {
            let local = Rc::new(17);
            std::future::ready(()).await;
            assert_eq!(*local, 17);
            Ok(())
        })
    });
}

#[allow(dead_code)]
fn scope_contract<'a, E>(scope: TransactionScope<'a, E>)
where
    E: TransactionExecutor + Send + Sync + 'static,
    for<'b> E::Tx<'b>: Send + Sync,
{
    #[cfg(not(feature = "bad_scope"))]
    assert_send(scope.execute_send(|_| Box::pin(async { Ok::<(), RuntimeError>(()) })));
    #[cfg(feature = "bad_scope")]
    let _invalid = scope.execute_send(|_| {
        Box::pin(async {
            let local = Rc::new(17);
            std::future::ready(()).await;
            assert_eq!(*local, 17);
            Ok::<(), RuntimeError>(())
        }) as Pin<Box<dyn Future<Output = Result<(), RuntimeError>> + Send>>
    });
}

fn main() {}
