# G16 dynamic storage and readonly-property requirements

Status: G16 qualified for all sixteen requirements. The
[source/model acceptance review](dynamic_storage_acceptance_5_0_6.md) and
[final record](parity/dynamic_storage_acceptance_5_0_6.json.gz) retain the complete
audit and terminal native46/module45 integration. This review preserves the
existing implementation/evidence and replaces historical pending lists with
explicit source boundaries; an inventory alone is not acceptance.
References remain the active Rust 5.0.6 and generator locks. The
[source review record](parity/g16_current_responsibility_review_5_0_6.json.gz)
preserves the previous nine questions and distinguishes primary bodies from
transitive boundaries that still need complete acceptance.

The [current source/model review](dynamic_storage_acceptance_5_0_6.md) joins
those boundaries to the implementation and existing observations. Its
[reconciliation record](parity/g16_source_model_reconciliation_5_0_6.json.gz)
adds the actual relation, graph-write and SQL-executor bodies: five primary and
fourteen transitive files. The original eleven-file inventory is preserved;
the complete 46-native/45-imported integration is terminal and passes.

The five primary sources are core `dynamic_fields.rs`/`dynamic_properties.rs`,
data-service `dynamic_fields.rs`, runtime `dynamic_fields.rs`, and SQLite
`dynamic_fields.rs`. Query/row metadata, generated macro adapters, typed/relation
loading, cursor hooks and governed Save are transitive responsibilities, not
alternative implementations inside the storage provider.

