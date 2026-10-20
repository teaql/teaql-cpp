# Core dynamic search source review: Rust 5.0.6

Current source disposition: **accepted** at the [installed cohort checkpoint](source_cohort_acceptance_5_0_6.md). Earlier staged/pending paragraphs retain their historical scope.

Status: complete-body pre-acceptance review. The production source and its
integration-test file remain `pending_review` until permanent module/reproduction
and full native integration gates are satisfied. This does not accept complete
G10/G13/G17 or P0-P7.

The reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Both the complete
297-line `teaql-core/src/dynamic_search.rs` and 221-line
`teaql-core/tests/dynamic_search.rs` are byte-identical to the retained exact
5.0.5 reference. Their query/value dependencies have nevertheless changed, so
the historical [source-test acceptance](dynamic_search_source_acceptance.md)
is preserved separately from the current replay below.

The C++ implementation, declarations, raw formatters and complete semantic test
port have been read alongside both original files. This is strict local UI
search, separate from the permissive JSON helpers in the
[request builder review](core_request_source_review_5_0_6.md). It does not
execute SQL or replace trusted application metadata, bindings or authorization.

## Complete source responsibilities

| ID | Original responsibility | C++ disposition and observation |
| --- | --- | --- |
| DS01 | Seven models and SearchModels alias | Preserve trusted ordered field/relation maps, warning/filter/order records, normalized result, merged query result and static-text error as distinct models. |
| DS02 | Derived capabilities | SearchModel has empty Default/Clone/Debug; warning/order/error retain Eq, filter retains PartialEq, and normalized/merged wrappers retain Clone/Debug. The sole serde model is DynamicSearchWarning with camelCase fieldPath; no other serialization/equality protocol is invented. |
| DS03 | Trusted setup before parsing | Reject a zero clause bound or absent root model before examining submitted JSON. Do not obtain models or limits from that JSON. |
| DS04 | JSON/root controls | Require one valid JSON object with only filter/orderBy controls. Missing controls produce empty collections; explicit NULL/wrong shapes fail. Unknown clauses still count toward the combined bound. |
| DS05 | Operators before field resolution | Bare values select $eq. An operator object must have exactly one of the original nine keys; malformed/unsupported operators fail even for an unknown field. |
| DS06 | IN bounds and shape | $in/$notIn require an array of at most 1,000 entries before field lookup. Empty IN arrays are valid. Known-field arrays under other operators fail rather than becoming permissive request ranges. |
| DS07 | Safe paths and relation metadata | Allow at most sixteen nonempty segments, reject dollar-prefixed/prototype-related segments, warn for unresolved fields/relations and fail for an installed relation whose trusted target model is absent. |
| DS08 | Integer/timestamp/number values | Retain finite-number checks, integral values within the 53-bit safe range for integer/timestamp fields and the original acceptance of integral floating JSON. Do not coerce numeric strings or Booleans. |
| DS09 | String/Boolean/decimal/NULL values | Known fields use their actual JSON scalar kinds. Decimal text permits the original optional sign and digit groups without narrowing it through floating point. NULL succeeds before the field-kind test. Unknown metadata kinds are not globally rewritten or assigned a new default. |
| DS10 | Date values | Require ten bytes, a real calendar date, identical formatted roundtrip and a year other than 0000. Preserve leap-day success and invalid-date errors. |
| DS11 | Ordering | Require exactly field/direction with string values and asc/desc. Preserve submitted order sequence and duplicates; unknown paths become ORDER_BY warnings. Filter iteration follows the ordered map. |
| DS12 | Deferred warning publication | Complete validation before calling a warning sink. A late fatal filter/order failure emits no previously collected warnings. Sink exceptions propagate rather than becoming input-validation errors. |
| DS13 | Normalized output | Retain canonical paths, original operator strings and complete JSON operands, alongside ordered filters/orders/warnings. Renaming C++'s keyword-conflicting operator field to operator_name does not change the original raw diagnostic label. |
| DS14 | Trusted merge callbacks | Normalize silently with the source fixed bound of 100, bind every filter before any order, and short-circuit binding errors without publishing warnings. Caller callback side effects are not a database transaction. |
| DS15 | Immutable scoped base | Clone only after successful binding, combine filters, append ordering and preserve the existing query's scope, slice, hard limit, intent and other metadata. The helper does not authorize a query merely because it preserves that base. |
| DS16 | Default versus custom warnings | Custom sinks and explicit warning JSON retain fieldPath. The default structured stderr projection substitutes <omitted> and contains no submitted values. Raw Debug is a separate model diagnostic, not a safe sink. |
| DS17 | Private helpers | Keep field_type, validate_scalar, decimal_text, warning construction and emit behavior separate from permissive request JSON inference. Scalar/path errors retain the original static messages and branch order. |
| DS18 | C++ ownership | Nested maps/strings/JSON/query/warnings use the explicit destination resource. Ordinary wrappers do not extend Context lifetime; explicit Global copies may survive source close. DynamicSearchError's diagnostic text follows its static-lifetime contract. |
| DS19 | Complete source tests | Preserve the School/Platform fixture, twelve fatal inputs, six-field valid input, four unknown warnings, 1,001-element IN rejection, missing relation metadata, immutable tenant scope/order/limits, late-failure behavior and parent/child warning canaries. |

