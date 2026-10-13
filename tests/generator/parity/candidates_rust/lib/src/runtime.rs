
use crate::*;
use teaql_core::TeaqlEntity;

use teaql_provider_sqlite::SqliteProviderExt as _;

pub type DataServiceDialect = teaql_provider_sqlite::SqliteDialect;
pub type DataServiceMutationExecutor = teaql_provider_sqlite::SqliteMutationExecutor;
pub type DataServiceMutationError = teaql_provider_sqlite::MutationExecutorError;
pub type DataServiceIdGenerator = teaql_provider_sqlite::SqliteIdSpaceGenerator;
pub type DataServicePool = std::sync::Arc<std::sync::Mutex<rusqlite::Connection>>;
pub type DataServiceExecutor = ServiceRuntimeExecutor;
pub type ServiceRuntime = teaql_runtime::UserContext;

pub const DATABASE_URL_ENV: &str = "CPP_CANDIDATES_CORE_DATABASE_URL";
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ServiceRuntimeConfig {
    pub database_url: String,
}

impl ServiceRuntimeConfig {
    pub fn from_env() -> Result<Self, ServiceRuntimeError> {
        Ok(Self {
            database_url: env_value(DATABASE_URL_ENV)?,
        })
    }
}

#[derive(Debug)]
pub enum ServiceRuntimeError {
    MissingEnv {
        name: &'static str,
        source: std::env::VarError,
    },
    ConnectionError(String),
    Configuration(String),
    Rusqlite(rusqlite::Error),
    Runtime(teaql_runtime::RuntimeError),
}

impl std::fmt::Display for ServiceRuntimeError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ServiceRuntimeError::MissingEnv { name, source } => {
                write!(f, "missing environment variable {name}: {source}")
            }
            ServiceRuntimeError::ConnectionError(err) => write!(f, "connection error: {err}"),
            ServiceRuntimeError::Configuration(err) => write!(f, "configuration error: {err}"),
            ServiceRuntimeError::Rusqlite(err) => write!(f, "rusqlite error: {err}"),
            ServiceRuntimeError::Runtime(err) => write!(f, "runtime error: {err}"),
        }
    }
}

impl std::error::Error for ServiceRuntimeError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            ServiceRuntimeError::MissingEnv { source, .. } => Some(source),
            ServiceRuntimeError::ConnectionError(_) => None,
            ServiceRuntimeError::Configuration(_) => None,
            ServiceRuntimeError::Rusqlite(err) => Some(err),
            ServiceRuntimeError::Runtime(err) => Some(err),
        }
    }
}

impl From<rusqlite::Error> for ServiceRuntimeError {
    fn from(err: rusqlite::Error) -> Self {
        ServiceRuntimeError::Rusqlite(err)
    }
}
impl From<teaql_runtime::RuntimeError> for ServiceRuntimeError {
    fn from(err: teaql_runtime::RuntimeError) -> Self {
        ServiceRuntimeError::Runtime(err)
    }
}

#[derive(Clone)]
pub struct LocalSchemaProvider;

impl teaql_data_service::SchemaProvider for LocalSchemaProvider {
    fn get_entity(&self, name: &str) -> Option<std::sync::Arc<teaql_core::EntityDescriptor>> {
        match name {
            "Platform" => Some(std::sync::Arc::new(crate::Platform::entity_descriptor())),
            "OrderStatus" => Some(std::sync::Arc::new(crate::OrderStatus::entity_descriptor())),
            "Record" => Some(std::sync::Arc::new(crate::Record::entity_descriptor())),
            "Person" => Some(std::sync::Arc::new(crate::Person::entity_descriptor())),
            _ => None,
        }
    }
}

#[derive(Clone)]
pub struct ServiceRuntimeExecutor {
    inner: teaql_sql::SqlDataServiceExecutor<
        DataServiceDialect,
        DataServiceMutationExecutor,
        LocalSchemaProvider
    >,
}

impl ServiceRuntimeExecutor {
    pub fn new(inner: DataServiceMutationExecutor) -> Self {
        Self {
            inner: teaql_sql::SqlDataServiceExecutor::new(
                DataServiceDialect::default(),
                inner,
                LocalSchemaProvider
            ),
        }
    }

}

impl teaql_data_service::DataServiceExecutor for ServiceRuntimeExecutor {
    type Error = teaql_sql::SqlExecutorError<DataServiceMutationError>;
    fn capabilities(&self) -> teaql_data_service::DataServiceCapabilities {
        teaql_data_service::DataServiceExecutor::capabilities(&self.inner)
    }
}

