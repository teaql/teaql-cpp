# Readonly schemas on resolved streams

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: successful chunk mapping in `fetch_stream_with_source_internal`, its
public stream entry, and Context-owned asynchronous cleanup (RQ13, RQS01).

## Original source and counterexample

The complete original runtime executes 769 inputs through its public resolved
stream method. The first 193 observations retain the existing stream corpus
unchanged. The 576 additions cover absent/U64/Text definitions, U64/Text/typed
NULL/untyped NULL values, capture enabled/disabled, empty chunks and two-row
chunks, errors at the first or second chunk, and four consumption boundaries:
no polling, one pending poll, one delivered item, and complete consumption.

The C++ pre-repair probe differs on 80 inputs. All differences concern successful
row schema observations; provider requests, pending counts and close counts
match. The runtime was returning the provider's chunks without attaching the
query's readonly property definitions. Original observations are retained in
`parity/resolved_stream_schema_source_5_0_6.json.gz`, and the native pre-repair
comparison is `parity/resolved_stream_schema_before_5_0_6.json.gz`.

The observer appends a caller to the unchanged original resolved-service source
using `ReferenceRuntimeView`. Guarded adaptations extend the existing source
provider fixture and result observations; no runtime orchestration is copied or
replaced. The corpus does not turn a provider mock into physical database or
complete runtime acceptance.

## Runtime correction

`EntityStreamOperation` passes the prepared query's registered property
schema to `EntityStreamItems`. That wrapper retains an `ArenaBorrow` of the
schema in the executing Context, copies each successful provider chunk into
that Context as before, then delegates schema attachment to the existing
`CompactRow` helper. Empty chunks and provider item errors retain their original
behavior; an error item does not prevent a later successful chunk from carrying
its schema. No eager materialization, new ownership scope or shared lifetime
extension is introduced.

Readonly schema attachment is separate from model validation. The probe calls
validation explicitly so that a missing schema cannot silently resemble a
successful type check. The runtime does not introduce universal type rejection
into an untyped stream or the base Entity trait.

An initial implementation stored definitions as an inline value in the wrapper.
The existing checked row-layout borrow correctly rejected that unregistered
subobject with a type-identity error. The implementation now borrows the query
schema already registered in Context; the guard was preserved. The failed
native gate and source snapshot remain in
`parity/resolved_stream_schema_inline_schema_failure_5_0_6.json.gz`.

The earlier fixture-order mismatch remains separately recorded in
`parity/resolved_stream_schema_fixture_mismatch_5_0_6.json.gz`. C++ provider rows
were initially constructed in insertion order while the source fixture used
sorted maps. Constructing both fixtures from maps removes that fixture mismatch;
original expected data remains unchanged. Shape equality alone is not an
order-independent test for schema presence.

## Lifetime and regression checks

The native lifetime program passes with the default PMR resource disabled. Its
six boundaries are an unpolled stream, a pending stream, a delivered chunk,
explicit early cancellation, consumption across an error through EOF, and a
first-item error. The request and its local definitions are destroyed before
polling. Actual UserContext deletion closes the provider once and rejects the
ordinary stale handle. Explicit Global row copies remain valid after rebinding
through a new UserContext and retain readonly validation.

Focused ASan/UBSan passes 4/4, including existing SQLite stream cleanup. Leak
scanning stays disabled. Evidence:
`parity/resolved_stream_schema_sanitize_5_0_6.json.gz`.

The installed source/native/module gate passes all 769 observations per frontend,
with native 10/10 and module 8/8. Canonical module consumers and final no-work
builds pass. Its receipt is
`parity/resolved_stream_schema_installed_5_0_6.json.gz`. The gate includes the new corpus and lifetime program, retained stream controls,
SQLite stream shapes and inherited results, dynamic stream provider contracts,
previous nonstream schema/typed/lifetime regressions, canonical module consumers
and final no-work builds.

```bash
source scripts/toolchains/activate.sh
CARGO_INCREMENTAL=0 python3 scripts/verify_resolved_stream_schema_5_0_6.py \
    --record /tmp/resolved_stream_schema_fresh.json.gz \
    --cpp build_native_current/teaql_entity_stream_probe \
    --module build_modules_native/modules/teaql_module_entity_stream_probe \
    --check-gates --jobs 4
```

Joined supplemental acceptance restores G09 under its existing EX01-EX14
contract: `parity/resolved_query_schema_verdict_requalified_5_0_6.json.gz`.
`scripts/verify_query_schema_acceptance_5_0_6.py` checks that the only production
difference since the previous nonstream 45/18 checkpoint is the stream header,
that generator inputs and nonstream query bodies are unchanged, that the current
769 observations agree on both frontends, and that scoped sanitizer receipts
match their protected sources. The previous 45/18 selection and compiler/Java
proofs retain their historical scopes; they are not relabeled as fresh runs.
The earlier pre-verdict join is retained separately.

This continuation does not qualify all of `resolved.rs`, dynamic-aware/enhanced
typed compositions, full model assembly or P0-P7. These remain under the existing
cross-capability requirements. Source totals remain 84 accepted, 38 pending
and 2 excluded; capability totals return to 11 qualified and 6 partial. Generator,
provider, base-service and Context-service implementations are unchanged.

Verification removed 552,933,241 bytes of regenerable installer packages and
leftover failed Rust-link temporary objects after confirming no matching Cargo
or rustc process was running. Installed toolchain checks still pass; installed
compilers, source and active build outputs were preserved.
