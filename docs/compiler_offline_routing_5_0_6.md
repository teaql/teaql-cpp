# Offline compiler source routing, Rust 5.0.6

## Source responsibility and implementation plan

The normal Spring host installs `CppCompilerGeneration` from trusted startup
properties through `CppCompilerHostConfiguration`. The default two-argument
`CppCoreGenerator.main` instead constructs an empty `DomainGenerationService`
after parsing the model. It never reads those settings. An otherwise identical
offline invocation therefore retains interpreted metadata even when the
generation process supplies the existing verified compiler capability.

1. Reproduce the default CLI with absent, complete and incomplete compiler
   properties using controlled tools. Capture actual source packages, output
   manifests, worker observations and terminal results. Preserve the exact
   preceding main/configuration sources.
2. Extract trusted property validation and service construction into one plain
   Java owner. Keep Spring property resolution and optional report assembly in
   its host adapter. Let the default offline entry resolve JVM properties, then
   the existing environment-variable spellings, and dispatch original source
   bytes through the same service. Unconfigured callers keep their existing
   parser/discovery behavior; partial or invalid supplied settings fail.
3. Preserve explicit compiled-model and qualification modes. They retain their
   existing capability/flag authority and do not bootstrap ambient worker
   settings. In particular, the compiler worker's explicit metadata invocation
   must not recursively start another compiler job. Keep generated-file
   publication and all public C++ APIs unchanged.
4. Run permanent CLI and shared-host regressions, including exact preceding-main
   failures. Execute a complete locked-original/C++ model through the actual
   configured offline entry and compare generated typed SQLite/Q/E and governed
   consumers. Preserve real original compiler refusals and no-fallback behavior.
5. Reproduce affected retained SDKs before updating producer identities; unchanged
   emitted bytes can reuse their explicitly bound prior compiler outcomes.
   Qualify exact source/receipt transitions in ordinary and optimized Python,
   run affected native checks, retain compact failure/source evidence and clean
   only invocation-owned outputs after children exit.

This is development-time generation infrastructure using existing dependencies.
It adds no C++ runtime dependency. Complete grammar/configuration routing,
unconfigured interpretation replacement, E03/E04 decisions, protected-consumer
composition and final G13/G14/G17/P0-P7 exits remain independent requirements.

## Confirmed CLI boundary and Java results

The [initial diagnosis](parity/compiler_offline_route_initial_5_0_6.json.gz)
compiles current original/C++ Java sources and executes the actual default CLI
three times. Absent, complete and partial compiler settings all exit zero and
produce the same twenty-six-file interpreted SDK. The complete controlled
worker is never reached. Both source/reference frames remain unchanged and the
owned directory is reclaimed.

`CppCompilerServiceConfiguration` now owns the existing property validation and
compiler-service construction. Spring delegates its original Environment and
lazy report-observer resolution to that owner. The default CLI resolves JVM
properties before environment variables and passes its original GenerateContext
to the service. Only `CppCoreGenerator.main` and its unused import change; all
entity/query/expression emission methods remain unchanged.

The [current Java run](parity/compiler_offline_route_units_5_0_6.json.gz) passes
all seven new CLI contracts and all 282 selected generator tests. The CLI tests
use actual isolated JVM processes and cover exact source bytes, environment/JVM
precedence, missing settings, invalid deadlines, compiler refusal, cleanup and
explicit-mode authority. The exact preceding main class, compiled separately
against the same classpath, fails precisely four new configured-default tests.
Unconfigured, qualification and explicit-metadata authority tests still pass.

## Retained generated artifacts

The [six-action SDK reproduction](parity/compiler_offline_route_sdks_5_0_6.json.gz)
freshly generates and independently checks all eighty retained SDKs. Every one
of their 3,560 code and Assist files remains identical. Each manifest updates
only its existing `CppCoreGenerator` source fingerprint. No generated source is
edited and no prior C++ compiler outcome is relabeled as new compilation.

The [complete default batch](parity/compiler_offline_route_default_sdks_5_0_6.json.gz)
freshly reproduces all 406 complete default SDKs from the new Java sources.
Its generation/check bodies and complete SDK batch are unchanged; only the
recorded preparation-step selection changes. Every file set and hash matches.
All 406 prior compiler outcomes are explicitly reused, with zero new C++ compiler
processes in that batch. Actual generated business execution and current-source
admission retain their separate gates.

## Actual offline compiler and SQLite consumers

The [complete-model execution](parity/compiler_offline_route_complete_5_0_6.json.gz)
invokes the actual default `CppCoreGenerator.main` with the shared trusted JVM
settings. It emits a complete compiler-backed SDK with explicit semantic-storage
provenance. The original model is generated and compiled independently; the
unchanged business observers consume the actual respective SDKs.

