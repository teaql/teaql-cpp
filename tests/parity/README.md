# Rust / C++ differential contracts

The baseline is the exact Rust v5.0.5 commit in `upstream.lock.json`. The
`rust-values` oracles call the pinned implementation rather than rewriting expected
behavior. Value-sensitive dependencies are chrono 0.4.45, rust_decimal 1.42.1, and
serde_json 1.0.150; the independent Cargo.lock pins remaining test dependencies.
Oracles neither participate in production builds nor modify reference sources.

`value_cases.json` contains 722 inputs and actual Rust observations, combining
seeded random inputs and difference-driven regressions. It covers every scalar
Value branch, accepted/rejected conversion, precision, rounding, nonfinite values,
chrono's full year range, timezones/leap seconds, integer overflow, JSON number
kinds, Unicode, and invalid input. `tests/value_model_tests.cpp` separately checks
Object/List projection, recursive allocation, and cross-domain failure semantics.

Ordinary CTest requires Python 3 and recorded observations. Comparison distinguishes
JSON integers/floats and negative zero; floating results must match exactly without
an epsilon. Live verification also checks recorded Rust results and never updates
expectations automatically. Default tests verify the observation commit against
the lock. New expectations must come from the pinned Rust oracle, never C++ output.

```sh
python3 scripts/verify-upstream.py
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml
python3 tests/parity/compare_values.py --cpp build/teaql_value_probe --rust build/rust-values/debug/teaql-cpp-value-oracle
```

Decimal uses a 96-bit coefficient and normally scale 0–28, preserving parser
rounding/separator boundaries. Arithmetic, rescaling, significant figures, and
binary encoding have dedicated contracts below; exact/radix parsing and remaining
reexports still need restoration. JSON is an independent allocator-aware tree
matching serde_json's default features, including disabled `float_roundtrip`.
Text output retains number kinds. `to_json_value(destination)` is a projection:
Rust Value has no Serialize/Deserialize typed wire protocol. Timestamp's transparent
integer JSON and serde_utils trimming are implemented; Date/Decimal have no heap.
SQLite binding/decoding are now covered independently below. Value tests alone
prove neither complete query/save/governance/log integration nor all six stages.

Small Decimal and JSON conversion algorithms follow their upstream implementations.
MIT notices are in `LICENSES/rust_decimal-MIT.txt` and
`LICENSES/serde_json-MIT.txt`; they add no linked production dependency.

## List IDs and JSON text

`format_cases.json` contains 848 actual Rust `SmartList::map_by_id` and JSON text
observations, compared byte for byte. Cases cover all Value ID prefixes, fixed
Object/List keys, Decimal scale, nonfinite floats, signed zero, subnormals, exponent
thresholds, Unicode, and escapes. Float inputs are hexadecimal bit patterns.

Rust f64 Display uses expanded decimal text and shortest midpoint rounding away
from zero. serde_json 1.0.150 uses pinned zmij 1.0.23 with even rounding and separate
exponent thresholds. C++ adapts standard `to_chars` for these two interfaces without
another numeric library. Seed 505 additionally checked 218,000 random/precision
observations; this is not exhaustive over every floating bit pattern. CTest uses
recorded boundary regressions.

```sh
python3 tests/parity/compare_formats.py --cpp build/teaql_value_probe --rust build/rust-values/debug/teaql-cpp-value-oracle
```

Native list tests cover metadata, unloaded/empty dirty sets, duplicate grouping and
merging, foreign mapped-result reallocation, and closing a copied source Context.
`facets` stays public with `facets_mut()`/`facet_mut()` accessors. Rust `summary(key)`
becomes `summary_value(key)` because of the public field. Returned containers use
the list domain; projection takes an explicit destination.

## Request and dynamic search

`request_search_tests` checks builder/query conversions, separate RelationSelection
and RelationFilter, independent aggregates, facets, every field operator, JSON
conversion, recursive copies, source close, and atomic allocation failure.

`search_cases.json` contains 491 actual `normalize_dynamic_search` observations:
results, errors, structured warnings, and callback output. Cases include unknown
fields, malformed controls, field/relation metadata, every operator, list sizes,
exact decimal strings, dates, and integer ranges. Fixture models are trusted test
metadata, not a production authorization source.

```sh
python3 scripts/verify-upstream.py
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin search_oracle
python3 tests/parity/compare_search.py --cpp build/teaql_search_probe --rust build/rust-values/debug/search_oracle
```

Merge tests preserve original query, scope, order, page, and comment; check filter
and sort binding order; and suppress accumulated warnings after late validation
or binding failure. Default stderr uses `<omitted>` instead of submitted values
or untrusted paths. Explicit sinks receive structured original paths as in Rust.
Normalization and trusted native binding do not directly compile untrusted fields
to SQL. `into_query` keeps RelationFilter, facets, and relation aggregates in their
source-defined separate conversions. Entity orchestration requires separate gates.
C++ names the keyword field `operator_name`; warning JSON retains `fieldPath`.
Operator-helper panic conditions map to `invalid_argument`; complete panic Debug
text is not yet reproduced byte for byte.

## Web/Xls core models

`presentation_cases.json` contains 785 actual Rust serde, action/response factory,
and Xls navigation observations. Object and array forms compare complete successful
JSON or rejection. C++ uses field-aware `ModelDecodeError`, without promising serde
error-text parity. Unknown object fields are ignored.

XlsBlock omits null `value` on output but requires it on input. Missing `properties`
is an empty map; explicit null rejects. Construction clips negative coordinates;
JSON decoding preserves them. Array Option fields require placeholders; only
serde(default) fields permit omitted trailing elements.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin presentation_oracle
python3 tests/parity/compare_presentation.py --cpp build/teaql_presentation_probe --rust build/rust-values/debug/presentation_oracle
```

Native tests check destination binding, old actionList wrapping, SmartList totals
and facets, recursive copies, close, failure atomicity, and all-block reclamation.
These models add no web server, HTTP, or spreadsheet-engine dependency. Public
fields remain; conflicting builders use `set_*`, and `page(...)` becomes
`for_page(...)`. JSON projection takes destination Memory. Coordinate overflow
maps Rust checked failure to `overflow_error`, avoiding signed UB and making no
claim of unchecked Rust Release wrapping.

## Business ID core contracts

`business_id_cases.json` contains 1,188 actual core observations: every u8
preserve_digits value, default/legacy/unknown profiles, u64 limits, Unicode/NUL
scope keys, and every error code's Display. Fixture JSON is not a new BusinessId
serde protocol.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin business_id_oracle
python3 tests/parity/compare_business_id.py --cpp build/teaql_business_id_probe --rust build/rust-values/debug/business_id_oracle
```

Allocator, Profile, and Slot stay separate. Virtual calls take target Memory;
shared implementations must satisfy the relevant Rust Send+Sync service contract,
while a User arena remains thread-bound. Fixture profiles/allocators are probes,
not production services. Runtime encoding, services, and SQLite persistence have
separate sections below. Managed GenerationRequest copies borrowed definitions
and strings into its explicit domain; cross-domain copies select a destination.
All returns become invalid on owner close. `namespace` becomes `namespace_name`.
Range validation returns `Result<std::monostate, BusinessIdError>` and only checks
initial_sequence <= max_sequence, matching Rust's unit result.

