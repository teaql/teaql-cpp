# Complete runtime error source review: Rust 5.0.6

Status: all 132 lines of `teaql-runtime/src/error.rs` are accepted under
RE01-RE08 with explicit C++ ownership and formatter adaptations. Current native
**7/7** and imported modules **6/6** pass; each frontend matches **2,362**
observations across three overlapping families. Source dispositions advance to
**33 accepted / 89 pending / 2 excluded_external**. No C++ implementation or
existing contract body changes. G13/G17 and P0-P7 remain open.

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The generic historical observer constructs fourteen RuntimeError variants;
its new-baseline replay alone cannot establish direct construction/diagnostics
for the three additional variants. The new matrix below fills that named gap.

## Complete responsibilities

| ID | Source contract | Mapping and evidence boundary |
| --- | --- | --- |
| RE01 | All seventeen RuntimeError payload variants | Preserve plain text errors, typed SQL/intent/dynamic errors, CheckResult vectors, entity/relation and entity/id pairs, and the recursive boxed after-commit source. Separate C++ Kind and typed variant storage retain those distinctions; numeric enum order is not an external wire contract. |
| RE02 | Complete Display dispatch | All ordinary prefixes and relation/conflict formatting remain exact. SQL, request intent and dynamic-field errors delegate to their own complete formatters. Check failures join ordered finding Display values with `; `, including an empty vector. |
| RE03 | AuditAfterCommit's distinct public message and nested payload | Public Display always warns that commit already happened and does not interpolate the inner failure. Raw Debug retains the complete nested source. This error must not be retried as a rolled-back mutation. Value construction/diagnostics do not independently verify transaction commit/delivery behavior. |
| RE04 | Two explicit From conversions | RequestIntentError and SqlCompileError enter their exact RuntimeError variants. Other variant constructors do not become invented Rust From implementations. C++ factories select an explicit destination allocator. |
| RE05 | RuntimeError's Debug and Error protocol | Full tuple/structure field names and nested Debug remain distinct from public Display. The source uses the default empty Error::source chain, including AuditAfterCommit; the retained boxed payload is not an invented std-error source chain. Raw Debug is not a safe logging sink. |
| RE06 | All three ContextError variants | Missing named resource, missing static typed resource and missing entity data service retain distinct data, Display prefixes and Debug variants. C++ owns the diagnostic string in its selected Context rather than requiring a leaked/static application string. |
| RE07 | Generic DataServiceError and trait bounds | Runtime, Entity and concrete Executor carriers remain separate, and Display delegates to the actual inner type. Debug is available when the payload supports it; the Rust Error implementation requires an Error payload with static lifetime. The source imposes no Clone bound. A String Display fixture is not claimed to implement Rust's Error trait. |
| RE08 | Explicit ownership and current frontend integration | Runtime/Context errors copy nested strings/checks/boxed errors explicitly into a destination scope. A noncopyable executor error stays move-only and is destroyed once; it does not gain a fabricated cross-Context copy. The new imported observer reads nested Global runtime errors only after actual User destruction and checks their owner resources with default PMR allocation disabled. |

## New direct source coverage

The [54-case original/imported comparison](parity/runtime_error_extended_source_5_0_6.json.gz)
calls the actual frozen public constructors and methods. It covers:

- Query/Mutation RequestIntentError with comment, purpose, another field and an
  empty field, preserving the source's code selection even for publicly
  constructible combinations that normal request validation does not produce.
- Both ordinary and custom DynamicFieldError codes, empty/Unicode/NUL/long
  fields and the same typed RuntimeError carrier.
- Direct errors and one/three nested AuditAfterCommit layers, preserving all
  nine raw Debug modes, Display and empty Error::source semantics.
- Generic Runtime wrapping and a genuine non-Clone executor error, including
  exact destruction counts.

All **54/54** complete outputs match. This is direct public value/trait evidence,
not persistence, authorization, sink or commit-queue execution. The C++ observer
adds independent allocator/actual-User-deletion assertions without normalizing
the Rust outputs or inventing Rust Context metadata.

The C++ observer uses the existing canonical move-only error and Debug-mode
fixtures. They are prepared by the unchanged repository module converter with
a temporary in-memory selection; their entire bodies remain equal. Its own
small new program only constructs public error values, formats them, explicitly
copies destination data and checks native ownership. No runtime formatting or
error-selection algorithm is copied into the observer.

The original observer links the actual frozen runtime/core packages with exact
reference registry versions/checksums. Its fixture-owned executor error has
matching explicit Display/Debug and Drop behavior; it does not substitute for a
runtime error implementation. An initial ambiguous fixture formatter invocation
failed to compile and was corrected to explicit Display dispatch. That
[development harness failure](parity/runtime_error_source_initial_harness_5_0_6.json.gz)
is preserved separately from the successful original runtime comparison.

## Existing complete families and remaining acceptance

