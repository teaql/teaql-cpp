# SQL dialect responsibility review

## Source acceptance, 2026-10-08

The dialect source group is accepted after complete production lines 1–1522,
all public/default/helper/override/error/ownership responsibilities, and final
current native 314/314 in 74.66 seconds with a stable post-test no-work build.
Fresh six-check native/sanitizer/MIPS selections pass 6/6; live native/MIPS
observations cover Unicode quoting and storage helpers, direct-IN preconditions
and partial bindings, mutation provider-call order, and the preserved schema/
mutation corpora. Existing direct source compiler/default ports and ownership/
override contracts are included in the full regression. This accepts the compact
provider-independent compiler under the documented C++ allocation adaptations;
it does not accept complete executor/provider or runtime graph/privacy stages.
Earlier remaining-gate paragraphs below retain their dated checkpoint context.

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-sql/src/dialect.rs`. The source has seven public helper functions,
SchemaIndexSpec and forty-three trait methods. This is a responsibility mapping
and work queue, not a declaration-count acceptance verdict. SQLite remains the
only implementation; the mandatory dialect boundary and overridable stages
must remain independently usable.

## Public responsibilities

| Source items | C++ contract and remaining review |
| --- | --- |
| quote_identifier_if_needed, bounded_sql_identifier | Preserve wrapper recognition, keyword/character quoting, escaping, byte limits, Unicode boundaries and stable FNV suffix. |
| schema_index_specs, schema_foreign_key_name | Preserve index order, model-derived role/column names, uniqueness, Unicode uppercase and shared generated-name bounds. |
| validate_schema_identifier_lengths | Structured error identifies object, complete identifier, actual bytes and maximum. No silent provider truncation. |
| storage_length_covers, storage_numeric_covers | Existing storage must cover the model's full declared domain; numeric precision/scale are separate constraints. |
| SchemaIndexSpec | Four public fields (name/table/ordered columns/unique), scoped Clone, structural PartialEq/Eq and complete derived Debug; source has no Default/Hash/serde derive. |
| kind, quote_ident, placeholder | Three mandatory provider hooks. Keeping a database discriminator is not implementing an excluded backend. |
| large_in_uses_array_param, prefers_small_parent_relation_probes, schema_setup_sqls | Source defaults false/false/empty; an adapter must not infer unsupported capabilities. |
| schema_type_sql, column_definition_sql, compile_create_table | Preserve type/default behavior and shape validation before DDL; ID/not-null responsibilities are distinct. |
| schema_indexes_sqls, fallback_default_value_sql, compile_add_column | Use the shared specs and separate provider hooks; adding a nonnullable non-ID column uses the original fallback rule. |
| compile_select, compile_select_sql | Preserve the public wrapper and overridable compiler body, projections, clauses, limits/offsets, ordered bindings, comments and compiler-owned provenance. |
| compile_insert, compile_batch_insert | Preserve ordinary/batch insertion independently, field/role validation, ordering and per-item trace responsibilities. |
| compile_update, compile_guarded_update, compile_update_with_guard, compile_batch_update | Keep four entry points and datastore guards distinct; source version, field, snapshot and batch rules require independent review. |
| compile_delete, compile_guarded_delete, compile_delete_with_guard | Keep soft/hard deletion, version and atomic guard behavior separate. |
| compile_recover, compile_guarded_recover, compile_recover_with_guard | Preserve negative-version validation and the separate recover/guard responsibilities. |
| column_sql, column_with_alias | Resolve metadata/aliases independently of execution; unknown fields retain structured errors. |
| order_by_sql, resolve_order_field | Ordered expressions, trusted fields, GBK and direction dispatch must remain overridable. |
| select_projection, aggregate_projection, compile_projection | Keep ordinary and aggregate projection stages separate, including expressions, aliases and fallback columns. |
| aggregate_call_sql, aggregate_function_sql, resolve_aggregate_field | Preserve aggregate naming and field/expression resolution rather than hard-coding all stages in compile_select. |
| compile_expr, compile_function, compile_single_arg_function, compile_gbk_function | Preserve AST variants, individual function arity/errors, zero-argument COUNT, independent SOUNDEX and GBK dispatch and scoped binding provenance. |
| compile_subquery, compile_joined, compile_in | Preserve operator validation, nullability, logical identities, input/binding order, large-IN capability hooks and entity-scope isolation. |

Private helpers, including schema-shape validation and identifier recognition,
remain part of these observable contracts. This table does not prove every
method/default/error/override path. C++ structured exceptions adapt source
Result errors; they must preserve the complete variant/payload and partial
compiler state at the same failure boundary.

## Public quoting and storage-helper follow-up

The original public quoting helper takes a Rust `char`, including every Unicode
scalar. The native eight-bit `char` signature silently narrows such a delimiter:
a direct call with U+00AB returns lone AB bytes instead of the original C2 AB
UTF-8 sequence. A pure-helper reproduction isolates this difference without
Context or provider execution. The new `char32_t` overload preserves the complete
source scalar domain, encodes its UTF-8 sequence, and doubles complete matching
scalar sequences in an identifier. The existing byte-character overload remains
compatible for provider ASCII delimiters. Already wrapped identifiers and normal
unquoted names retain the original early-return rules. C++ scalar values outside
the Rust char domain are rejected before encoding.

The test oracle now calls the actual original public quote and length/numeric
coverage functions directly. Two source executions reproduce 262 observations:
quoted/unquoted/wrapped/keyword/empty/NUL identifiers, Unicode delimiters at UTF-8
and scalar boundaries, and optional length/precision/scale containment including
u32 extrema and saturated integer-digit subtraction. The original 136 helper
observations and all 1,305 existing schema observations remain unchanged.
The registered counterpart is sql_dialect_helpers_rust_parity. Native ownership
contracts retain quoted text through the User-facing Global allocation proxy,
delete User, rebind the Global reference, and verify the complete text with the
default PMR disabled. Fresh selected builds finish in an isolated native CMake directory and the
sanitizer/MIPS directories. All three named checks pass native (1.53 seconds),
ASan/UBSan with leak scanning disabled (2.38 seconds), and MIPS/QEMU (3.47
seconds). Both native and MIPS match all 262 helpers and all 1,305 schema inputs
against live original Rust. The ownership test also rejects C++ surrogate and
out-of-range scalar inputs, which cannot be constructed as Rust char values.
The complete native consumer refresh remains running and must be incrementally
refreshed for late headers and the new target. Complete dialect/source and
Stage 3 gates below remain open; these checks cover helpers and their callers,
not every compiler/default/override responsibility.

## Complete compiler-body review and provider order repair

The review reads every production line before the test module (lines 1–1522)
in the pinned 1,606-line dialect.rs, including every public trait default and
private schema/identifier helper. It maps schema hooks, SELECT clause order,
partitioned early return, projection deduplication, expression/subquery scopes,
function dispatch, insert/batch shapes, version arithmetic, guarded mutations,
and complete error/partial-binding boundaries to the separate C++ implementations.
This reading is not an acceptance verdict: direct default calls are audited in
addition to ordinary query/mutation entry points and provider hook counts.

Instrumented mandatory provider hooks call the actual original mutation defaults.
Forty-eight inputs observe kind(), identifier quoting and placeholder order,
including a provider failure during final table rendering. All 2,505 previous
mutation observations are revalidated unchanged. The pre-repair native probe
matches zero of the 48 hook observations: it captures provenance after final SQL
rather than before it. For soft delete/recover, it also renders the assignment
before the table, reversing the original provider calls.

The private C++ completion helper now captures provenance before invoking a
stack-local SQL builder. Final soft-delete/recover rendering quotes the table,
then the version assignment and placeholder. No heap callback, new runtime
abstraction or provider support is added. Fresh two-check selections pass native
(5.65 seconds), ASan/UBSan with leak scanning disabled (11.96 seconds), and
MIPS/QEMU (8.42 seconds); all 48 hook observations match live original Rust on
native/MIPS, alongside the unchanged 2,505 mutation corpus.

Further direct compile_in review reproduces an unvalidated operator path: the
source compiles the left operand, then panics for non-IN operators; the native
method silently treats them as NOT IN. A separate 216-input original corpus
covers all twelve BinaryOp values, literal/credential/missing left operands,
list/empty/missing right operands, numbered placeholders and partial values/
policies after error or panic. Two actual source runs agree, with all 262 earlier
quote/storage inputs unchanged. The pre-repair native counterpart matches only 120/216 observations. The
96 mismatches expose silent NOT IN fallback and incorrect right-side errors for
invalid operators. Validation now occurs immediately after left compilation and
before right evaluation. It preserves earlier left errors and partial bindings,
and maps the source unreachable precondition to a C++ logic error. The direct
comparison is registered as sql_dialect_in_rust_parity. Fresh combined six-check
selections pass native 6/6 (7.04 seconds), ASan/UBSan with leak scanning disabled
6/6 (13.96 seconds), and MIPS/QEMU 6/6 (10.26 seconds). They include dialect
ownership/override contracts, 262 quote/storage helpers, 216 direct-IN cases,
48 provider-hook cases, 1,305 schema inputs and 2,505 mutation inputs. Both
native and MIPS match all 216 direct-IN cases against live original Rust.
Both native and MIPS also match all 2,505 mutation inputs against the live
original after these corrections. Complete current consumers and dialect/Stage 3
acceptance remain open; the
whole-project native refresh is still running and requires a final incremental
refresh for the late implementation/fixture/CMake changes.

## SchemaIndexSpec diagnostic repair

A public-facade compile check reproduces the missing SchemaIndexSpec Debug
overload. model_debug.hpp now writes the exact source type and all four fields,
using existing string/sequence/bool formatter behavior. It adds no provider or
runtime dependency. Raw diagnostics are separate from privacy projection.

The existing 1,305 schema inputs are observed against actual original Rust.
Every input and previous expected field is checked unchanged before adding
derived Debug in nine modes and actual Clone equality. The new fields contain
5,319 exact diagnostic outputs. The isolated native probe matches all 1,305
expanded live observations. Native ownership checks additionally preserve
ordered duplicate columns, compare changed columns, retain full name/table/
column allocations into Global, delete User and compare complete diagnostics.
The standalone native ownership executable passes. Its fresh named native
selection passes both schema comparison and model ownership checks (2/2,
2.17 seconds). The same named checks pass ASan/UBSan with leak detection 2/2
(2.63 seconds) and MIPS/QEMU 2/2 (20.86 seconds). MIPS also matches all 1,305
expanded observations against live original Rust. Whole-project integration
remains required; earlier full-project results cannot be reused for this header.

Existing dialect contracts verify provider override dispatch, ordinary versus
aggregate projection, schema type/default hooks, independent capability defaults,
scope restoration after subquery errors and allocation failure, destination-owned
compiled bindings and index/domain retention. Existing SQL/schema/mutation
comparisons remain complementary evidence for their actual scope.

## Remaining acceptance

The complete production-body reading and the named helper/default corrections
above are finished. Final public-default/override composition acceptance, mutation
guard/trace/provenance integration, and whole-project consumers remain required. The source group stays
partial; this repair does not accept the complete dialect, Stage 3 or the
six-stage migration. Generator and non-SQLite implementation changes remain
outside the authorized scope.