## Decimal arithmetic

`decimal_cases.json` contains 963 actual rust_decimal 1.42.1 observations: five
checked operations, three saturating operations, ordering/sign, truncation/fraction,
floor/ceil, normalization, and all 12 rounding enum variants including deprecated
aliases. Compare coefficient, scale, sign, and display exactly rather than numeric
approximations that hide trailing zeros. Seed 505 also checked 17,437 observations;
recorded regressions include width/precision boundaries, signed zeros, overflow,
division by zero, and batch zero stripping, without an exhaustive claim.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin decimal_oracle
python3 tests/parity/compare_decimal.py --cpp build/teaql_decimal_probe --rust build/rust-values/debug/decimal_oracle
```

Six stack uint32_t limbs support 96×96 products and exact scale alignment through
28 decimal places. No float intermediate or big-number dependency is used.
Division preserves extension/zero-stripping order: `1 / 2` is `0.50`, whereas
`1.0 / 2` is `0.5`. Results are not automatically normalized. Checked calls return
optional; operators throw `overflow_error` or `domain_error` for division by zero.
Failed compound assignment preserves the old value. Arithmetic allocates no
Context data; formatting selects its domain. Remaining reexports need restoration;
provider encoding has its own contracts.

## Decimal scale and binary encoding

`decimal_scale_cases.json` records 1,148 observations; seed 505 checked another
5,636 inputs. Coverage includes set_scale/rescale/trunc_with_scale, all 12 round_sf
strategies, 16-byte little-endian encoding, all 256 scale-flag patterns, signed
zero, extremes, and exact float bit patterns.

```sh
python3 tests/parity/compare_decimal_scale.py --cpp build/teaql_decimal_probe --rust build/rust-values/debug/decimal_oracle
```

set_scale keeps the coefficient and changes value; scale above 28 returns a
DecimalError retaining precision. Error text supports explicit copying. rescale
preserves value where possible and rounds midpoint away from zero when reducing
scale, unlike round_dp's default even rounding. trunc_with_scale may expand scale;
round_sf returns empty optional for an unrepresentable result.

Keep observed upstream edges: nonzero rescale may exceed 28, such as expanding
1e-28 to scale 56. Display-capacity overflow maps to `length_error`; 128-bit decimal
power overflow and round_sf u32 addition overflow map to `overflow_error`.
Binary decoding masks reserved flags and clamps scale 29–31 to 28 using the source's
full-remainder comparison with five. These are pinned-dependency observations,
not a new specification. All arithmetic combinations with extended scales remain
unverified; ordinary-scale arithmetic retains independent regressions.

## Data-service models and log provenance

Independent `teaql::data_service` does not link SQLite. SQL reuses its parameter
policies and result-kind enum. QueryRequest, recursive MutationRequest,
GuardedMutationRequest, QueryResult, MutationResult, ExecutionMetadata, StreamChunk,
and schema models stay distinct. Commands, generated values, and persisted
snapshots are not merged. Request trace/comment and SelectQuery trace/comment are
independent. `unrecorded_query` retains row count with epoch time and no invented SQL.

`sql_log_cases.json` contains 1,008 actual observations: every policy, mismatched
provenance length, generated/raw SQL, nested Object/List/Json credentials, all
scalars, intent inheritance/merge/duplicates, forced target-ID hiding, and ASCII
normalization. Redaction order and duplicate counts are preserved. Plaintext may
exclude Masked, but Unknown/Credential and nested credentials remain hidden.
Collecting intent does not grant permission to log it.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sql_log_oracle
python3 tests/parity/compare_sql_log.py --cpp build/teaql_sql_log_probe --rust build/rust-values/debug/sql_log_oracle
```

Native tests cover intent fallback through filter/projection/having/search/subquery,
excluding separately executed relation siblings. Diagnostics show counts or
`<private>`, not intent text. Concrete SqlProjectionState registers arena cleanup.
Same-domain clones borrow identity; explicit cross-domain copies rebuild state
without shared ownership. Null-default-PMR and sanitizer tests cover nested
metadata, batch/guard/commands, deep copy, and close. Executor/observer/stream
foundations are tested separately; this corpus alone proves no complete runtime
or privacy integration.

## SQL models, bindings, and SQLite diagnostic rendering

`sql_types_cases.json` contains 1,538 actual observations: optional comments,
terminator escaping, placeholder counts/callback order, quotes/nested comments,
rejected numbered/named bindings, all Value literals, SqlBindings policies, and
every SqlCompileError Display branch. Compare reasons and nonreplayable Debug.
Diagnostic interpolation is never database execution; callers still project privacy.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sql_types_oracle
python3 tests/parity/compare_sql_types.py --cpp build/teaql_sql_types_probe --rust build/rust-values/debug/sql_types_oracle
```

Preserve all three DatabaseKind discriminants as models, with SQLite-only production
execution/rendering. Other rendering kinds return unsupported. Compiled results
contain complete SqlLogContext, without a second parameter-policy list. Values and
policies share SqlBindings; allocation failure restores logical lengths and normal
growth remains amortized. Scalar policies combine Credential > Unknown > Masked >
Plain. Subqueries clear outer scope temporarily and restore it even on failure.

Identifier Debug uses compact static Unicode 17.0.0 facts observed from the Rust
standard library, reproducible with `unicode_debug_table`; it is test-toolchain
data, not a TEAQL generator change. Cases cover controls, combining/unassigned
characters, and multibyte names. The legacy SQLite compiler takes explicit Memory
rather than UserContext; retain results through `context.make<CompiledQuery>`.
Native tests check copy/errors/scopes/OOM/nested credential protection/empty comment.
Independent dialect, executor, transport, provider, and schema contracts follow.

## Generic SqlDialect compilation

`sql_dialect_cases.json` contains 2,206 actual pinned teaql-sql observations.
Fixture dialects implement SQLite kind, quoting, and placeholders; other calls use
generic defaults. Optional numbered placeholders expose compilation order without
adding another database. Coverage spans every expression, 14 functions, 12 aggregates,
projection deduplication, partition pagination, text search, raw SQL/comments,
subquery scope and recovery after failure, table/column shapes, UTF-8 truncation,
quoting, and storage-width compatibility. Compare SQL, parameter order/privacy,
provenance, and errors, including deliberately invalid recursive combinations.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sql_dialect_oracle
python3 tests/parity/compare_sql_dialect.py --cpp build/teaql_sql_dialect_probe --rust build/rust-values/debug/sql_dialect_oracle
```

Fixture AST JSON is only a test protocol; Expr/SelectQuery gain no serialization.
Preserve partition pagination's early return before GROUP BY/HAVING, OFFSET without
automatic LIMIT, and aggregate bindings collected before output deduplication.
Not every compiled combination is executable SQLite; provider gates are separate.
`sql_dialect_contracts` checks virtual overrides, scope restoration after errors/OOM,
null-default-PMR Global compilation, and nested copies after User close. Rust Result
maps here to scoped, explicitly copyable structured `SqlCompileError` exceptions.

