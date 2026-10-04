# Rust 5.0.5 source concept inventory

Pinned commit: `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

Generated from committed source objects by `scripts/inventory-upstream.py`.
Declarations, locations, and test counts do not establish parity. The index includes
conditional and unexported sources; see the [design contract](../design-parity.md).

| Rust file | Declared core types / abstractions | Source tests |
| --- | --- | --- |
| teaql-core/src/business_id.rs | `BusinessIdErrorCode`, `BusinessIdError`, `BusinessIdDefinition`, `BusinessIdScope`, `BusinessIdGenerationRequest`, `BusinessIdPlan`, `BusinessIdAllocation`, `BusinessIdValue`, `BusinessIdAllocator`, `BusinessIdProfile`, `BusinessIdSlot` | 0 |
| teaql-core/src/dynamic_search.rs | `SearchModel`, `SearchModels`, `DynamicSearchWarning`, `DynamicSearchFilter`, `DynamicSearchOrder`, `NormalizedDynamicSearch`, `DynamicSearchResult`, `DynamicSearchError` | 0 |
| teaql-core/src/entity.rs | `TeaqlEntity`, `EntityError`, `Entity`, `Audited`, `BaseEntityData`, `BaseEntity`, `IdentifiableEntity`, `VersionedEntity`, `TeaqlBoxedRelations`, `EntityDescriptorStore` | 0 |
| teaql-core/src/entity_graph.rs | `EntityGraphOperation`, `EntityGraphNode`, `EntityGraphBuilder`, `EntityGraph` | 1 |
| teaql-core/src/eval.rs | `LoadState`, `EvalResult` | 3 |
| teaql-core/src/expr.rs | `BinaryOp`, `ExprFunction`, `Expr` | 0 |
| teaql-core/src/list.rs | `SmartList` | 2 |
| teaql-core/src/meta.rs | `PropertyDescriptor`, `RelationDescriptor`, `EntityDescriptor` | 3 |
| teaql-core/src/mutation.rs | `MutationValues`, `GeneratedValues`, `EntitySnapshot`, `MutationKind`, `InsertCommand`, `UpdateCommand`, `BatchInsertCommand`, `BatchUpdateCommand`, `DeleteCommand`, `RecoverCommand` | 0 |
| teaql-core/src/query.rs | `SortDirection`, `NamedExpr`, `OrderBy`, `AggregateFunction`, `Aggregate`, `Slice`, `RelationLoad`, `RelationAggregate`, `RawSqlProjection`, `ObjectGroupBy`, `AggregationCacheOptions`, `StreamConfig`, `ContinuousPageFetchOptions`, `IdSetPaginationOptions`, `SelectQuery`, `Record`, `CompactRow` | 6 |
| teaql-core/src/request.rs | `FieldOperator`, `DateRange`, `EntityReference`, `QuerySelection`, `RelationSelection`, `RelationFilter`, `QueryOptions`, `UnsafeRawSqlSegment`, `RawDynamicProperty`, `RawProjection`, `RelationAggregate`, `FacetRequest`, `ObjectGroupBy` | 0 |
| teaql-core/src/safe_expression.rs | `TeaqlEmpty`, `SafeExpression` | 4 |
| teaql-core/src/time.rs | `Timestamp` | 2 |
| teaql-core/src/trace.rs | `TraceKind`, `TraceNode` | 0 |
| teaql-core/src/value.rs | `DataType`, `Value` | 13 |
| teaql-core/src/web.rs | `WebStyle`, `WebAction`, `WebResponse` | 1 |
| teaql-core/src/xls.rs | `XlsBlock`, `XlsBlockBuildContext`, `XlsPage`, `XlsWorkbook` | 0 |
| teaql-data-service/src/lib.rs | `DataServiceCapabilities`, `QueryRequest`, `QueryResult`, `MutationRequest`, `MutationResult`, `DataServiceOperation`, `ExecutionMetadata`, `DataServiceExecutor`, `QueryExecutor`, `StreamChunk`, `QueryStream`, `ExecutionObserver`, `StreamQueryExecutor`, `MutationExecutor`, `GuardedMutationRequest`, `GuardedMutationExecutor`, `TransactionExecutor`, `Transaction`, `SchemaRequest`, `SchemaResult`, `SchemaExecutor`, `IdGeneratorExecutor`, `SchemaProvider` | 1 |
| teaql-data-service/src/sql_log.rs | `SqlParameterLogPolicy`, `SqlExecutionOutcome`, `SqlLogContext`, `SqlProjectionState`, `SqlIntentRedactions` | 1 |
| teaql-provider-sqlite/src/lib.rs | `SqliteDialect`, `MutationExecutorError`, `SqliteMutationExecutor`, `SqliteSchemaProvider`, `SqliteProviderExt`, `SqliteIdSpaceGenerator`, `SqliteBusinessIdAllocator` | 42 |
| teaql-runtime/src/business_id.rs | `BusinessClock`, `SystemBusinessClock`, `FixedBusinessClock`, `BusinessIdSchemaContributor`, `BusinessIdSchemaService`, `BusinessDate`, `DailySequenceBusinessIdProfile`, `BusinessIdEncodingKey`, `BusinessIdKeyProvider`, `StaticBusinessIdKeyProvider`, `BusinessIdPermutationV1`, `PermutedDailyBusinessIdProfile`, `InMemoryBusinessIdAllocator`, `BusinessIdService` | 0 |
| teaql-runtime/src/checker.rs | `CheckObjectStatus`, `CheckRule`, `LocationSegment`, `JsonFieldNamingProfile`, `ObjectLocation`, `CheckResult`, `WireCheckResult`, `WireLocationSegment`, `CheckResults`, `Checker`, `CheckerRegistry`, `InMemoryCheckerRegistry`, `TypedChecker`, `TypedEntityChecker` | 5 |
| teaql-runtime/src/context.rs | `ContextEntityRef`, `ContextRootError`, `SchemaInvocation`, `SchemaProvider`, `GeneratedSchemaBootstrapFuture`, `GeneratedSchemaBootstrap`, `FixEvidenceSource`, `FixEvidence`, `UserContext`, `DataStore`, `InMemoryDataStore` | 6 |
| teaql-runtime/src/context/locking.rs | `RemoteLockProvider` | 0 |
| teaql-runtime/src/context/logging.rs | `SqlLogOperation`, `SqlLogOptions`, `SqlLogEntry`, `UnifiedLogEntry`, `LogPayload`, `InfoLogEntry`, `UnifiedLogBuffer` | 0 |
| teaql-runtime/src/context/pagination.rs | `ContinuousPageCursor`, `ContinuousPageCursorStore`, `InMemoryContinuousPageCursorStore`, `RetainedIdSet`, `IdSetStore`, `InMemoryIdSetStore` | 0 |
| teaql-runtime/src/context/transaction.rs | `TransactionScope` | 0 |
| teaql-runtime/src/data_service/cache.rs | `InMemoryAggregationCache`, `AggregationCacheBackend` | 0 |
| teaql-runtime/src/data_service/executor.rs | `GraphTransactionBoundary` | 0 |
| teaql-runtime/src/data_service/types.rs | `RuntimeDataService`, `ContextDataService`, `EntityDataService`, `RelationLoadPlan`, `UserContextMetadata` | 0 |
| teaql-runtime/src/entity_reference.rs | `EntityReferenceClaims`, `EntityReferenceTokenError`, `EntityReferenceCodec`, `EntityReferenceCodecResource`, `AeadEntityReferenceCodec` | 2 |
| teaql-runtime/src/entity_runtime.rs | `LoadedRelation`, `RelationHandle`, `EntityKey`, `EntityGraphBuilder`, `EntityChangeSet`, `ChangeSetStack`, `EntityRuntimeState`, `LedgerCompositionError`, `LedgerEntity` | 15 |
| teaql-runtime/src/entity_save.rs | `DynGraphSaver`, `GraphSaverFor`, `AuditedSaveExt` | 8 |
| teaql-runtime/src/entity_status.rs | `EntityAction`, `EntityStatus` | 16 |
| teaql-runtime/src/error.rs | `RuntimeError`, `ContextError`, `DataServiceError` | 0 |
| teaql-runtime/src/event.rs | `RawAuditEventKind`, `EntityPropertyChange`, `RawAuditEvent`, `BootstrapAuditIdentity`, `RawAuditEventSink`, `InMemoryRawAuditEventSink`, `SafeAuditField`, `SafeAuditEvent`, `SafeAuditEventSink` | 0 |
| teaql-runtime/src/generated_support.rs | `TeaqlQueryDataService`, `TeaqlEntityDataService`, `TeaqlDataServiceError`, `TeaqlRuntime`, `AuditedSave`, `PurposedQuery`, `PurposedSelectQuery` | 0 |
| teaql-runtime/src/graph.rs | `EntityValues`, `GraphOperation`, `GraphMutationKind`, `TraceScopeToken`, `GraphMutationPlanItem`, `GraphMutationBatch`, `GraphMutationPlan`, `GraphNode`, `ScopedCommentNode` | 2 |
| teaql-runtime/src/i18n.rs | `I18nCatalog` | 2 |
| teaql-runtime/src/id.rs | `InternalIdGenerator`, `AtomicCounterIdGenerator`, `SnowflakeIdGenerator` | 0 |
| teaql-runtime/src/inmemory_engine.rs | `InMemoryQueryEngine`, `ExprEvaluator` | 19 |
| teaql-runtime/src/language.rs | `Language`, `Locale`, `MessageTranslator`, `BuiltinTranslator` | 2 |
| teaql-runtime/src/log_formatter.rs | `LogFormatter`, `HumanReaderFormatter`, `DebugReaderFormatter`, `LogFormatterFactory`, `LogLevel`, `LogConfig`, `LogManager` | 4 |
| teaql-runtime/src/memory.rs | `MemoryDataServiceError`, `MemoryDataService` | 0 |
| teaql-runtime/src/mutation_policy.rs | `MutationPolicyIdentity`, `MutationOperationKind`, `MutationOperation`, `MutationPlan`, `MutationVerdict`, `MutationDecision`, `MutationPolicy`, `MutationPolicyRegistry`, `EmptyMutationPolicyRegistry`, `MutationPolicyApproval`, `MutationPolicyApprovalProvider`, `NoMutationPolicyApprovalProvider`, `MutationPolicySource`, `MutationPolicyApprovalStatus`, `MutationOperationSummary`, `MutationGovernanceSnapshot`, `MutationGovernanceEvent`, `MutationGovernanceSink`, `DefaultMutationGovernanceSink` | 5 |
| teaql-runtime/src/registry.rs | `InMemoryEntityGraphDecoderRegistry`, `MetadataStore`, `EntityRegistry`, `RequestPolicy`, `EntityDataServiceBehavior`, `EntityDataServiceBehaviorRegistry`, `InMemoryMetadataStore`, `InMemoryEntityRegistry`, `InMemoryEntityDataServiceBehaviorRegistry`, `RuntimeModule` | 0 |
| teaql-runtime/src/round_trip_reference.rs | `DeploymentProfile`, `ReferenceMode`, `TrustedReferencePrincipal`, `ReferenceDocumentScope`, `ReferenceIdentity`, `ExternalEntityReference`, `ResolvedEntityReference`, `ReferenceStartupNotice`, `RoundTripReferenceError`, `ReferenceKey`, `ReferenceKeyProvider`, `StaticReferenceKeyProvider`, `ReferenceAuthorizationPolicy`, `ReferenceWireCodec`, `ContextBoundReferenceRuntime`, `ContextBoundReferenceRuntimeResource`, `TrustedReferencePrincipalResource` | 17 |
| teaql-runtime/src/telemetry.rs | `RuntimeAttributeValue`, `RuntimeOperation`, `RuntimeTelemetryScope`, `RuntimeTelemetry`, `RuntimeTelemetryPropagationContext`, `FailOpenRuntimeTelemetryPropagationContext`, `NoopRuntimeTelemetry`, `FailOpenRuntimeTelemetryScope` | 3 |
| teaql-runtime/src/telemetry_opentelemetry.rs | `OpenTelemetryRuntimeTelemetry` | 4 |
| teaql-sql/src/bindings.rs | `SqlBindings` | 0 |
| teaql-sql/src/diagnostic_execution.rs | `StatementDiagnostic`, `FailureJournal` | 0 |
| teaql-sql/src/diagnostic_stream.rs | `DiagnosticStream` | 7 |
| teaql-sql/src/dialect.rs | `SchemaIndexSpec`, `SqlDialect` | 3 |
| teaql-sql/src/executor.rs | `SqlTransport`, `StreamingSqlTransport`, `SqlTransactionTransport`, `SqlTransaction`, `SqlExecutorError`, `SqlDataServiceExecutor`, `SqlDataServiceTransaction` | 12 |
| teaql-sql/src/types.rs | `DatabaseKind`, `CompiledQuery`, `SqlCompileError` | 0 |