impl teaql_data_service::QueryExecutor for ServiceRuntimeExecutor {
    fn dynamic_field_store(&self) -> Option<&dyn teaql_data_service::dynamic_fields::DynamicFieldStore> {
        teaql_data_service::QueryExecutor::dynamic_field_store(&self.inner)
    }

    async fn query(&self, request: teaql_data_service::QueryRequest) -> Result<teaql_data_service::QueryResult, Self::Error> {
        teaql_data_service::QueryExecutor::query(&self.inner, request).await
    }
}

impl teaql_data_service::StreamQueryExecutor for ServiceRuntimeExecutor {
    fn query_stream_with_dynamic_fields<'a>(
        &'a self,
        request: teaql_data_service::QueryRequest,
        chunk_size: usize,
        plan: teaql_data_service::dynamic_fields::DynamicFieldStreamPlan,
        observer: Option<teaql_data_service::ExecutionObserver<'a>>,
    ) -> Result<teaql_data_service::QueryStream<'a, Self::Error>, teaql_core::dynamic_fields::DynamicFieldError> {
        teaql_data_service::StreamQueryExecutor::query_stream_with_dynamic_fields(
            &self.inner, request, chunk_size, plan, observer,
        )
    }

    fn query_stream_observed<'a>(
        &'a self,
        request: teaql_data_service::QueryRequest,
        chunk_size: usize,
        observer: teaql_data_service::ExecutionObserver<'a>,
    ) -> teaql_data_service::QueryStream<'a, Self::Error> {
        teaql_data_service::StreamQueryExecutor::query_stream_observed(&self.inner, request, chunk_size, observer)
    }

    fn query_stream(&self, request: teaql_data_service::QueryRequest, chunk_size: usize) -> teaql_data_service::QueryStream<'_, Self::Error> {
        teaql_data_service::StreamQueryExecutor::query_stream(&self.inner, request, chunk_size)
    }
}

impl teaql_data_service::MutationExecutor for ServiceRuntimeExecutor {
    async fn mutate(&self, request: teaql_data_service::MutationRequest) -> Result<teaql_data_service::MutationResult, Self::Error> {
        teaql_data_service::MutationExecutor::mutate(&self.inner, request).await
    }
}

impl teaql_data_service::TransactionExecutor for ServiceRuntimeExecutor {
    type Tx<'a> = teaql_sql::SqlDataServiceTransaction<'a, DataServiceDialect, <DataServiceMutationExecutor as teaql_sql::SqlTransactionTransport>::Tx<'a>, LocalSchemaProvider> where Self: 'a;

    async fn begin(&self) -> Result<Self::Tx<'_ >, Self::Error> {
        teaql_data_service::TransactionExecutor::begin(&self.inner).await
    }
}

pub async fn service_runtime_from_env() -> Result<ServiceRuntime, ServiceRuntimeError> {
    service_runtime(ServiceRuntimeConfig::from_env()?).await
}

pub async fn service_runtime(config: ServiceRuntimeConfig) -> Result<ServiceRuntime, ServiceRuntimeError> {
    let pool = connect_data_service_pool(&config).await?;
    service_runtime_from_pool(pool).await
}

pub async fn service_runtime_from_pool(pool: DataServicePool) -> Result<ServiceRuntime, ServiceRuntimeError> {
    let mutation_executor = DataServiceMutationExecutor::new(pool);
    let id_generator = DataServiceIdGenerator::from_executor(mutation_executor.clone());let mut context = module_with_behaviors_and_checkers().into_context();
    context.set_internal_id_generator(id_generator);
    context.use_sqlite_provider(mutation_executor.clone());
    let executor = ServiceRuntimeExecutor::new(mutation_executor);
    context.register_executor(executor.clone());
    context.insert_resource(executor);

    // SQL logging is owned by teaql-runtime. The legacy teaql-tool-core
    // environment whitelist rejects runtime logging variables and must not
    // gate generated application startup. Schema remains explicit.

    Ok(context)
}



fn env_value(name: &'static str) -> Result<String, ServiceRuntimeError> {
    std::env::var(name).map_err(|source| ServiceRuntimeError::MissingEnv { name, source })
}

async fn connect_data_service_pool(config: &ServiceRuntimeConfig) -> Result<DataServicePool, ServiceRuntimeError> {
    let url = &config.database_url;
    let sanitized_url = if url.starts_with("sqlite:") { url.strip_prefix("sqlite:").unwrap().trim_start_matches("//") } else { url };
    let pure_file_path = sanitized_url.split('?').next().unwrap_or(sanitized_url);
    let path = std::path::Path::new(pure_file_path);
    if let Some(parent) = path.parent() { if !parent.as_os_str().is_empty() { std::fs::create_dir_all(parent).map_err(|e| ServiceRuntimeError::ConnectionError(e.to_string()))?; } }
    Ok(std::sync::Arc::new(std::sync::Mutex::new(rusqlite::Connection::open(pure_file_path).map_err(|e| ServiceRuntimeError::ConnectionError(e.to_string()))?)))
}

