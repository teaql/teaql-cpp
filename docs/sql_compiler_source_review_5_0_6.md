# Complete SQL dialect and root-source review

Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This review covers complete `teaql-sql/src/dialect.rs` (**1,618 lines**) and
`teaql-sql/src/lib.rs` (**911 lines**): seven public helpers, SchemaIndexSpec,
all forty-three trait methods, private validation/identifier helpers, twenty-two
root exports, three dialect tests and thirty root tests. SQLite is the only
native backend in scope. Executor, mutation orchestration, SQL types/bindings
and diagnostics retain their separate active-baseline source qualifications.

## Active-baseline changes and evidence provenance

The exact 5.0.5-to-5.0.6 dialect diff moves grouped clauses/HAVING ahead of the
partition wrapper return, adds standalone LikePattern compilation and retains
original operands for lowered LIKE/NOT LIKE bindings. These runtime repairs
already exist. Fresh comparison still checks every unchanged responsibility;
neither the small source diff nor the historical full-suite result establishes
new-baseline acceptance by itself.

The historical SQL baseline project pins futures-core 0.3.34 and has registry
identities differing from the reference. A new independent
`tests/parity/rust_sql_compiler_source_5_0_6` project uses exact reference
identities, including futures-core 0.3.32. It calls the unchanged original
compiler observers and actual SQLite provider. All existing inputs, expected
outcomes and observer bodies remain unchanged.

Public compiler/provider observations use the original crates directly, so
runtime/provider dependencies refer to the same original SQL package. Original
unit tests run separately in a disposable byte-preserved SQL view; this avoids
introducing two same-version path packages into one provider graph. That view
retains all original dependencies and source/test bodies. Its existing appended
diagnostic observer is unused by these tests; serde_json is the sole supplemental
observer dependency and is already in the reference lock.

## Complete dialect responsibilities

Native implementation remains in `teaql_sql/include/teaql/sql/dialect.hpp` and
the independent `src/dialect.cpp`, `src/schema.cpp` and `src/mutation.cpp`.
The compiler receives an explicit destination memory resource, consults no
connection or ambient UserContext, and does not implement a data service.