## Generic mutation compilation

`sql_mutation_cases.json` contains 2,505 actual observations of single/batch
insert/update, guarded update/delete/recover, soft delete/recovery, metadata order,
unknown fields, version selection, duplicates, unequal batch lengths, typed null,
and every Value branch. Record parameter branches and TypedNull DataType alongside
SQL/privacy/provenance so JSON projection cannot erase typed-null differences.
Shared fixture_sql helpers provide test metadata/AST decoding, not a product wire API.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sql_mutation_oracle
python3 tests/parity/compare_sql_mutation.py --cpp build/teaql_sql_mutation_probe --rust build/rust-values/debug/sql_mutation_oracle
```

Checked Rust i64 version overflow maps to `overflow_error`, avoiding signed UB
without inventing SqlCompileError variants. Batch bounds failures map to
`out_of_range` in source access order, preserving structured-error priority. The
oracle classifies panic with catch_unwind; unchecked Release wrapping is not claimed.
Native tests execute compiled SQL in real in-memory SQLite for batch inserts,
guards/stale versions, mixed expected-version updates, soft/hard delete, recovery,
virtual dispatch, and copies after close. Their fixture transport validates SQL;
it does not replace production transport or prove graph save/governance/audit.

## Schema names, indexes, and identifier validation

`sql_schema_cases.json` contains 1,305 actual observations: version-unique indexes,
logical-suffix indexes, ordered/duplicate metadata, physical `id` fallback, foreign
key names, name limits, and FNV-1a suffixes. Validate physical names by UTF-8 bytes;
only double quotes/backticks decode, while brackets remain literal. Return the
first failing object and original name. Shortening generated names does not rewrite
physical model names.

Unicode 17.0.0 uppercase ranges and expansions live in `uppercase_tables.hpp`,
without locale, global cache, or another library. Cases cover each mapped scalar
and its neighbors (2,325 scalars), random unmapped inputs, and UTF-8 truncation.
The second command reproduces fact tables without changing the model generator.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sql_schema_oracle
python3 tests/parity/compare_sql_schema.py --cpp build/teaql_sql_schema_probe --rust build/rust-values/debug/sql_schema_oracle
```

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo run --quiet --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin unicode_uppercase_table > include/teaql/core/detail/uppercase_tables.hpp
```

Native dialect tests cover nested SchemaIndexSpec copies, errors after close, and
invalid UTF-8 outside Rust str's domain. Mutation tests execute version indexes
with the independent SqliteDialect. Provider dialect checks follow pinned type,
capability, and no-DEFAULT add-column rules without claiming full differential
coverage. Legacy compilation belongs to runtime/legacy; SQLite dialect is separate.
Complete entity orchestration still needs its own gates.

## SQL transport defaults and errors

`sql_transport_cases.json` contains 344 actual observations using a ready fixture
transport for default `fetch_repeated_compact_sql`. Cover lazy start, replacement
of only the chosen parameter, input/output/duplicate order, zero/multiple rows,
first-error stop, empty input avoiding an invalid index, nonempty bounds failure,
and unchanged comment/provenance. Check Compile/Transport/PersistedRecord error text.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sql_transport_oracle
python3 tests/parity/compare_sql_transport.py --cpp build/teaql_sql_transport_probe --rust build/rust-values/debug/sql_transport_oracle
```

Native `sql_transport_contracts` checks child wake forwarding through nested layers,
no notification into detached parent signals, join before cancel/close, transaction
consumption and pending-borrow rejection, and explicit input/error copies with
null default PMR. CTest's 30-second timeout catches lost wakes. Data-service pending
work also blocks commit. Executor, diagnostics, and production SQLite need separate
evidence; this corpus proves the transport default only.

## SQL query executor

`sql_query_cases.json` contains 381 actual public SqlDataServiceExecutor query
observations. Multiple requests run through one executor and its clone, checking
descriptor/query-shape caching, recollected bindings, log provenance, distinct
request/compiled comments, intent fallback, metadata switches, partition probes,
and observer routing for success/error/cancel/exception. Compare epoch only when
unrecorded, not actual cross-language clock instants.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sql_query_oracle
python3 tests/parity/compare_sql_query.py --cpp build/teaql_sql_query_probe --rust build/rust-values/debug/sql_query_oracle
```

Native `sql_query_executor_contracts` checks the 256-entry FIFO, descriptor origin
changes, cross-User request/result copies, cache reuse after source close,
FailureJournal flush/disarm, and stopping native work before observer destruction.
Mutation, transactions, streams, and production provider integration have separate
contracts below.

## SQL mutation execution and journals

`sql_execution_cases.json` contains 1,012 actual public mutation/guarded-executor
observations: four commands, nested/empty batch, statement metadata and binding
positions, compile/transport failures, pending cancellation, panic, journal order,
target/intent redactions, guarded rereads, and row-count checks. Ordinary mutations
do not reread; success does not duplicate metadata through observers. Actual clock
values are excluded from differential comparison.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sql_execution_oracle
python3 tests/parity/compare_sql_execution.py --cpp build/teaql_sql_execution_probe --rust build/rust-values/debug/sql_execution_oracle
```

Native tests use a fixture SQLite transport for real atomic guards, explicit
expected_version, default-value rereads, stopping after later compile failure,
write-success/reread-failure journals, stop-before-observer close, and copies.
This limited binder is not the production provider. These tests do not implement
graph save; transactions and streams have separate evidence.

## SQL transaction execution

`sql_transaction_cases.json` contains 1,012 public begin/transaction mutation and
guarded observations, using execution probe/oracle transaction mode. Transaction
ordinary mutations also reread; batch writes/rereads follow source order. Compare
snapshot row counts, metadata, journal, error, and cancellation paths.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sql_execution_oracle
python3 tests/parity/compare_sql_transaction.py --cpp build/teaql_sql_execution_probe --rust build/rust-values/debug/sql_execution_oracle
```

Native tests use fixture SQLite for commit, rollback, close-time rollback, shared
query cache, transaction queries without per-value probes, pending-work termination
rejection, consumed handles after failure, invalid domains, Global diagnostic
copies, and resource/journal cleanup order. Production transport and streaming are
separate; runtime TransactionScope/ledger governance remains incomplete.

## SQL streaming execution

`sql_stream_cases.json` contains 880 actual public native/buffered transaction
stream observations. Cover compile timing, chunk_size zero, content/order/index/
is_last, empty chunks/streams, cancellation after delivery, pending, typed error,
unwinding, early is_last, sink panic, and metadata opt-out. Record whether cursors
are already released at observer notification and how many rows were delivered.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sql_stream_oracle
python3 tests/parity/compare_sql_stream.py --cpp build/teaql_sql_stream_probe --rust build/rust-values/debug/sql_stream_oracle
```

The oracle directly imports existing locked futures-core 0.3.34 for test Stream;
this adds no C++ dependency. Native tests check real-thread wakes through two
adapters, stop/join before observers, close, still-valid old chunks, real SQLite
transaction buffering, and open streams blocking termination. Production provider
and runtime relation streams require independent gates.