pub fn repository_registry() -> teaql_runtime::InMemoryEntityRegistry {
    teaql_runtime::InMemoryEntityRegistry::new()
        .with_entity("Platform")
        .with_entity("OrderStatus")
        .with_entity("Record")
        .with_entity("Person")
}

pub fn behavior_registry() -> teaql_runtime::InMemoryEntityDataServiceBehaviorRegistry {
    teaql_runtime::InMemoryEntityDataServiceBehaviorRegistry::new()
        .with_behavior("Platform", PlatformBehavior::default())
        .with_behavior("OrderStatus", OrderStatusBehavior::default())
        .with_behavior("Record", RecordBehavior::default())
        .with_behavior("Person", PersonBehavior::default())
}

pub fn checker_registry() -> teaql_runtime::InMemoryCheckerRegistry {
    teaql_runtime::InMemoryCheckerRegistry::new()
        .with_checker(teaql_runtime::TypedEntityChecker::<Platform, _>::new(PlatformChecker::default()))
        .with_checker(teaql_runtime::TypedEntityChecker::<OrderStatus, _>::new(OrderStatusChecker::default()))
        .with_checker(teaql_runtime::TypedEntityChecker::<Record, _>::new(RecordChecker::default()))
        .with_checker(teaql_runtime::TypedEntityChecker::<Person, _>::new(PersonChecker::default()))
}

fn is_generated_bootstrap_retryable_conflict(error: &teaql_runtime::RuntimeError) -> bool {
    if matches!(error, teaql_runtime::RuntimeError::OptimisticLockConflict { .. }) { return true }
    let teaql_runtime::RuntimeError::Graph(message) = error else { return false };
    (message.contains("UNIQUE constraint failed:") && message.contains("_data.id"))
        || (message.contains("duplicate key value violates unique constraint") && message.contains("_data_pkey"))
        || (message.contains("Duplicate entry") && message.contains("PRIMARY"))
}

fn ensure_generated_bootstrap<'a>(context: &'a teaql_runtime::UserContext) -> teaql_runtime::GeneratedSchemaBootstrapFuture<'a> {
    Box::pin(async move {
        for attempt in 0..5 {
            match ensure_generated_bootstrap_once(context).await {
                Ok(()) => return Ok(()),
                Err(error) if attempt < 4 && is_generated_bootstrap_retryable_conflict(&error) => tokio::task::yield_now().await,
                Err(error) => return Err(error),
            }
        }
        unreachable!("bootstrap attempts always return or fail")
    })
}

