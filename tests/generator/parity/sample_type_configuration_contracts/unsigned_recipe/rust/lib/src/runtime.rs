
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

pub const DATABASE_URL_ENV: &str = "CPP_SAMPLE_TYPES_UNSIGNED_RECIPE_CORE_DATABASE_URL";
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
            "Priority" => Some(std::sync::Arc::new(crate::Priority::entity_descriptor())),
            "Platform" => Some(std::sync::Arc::new(crate::Platform::entity_descriptor())),
            "SampleItem" => Some(std::sync::Arc::new(crate::SampleItem::entity_descriptor())),
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
        .with_entity("Priority")
        .with_entity("Platform")
        .with_entity("SampleItem")
}

pub fn behavior_registry() -> teaql_runtime::InMemoryEntityDataServiceBehaviorRegistry {
    teaql_runtime::InMemoryEntityDataServiceBehaviorRegistry::new()
        .with_behavior("Priority", PriorityBehavior::default())
        .with_behavior("Platform", PlatformBehavior::default())
        .with_behavior("SampleItem", SampleItemBehavior::default())
}

pub fn checker_registry() -> teaql_runtime::InMemoryCheckerRegistry {
    teaql_runtime::InMemoryCheckerRegistry::new()
        .with_checker(teaql_runtime::TypedEntityChecker::<Priority, _>::new(PriorityChecker::default()))
        .with_checker(teaql_runtime::TypedEntityChecker::<Platform, _>::new(PlatformChecker::default()))
        .with_checker(teaql_runtime::TypedEntityChecker::<SampleItem, _>::new(SampleItemChecker::default()))
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
        let rows_constant_priority_1 = crate::Q::priorities().select_self_fields().with_id_is(1_u64).comment("what: locate generated constant").purpose("why: idempotent runtime bootstrap").execute_for_list(context).await.map_err(|e| teaql_runtime::RuntimeError::Graph(e.to_string()))?;
        if let Some(mut constant_priority_1) = rows_constant_priority_1.data.into_iter().next() {
            let mut changed = false;
            if constant_priority_1.platform_id() != 1_u64 { constant_priority_1.update_platform_id(1_u64); changed = true; }
            if constant_priority_1.name() != "Basic" { constant_priority_1.update_name("Basic"); changed = true; }
            if constant_priority_1.code() != "BASIC" { constant_priority_1.update_code("BASIC"); changed = true; }
            if constant_priority_1.value() != "7" { constant_priority_1.update_value("7"); changed = true; }
            if changed { let _ = teaql_runtime::AuditedSaveExt::save(constant_priority_1.audit_as("reconcile model constant Priority(1)"), context).await?; }
        } else {
            let mut constant_priority_1 = Priority::runtime_new(context.entity_runtime_state());
            constant_priority_1.update_id(1_u64);
            context.initialize_generated_bootstrap_entity(&mut constant_priority_1, Priority::ENTITY_NAME, 1_u64)?;
            constant_priority_1.update_platform_id(1_u64);
            constant_priority_1.update_name("Basic");
            constant_priority_1.update_code("BASIC");
            constant_priority_1.update_value("7");
            let _ = teaql_runtime::AuditedSaveExt::save(constant_priority_1.audit_as("create model constant Priority(1)"), context).await?;
        }
        let rows_constant_priority_22 = crate::Q::priorities().select_self_fields().with_id_is(22_u64).comment("what: locate generated constant").purpose("why: idempotent runtime bootstrap").execute_for_list(context).await.map_err(|e| teaql_runtime::RuntimeError::Graph(e.to_string()))?;
        if let Some(mut constant_priority_22) = rows_constant_priority_22.data.into_iter().next() {
            let mut changed = false;
            if constant_priority_22.platform_id() != 1_u64 { constant_priority_22.update_platform_id(1_u64); changed = true; }
            if constant_priority_22.name() != "Extended" { constant_priority_22.update_name("Extended"); changed = true; }
            if constant_priority_22.code() != "EXTENDED" { constant_priority_22.update_code("EXTENDED"); changed = true; }
            if constant_priority_22.value() != "9" { constant_priority_22.update_value("9"); changed = true; }
            if changed { let _ = teaql_runtime::AuditedSaveExt::save(constant_priority_22.audit_as("reconcile model constant Priority(22)"), context).await?; }
        } else {
            let mut constant_priority_22 = Priority::runtime_new(context.entity_runtime_state());
            constant_priority_22.update_id(22_u64);
            context.initialize_generated_bootstrap_entity(&mut constant_priority_22, Priority::ENTITY_NAME, 22_u64)?;
            constant_priority_22.update_platform_id(1_u64);
            constant_priority_22.update_name("Extended");
            constant_priority_22.update_code("EXTENDED");
            constant_priority_22.update_value("9");
            let _ = teaql_runtime::AuditedSaveExt::save(constant_priority_22.audit_as("create model constant Priority(22)"), context).await?;
        }
        Ok(())
}