| ID | Source responsibility | Native reconciliation |
| --- | --- | --- |
| DC01 | SQL_KEYWORDS, quote_identifier_if_needed, private wrapped/quoted predicates | Preserve ASCII keyword/identifier rules, already-wrapped names, empty/NUL text and quote doubling. The char32_t overload covers the complete Rust scalar domain; the legacy byte overload serves ASCII providers. |
| DC02 | bounded_sql_identifier | Preserve short names, UTF-8 prefix boundaries, uppercase sixteen-digit FNV-1a suffix and minimum bound precondition. Native invalid scalar/UTF-8 inputs are explicit extra boundaries, not Rust-valid positive cases. |
| DC03 | SchemaIndexSpec | Four distinct fields, ordered columns, Clone, structural equality and full derived Debug. Explicit allocator construction/destination copies replace owned source strings/vectors. No source Default/Hash/serde protocol is invented. |
| DC04 | schema_index_specs | First version property selects the unique ID/version index with source fallback ID; matching property-name suffixes create ordinary indexes in model order. Preserve physical column names, Unicode uppercase and optional shared bounds. |
| DC05 | schema_foreign_key_name | Combine all four physical identifiers before Unicode uppercasing and shared length bounding. Creation and collision callers can consume the same name. |
| DC06 | validate_schema_identifier_lengths | Check entity then properties in order; unwrap/deduplicate only double quotes/backticks for physical byte length. Preserve complete structured error fields and original identifier. Brackets and lone quotes follow their distinct source rules. |
| DC07 | storage_length_covers / storage_numeric_covers | Preserve unbounded length/numeric cases, fractional and saturated integer-digit containment, partial-shape rejection and u32 extrema. |
| DC08 | kind / quote_ident / placeholder | Three mandatory virtual provider hooks; preserving the discriminator does not add other backend implementations. |
| DC09 | large_in_uses_array_param / prefers_small_parent_relation_probes / schema_setup_sqls | Preserve false/false/empty defaults and override dispatch. No unsupported optimization is inferred. |
| DC10 | schema_type_sql / private validate_schema_shape | Preserve all ten type mappings and Text/Decimal shape rules. Direct type rendering retains its source contract; column/DDL validation occurs at the separate validating entry. Validation order and structured reason text remain exact. |
| DC11 | column_definition_sql / compile_create_table | Validate before rendering; quote/type hooks, PRIMARY KEY and ID-or-required NOT NULL are separate responsibilities. Preserve property order and final table-hook order. |
| DC12 | schema_indexes_sqls | Consume the shared index specs, preserve ordered column quoting and unique/nonunique DDL, then quote generated name/table through provider hooks. |
| DC13 | fallback_default_value_sql / compile_add_column | Preserve type-specific zero/empty/epoch defaults; add a fallback only for nonnullable non-ID columns after normal column validation. |
| DC14 | compile_select / compile_select_sql | Preserve overridable SQL compilation, current comment, binding log context and final kind hook. Raw SQL/fragments mark provenance untrusted before any early return. |
| DC15 | SELECT clause order and partition branch | Projection/window-order bindings precede WHERE/search; trusted field policies govern search expansion. GROUP BY/HAVING remain inside ranked SQL, ordinary order follows outside that branch, and offset-plus-limit saturates. Ordinary LIMIT/OFFSET semantics remain separate. |
| DC16 | column_sql / column_with_alias | Resolve logical fields to physical columns, reject unknown fields and alias only when physical/logical names differ. Preserve quote-hook order. |
| DC17 | order_by_sql / resolve_order_field | Resolve explicit expression or field through the correct overridable stage, then append direction. Preserve expression parameter order. |
| DC18 | select_projection / compile_projection | Empty selection means default modeled columns; explicit fields deduplicate by first occurrence. Preserve expressions, raw projections, dynamic properties and aliases in source order. Aggregates choose the distinct projection path. |
| DC19 | aggregate_projection / resolve_aggregate_field | Ignore ordinary/default selected fields, retain unique grouping/expression/raw fragments, then append aggregates. COUNT(*) differs from ordinary field resolution. |
| DC20 | aggregate_call_sql / aggregate_function_sql | Preserve all twelve function names and both independent override points; emitted SQL does not guarantee SQLite implements every function. |
| DC21 | compile_expr | Preserve all expression variants, ordered bindings, field-sensitive scope combination/restoration, standalone LikePattern and binary lowered LIKE original provenance. Errors retain earlier partial bindings. |
| DC22 | compile_function / compile_single_arg_function | SOUNDEX keeps its own arity/dispatch branch, COUNT with no arguments emits COUNT(*), and other aggregate functions use the generic single-argument hook with exact errors. |
| DC23 | compile_gbk_function | Actual default validates one argument and passes it through. Its stale source comment suggesting a default error is not followed over the executable body. Conversion remains a provider override; no native GBK backend is added. |
| DC24 | compile_subquery | Compile left first, validate supported membership operators, temporarily clear inherited binding scope, compile nested SELECT with its own descriptor and restore scope even on failure. |
| DC25 | compile_joined / compile_in | Preserve child order, parentheses and empty joined expressions. Direct IN compiles left before validating its operator precondition; lists expand in order and empty lists retain their structured error. Nonlists compile as expressions. |
| DC26 | compile_insert / compile_batch_insert | Preserve descriptor-order selection, first-record batch shape, later missing-field typed NULL and ignored later extras. Explicit TypedNull retains its type; only bare/missing NULL acquires property type. Empty mutation and hook order remain exact. |
| DC27 | compile_update / compile_guarded_update / compile_update_with_guard | Preserve distinct wrappers, ID requirement, ID/version field exclusions, checked version increment, empty-write error, target/version predicates and same-statement guard. |
| DC28 | compile_batch_update | Preserve declared field order, CASE arms, separate version update/predicate arms, row/ID indexing preconditions and parameter order. Partial metadata follows source access/error order. |
| DC29 | compile_delete / compile_guarded_delete / compile_delete_with_guard | Preserve ID-first validation, soft/hard differences, negative version assignment, optional optimistic predicate and atomic guard. Final logging kind/table/assignment hooks retain source evaluation order. |
| DC30 | compile_recover / compile_guarded_recover / compile_recover_with_guard | Reject nonnegative versions before ID/version metadata lookup, restore version with checked arithmetic, retain target/old-version predicates and optional guard. |
| DC31 | Shared native completion and ownership | Capture log provenance before final mutation SQL hooks, return distinct scoped CompiledQuery values and preserve compiler errors versus panic/precondition exceptions. Nested strings/containers/bindings copy explicitly into the chosen domain; registered resources are not hidden owners. |

The mutation arithmetic comparison uses the reference Debug behavior. Native code
throws on overflow rather than invoking signed-overflow UB. Existing native64
usize extremes retain their stated scope; this checkpoint does not claim a new
MIPS/release arithmetic result. No production body change was needed here.

## Complete root facade and tests

The root has no production algorithm beyond reexports and test-module wiring.
All **22 names** map directly to `teaql::sql` aliases; thirteen type/template
identities and all nine function groups are checked in the canonical compiler
port. The Unicode quote overload is checked alongside the ASCII overload.
Native and import-only consumers compile these same assertions. No wrapper
ownership, serialization or execution behavior is introduced.

The root now has **30 tests**, including the newly retained
`partitioned_grouping_preserves_having_before_window_slice` test. Its C++ port
compares the complete original SQL literal (with the documented numbered SQLite
sigil adaptation) and exact WHERE/HAVING parameter vector. It was missing from
the historical 29-test source review.