async fn ensure_generated_bootstrap_once(context: &teaql_runtime::UserContext) -> Result<(), teaql_runtime::RuntimeError> {
        use teaql_core::Entity as _;
        let root_rows = crate::Q::platforms().select_self_fields().with_id_is(1_u64).comment("what: locate generated Domain Root").purpose("why: idempotent runtime bootstrap").execute_for_list(context).await.map_err(|e| teaql_runtime::RuntimeError::Graph(e.to_string()))?;
        let domain_root = if let Some(entity) = root_rows.data.into_iter().next() { entity } else {
            let mut entity = Platform::runtime_new(context.entity_runtime_state());
            entity.update_id(1_u64);
            context.initialize_generated_bootstrap_entity(&mut entity, Platform::ENTITY_NAME, 1_u64)?;
            entity.update_name("string()");
            teaql_runtime::AuditedSaveExt::save(entity.audit_as("create generated Domain Root Platform"), context).await?
        };
        context.set_generated_bootstrap_active_root(Platform::ENTITY_NAME, domain_root.id())?;
        let rows_constant_order_status_11 = crate::Q::order_statuses().select_self_fields().with_id_is(11_u64).comment("what: locate generated constant").purpose("why: idempotent runtime bootstrap").execute_for_list(context).await.map_err(|e| teaql_runtime::RuntimeError::Graph(e.to_string()))?;
        if let Some(mut constant_order_status_11) = rows_constant_order_status_11.data.into_iter().next() {
            let mut changed = false;
            if constant_order_status_11.platform_id() != 1_u64 { constant_order_status_11.update_platform_id(1_u64); changed = true; }
            if constant_order_status_11.name() != "Draft order" { constant_order_status_11.update_name("Draft order"); changed = true; }
            if constant_order_status_11.code() != "DRAFT" { constant_order_status_11.update_code("DRAFT"); changed = true; }
            if changed { let _ = teaql_runtime::AuditedSaveExt::save(constant_order_status_11.audit_as("reconcile model constant OrderStatus(11)"), context).await?; }
        } else {
            let mut constant_order_status_11 = OrderStatus::runtime_new(context.entity_runtime_state());
            constant_order_status_11.update_id(11_u64);
            context.initialize_generated_bootstrap_entity(&mut constant_order_status_11, OrderStatus::ENTITY_NAME, 11_u64)?;
            constant_order_status_11.update_platform_id(1_u64);
            constant_order_status_11.update_name("Draft order");
            constant_order_status_11.update_code("DRAFT");
            let _ = teaql_runtime::AuditedSaveExt::save(constant_order_status_11.audit_as("create model constant OrderStatus(11)"), context).await?;
        }
        let rows_constant_order_status_22 = crate::Q::order_statuses().select_self_fields().with_id_is(22_u64).comment("what: locate generated constant").purpose("why: idempotent runtime bootstrap").execute_for_list(context).await.map_err(|e| teaql_runtime::RuntimeError::Graph(e.to_string()))?;
        if let Some(mut constant_order_status_22) = rows_constant_order_status_22.data.into_iter().next() {
            let mut changed = false;
            if constant_order_status_22.platform_id() != 1_u64 { constant_order_status_22.update_platform_id(1_u64); changed = true; }
            if constant_order_status_22.name() != "In progress order" { constant_order_status_22.update_name("In progress order"); changed = true; }
            if constant_order_status_22.code() != "IN_PROGRESS" { constant_order_status_22.update_code("IN_PROGRESS"); changed = true; }
            if changed { let _ = teaql_runtime::AuditedSaveExt::save(constant_order_status_22.audit_as("reconcile model constant OrderStatus(22)"), context).await?; }
        } else {
            let mut constant_order_status_22 = OrderStatus::runtime_new(context.entity_runtime_state());
            constant_order_status_22.update_id(22_u64);
            context.initialize_generated_bootstrap_entity(&mut constant_order_status_22, OrderStatus::ENTITY_NAME, 22_u64)?;
            constant_order_status_22.update_platform_id(1_u64);
            constant_order_status_22.update_name("In progress order");
            constant_order_status_22.update_code("IN_PROGRESS");
            let _ = teaql_runtime::AuditedSaveExt::save(constant_order_status_22.audit_as("create model constant OrderStatus(22)"), context).await?;
        }
        let rows_constant_order_status_2104194 = crate::Q::order_statuses().select_self_fields().with_id_is(2104194_u64).comment("what: locate generated constant").purpose("why: idempotent runtime bootstrap").execute_for_list(context).await.map_err(|e| teaql_runtime::RuntimeError::Graph(e.to_string()))?;
        if let Some(mut constant_order_status_2104194) = rows_constant_order_status_2104194.data.into_iter().next() {
            let mut changed = false;
            if constant_order_status_2104194.platform_id() != 1_u64 { constant_order_status_2104194.update_platform_id(1_u64); changed = true; }
            if constant_order_status_2104194.name() != "Completed order" { constant_order_status_2104194.update_name("Completed order"); changed = true; }
            if constant_order_status_2104194.code() != "DONE" { constant_order_status_2104194.update_code("DONE"); changed = true; }
            if changed { let _ = teaql_runtime::AuditedSaveExt::save(constant_order_status_2104194.audit_as("reconcile model constant OrderStatus(2104194)"), context).await?; }
        } else {
            let mut constant_order_status_2104194 = OrderStatus::runtime_new(context.entity_runtime_state());
            constant_order_status_2104194.update_id(2104194_u64);
            context.initialize_generated_bootstrap_entity(&mut constant_order_status_2104194, OrderStatus::ENTITY_NAME, 2104194_u64)?;
            constant_order_status_2104194.update_platform_id(1_u64);
            constant_order_status_2104194.update_name("Completed order");
            constant_order_status_2104194.update_code("DONE");
            let _ = teaql_runtime::AuditedSaveExt::save(constant_order_status_2104194.audit_as("create model constant OrderStatus(2104194)"), context).await?;
        }
        Ok(())
}