/// Canonical KSML field to selected JSON wire name, consumed by HTTP/TFP adapters.
pub fn generated_wire_field_mappings() -> std::collections::BTreeMap<String, std::collections::BTreeMap<String, String>> {
    std::collections::BTreeMap::from([
        ("Priority".to_owned(), std::collections::BTreeMap::from([
            ("id".to_owned(), "id".to_owned()),
            ("platform".to_owned(), "platform".to_owned()),
            ("name".to_owned(), "name".to_owned()),
            ("code".to_owned(), "code".to_owned()),
            ("value".to_owned(), "value".to_owned()),
            ("version".to_owned(), "version".to_owned())
        ])),
        ("Platform".to_owned(), std::collections::BTreeMap::from([
            ("id".to_owned(), "id".to_owned()),
            ("name".to_owned(), "name".to_owned()),
            ("version".to_owned(), "version".to_owned())
        ])),
        ("SampleItem".to_owned(), std::collections::BTreeMap::from([
            ("id".to_owned(), "id".to_owned()),
            ("platform".to_owned(), "platform".to_owned()),
            ("name".to_owned(), "name".to_owned()),
            ("value".to_owned(), "value".to_owned()),
            ("optional_value".to_owned(), "optionalValue".to_owned()),
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
        .entity::<Priority>()
        .entity::<Platform>()
        .entity::<SampleItem>()
        .generated_schema_bootstrap(ensure_generated_bootstrap)
}

pub fn module_with_checkers() -> teaql_runtime::RuntimeModule {
    let mut module = teaql_runtime::RuntimeModule::new();
    module = module.entity::<Priority>();
    module = module.checker(teaql_runtime::TypedEntityChecker::<Priority, _>::new(PriorityChecker::default()));
    module = module.entity::<Platform>();
    module = module.checker(teaql_runtime::TypedEntityChecker::<Platform, _>::new(PlatformChecker::default()));
    module = module.entity::<SampleItem>();
    module = module.checker(teaql_runtime::TypedEntityChecker::<SampleItem, _>::new(SampleItemChecker::default()));
    module = module.generated_schema_bootstrap(ensure_generated_bootstrap);
    module
}

pub fn module_with_behaviors() -> teaql_runtime::RuntimeModule {
    let mut module = teaql_runtime::RuntimeModule::new();
    module = module.entity_with_behavior::<Priority, _>(PriorityBehavior::default());
    module = module.entity_with_behavior::<Platform, _>(PlatformBehavior::default());
    module = module.entity_with_behavior::<SampleItem, _>(SampleItemBehavior::default());
    module = module.generated_schema_bootstrap(ensure_generated_bootstrap);
    module
}

pub fn module_with_behaviors_and_checkers() -> teaql_runtime::RuntimeModule {
    let mut module = teaql_runtime::RuntimeModule::new();
    module = module.entity_with_behavior::<Priority, _>(PriorityBehavior::default());
    module = module.checker(teaql_runtime::TypedEntityChecker::<Priority, _>::new(PriorityChecker::default()));
    module = module.entity_with_behavior::<Platform, _>(PlatformBehavior::default());
    module = module.checker(teaql_runtime::TypedEntityChecker::<Platform, _>::new(PlatformChecker::default()));
    module = module.entity_with_behavior::<SampleItem, _>(SampleItemBehavior::default());
    module = module.checker(teaql_runtime::TypedEntityChecker::<SampleItem, _>::new(SampleItemChecker::default()));
    module = module.generated_schema_bootstrap(ensure_generated_bootstrap);
    module
}