| Group | Complete source disposition |
| --- | --- |
| Identifier helpers (5) | Quote recognition, bounded names, index specs, physical identifier validation and foreign-key bounds retain literal values/errors/order in the canonical compiler port. |
| SELECT/compiler semantics (20) | Basic filtering/order/slice; partition rows and grouping/HAVING; projection deduplication; ordinary versus aggregate projections; grouped predicates; sound-like and six Java-style matches; search; empty schema setup; ordinary/guarded mutations; IN/large-IN/property comparisons; trusted raw escape hatches/comment; subqueries; extended aggregate/expression/order/HAVING. All helpers and assertions are reconciled. |
| SQLite renderer (2) | Escaped values and untouched quoted placeholders; comments plus Date/millisecond Timestamp storage literals. These are diagnostic-rendering contracts, not authorization for raw logs. |
| Excluded renderer-only tests (3) | `renders_postgres_debug_sql_with_inlined_params`, `debug_sql_renders_copy_paste_statement_with_shared_semantics`, `postgres_and_mysql_debug_sql_use_typed_temporal_literals` remain outside native SQLite scope. Native unsupported-backend checks are boundaries, not positive parity for these tests. |
| Dialect source tests (3) | Typed-null regression, four invalid schema-shape cases and full storage containment assertions are retained in `default_contracts.cpp`. |

The actual count of root compiler-semantic tests is derived from all thirty
registered source names and their explicit renderer/helper dispositions. The
synthetic root dialect uses PostgreSQL-kind `$n` and a GBK override; native ports
preserve literal SQL with SQLite-kind `?n` and the same controlled override.
The MySQL-named projection-deduplication test uses that generic source dialect,
so it remains included. No external backend or service is used by those fixtures.

## Exact observation matrix and reproduction

| Matrix | Exact source outcomes |
| --- | ---: |
| Quote/storage helpers | 262 |
| Direct IN preconditions / partial bindings | 216 |
| Index / FK / physical identifier schema | 1,305 |
| Insert / batch / update / delete / recover | 2,505 |
| Mutation provider-hook order/failure | 48 |
| Expression / SELECT / CREATE / ADD | 2,206 |
| Actual grouped-partition SQLite | 146 |
| Full SQL model protocols, including SchemaIndexSpec | 40 |
| Total per original/native/imported frontend | **6,728** |

All observers call the original implementations; none replaces query, mutation,
grouping or persistence algorithms. The SQLite matrix opens actual databases
and compares complete SQL, policies, parameters and returned rows. Corpus and
driver fingerprints are checked before replay. Comparisons are exact typed JSON
with no output normalization. Root synthetic-dialect adaptations are confined
to their separately reviewed literal-SQL port.

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_sql_compiler_source_5_0_6.py \
    --record /tmp/sql_compiler_source_506.json.gz
```

The controller validates the full registry before running **30 root + 3 dialect
original tests** unchanged. libtest uses substring filters, so the root selection
explicitly skips all other registered top-level modules; a zero-test or unrelated
`executor::tests` run cannot satisfy the root gate. Native/imported profiles each
run eight matrices and five default/override/model/real-SQLite contracts, followed
by no-work rebuilds. This compiler/test-only continuation uses no new sanitizer
or MIPS profile. Complete provider/runtime callers, full current integration and
P0-P7 exits retain independent requirements.

## Terminal qualification

The [execution record](parity/sql_compiler_source_execution_5_0_6.json.gz)
passes all **6,728 exact source/native/imported outcomes per frontend**, all
**33 unchanged original tests** and **native 13/imported 13** checks. The
standalone observer uses **129 exact registry identities** with no reference
differences; the old observer's **44 differences** remain recorded separately.
Actual SQLite execution is included in the grouped-partition matrix and native
mutation contracts. Selected final builds report no work.

[Static verification](parity/sql_compiler_source_static_5_0_6.json.gz) passes
module paths, **1,516 exports**, **1,009 canonical bodies**, 42 generator anchors
and 17 capability records. Pinned format/check passes **1,706 files**. Native
discovery remains **778**; the module preset discovers **569 imported tests**.
Those are inventory counts, not complete integration results.

The [preservation record](parity/sql_compiler_source_preservation_5_0_6.json.gz)
verifies **6,091 protected artifacts**, including all retained **78 SDKs /
3,457 files**, and unchanged production compiler/header/schema/mutation code.
The only semantic-port changes add the missing grouped-partition regression
and compile the root namespace's thirteen type/template and nine function-group
identities. No generated file or original expected output was edited.

The [qualification receipt](parity/sql_compiler_source_qualification_5_0_6.json.gz)
accepts only dialect.rs and lib.rs under the stated SQLite dispositions, moving
source status to **65 accepted / 57 pending / 2 excluded_external**. All capability
verdicts and full P0-P7 exits remain unchanged. Pending provider/runtime/caller
qualification and current full integration remain required; historical failed
735/736 and 718/721 epochs stay failed. No fresh sanitizer/MIPS/device or
Java/generation-reproduction acceptance follows from this continuation.
