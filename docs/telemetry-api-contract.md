# Runtime telemetry source contract

Baseline: Rust v5.0.5 commit `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
telemetry.rs (356 lines including its three source tests).

| Source responsibility | Native mapping and gate |
| --- | --- |
| RuntimeAttributeValue | Distinct String/Integer/Float/Boolean payloads, scoped string conversion/copy, source PartialEq and derived Debug. |
| From borrowed/owned string, i64, usize, bool | Destination-aware constructors preserve the value. The usize path uses the actual native pointer width; the existing u64 constructor remains an explicit extension. |
| RuntimeOperation new / public fields / attribute | Separate family/name/ordered attributes, seeded family/name attributes, exact nine-key filtering and replace-on-duplicate. Public attributes can override a seeded value without rewriting the family/name fields. |
| Model Clone/PartialEq/Debug | Destination copies, active-variant equality including NaN behavior, complete source field and variant diagnostics. No source Eq/Hash/serde derive is invented. |
| runtime_error_category | Ordered lowercase type-name categories; error messages are not inputs. Existing actual-call corpus and source port. |
| RuntimeTelemetryScope / RuntimeTelemetryPropagationContext | Separate activation interfaces with direct default callback invocation. |
| RuntimeTelemetry | Independent start/is_noop/extract/flush/shutdown responsibilities; default non-noop, empty propagation and no-op lifecycle hooks. |
| NoopRuntimeTelemetry | Context-owned equivalent of Default; no-op scope success/failure, true is_noop and default propagation. |
| extract_runtime_context / start_runtime_operation | Failure-isolated delegate creation and separate propagation/scope wrappers; provider exceptions do not replace business results. |
| FailOpen propagation run | Activate each actual future poll; preserve an existing result, propagate business panic before a result, otherwise poll without the broken adapter. |
| FailOpen scope run / success / failure / private finish | Same activation roles plus one consuming terminal action; poisoned business execution bypasses future activation/finish. Scope payload destruction remains distinct from arena release. |

Native Context access is owner-thread confined. The source mutex/Send/Sync and
Arc/Box responsibilities are mapped explicitly to serialized owner-thread
activation, checked domain borrows and ContextBox payload ownership. Native work
must stop/join before host-memory reclamation. This does not establish arbitrary
cross-thread access to User data or implicit lifetime extension through a shared
owner. Reentrant activation/finish is rejected. Existing 1,032 original API
observations and lifecycle tests cover activation, failures, poisoning,
propagation and destruction; broader runtime/save call sites remain open.

## Factory cleanup and unconstrained completion follow-up

Full trait/body review reproduces two native gaps. A factory-returned fresh
ContextBox in a foreign domain was ignored without releasing its concrete
payload; a controlled payload retains the caller's operation/carrier name and
fails the required live-input destruction assertion. The fixture resets its
failed negative control before User destruction. create_fail_open now resets
available foreign boxes immediately and returns the ordinary empty fail-open
wrapper. Already claimed boxes remain with their original owner; the helper
does not steal or destroy their payloads.

The original run methods constrain only Future, with no output Clone bound.
A direct original oracle compiles a non-Clone OwnedOutput and returns all eight
scope/propagation text inputs intact. A native compile check previously rejects
run with MoveOnlyError because TelemetryRun cloned its const completion.
The adapter now uses the existing copy_or_take_completed ownership helper:
copyable child results retain their borrows, while noncopyable completions move
exactly once into the wrapper. Both scope and propagation native tests verify
actual payload preservation, consumed-child rejection and one destruction before
arena reclamation. Neither change adds a production dependency or shared owner.

Fresh telemetry targets finish before three-check selections: ASan/UBSan passes
3/3 in 3.77 seconds with leak scanning disabled, and MIPS/QEMU passes 3/3 in
2.88 seconds. Both include the new foreign-box/claimed-owner/move-only lifecycle
checks plus all 1,032 activation and 110 model observations. The full
native consumer pass is still active and needs a final refresh for the changed
telemetry/schema implementation and telemetry template. The complete telemetry
source group and Stage 4 remain open until current integration and all source
contracts are accepted. Runtime/save telemetry call sites remain independent.

## Reproduced model capability gaps

The public runtime-facade compile fixture reproduces missing Debug for both
models. telemetry_debug.hpp restores source variant/field names, nested ordered
attributes and inherited formatter options, including floats. Returned output
and formatter temporaries use the caller-selected allocator. Diagnostic JSON
is only a fixture envelope, not new telemetry serialization or an exporter.

A separate MIPS compiler fixture rejects the std::size_t attribute constructor
as ambiguous among bool/double/signed/unsigned overloads. A constrained native
size_t constructor handles the 32-bit case by promotion into the existing
unsigned conversion. It is absent when size_t already aliases uint64_t, avoiding
duplicate overloads. The same cross-compiler fixture then passes. Source usize
casts into signed i64; native 32-bit maxima remain positive, while the existing
64-bit conversion preserves the source two's-complement cast. No constructor
silently narrows through a 32-bit count.

## Direct original-call evidence

The independent telemetry_model_scope oracle calls source conversions, Clone,
PartialEq and Debug directly. All 110 inputs match live Rust on standalone
native C++ in nine formatter modes. Inputs cover owned/Unicode/long strings,
signed extrema, common native usize values, booleans, signed zero, subnormal,
finite maximum, infinities and positive/negative NaN payloads. Keys include an
ordinary attribute, both seeded-operation overrides and two forbidden names.
The observation retains attribute/clone diagnostics and equality, plus complete
operation/clone diagnostics and equality. Filtering a NaN leaves operation
equality true while its independent attribute equality stays false, as in Rust.

All corpus usize values fit both native widths; the ownership unit separately
uses the actual platform's SIZE_MAX. The native contract passes with default
PMR disabled, then actually deletes its source User and formats a Global-retained
operation into a new User. It checks retained long text, negative-zero Debug,
forbidden-ID absence and output allocator. Earlier scope/payload/thread/stop/join
tests remain included. Public model compile diagnostics are restored.

Fresh named sanitizer/MIPS consumer builds include those ownership checks,
the existing activation/default corpus and the new model corpus. Terminal
selected results and the latest isolated full native consumer refresh/regression
remain required. The group and stages 2–6 remain open; this repair does not accept
all entity/save telemetry call sites, external adapters or complete runtime
composition.

The selected MIPS build finishes all twelve tasks and its final refresh reports
no work. All 311 compiled dependency records are nonempty; the three
ownership/activation/model checks pass 3/3 in 2.53 seconds. The sanitizer build
also finishes all twelve tasks and reports no work; all 103 compiled dependency
records are nonempty and the same checks pass 3/3 in 3.57 seconds with ASan/UBSan
and leak scanning disabled. Existing 1,032 activation observations and all 110
new model observations are included. Current isolated whole-native integration
remains pending; no complete source group or stage is accepted from selections.

Complete production review covers every item through line 302, before the
source test module. The native consumer refresh now finishes and a subsequent
build reports no work; the 318-check current regression is running. Selected
telemetry and source-body evidence remain valid, but final integration is still
required before formal source-group acceptance.

## Source-group acceptance, 2026-10-08

The current complete native suite passes 318/318 in 133.86 seconds after a
terminal no-work build. Fresh named source-test/query-composition targets finish
before six-check selections: ASan/UBSan passes 6/6 in 1.51 seconds with leak
scanning disabled; MIPS/QEMU passes 6/6 in 1.59 seconds. These include all three
original telemetry tests, fail-open activation/model/ownership contracts and
Context query call sites. The complete telemetry source group is accepted within
its documented owner-thread and Context lifetime adaptations. Earlier pending
paragraphs record intermediate evidence. Complete Context, graph-save call
sites, runtime privacy and Stage 4/5/6 acceptance remain separate gates.