## SQLite native bindings

`sqlite_bindings_cases.json` contains 1,084 public provider execute observations
of actual SQLite storage types/values: all Value branches, nine affinities, numeric
Decimal storage, Unicode/NUL, special floats, i64/u64 limits, millisecond Timestamp,
parameter counts, and first-error order. Row decoding is independent. Rust-only
test dependencies use pinned rusqlite 0.40.1, libsqlite3-sys 0.38.1, and provider.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sqlite_bindings_oracle
python3 tests/parity/compare_sqlite_bindings.py --cpp build/teaql_sqlite_bindings_probe --rust build/rust-values/debug/sqlite_bindings_oracle
```

`sqlite_bindings_engine_differences.json` records two extreme REAL-to-TEXT differences
between bundled SQLite 3.53.2 and local 3.37.2. They are excluded from the pass count;
the provider does not override affinity with its own text formatting. Controlled
same-engine comparison links C++ to the oracle's libsqlite3.a. Native contracts
check numeric Decimal predicates/aggregates, complete retained text after close,
nested PMR, error copies, count failures, and native SQLITE_TOOBIG with null default
PMR and ASan/UBSan.

## SQLite row decoding

`sqlite_decoding_cases.json` contains 6,029 public fetch_all_compact observations,
including 1,061 deterministic corrupt inputs. Cover declared/physical type
combinations, aliases/duplicates, text inference, Date/Timestamp zones/fractions/
leap seconds/limits, Decimal, JSON error reason/location, invalid UTF-8, and BLOB
rejection. INTEGER/REAL values do not acquire logical Decimal/Date/Timestamp types
merely from declarations.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --release --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sqlite_decoding_oracle
python3 tests/parity/compare_sqlite_decoding.py --cpp build/teaql_sqlite_decoding_probe --rust build/rust-values/release/sqlite_decoding_oracle
```

This oracle requires Release. Source parse_fixed_sqlite_timestamp evaluates a
then_some argument even when its condition fails; `byte - b'0'` can panic in Debug
for malformed bytes below 0x30, such as `2024-02-29 01:-2:03.123+08:00`. Release
falls back and returns typed Bind error. C++ checks digits first and matches that
path, without claiming Debug panic parity. Native contracts check rows after
cursor/connection release, borrowed same-domain layouts, copied rows/nested JSON,
null default PMR, stale-layout index errors, and UTF-8 validation before logical
parsing. Layout caching, transactions, and schema have separate contracts.

## Production SQLite transport

`sqlite_transport_cases.json` contains 796 actual synchronous/asynchronous public
provider observations. The C++ probe uses real SqliteMutationExecutor, checking
reads/writes, repeated-parameter replacement/failure order, storage effects,
zero-size/divisible/empty native and buffered streams, early cancel, empty and
multi-statement SQL, native errors, and poisoned locks. This is production
transport evidence, not a fake, but it does not cover every provider API.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sqlite_transport_oracle
python3 tests/parity/compare_sqlite_transport.py --cpp build/teaql_sqlite_transport_probe --rust build/rust-values/debug/sqlite_transport_oracle
```

Native contracts connect the real SQL executor for queries, guarded mutation, and
streams; check 64-entry statement LRU, whole layout-index eviction, comment keys,
stale layouts after schema changes, wait cancellation, result borrows, domains,
threads, and SQLite cleanup before blocks. Synchronous contention returns Lock as
a scheduling adaptation. SQLite 3.38+ supplies sqlite3_error_offset for SQL/offset
context; 3.37 lacks it. This corpus makes no cross-engine error-byte guarantee.
Controlled schema entry, runtime ID registration, physical schema, and business
schema-contributor installation are now connected and tested separately.

## Production SQLite transactions

`sqlite_transaction_cases.json` contains 366 actual synchronous primitives and
associated SqlTransactionTransport/SqlTransaction handle observations. Compare
per-operation results/native errors/autocommit and final database contents for
commit/rollback, nested begin, termination without a transaction, continuation
after write failure, RETURNING, external same-connection calls, and deferred
foreign-key commit failure. A failed commit leaves the native transaction active.
Saved Rust observations precede Context exit, so C++ close-time rollback is not
counted as Rust differential equivalence.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sqlite_transport_oracle
python3 tests/parity/compare_sqlite_transaction.py --cpp build/teaql_sqlite_transport_probe --rust build/rust-values/debug/sqlite_transport_oracle
```

Native contracts exercise production transaction queries, authoritative ordinary
mutation rereads/rollback, pending-borrow rejection, queued-begin close, canceled
termination, failed-commit cleanup, and an old Context leaving later transactions
alone. Cancel rollback with external cursors retains their stream memory. Resources
finish before block reclamation. Full runtime ledger/graph-save governance remains
a later-stage requirement.

## SQLite ID-space generator

`sqlite_id_cases.json` contains 261 cross-engine public ensure_table/next_id/
ensure_floor observations: normalized shared names, Unicode/reserved/quoted tables,
floors, integer limits, strict storage reads, errors after negative updates,
rollback, ignored trigger writes, insertion races, and full 100-attempt exhaustion.
The generator remains distinct from business-ID allocation; its runtime SPI
adapter now has dedicated internal-ID contracts.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sqlite_transport_oracle
python3 tests/parity/compare_sqlite_id.py --cpp build/teaql_sqlite_transport_probe --rust build/rust-values/debug/sqlite_transport_oracle
```

`sqlite_id_engine_differences.json` records two missing-column errors: SQLite 3.37.2
returns SqliteFailure without error_offset, while 3.53.2 returns SqlInputError with
SQL/offset. A same-engine comparison using 3.53.2 headers and oracle libsqlite3.a
matched all 263 cases. The two differences are excluded from cross-engine passes.
SqlInputError has no rusqlite sqlite_error payload, so the probe reports native code
zero; ordinary failures preserve extended codes. Native tests disable default PMR,
check copies/borrow rejection/rollback reuse/strict INTEGER/negative-update errors,
and allocate 240 continuous unique IDs using four independent Contexts/connections.

## SQLite business ID allocator

`sqlite_business_id_cases.json` contains 198 actual allocator/SPI observations.
Compare code/message/display, autocommit, final level/version/storage types for
explicit schema, normalized scopes, validation/exhaustion, i64 initial boundaries,
strict type errors/overflow, ignored/rejected trigger writes, message retries,
nonunique multi-row updates, nested transactions, and deferred commit failure.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin sqlite_transport_oracle
python3 tests/parity/compare_sqlite_business_id.py --cpp build/teaql_sqlite_transport_probe --rust build/rust-values/debug/sqlite_transport_oracle
```

Native contracts check SPI dispatch, copied scope/errors and Global retention,
independent-connection allocation surviving aggregate rollback, failed nested begin
leaving an existing transaction intact, close cleanup after failed commit, and
160 continuous unique allocations across four independent Contexts/connections.
Default PMR is disabled; ASan/UBSan validates ownership with no new C++ dependency.