## Current source and imported evidence

The [terminal receipt](parity/dynamic_search_source_import_5_0_6.json.gz)
is complete and records:

- All **eight unchanged original integration tests pass** in the real locked
  core crate, including its actual subprocess canary. No copied normalizer or
  substitute test harness supplies their behavior.
- All **509 search observations** and **94 auxiliary-model observations** match
  the immutable active corpora and the actual original implementations through
  C++ imports. Expected errors, values and diagnostic fields are not normalized.
- The auxiliary observer includes raw diagnostics for all seven search models,
  source clone/equality checks for warning/filter/order/error and the existing
  core models in the same corpus. Complete outputs are compared; unrelated
  fields are not discarded to make the search subset pass. The remaining clone
  paths have the separate direct-source controls below.
- The unchanged request/search ownership program and complete original-search
  semantic port both pass. The real Python parent also executes the imported
  canary child, requires its original run marker and checks both output streams
  for the two forbidden canaries and the required omitted-path marker.
- All four programs compile and link successfully before execution. Six canonical
  entry/fixture bodies preserve the original handwritten functions and assertions;
  only include prologues/paths differ.
- All **87 registry identities** match the exact reference lock. Module archive/
  BMI fingerprints and all **10,653 frozen execution inputs** are unchanged after
  the gate. The independent producer, generated SDKs and both references remain
  untouched.

The ownership program disables default PMR and checks ordered callback failures,
warning exceptions, default-log omission, recursive allocation domains, explicit
Global retention and failed cross-domain copy cleanup. The source semantic port
uses a separate explicit arena fixture with default PMR disabled. Neither creates
another application query ownership domain or a shared Context owner.

## Default and remaining Clone controls

The [supplemental receipt](parity/dynamic_search_clone_source_5_0_6.json.gz)
closes the distinct Default/Clone observation gap rather than inferring it from
raw formatting. All **32 actual original/imported observations agree** for
SearchModel Default and SearchModel/NormalizedDynamicSearch/DynamicSearchResult
cloning. Inputs cross empty/populated filters, orders and warnings with eight
text forms, including long text, Unicode, quotes, newline and NUL data.

The Rust fixture invokes actual Default/Clone and then mutates the copies.
Complete source snapshots must stay unchanged and equal their initial copies.
It explicitly drops all three originals before inspecting and mutating retained
copies again. No alternate clone or normalization algorithm supplies results.

The C++ fixture makes explicit Global copies, captures the original snapshots,
then actually deletes UserContext. It requires zero live User bytes and Global
allocators on every retained model map/key/value, normalized filter/JSON/order,
query and warning collection/field before using the retained copies. Default
PMR is disabled. Its exact post-deletion diagnostics match the original
post-drop observations, including independent later mutations.

The new temporary C++ fixture is formatted and checked with pinned clang-format
18.1.8 before compilation. All seven controller commands exit zero; compilation
and linking finish before execution. Module artifacts and all 10,653 frozen
execution inputs remain unchanged. Together with the preceding immutable
families this supplies **635 exact original/imported comparisons**;
it remains scoped evidence, not permanent integration or a new sanitizer result.

## Remaining integration boundary

The full native epoch continues building its existing frozen inputs. Temporary
Rust manifests, canonical mirrors and compiled import programs live under
`/tmp/teaql_dynamic_search_source_review_5_0_6`; the compressed receipt preserves
their controller, exact sources, dependency lock and terminal commands.

After that epoch ends, install the missing permanent request/search and source-
port module registrations and subprocess gate, retain the supplemental clone
fixtures/corpus, and provide repository-owned reproduction. The existing
search/auxiliary probes already have module entries;
this temporary execution does not silently add the missing unit/child gates.
Promote neither source file until its remaining integration obligations pass.

SQLite search execution, guarded writes, broader generated JSON, runtime policies
and protected trace/publication callers retain separate requirements. This
checkpoint changes no production function, generated artifact, source disposition
or capability verdict and claims no new sanitizer/MIPS/device result.