The unchanged runtime-check corpus has 1,134 observations: 1,014 CheckResult
cases, 112 ordinary runtime-error cases and four cases each for entity/executor
wrapping. The 1,174-case raw Debug corpus adds twelve ContextError cases and
twenty-eight EntityStatus cases to that broader input surface. These are
overlapping complete families, not 2,308 distinct runtime-error variants.

Their [current original/imported replay](parity/runtime_error_existing_replay_current_5_0_6.json.gz)
passes **1,134/1,134** and **1,174/1,174**, using completed current binaries and
reference-lock registry identities. No output normalization is introduced; the
existing native64-only location-width guard remains explicit. These two complete
families and the 54 new cases have separate scopes and receipts.
At the initial review, native check/error, generic completion and commit-audit
contracts belonged to the frozen full native gate. Source-disposition JSON and
module registrations remained frozen then. That epoch has ended; the separate
durable qualification below joins current source, native and module evidence.
The [earlier review](parity/runtime_error_review_before_acceptance_5_0_6.json.gz)
retains the exact provisional statements instead of relabeling their scope.

The existing recursive-error retention test in `commit_audit_tests.cpp` remains
unchanged. Earlier targeted sanitizer/MIPS and commit/delivery comparisons keep
their own scopes; this source-value review does not relabel them as new runs.
No new dependency, exporter, cross-Context shared owner or safe-log guarantee is
introduced. See the [current native epoch](native_integration_5_0_6.md).

## Durable source and frontend qualification

The [repository-relative Rust project](../tests/parity/rust_runtime_error_reference_5_0_6/Cargo.toml)
links the actual unchanged reference crates with an exact-reference-subset lock.
Its direct observer is byte-identical to the successful temporary observer; its
second entry reuses the complete historical check/error observer unchanged.
The [immutable direct corpus](../tests/parity/runtime_error_extended_5_0_6_cases.json.gz)
retains all 54 earlier inputs and original outputs. No temporary source path,
copied runtime enum or local formatting algorithm is needed for reproduction.

The new [canonical C++ observer](../tests/parity/runtime_error_source_probe.cpp)
preserves the temporary observer's entire body, changing only includes/imports.
The complete existing `runtime_check_tests.cpp` and `error_generic_tests.cpp`
bodies now also run permanently through imports. Their nested diagnostic copies,
long CheckResult retention, custom formatter, rejected foreign moves, move-only
Result nesting and exact destruction assertions remain unchanged. The module
converter's **916** canonical bodies pass its identity check.

The [execution receipt](parity/runtime_error_durable_execution_retry_5_0_6.json.gz)
replays actual Rust and both current C++ frontends for all three matrices:
54 direct cases, 1,134 check/error observations and 1,174 raw Debug observations.
These total 2,362 replay observations per frontend; the larger families overlap
and include adjacent check/status responsibilities. They are not 2,362 distinct
RuntimeError contracts. The native64 location-width guard remains explicit.

The source file itself has no upstream unit tests. Three **authored** tests of
the actual crate qualify construction without Display/Debug/Clone, Display
without Debug/Clone and owned non-Clone Error values. Four actual Rust compile
refusals check missing Display, missing Debug, String's lack of Error and a
nonstatic executor payload. These checks are not presented as unchanged upstream
unit tests. The C++ runtime/generic tests independently qualify conditional
Debug/copy availability and explicit custom formatting.

C++ has no Rust Display trait. Its default payload formatter uses `to_string`,
`what` or string conversion; applications can supply a borrowed formatter for
other payload types. That is an explicit language adaptation, not a claim that
every Rust-displayable type gets an automatic C++ formatter. C++ value `source()`
conveniences do not establish Rust's Error trait or an external standard-library
exception ABI for String or an arbitrary payload.

Native 7/7 additionally covers actual generic runtime completion. Imported 6/6
covers the canonical error bodies, commit-audit contract and all three comparison
entries. Both builds end with no work pending. Existing implementation, test,
fixture and historical-corpus bytes remain unchanged; the new observer's body
and the relocated Rust main retain their prior identities. The exact before
state is preserved in the [archive](parity/runtime_error_durable_before_5_0_6.json.gz).

The first durable driver passed source traits/comparisons and native 7/7, then
looked for its JUnit file relative to the repository even though CTest wrote it
relative to the build directory. Its [failed driver/receipt](parity/runtime_error_durable_driver_failure_5_0_6.json.gz)
is retained. Resolving the output path before passing it to CTest fixes the
development driver; the separate fresh retry passes every gate. The first
incomplete run is not relabeled as passing.

The [terminal acceptance](parity/runtime_error_source_acceptance_5_0_6.json.gz)
joins RE01-RE08 with those gates and immutable bodies. The native registry is now
739; this is not a complete 739-test run, and historical failed full epochs stay
failed. Governance, actual commit/delivery, safe sinks and wider model/Context
callers retain independent source and capability obligations. No generator/SDK,
runtime dependency, sanitizer, MIPS or physical-device change/run is claimed.

With the locked tools, reference checkout and existing Rust dependency cache:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --preset modules_native
python3 scripts/verify_runtime_error_source_5_0_6.py \
    --record /tmp/runtime_error_source_fresh.json.gz
```