## Runtime Context resources and locals

`runtime_context_cases.json` contains 410 actual observations, including 300
seeded mixed-operation scripts. Cover independent typed/named indexes, replacement/
type changes, missing/mismatch, Unicode/NUL keys, every Value local, removal, and
three ContextError displays. A fixed custom MissingTypedResource name checks
formatting; automatic C++ compiler type names are a language adaptation, not Rust
paths. Immediate Rust Box drop versus deferred arena destruction is documented
separately from lookup equivalence.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin runtime_context_oracle
python3 tests/parity/compare_runtime_context.py --cpp build/teaql_runtime_context_probe --rust build/rust-values/debug/runtime_context_oracle
```

The oracle imports already-locked teaql-runtime without upgrading dependencies.
Native contracts with null default PMR check independent indexes, explicit Global
borrows, rejected User retention, actual allocator origins, const access, nested
local copies, quiescence lookup, and destruction before reclamation. Schema,
registries, and Runtime/DataService errors have separate gates.

## Check models and RuntimeError

`runtime_check_cases.json` contains 1,134 actual observations of all eight rules,
constructors, root/empty/array/mixed paths, 64-bit usize limits, three naming modes
and invalid configuration, Unicode uppercase, JSON Pointer escaping, every Value,
optional presence, empty-message overrides, independent wire fields, and nullable
serialization. Also cover all 14 RuntimeError branches, 12 nested SQL error
branches, check-list display, and three DataServiceError forwarding categories.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin runtime_check_oracle
python3 tests/parity/compare_runtime_check.py --cpp build/teaql_runtime_check_probe --rust build/rust-values/debug/runtime_check_oracle
```

Native contracts disable default PMR and read User-to-Global copies/assignments of
models, strings, Values, JSON, and error lists after source destruction. Check
concrete payloads, custom formatters, and categories even where executor/runtime
reuse a C++ type. ASan/UBSan checks domains. This batch does not prove Checker or
CheckObjectStatus execution or mutation governance. EntityValues has separate
model coverage; ready errors do not prove a complete propagation chain.

## Runtime metadata and entity registries

`runtime_registry_cases.json` contains 321 observations, including 300 scripts
with 32 mixed operations each. Cover replacement, Unicode/NUL/empty names, sorting,
missing entries, independent data-service snapshots, copied store/registry installation,
Context entity/all_entities/require_entity, service availability from either index
or metadata, and default observer presence/forwarding under metadata/debug flags.
Registry availability does not prove EntityDataService execution.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin runtime_registry_oracle
python3 tests/parity/compare_runtime_registry.py --cpp build/teaql_runtime_registry_probe --rust build/rust-values/debug/runtime_registry_oracle
```

Native null-default-PMR tests check complete registry/store copies, nested
descriptors, schema snapshots after replacement, close, foreign result domains,
and observer borrow validation. RuntimeModule, behavior/policy, decoder, and log
registration have separate sections. Full checker and business orchestration are
still pending.

## Runtime business ID policy and V1 encoding

`runtime_business_id_cases.json` contains 2,150 actual legacy/V1 plan, format,
validate, error-order, scope/range, dateless-plan, malformed configuration, signed-year,
and Unicode-whitespace observations. Preserve source differences: format does not
insert plan validation; V1 validate does not add namespace/split/digits validation,
and format checking is not key authentication.

V1 includes 600 seeded key/version/scope/sequence cases, digest-block boundary
lengths, Unicode/NUL, blank scope, u64 limits, and cycle walking. It uses length
prefixes, eight fixed HMAC-SHA256 permutation rounds, and six base36 digits rather
than hash truncation. The fixed-size C++ HMAC adds no dependency and is checked
against actual Rust hmac/sha2 output.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin runtime_business_id_oracle
python3 tests/parity/compare_runtime_business_id.py --cpp build/teaql_runtime_business_id_probe --rust build/rust-values/debug/runtime_business_id_oracle
```

Native null-default-PMR tests check Global/User allocation and retained profile,
key provider, plan, encoding, and error copies after close, then reuse retained
profiles in another User. Existing SQLite schema tests invoke the contributor SPI.
Clock/allocator/service and schema pipeline are separate. JSON Lines splits only
on LF, not valid string U+0085/U+2028/U+2029; date cases cover those characters.

## Business clocks, in-memory allocator, and service

`business_service_cases.json` contains 859 actual observations. UTC seconds/nanos
cover negative epoch, chrono bounds, leap seconds, and invalid nanos. FixedClock
and BusinessDate overrides preserve time. Allocator cases cover scopes, changed
initial/maximum, exhaustion/overflow, and sequence continuity. Service cases compare
key/current/fresh/date/time/allocate/assign call order, results, and slots for
missing/failing/rotated keys, existing IDs, invalid policy, nonnew aggregates,
date overrides, allocator errors/wrong scope/range, and consumed sequence after
formatting failure.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --manifest-path tests/parity/rust-values/Cargo.toml --bin business_service_oracle
python3 tests/parity/compare_business_service.py --cpp build/teaql_business_service_probe --rust build/rust-values/debug/business_service_oracle
```

Native tests disable default PMR and check injectable/system clocks, shutdown
protection, shared allocator in Global copies, rejected User allocator/key retention,
OOM poison, worker-thread rejection, and real SQLite passive installation/explicit
DDL/generation. Context owner-thread restrictions are documented separately from
Rust service concurrency. Public comparisons do not cover uninjectable mutex
poison. Schema/bootstrap and physical audit have separate evidence; business-save
integration remains pending.

## Internal IDs and fixed bootstrap initialization

`internal_id_cases.json` contains 171 pinned v5.0.5 observations: AtomicCounter
start/shared entity sequence/monotonic floor, missing/success/failing/zero Context
generators, bootstrap marking new without assigning ID, and Unicode canonical names.
Snowflake real system time is not replayable byte for byte; compare argument errors,
bit layout, strict ordering, and generation windows rather than identical IDs.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin internal_id_oracle
python3 tests/parity/compare_internal_id.py --cpp build/teaql_internal_id_probe \
  --rust /home/philip/shared-cargo-target/debug/internal_id_oracle
```

Native tests check same/foreign Global fallback isolation, errors without fallback,
invalid borrows/close, real SQLite runtime adapter, concurrent counters, Snowflake
clock rollback, 4096-slot exhaustion, wait failures, and pre-epoch updates. Boundary
tests call production internal transitions rather than changing system time.
AtomicCounter overflow is tested independently under Debug exceptions and Release
wrapping, never mixing build modes in recorded observations. Complete schema and
bootstrap execution require the next section's evidence.

## Controlled schema entry and dynamic bootstrap mode

`schema_runtime_cases.json` contains 384 actual observations with real RuntimeModule
registering zero to three callbacks, absent providers, optional business contributor,
failures at every stage, repeated suspension, success, and unpolled/midflight cancel.
Compare factory/poll/completion/cancel order and errors. Rust registers a real raw
sink and observes send_event bootstrap_audit; C++ reads internal mode. This proves
scope behavior, while complete event dispatch has separate contracts. External
Rust log output is disabled in the fixture.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin schema_runtime_oracle
python3 tests/parity/compare_schema_runtime.py --cpp build/teaql_schema_runtime_probe \
  --rust /home/philip/shared-cargo-target/debug/schema_runtime_oracle