| ID | Source responsibility | Current implementation/evidence and remaining audit |
| --- | --- | --- |
| DS01 | Trusted immutable definitions: owner/revision/names/types, full equality versus same-schema, optional namespace/executor binding and exact validation failures. | [Core contracts](dynamic_fields_5_0_6.md), G15 metadata/geometry review. Preserve actual type variants and namespace bytes; request JSON does not mint definitions. |
| DS02 | All versus typed selection; Value/NULL/NotLoaded/Delete; independent payloads and shared selected-name geometry; subset/union merge and incoming-value precedence. | Core 176-case carrier and generated carrier comparisons; [generated adapters](generated_dynamic_fields_5_0_6.md). Missing incoming values do not clear loaded fields. |
| DS03 | Readonly `_` definitions and values stay distinct from fixed slots and persistent `#` storage. Schema validation, merge conflicts, cached hash, declared type and actual presence remain separate. | [Readonly access](generated_readonly_properties_5_0_6.md), [schema hash](readonly_schema_hash_5_0_6.md), G15 initialized-cache/row comparisons. Preserve all original Value/JSON acceptance branches. |
| DS04 | CompactRow sidecars and model hydration retain definitions/provenance without smuggling storage capability through ordinary map/JSON presentation. | [Row contracts](dynamic_fields_5_0_6.md), [JSON hydration](generated_json_hydration_5_0_6.md), typed source/result and graph readback. Audit source conversion boundaries rather than claiming every conversion retains durable data. |
| DS05 | Store SPI separates ambient reads/schema from transaction writes; binding key and non-reused source identity are comparison-only capabilities. Missing store/identity fails at its original stage. | [Storage SPI](dynamic_storage_spi_5_0_6.md), actual SQLite store/transaction and provider matrices. Retain query/mutation/transaction layers and optional unsupported defaults. |
| DS06 | Cursor plan validates selection/namespace/identity and exact executing store. Complete batch validation checks owner sequence/cardinality, omitted/unrequested/unselected data and types before publication. | SPI pure-plan observations and actual dynamic stream matrices. Duplicate owner views have independent values; a failed private chunk must not escape as partial results. |
| DS07 | Context-owned provider registration, lookup and optional schema initialization; custom provider graph-save/stream support is explicit and defaults to unsupported. | [Runtime provider](dynamic_provider_5_0_6.md), generated assembly/registry evidence. No ambient provider, hidden I/O or independent dynamic save path is introduced. |
| DS08 | Database and in-memory provider construction/loading preserve duplicate-owner, schema, executor/store, selection and provenance failure order. New-entity definitions bind through the registered executor without fetching rows. | Runtime provider observations and generated definition binding. Read-load identity is checked after the original store load; metadata/stream preparation checks it before cursor execution. Do not reorder these paths to make errors agree. |
| DS09 | SQLite physical schema and codecs preserve owner IDs, revisions/types, explicit NULL, exact finite f64 bits/Decimal strings and all supported scalar variants. Reads batch 400 IDs; malformed storage uses safe codes. | [SQLite store checkpoint](sqlite_dynamic_storage_5_0_6.md), [123 unchanged plus 34 new direct storage boundaries](dynamic_storage_boundaries_5_0_6.md) and later provider/Save evidence. Inspect full codec/driver branches, not just ordinary Text roundtrips. |
| DS10 | Actual transaction/connection lease, active-handle validation, whole-batch prevalidation, commit/rollback and original native/dynamic write ordering. Schema and reads remain independent contracts. | SQLite and [governed graph Save](dynamic_graph_save_5_0_6.md) matrices; retain actual database state on rejection/conflict/storage errors. An ambient executor cannot become a writer by reusing a connection pointer. |
| DS11 | Root and relation typed loading, readonly schema attachment, embedded batches and flat/compact graph decoding preserve selected availability and original sidecar boundaries. | [Relation hydration](dynamic_relation_hydration_5_0_6.md), G15 loaded graph and source/result matrices, [simultaneous facet composition](dynamic_json_facets_5_0_6.md). Preserve omitted compact facet payloads and actual source SQL failures. |
| DS12 | Dynamic cursor enhancement uses the cursor's own connection and validated plan. Complete chunks, early close, item errors, pending cancellation and subsequent reuse retain the original supported-shape contract. | Actual SQLite stream/lifetime gates and protected stream shape evidence. Relation/aggregate enhancement is not invented for unsupported streams. C++ shutdown must join native memory access. |
| DS13 | Generated indexed carrier hooks validate canonical layout/owner before install/update/delete, update availability only when selected shape changes and delegate to shared runtime. | Generated carrier/readonly acceptance, G01/G04/G10/G15 evidence and unchanged SDK preservation. Application-facing spelling remains lowerCamelCase; framework protocols retain their names. |
| DS14 | Governed dynamic-only and mixed graph mutations participate in Checker/Fix, immutable Policy plans, original versions, ID allocation, transaction validation/write/readback and actual-value audit. | [Preflight](pending_dynamic_preflight_5_0_6.md), [typed Save/audit](dynamic_typed_save_audit_5_0_6.md), [audit readback](dynamic_save_audit_5_0_6.md). Check pending non-root/unrepresented keys as well as traversed objects; retaining old CRUD tests is insufficient. |
| DS15 | Trusted input and authoritative configuration remain distinct. Accepted presentation values and loaded-field annotations do not supply an executor, namespace, tenant rule or permission decision. | G10 protected/provider-present/provider-missing/JSON composition matrices. Preserve application-owned authorization and the original unsupported/manual-carrier outcomes; do not claim JSON acceptance itself grants persistence authority. |
| DS16 | Privacy-safe storage errors/write diagnostics, actual Context/resource ownership, explicit retention and checked invalidation after deletion. Mutation audit remains commit-aware, with broader provenance/privacy owned separately. | Core/provider/SQLite/generated lifecycle evidence and G14/G17 audit contracts. Binding IDs are not wire data; malformed stored payloads must not appear in default errors. |

## Historical question reconciliation

Readonly and persistent validators cannot be merged. The original readonly
validator accepts a name absent from its schema and permits supported native
JSON-shaped presentation values. Persistent definitions reject unknown codes
and require the declared Value variant; a durable Json field requires the Json
variant rather than native Object/List. Both preserve untyped NULL, while typed
NULL must match its declaration. These distinct contracts remain separate in
the C++ core.

The current ledger still contained the original row-format/SipHash defects,
missing generated carriers, partial facet wiring and early sample/configuration
gaps. Later row diagnostics, schema hashing, initialized-cache, generated
readonly/durable adapters, graph reconstruction and real Policy/SQLite/facet
matrices provide scoped evidence for those paths. Preserve the original failures
and counts; do not treat them as new missing algorithms or as full acceptance.

Source-specific limits remain visible. Ordinary embedded map conversion does not
always preserve a dynamic-only sidecar. Compact generated facets do not perform
typed persistent loading. Some restricted readonly facet requests retain the
original two-column SQLite subquery error. These are neither successful durable
facet hydration nor permission to omit the underlying protocols.

The final audit joins the primary and transitive source review to the exact
current contracts and observations. G15's qualified
41-native/29-imported gate provides shared geometry/model/composition evidence;
the separate G16 native46/module45 gate qualifies provider/store/stream/write
integration. No production function or generated body changes in this acceptance;
four existing helper declarations are added to the module facade and lookup.