All forty-one recorded execution commands pass. Twelve Save pairs match across
single-entity and graph changes, whole-plan Policy denial/approval, transport
rollback, optimistic conflicts and committed audit-delivery failure. Typed
SQLite query and masked SQL results also match. The
[publication matrix](parity/compiler_offline_route_complete_sinks_5_0_6.json.gz)
matches all twelve ordinary/debug sink observations. Runtime/provider/Context
and graph/ledger implementation bodies remain unchanged.

The same offline entry then receives the exact retained Boolean-projection
negative model. The original compiler refuses it with E0277; the CLI propagates
`REFERENCE_COMPILER_REFUSAL`, exits nonzero and publishes no SDK. Source,
reference and native-artifact frames remain frozen and all owned service and
compiler jobs are absent after completion.

The CLI's default report observer discards the private worker journal after
existing cleanup. This receipt therefore retains the actual top-level CLI
command/outcome, complete SDK hashes/provenance, capability/tool/source inputs
and independent original/native business observations. It does not claim a
retained private worker journal or a new cross-runtime capability version.

## Source and result admission

The active source/receipt entry is:

```sh
source scripts/toolchains/activate.sh
python3 scripts/compiler_offline_source_admission_5_0_6.py --check
python3 -O scripts/compiler_offline_source_admission_5_0_6.py --check
```

The [source receipt](parity/compiler_offline_route_sources_5_0_6.json.gz) binds
five producer and three runtime source/SDK-guard images. An exact reverse of the
main-branch/import patch recovers the complete original `CppCoreGenerator`;
every emitter method remains unchanged. All eighty manifest transitions are
derived from actual regeneration and permit only the existing main-source hash.
Unknown source additions and unregistered identities remain rejected.

The [first compatibility cohort](parity/compiler_offline_route_validation_5_0_6.json.gz)
passes the new offline guard, then exposes recursion in the preceding dynamic
reader's `current_input_matches`. That helper calls its own complete evidence
validator when a known source hash changes; the validator asks the same helper
to admit that change again. The
[bounded continuation](parity/compiler_offline_route_reader_continuation_5_0_6.json.gz)
changes only this identity helper to use the new immutable source/SDK epoch
checks. Business validation remains at the explicit reader entry. It does not
re-enter itself while checking a compilation input.

The original failed CLI and source receipt remain immutable. All successful
Java, SDK, compiler, SQLite and publication execution is retained separately.
Only valid/invalid reader and imported-helper checks are repeated for this
identity repair; no business programs are rebuilt or rerun.

The [repaired cohort](parity/compiler_offline_route_validation_repaired_5_0_6.json.gz)
passes all twenty-four current/historical reader commands on frozen source,
native and reference inputs. Its twelve reports per Python mode reject 220
corruptions. The [imported-helper continuation](parity/compiler_offline_route_imported_helpers_repaired_5_0_6.json.gz)
adds five direct rejections per mode, including incorrect committed storage
even when both language observations are changed together: **225 per mode**.

The [selected native CTests](parity/compiler_offline_route_native_source_5_0_6.json.gz)
pass 2/2 with exact registry/JUnit membership. The subsequent identity repair
changes the new validator's own source after this execution. Its
[exact historical/current images](parity/compiler_offline_route_leaf_history_5_0_6.json.gz)
recover the original bytes, matching the actual native and failed-reader input
frames, and bind current bytes to the passing twenty-four-reader cohort.
The [ordinary/optimized metadata check](parity/compiler_offline_route_leaf_history_validation_5_0_6.json.gz)
accepts this join and rejects six corruptions per mode. No native execution is
repeated for this metadata repair.

The [retention check](parity/compiler_offline_route_retention_5_0_6.json.gz)
freshly hashes all eighty SDKs and 3,560 files, and preserves all 1,824 C++
source/fixture inputs. Ten exact owned paths are absent; ten distinct measured
totals reclaim 418,232,958 bytes. The private CLI worker journal is discarded,
so its unmeasured cleanup is excluded. Shared caches and stable preset outputs
remain, alongside compact source, command, outcome and failure evidence.

The [final development checks](parity/compiler_offline_route_development_5_0_6.json.gz)
apply/check clang-format 18.1.8 over 1,820 handwritten C++ files, then verify the
locked reference, active module paths, capability ledger and both repository
diffs. Their source/native/reference frames remain unchanged. The
[milestone](parity/compiler_offline_route_milestone_5_0_6.json.gz) binds all
retained reports and exact source transitions to the pushed producer checkpoint.
The complete goal remains active at fourteen qualified capabilities and
G13/G14/G17 partial; this configured-route checkpoint does not waive a P0-P7 exit.