```

Native tests check private/noncopyable SchemaInvocation, separate schema-provider
types, immutable borrow gates, wake forwarding, unwind restoration, same/foreign
Global boundaries, nested initialization, and close cancellation order. Modules
copy descriptors/callback lists and survive source close; installing an empty module
replaces old registration. Physical SQLite schema/audit follows; module registration
alone is not full runtime execution.

## Audit events, entity identity, and privacy projection

`audit_event_cases.json` contains 1,427 actual observations. The oracle directly
compiles pinned event.rs/log_privacy.rs to access private projection, using actual
runtime EntityKey/governance types. Direct sha2 import is an existing Rust test
dependency, not a C++ dependency. Cases cover every event factory, absence versus
Null, duplicate/missing fields, old/new snapshots, bootstrap/full governance,
all EntityKey comparison/cache kinds, Unicode masking/trimming, exact plaintext
gates, nested credentials, underscore fields, trace/ID removal, overlapping secrets,
and Value/JSON Debug.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin audit_event_oracle
python3 tests/parity/compare_audit_event.py --cpp build/teaql_audit_event_probe \
  --rust /home/philip/shared-cargo-target/debug/audit_event_oracle
```

Native tests use null default PMR for nested Global copies after source close,
sink registration/failure order, borrow rejection, invalid UTF-8, and recovery-version
Debug overflow/Release wrapping. Sanitizers check source lifetime. These model
comparisons do not prove governance execution or business-save audit timing.
Context dispatch and physical schema exercise separate event paths; formatter,
manager, and one-time plaintext warnings have dedicated gates.

## Runtime SQL logs and reprojection

`runtime_log_cases.json` contains 425 actual observations of pinned sql_entry,
comparing complete original/reprojected SqlLogEntry Debug. Cover every Value,
compiled policies, generated SQL, mask markers, unknown dialect, SQLite binding/
template failure, intent, counts, time bounds/full u64 Duration, repeated safe/debug
projection, and fallback after changing SQL/parameters/time/comment/trace/policy/
mode or losing private state. PostgreSQL/MySQL implementations remain out of scope.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin runtime_log_oracle
python3 tests/parity/compare_runtime_log.py --cpp build/teaql_runtime_log_probe \
  --rust /home/philip/shared-cargo-target/debug/runtime_log_oracle
```

Native tests disable default PMR and check safe reprojection of copied sensitive
logs/private safe state, nested UnifiedLogBuffer copies, unchanged inputs, tamper
fallback, credentials/unknown provenance, and standard SHA-256 known answers.
These tests do not validate Rust's wider SystemTime range. Formatting, manager,
and Context recording are separate.

## Log formatting, configuration, and output boundaries

`log_formatter_cases.json` contains 927 actual observations: 144 audit formats,
315 SQL formats, and 468 EntityKey Debug/configuration/truncation cases. The oracle
includes pinned formatter code and its private truncation helper. Human comparison
removes only the volatile current-time prefix; trace/intent/payload remain intact.
Debug compares fully. Source json configuration selects DebugReaderFormatter,
not a JSON serialization protocol.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin log_formatter_oracle --bin audit_event_oracle --bin runtime_log_oracle
python3 tests/parity/compare_log_formatter.py --cpp build \
  --rust /home/philip/shared-cargo-target/debug
```

Native tests check one-time lazy Global selection, independent applications,
rejected allocation origins, nested configuration copies, output after User close,
and every format entry with null default PMR. Third-party sensitive defaults use
ordinary output. UTC prefix is checked against real time and millisecond shape.
Additional cases cover field names, explicit Null IDs, missing projection, duration
rounding, UTF-8 byte truncation, and invalid environment UTF-8. Manager and Context
dispatch have their own gates.

## LogManager output and application lifetime

`log_manager_cases.json` contains 94 isolated-process v5.0.5 observations. Fresh
directories compare actual files/stdout/stderr and lazy state after environment
changes: ordinary/sensitive output, filters/close order, absent endpoint caching,
one normal-header attempt, late directory creation, direct field/event plaintext
warnings, repeated safe projection, and large UTF-8 truncation. Normalize only
Human time, actual executable name, and the source's JSON header claim; C++ headers
accurately state that json/debug select DebugReaderFormatter.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin log_manager_oracle
python3 tests/parity/compare_log_manager.py --cpp build/teaql_log_manager_probe \
  --rust /home/philip/shared-cargo-target/debug/log_manager_oracle
```

Native null-default-PMR tests check per-application warning isolation, sharing
between direct projection/manager, rejected foreign/closed scratch, no Global
payload growth after repeated requests, output during stop, poisoned stdout/stderr,
and ignored file failure. Sanitizers verify these C++ lifetime adaptations. Native
output tests use Linux /dev/null, /dev/full, and dup2; non-Linux executable-path
adaptation and complete governance are outside this corpus. Context dispatch is
covered separately.

## Context metadata history and unified buffering

`context_logging_cases.json` contains 315 actual observations: seven operations,
complete/rebuilt trace, duplicate/empty intent, default backend, separate select/
mutation flags, disable versus clear, leaf-only nested statements, result-count
priority, reversed clocks/signed nanosecond extremes, and identity snapshots.
The oracle includes original context/logging.rs with only required fields/resource
lookup scaffolding; entries, privacy, and manager are locked implementations.
The scaffold does not claim default OS-identity equivalence; native tests cover
construction capture, empty identity, UTF-8, and fallback.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin context_logging_oracle
python3 tests/parity/compare_context_logging.py --cpp build/teaql_context_logging_probe \
  --rust /home/philip/shared-cargo-target/debug/context_logging_oracle
```

Native tests disable default PMR for foreign-buffer copies, independent snapshots,
close reclamation, final records during stop, real file normal/sensitive ordering,
and credential hiding. /dev/full verifies both buffers save before output throws.
EntityDataService and TransactionScope business call sites remain to be fully
connected; available recording methods do not make those paths complete.

## Telemetry, propagation, and exception isolation

`telemetry_cases.json` contains 1,032 actual public observations: four attribute
kinds including u64-to-i64/signed-zero/NaN bits, exact disable keys, overwrite, error
classification priority, Noop/create failure, successful/failed completion,
activation throwing before/after callbacks, skipped/repeated callbacks, and business
panic interacting with adapter catches. Compare inner/outer Context on every poll,
results, exceptions, and destruction. Real Rust drives scope poisoning versus
nonpoisoned propagation.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin telemetry_oracle
python3 tests/parity/compare_telemetry.py --cpp build/teaql_telemetry_probe \
  --rust /home/philip/shared-cargo-target/debug/telemetry_oracle