/// Canonical KSML field to selected JSON wire name, consumed by HTTP/TFP adapters.
pub fn generated_wire_field_mappings() -> std::collections::BTreeMap<String, std::collections::BTreeMap<String, String>> {
    std::collections::BTreeMap::from([
        ("Platform".to_owned(), std::collections::BTreeMap::from([
            ("id".to_owned(), "id".to_owned()),
            ("name".to_owned(), "name".to_owned()),
            ("version".to_owned(), "version".to_owned())
        ])),
        ("OrderStatus".to_owned(), std::collections::BTreeMap::from([
            ("id".to_owned(), "id".to_owned()),
            ("platform".to_owned(), "platform".to_owned()),
            ("name".to_owned(), "name".to_owned()),
            ("code".to_owned(), "code".to_owned()),
            ("version".to_owned(), "version".to_owned())
        ])),
        ("Record".to_owned(), std::collections::BTreeMap::from([
            ("id".to_owned(), "id".to_owned()),
            ("platform".to_owned(), "platform".to_owned()),
            ("name".to_owned(), "name".to_owned()),
            ("status".to_owned(), "status".to_owned()),
            ("phase".to_owned(), "phase".to_owned()),
            ("tag".to_owned(), "tag".to_owned()),
            ("version".to_owned(), "version".to_owned())
        ])),
        ("Person".to_owned(), std::collections::BTreeMap::from([
            ("id".to_owned(), "id".to_owned()),
            ("platform".to_owned(), "platform".to_owned()),
            ("name".to_owned(), "name".to_owned()),
            ("status".to_owned(), "status".to_owned()),
            ("phase".to_owned(), "phase".to_owned()),
            ("version".to_owned(), "version".to_owned())
        ]))
    ])
}

/// Accepted legacy aliases; empty until explicitly declared by the model.
pub fn generated_wire_field_aliases() -> std::collections::BTreeMap<String, std::collections::BTreeMap<String, String>> {
    std::collections::BTreeMap::new()
}

pub fn module() -> teaql_runtime::RuntimeModule {
    teaql_runtime::RuntimeModule::new()
        .entity::<Platform>()
        .entity::<OrderStatus>()
        .entity::<Record>()
        .entity::<Person>()
        .generated_schema_bootstrap(ensure_generated_bootstrap)
}

pub fn module_with_checkers() -> teaql_runtime::RuntimeModule {
    let mut module = teaql_runtime::RuntimeModule::new();
    module = module.entity::<Platform>();
    module = module.checker(teaql_runtime::TypedEntityChecker::<Platform, _>::new(PlatformChecker::default()));
    module = module.entity::<OrderStatus>();
    module = module.checker(teaql_runtime::TypedEntityChecker::<OrderStatus, _>::new(OrderStatusChecker::default()));
    module = module.entity::<Record>();
    module = module.checker(teaql_runtime::TypedEntityChecker::<Record, _>::new(RecordChecker::default()));
    module = module.entity::<Person>();
    module = module.checker(teaql_runtime::TypedEntityChecker::<Person, _>::new(PersonChecker::default()));
    module = module.generated_schema_bootstrap(ensure_generated_bootstrap);
    module
}

pub fn module_with_behaviors() -> teaql_runtime::RuntimeModule {
    let mut module = teaql_runtime::RuntimeModule::new();
    module = module.entity_with_behavior::<Platform, _>(PlatformBehavior::default());
    module = module.entity_with_behavior::<OrderStatus, _>(OrderStatusBehavior::default());
    module = module.entity_with_behavior::<Record, _>(RecordBehavior::default());
    module = module.entity_with_behavior::<Person, _>(PersonBehavior::default());
    module = module.generated_schema_bootstrap(ensure_generated_bootstrap);
    module
}

pub fn module_with_behaviors_and_checkers() -> teaql_runtime::RuntimeModule {
    let mut module = teaql_runtime::RuntimeModule::new();
    module = module.entity_with_behavior::<Platform, _>(PlatformBehavior::default());
    module = module.checker(teaql_runtime::TypedEntityChecker::<Platform, _>::new(PlatformChecker::default()));
    module = module.entity_with_behavior::<OrderStatus, _>(OrderStatusBehavior::default());
    module = module.checker(teaql_runtime::TypedEntityChecker::<OrderStatus, _>::new(OrderStatusChecker::default()));
    module = module.entity_with_behavior::<Record, _>(RecordBehavior::default());
    module = module.checker(teaql_runtime::TypedEntityChecker::<Record, _>::new(RecordChecker::default()));
    module = module.entity_with_behavior::<Person, _>(PersonBehavior::default());
    module = module.checker(teaql_runtime::TypedEntityChecker::<Person, _>::new(PersonChecker::default()));
    module = module.generated_schema_bootstrap(ensure_generated_bootstrap);
    module
}