```

The C++ poll_with_context fixture checks the same callback-selection algorithm;
actual AsyncOperation composition is checked natively with null default PMR,
Global copies, consumed-box rejection, early destruction, and threads for wake,
finish, cancel, and close. Destructors are noexcept and Context access owner-thread-only;
Context sharing across threads and external exporters are not tested/in scope.
Full business instrumentation remains pending.

## Production SQLite physical schema

`sqlite_schema_cases.json` contains 735 real public v5.0.5 observations through
RuntimeModule/SqliteProviderExt/UserContext.ensure_schema, without rewriting schema
algorithms. C++ uses the controlled runtime entry too. Coverage includes all
DataType families/nullability/length/numeric domains; create/repeat/add-column;
required columns on empty/populated tables; DDL rollback; index table/uniqueness/
partial/direction/expression/collation; to-one/to-many foreign keys/service boundaries;
missing/composite/action constraints; quoted/Unicode names; and soundex native types.
Compare full sqlite_master, query output, autocommit, events, and event transaction
state. Check ignored sink Result errors, postcommit rejection of legacy initial/root
graphs, and begin failure during an active transaction. All 735 compare verbatim
without hiding database errors.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin sqlite_schema_oracle
python3 tests/parity/compare_sqlite_schema.py --cpp build/teaql_sqlite_schema_probe \
  --rust build/rust-values/debug/sqlite_schema_oracle
# After changing fixtures, observe actual Rust again; reference sources and the model generator remain unchanged:
python3 tests/parity/observe_sqlite_schema.py --rust build/rust-values/debug/sqlite_schema_oracle
```

```sh
rustc -O tests/parity/rust_unicode_lower.rs -o build/rust_unicode_lower
./build/rust_unicode_lower > build/unicode_lower_data.inc
cmp build/unicode_lower_data.inc src/provider_sqlite/unicode_lower_data.inc
```

Native tests add JSON-inexpressible commit hooks, cache cleanup, sink exceptions/
poison, connection wake/cancel, close, and Global copies. Leases revoke before close
and nested data follows its domain. Unicode contracts check all 1,112,064 scalars'
lowercase and four Greek final-sigma contexts. Tables contain only Unicode 17 facts
observed with Rust 1.97.1, without ICU/locale/Rust runtime. Extraction produces fact
data, not entity-generator output. This evidence covers physical schema; complete
entity execution, graph save, governance, and business-save audit remain pending.

## Identity graphs and mutation ledgers

`entity_runtime_cases.json` contains 241 actual entity_runtime.rs observations,
including 128 seeded `0x5051ED6E` sequences of 18 operations. Compare three states'
equality, lazy ledger, changes, layered fields, original/version, new/deleted keys,
comments, and composition errors. Other cases cover numeric/complex keys, loaded
fallback, atomic version conflict/order, weak publication authority, refreeze,
typed queries, relation owners, and loaded/empty/missing states.

The oracle includes the complete source unchanged; thin same-module wrappers expose
private lazy/weak/composition checks, not replacement algorithms. Private visibility
and public reachability are recorded separately. Native tests cover LedgerEntity
defaults, Context lifetime, copied cycles/shared ledgers, nested PMR, noncopyable
payload rejection, and resource destruction before blocks.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin entity_runtime_oracle
python3 tests/parity/compare_entity_runtime.py --cpp build/teaql_entity_runtime_probe \
  --rust build/rust-values/debug/entity_runtime_oracle
python3 tests/parity/observe_entity_runtime.py --rust build/rust-values/debug/entity_runtime_oracle
```

Same-domain sharing is a Context borrow; cross-domain retention is explicit copying,
not independent Rust Arc lifetime. Context thread safety is not claimed. Decoder
and relation kernels have separate evidence. Full EntityDataService orchestration
and ledger saving remain incomplete; graph/ledger models are not those workflows.

## Graph decoders and module registration

`entity_decoder_cases.json` contains 182 public registry/RuntimeModule observations.
Handwritten entities implement Entity/IdentifiableEntity and on_loaded without
copying decoder algorithms. Cover compact/batch ID checks, partial batch installation,
atomic list replacement, option first-row/empty behavior, same-name type replacement,
registry cloning, per-decode ledger groups, and 64 module registration orders.
Native tests add Context delegation, default/custom hooks/policy, PMR copies,
short-lived-borrow rejection, close-time empty reads, and real SQLite row decoding.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin entity_decoder_oracle
python3 tests/parity/compare_entity_decoder.py --cpp build/teaql_entity_decoder_probe \
  --rust build/rust-values/debug/entity_decoder_oracle
python3 tests/parity/observe_entity_decoder.py --rust build/rust-values/debug/entity_decoder_oracle
```

Default behavior/RequestPolicy interfaces and registration are wired. Registration
alone does not mean complete query/save entry points execute them automatically.

## Base query adaptation and relation plans

`runtime_base_cases.json` contains 256 actual pinned base.rs observations. build.rs
extracts fetch_all/fetch_smart_list/fetch_entities/enhanced aliases unchanged with
MetadataStore/QueryExecutor test implementations. Combine capture/debug/executor
failure/decoder failure/observer/trace; compare fields, log timing, loaded state,
original errors, and entities.

`entity_service_cases.json` contains 112 relation-plan/query observations. Behavior
uses actual public relation_plans/relation_query; explicit RelationLoad and compact
paths use unchanged private methods with field-only wrappers. Private behavior
plans also compare with public API. Cover missing descriptors/relations, dotted
sorting, duplicate explicit loads, every identity kind, IN thresholds, projection,
slice/partition, ID tie-breakers, nested entity override, and compact child clearing.
Equal-bucket representation is not promised by Rust unstable sort; all equal-key
layouts are not claimed.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin runtime_base_oracle --bin entity_service_oracle
python3 tests/parity/compare_runtime_base.py --cpp build/teaql_runtime_base_probe \
  --rust build/rust-values/debug/runtime_base_oracle
python3 tests/parity/compare_entity_service.py --cpp build/teaql_entity_service_probe \
  --rust build/rust-values/debug/entity_service_oracle
```

Native tests check binding order, query/update behavior-policy dispatch, bootstrap
skipping only behavior, private-tree privacy snapshots, callbacks/operations after
temporary wrapper destruction, close cancellation, copied plans/SmartLists, and
real SQLite entity lists. Basic compatibility adapters are not complete public
fetch, automatic hydration, or graph save.

## Context query observation and relation execution

`context_query_cases.json` contains 468 observations from unchanged context observe,
four query methods, resolve_final_comment, and base query methods. Real UserContext
supplies scopes/flags/identity. Thin private bridges expose Noop and a log-call spy;
full projection is tested separately. Compare two scopes' start/enter/leave/finish/
drop, suspension/cancel/adapter exceptions, requests, and errors. Only cross-language
provider.kind/derived names normalize to Mock.

`relation_execution_cases.json` contains 432 observations from original top-N,
telemetry, query/base/compact construction, and fetch_relation_rows. Wrappers use
real metadata; child fetch doubles record requests and simulate pending/failure.
Compare thresholds, capabilities, metadata flags, empty/duplicate parents, predicates,
partition/projection/nesting/order, returns, cancel, and first-error stop. Compact
per-parent probes retain nested relations rather than being forced to match the
window path.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin context_query_oracle --bin relation_execution_oracle
python3 tests/parity/compare_context_query.py --cpp build/teaql_context_query_probe \
  --rust build/rust-values/debug/context_query_oracle
python3 tests/parity/compare_relation_execution.py --cpp build/teaql_relation_execution_probe \
  --rust build/rust-values/debug/relation_execution_oracle
```

Native null-default-PMR Context tests check temporary wrappers, lazy Noop naming,
invalid result domains, Global copies, and real-thread completion/cancel/close.
Relation tests connect executor/transport/SQLite for per-parent pagination, stable
order, both metadata routes, empty skip, wake, and close. These are composable
kernels; full entity query, cache/pagination, and mutation orchestration need separate
gates. Recursive hydration follows.

## Recursive relation hydration and installation

`relation_hydration_cases.json` contains 356 observations compiling original
enhance_plan/hydrate_flat_plan/compact/leaf/install/enhance_child_record/
relation_child_repo/attach_relation_rows and query/execution methods. Field wrappers
and private decode bridges use the real four registries. Entity fetch doubles return
rows, suspend/fail, and record lineage; handwritten models share EntityRuntimeState.
Compare graph entities/relations, loaded flags, ledger equivalence, duplicate/missing/
different identity kinds, inverse cardinality/existing type, empty parents, missing
decoders, partial decode failure, nesting order, canceled mem::take state, and
telemetry without a replacement Rust algorithm.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin relation_hydration_oracle
python3 tests/parity/compare_relation_hydration.py --cpp build/teaql_relation_hydration_probe \
  --rust build/rust-values/debug/relation_hydration_oracle
```

Native null-default-PMR tests connect independent entity fetch to SQLite and check
three recursive paths, absent targets, shared graph identity, inverse cycles,
Global copies, deep native suspension completion/cancel/close, plan isolation,
domain rejection, and scope cleanup order. Complete public fetch, purpose,
pagination, and cache integration are not proved. Other enhancement follows.

## Relation aggregation and row enhancement

`relation_enhancement_cases.json` contains 266 observations compiling three original
enhance methods, aggregate execution/planning, and helper defaults/identity/assembly.
Wrappers provide real metadata and suspend/fail fetch doubles, not algorithms.
Cover count alias, scalar/object/list results, duplicate parents/last target wins,
column mapping, empty/missing keys, all query fields, predicates/ID order, trace,
cache-option propagation, first error/cancel, and partial updates.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin relation_enhancement_oracle
python3 tests/parity/compare_relation_enhancement.py --cpp build/teaql_relation_enhancement_probe \
  --rust build/rust-values/debug/relation_enhancement_oracle
```

Native SQLite tests check grouped count/sum, empty defaults, object lookup, and
field overwrite. Null-default-PMR tests check Global copies, input snapshots, and
overlapping rows. Threads exercise finish/cancel/close for all three enhancements,
retaining child repository/query borrows until native access stops. Foreign parents
and children reject. Propagated cache options do not prove cache execution.

## Query cache and pagination stores

`query_store_cases.json` contains 259 observations compiling original cache/store
implementations and independent pagination observation methods. Bridges only set
private capacity/byte limits/stored_at and inspect keys/current bytes. Defaults
come from public UserContext. Direct async-trait import compiles an already-locked
Rust trait and adds no C++ dependency.

Cover put/get/clear/invalidate, zero age, past/future monotonic starts, retention or
removal of expired entries, eviction before full-capacity replacement, oversized
ID rejection before expired cleanup, prefix/exact matching, cancel before poll,
nested boundaries, far-future SystemTime, and defaults. Different deadlines avoid
specifying Rust HashMap's equal-deadline ordering. Hits use generous validity;
expiry adjusts stored_at rather than depending on build timing.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin query_store_oracle
python3 tests/parity/compare_query_store.py --cpp build/teaql_query_store_probe \
  --rust build/rust-values/debug/query_store_oracle
```

Native null-default-PMR tests check User isolation, explicit Global sharing/copies,
old futures after replacement, independent read copies, canceled writes, domains,
and nanosecond expiry. Upstream allocation timestamps prove lifetime starts after
large copies finish, without sleeps. Native IdSetStore threads stop before resource
and arena destruction. Store access remains owner-thread-bound; no claim covers
Rust mutex poison, shared Context concurrency, or complete pagination. Canonical
keys and entity-level execution remain pending; coordination follows.

## Same-key ID-set build coordination

`id_set_build_cases.json` contains 144 scripts compiling the original pagination
registry with real locked futures-util 0.3.34 async mutex. Owned lock futures avoid
self-referential fixtures; owned/borrowed poll/drop share try-lock/slot/wake logic.
Step observations compare identity, ready/pending, guards, and wake counts for
same/different/empty/Unicode/NUL keys, canceled-wake transfer, slot reuse, new-poller
overtaking, repeated release, and lookup after leases disappear. C++ subtracts its
generic cancellation WakeSignal notification to compare only mutex wakes. Direct
futures-util import is an existing Rust dependency and adds no C++ library.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin id_set_build_oracle
python3 tests/parity/compare_id_set_build.py --cpp build/teaql_id_set_build_probe \
  --rust build/rust-values/debug/id_set_build_oracle
```

Native null-default-PMR tests check cross-User reuse, separate Globals, copied
registry keys, invalid domains, reset borrow gates, cancel/close wake transfer,
registry reuse, and all-block reclamation. A suspended native worker holds a guard,
reads User input, and verifies the lock before join; release follows join.
These contracts accept the coordination primitive only. Cache-miss rereads, ID
building, and fallback orchestration still need implementation and differential gates.


## Canonical aggregation and pagination query keys

`query_keys_cases.json` records 451 actual observations. build.rs extracts the
original aggregation namespace/key/canonicalization helpers and both pagination
key methods unchanged. A thin field wrapper supplies optional user/root identity;
it does not implement the key algorithm. The oracle uses the real core query,
metadata, Expr, and Value Debug implementations and standard DefaultHasher.

Compare complete query Debug, canonical Debug, aggregation keys, continuous-page
keys, ID-set keys, and entity namespaces. Cases cover all expression/function/
aggregate kinds, every Value branch, recursive descriptors/relations/enhancements,
metadata normalization, stable deduplication including NaN and signed zero,
Unicode/NUL, hash block boundaries, identity absence/empty strings, and root values.
Hash framing follows the observed 64-bit little-endian Rust test toolchain;
process-local pagination keys are not a portable wire protocol.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin query_keys_oracle
python3 tests/parity/observe_query_keys.py \
  --rust /home/philip/shared-cargo-target/debug/query_keys_oracle
python3 tests/parity/compare_query_keys.py --cpp build/teaql_query_keys_probe \
  --rust /home/philip/shared-cargo-target/debug/query_keys_oracle
```

`query_keys_contracts` disables default PMR and checks input immutability, retained
semantic distinctions, pagination identity boundaries, stable zero/NaN behavior,
and recursive Global copies after User close. Internal Debug contains values and
is not a safe logging interface. Pure helper acceptance does not establish
cache/pagination orchestration or Context active-root integration.
