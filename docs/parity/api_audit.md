# Public API and behavioral completion audit

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Root export names and module/source-test ports are not sufficient evidence for
stage completion. The audit must account for every public field, enum variant,
method, trait default, constant, conversion, comparison/hash capability and
serialized representation in each mapped source, including supported external
reexports such as Decimal. Allocation-domain and native-borrow adaptations must
be explicit; they cannot delete a source responsibility.

## Final acceptance, 2026-10-10

All six migration stages are accepted within the documented C++ ownership,
owner-thread and supported-protocol adaptations. The source ledger is **55
accepted / 0 partial / 2 external exclusions / 1 reference-only helper**.

| Final profile | Complete CTest | Live original examples |
| --- | --- | --- |
| Native Debug | 388/388 | 2/2 |
| ASan/UBSan Debug | 388/388 | 2/2 |
| OpenWrt musl MIPS/QEMU, SQLite 3.53.2 | 387/387 | 2/2 |

All profiles have 427 nonempty object dependency records and no-work builds
before/after testing. Final sanitizer leak scanning is disabled. One raw FromSql
Debug comparison is native64-only; structured errors and provider contracts
remain covered on MIPS. Physical WNDR4300 execution is deferred by the user.

All 451 original source tests have reviewed dispositions: 441 covered, five
non-SQLite backend exclusions and five external exclusions. Complete leaf and
root responsibility reviews accompany the tests; names and counts alone are
not acceptance. Handwritten examples use the actual entity/query/graph runtime.
The generator is unchanged. C++20 standard facilities and SQLite remain the
production dependency scope; GPU adapters and external integrations are excluded.

See [the final requirement audit](../../measurements/parity/2026-10-10-stage6-final-review/README.md)
and [the migration plan](../migration_plan.md).

## Earlier checkpoints

The sections below describe their recorded source snapshots. Their open gates
and test totals are historical; the current verdict is stated above.

## Reference and language source acceptance, 2026-10-08

Complete reviewed reference and language/catalog groups are accepted after
stable native 322/322 in 119.11 seconds, a no-work build and 390 nonempty compiled
dependency records. Current reference/Context selections pass 21/21 on sanitizer
and MIPS; language/catalog selections pass 4/4. Live original comparisons retain
229 model/548 portable/933 round-trip reference observations and 4,575 language
calls plus sixty diagnostics. Exact source/platform/ownership boundaries remain
in their complete reviews. The ledger is **30 accepted / 25 partial**.

Entity-runtime structural diagnostics are installed, with six selected
sanitizer/MIPS checks passing and all 252 model observations matching live
original Rust on MIPS. Whole-native build/final corrected-probe refresh and
324-check regression remain pending. Runtime ID and cache/pagination model
repairs are staged separately; each passes native/sanitizer/MIPS live original
comparisons (82/82 and 74/74) and default-PMR-disabled lifetime contracts.
These repairs are not yet production/CMake or complete source acceptance.
Context, entity/service/cache/ID runtime, graph/governance/privacy and final
stages retain independent complete gates.

## Stage 4 source acceptance checkpoint, 2026-10-08

Complete in-memory engine, generated runtime support and telemetry source
contracts are accepted. Final native integration passes 318/318 after a no-work
build with 386 nonempty dependency records. Fresh sanitizer/MIPS selections
pass 3/3 for engine and purpose support and 6/6 for telemetry/source-test/query
composition. Live original comparisons retain 6,532 engine observations and
46 new purpose-wrapper diagnostics; earlier semantic corpora remain unchanged.

Full source responsibility/default/trait/protocol/ownership reviews are in
[the engine contract](../inmemory_engine_contract.md),
[the generated support contract](../generated_runtime_support.md) and
[the telemetry contract](../telemetry_api_contract.md). Evidence is retained in
[the measurements](../../measurements/parity/2026-10-08-stage4-runtime-review/README.md).
The ledger is **28 accepted / 27 partial**. Complete Context, service/reference
integration, graph/governance/privacy and final stages remain open; no physical
WNDR4300 execution is inferred from QEMU.

## SQLite provider acceptance and Stage 3 completion, 2026-10-08

The complete provider production body through line 1753 and all source public/
default/trait/protocol/ownership responsibilities are reviewed and accepted.
Current native 315/315, stable builds/source fingerprints, provider 32/32 sanitizer
and 31/31 same-engine MIPS evidence complete the eight Stage 3 source groups.
Live native/MIPS comparisons and exact engine/profile limits are retained in
[the provider contract](../sqlite_provider_api_contract.md) and its measurements.
The ledger is25 accepted / 30 partial. Stages 4–6 remain open; no runtime privacy,
graph/governance or physical-router acceptance is inferred.

## Complete SQL executor/transport acceptance, 2026-10-08

The complete executor/mutation-execution group is accepted after full production,
public/default/trait/protocol/ownership review and the executor-error Debug repair.
Current native integration passes 315/315 in 73.69 seconds with no pending work;
fifteen selected checks pass sanitizer/MIPS. Live source comparisons preserve
transport, query, mutation, transaction, stream, backend capture and error
protocol behavior. The ledger is 24 accepted / 31 partial. See
[the complete review](../sql_execution_source_test_acceptance.md) for responsibility
mapping and explicit ownership/scheduling adaptations. SQLite provider and
runtime/save/privacy/final stage acceptance remain open.

## SQL root facade acceptance, 2026-10-08

All 22 root exports have direct aliases with independently compiled type/helper
reachability; the root adds no default, protocol, state or ownership behavior.
All 29 source root tests have reviewed dispositions and the applicable compiler/
renderer contracts pass current native, sanitizer and MIPS selections after
builds. See [the complete root review](../sql_execution_source_test_acceptance.md).
The ledger is 23 accepted / 32 partial. Complete SQL executor/provider behavior
and stages 3–6 remain open; alias reachability is not their acceptance evidence.

## Data-service API source acceptance, 2026-10-08

The lib.rs source group is accepted after its complete 310-line production-body,
public/default/trait/diagnostic/ownership review. Current native integration is
314/314 with no pending build work; fresh named sanitizer/MIPS selections pass
9/9, plus the async lifetime check. Live original model, scoped Clone, inherited
observed-default and full-range SystemTime comparisons all match. The ledger
is 22 accepted / 33 partial. See [the API contract](../data_service_api_contract.md)
for exact ownership/scheduling adaptations and named evidence. SQL executor,
provider, runtime and privacy acceptance remain independent.

## Projection-state capability and identity repair, 2026-10-08

The native noncopyable fixture is now registered and passes all eight live
original observations. Guarded clone dispatch removes the accidental same-domain
payload-copy requirement. Explicit copies of copyable allocator-aware payloads
preserve a scalar origin identity while allocating payload and nested storage in
the destination; noncopyable retention rejects before publication. Bounded
non-reused arena namespaces combined with the existing object slot prevent
address-reuse equality, without retaining Context data or introducing shared
ownership. Comparison tokens are distinct from checked references.

Direct tests cover unsupported-copy rollback, same-domain borrows, cleanup after
actual User deletion, nested destination allocation, empty/type/identity behavior,
same-address arena reconstruction and quota-failure rollback. The initial four
selected checks pass native, ASan/UBSan with leak scanning disabled and MIPS/QEMU;
the 1,008 existing intent observations also match live original Rust. The final standalone ownership test additionally passes native, sanitizer with
leak scanning disabled and MIPS/QEMU, including private shutdown identity with
an exhausted public object quota. The MIPS test links without `libatomic` after
including the SDK's standard exception-support libraries. CMake memory selections
and full native consumers are being refreshed; their active build directories
must be incrementally refreshed after the late shutdown-header change before
running regression. The
303/303 result below describes the pre-repair checkpoint. The ledger remains
17 accepted / 38 partial; SQL-log and stages 3–6 are not yet accepted.

The final-header MIPS consumer selection now finishes and reports no pending
build work. All thirteen memory/projection/data-service/executor checks pass
in 2.71 seconds, including noncopyable state, retained metadata origin equality,
shutdown diagnostics, allocation quotas, and the 1,008 intent observations.
The sanitizer final-header refresh completes and reports no pending work. Its
same thirteen-check selection passes in 2.03 seconds with ASan/UBSan and leak
scanning disabled. The final MIPS projection probe matches eight live original
inputs. Whole-project native integration remains running; no new source-group
acceptance is inferred from these selections.

## SQL public-helper repair verified, 2026-10-08

A direct native reproduction narrows a Unicode Rust char delimiter to an invalid
single byte. The new char32_t overload encodes and escapes full scalar sequences
while retaining ASCII overload and wrapped/unquoted behavior. The source oracle
now calls actual public quoting and storage-containment helpers. All 262 inputs
are observed twice, preserving the initial 136 helper and all 1,305 existing
schema observations. No source algorithm is copied into the oracle.

Fresh three-check CMake selections pass native 3/3 (1.53 seconds), ASan/UBSan
with leak scanning disabled 3/3 (2.38 seconds), and MIPS/QEMU 3/3 (3.47 seconds).
Both native and MIPS match all 262 helpers and 1,305 schema inputs against live
original Rust. Ownership checks retain complete quoted text via the User Global
allocation proxy, actually delete User, and rebind the Global reference with
default PMR disabled. Complete consumer and dialect/source acceptance remain
open. The ongoing whole-project native build requires a post-terminal refresh
for the late public declaration and newly registered helper target; the expected
final registered suite is 306 checks, not the previous 303 or intermediate 305.

The initial 618-task native build stops at its SQLite error-model probe because
shared source edits introduced SqliteBoxedError overloads after the provider
library was compiled. The current source contains all three definitions. A full
incremental refresh begins only after that build is confirmed terminal, to
capture the provider edits, late arena/dialect headers and new helper target.
The failed link is not passing evidence or a complete native regression.

## Complete dialect-body follow-up and default corrections, 2026-10-08

Every production line before the test module (1–1522 of dialect.rs) is reviewed,
including each trait default and private helper. Direct mandatory provider hooks
reproduce 0/48 pre-repair native matches: final mutation SQL rendering occurs
before provenance capture, and soft-delete/recover assignment hooks occur before
table hooks. A private stack-local builder now preserves source field/provider
order, including final-rendering failure; no heap callback or layer merge is
introduced. Original 2,505 mutation observations remain unchanged.

Direct public compile_in further reproduces 120/216 pre-repair matches. Native
fallback silently accepts invalid operators or observes right-side errors where
the source panics after left evaluation. Corrected validation preserves early
left errors and partial bindings, then maps the unreachable precondition to a
C++ logic error. All 216 actual original observations are captured twice and
match live native/MIPS calls after repair; the earlier 262 helpers are unchanged.

Fresh six-check CMake selections pass native 6/6 in 7.04 seconds, ASan/UBSan
with leak scanning disabled 6/6 in 13.96 seconds, and MIPS/QEMU 6/6 in 10.26
seconds. The selection includes direct defaults, provider hooks, ownership/
override contracts and unchanged helper/schema/mutation corpora. Whole-project
native integration remains running and needs a post-terminal incremental refresh
for late code and the two new registered tests. The expected suite is now 308;
no full regression or source/stage acceptance is inferred from these selections.

## SQL binding/type production review and refreshed consumers, 2026-10-08

All 102 binding and 332 type source lines are reviewed, including private
provenance, finiteness, literal quoting, SQLite lexer, every model field and
all twelve active error payloads. Existing differential probes record callback
indices and failure boundaries; the source scope excludes non-SQLite rendering/
execution while retaining all DatabaseKind discriminators. Domain-aware copying
and rollback remain explicit C++ ownership/allocation adaptations.

Fresh 23-task sanitizer/MIPS selected builds finish before the five named model/
renderer checks. ASan/UBSan with leak scanning disabled passes 5/5 in 15.91
seconds; MIPS/QEMU passes 5/5 in 8.26 seconds. Complete native integration still
runs. The initial 495-task native refresh finishes successfully; its subsequent
refresh captures late defaults and shared provider/FromSqlError additions.
Current CTest registration is 311 after shared provider tests, replacing the
previous expected 308. No full 311 result or source/stage acceptance is claimed
before the final build/no-work/regression gates finish.

## Diagnostic guard/journal/stream body review

Complete diagnostic_execution.rs and the DiagnosticStream production body are
reviewed against C++ take-before-notify semantics, success-result ownership,
execution-order journal draining/disarm, cursor release before sinks, delivered-
only counts, terminal chunk/end/pending behavior, and isolated sink exceptions.
The source Arc/Mutex recorder is explicitly adapted to a Context-bound callback
and owner-thread vector; it cannot retain User data beyond Context close.
Existing direct source ports and noncopyable integration finish current eight-
task sanitizer/MIPS selected builds. Their four named checks pass 4/4 in 0.39
seconds under ASan/UBSan with leak scanning disabled and 4/4 in 0.77 seconds
on MIPS/QEMU. Native integration and runtime privacy acceptance remain separate.

Native final refresh stops at a provider driver-error test because late shared
edits add explicit SqliteNativeError overloads after the archive was compiled.
The current implementation contains both native_failure and sql_input overloads.
After confirming session 55322 terminal, a new full incremental refresh begins;
the failed link is not a full current-state regression result.

## Observed mutation backend snapshot repair

The production executor/mutation review adds actual mandatory kind() hook
observations: 44 cases, each observed twice, with all 1,012 prior execution
outputs unchanged. The pre-repair native result matches 19/44; repeated backend
lookup during observed metadata creation can fail before writes that the source
completes. The observed adapter now owns a Context-allocated backend snapshot,
with guarded capture before compilation and guarded/batch returned metadata
getter timing preserved. Both native and MIPS match all 44 live original inputs.

Fresh two-check selections pass native 2/2 in 0.81 seconds, ASan/UBSan with leak
scanning disabled 2/2 in 2.05 seconds, and MIPS/QEMU 2/2 in 1.16 seconds. After
rebuilding composed MIPS consumers, eight execution/diagnostic/transaction/
noncopyable checks pass 8/8 in 4.56 seconds. The current eleven-task sanitizer
consumer build also finishes and passes the same eight checks in 5.93 seconds
with ASan/UBSan and leak scanning disabled. The full native refresh remains running; the earlier full native 312/312 result in
73.53 seconds is explicitly a pre-repair checkpoint. No current-layout source
or stage acceptance is inferred from that older full result.

## Four Stage 3 source groups accepted, 2026-10-08

The final current native consumer build completes, reports no pending work,
and passes the entire CTest suite 314/314 in 74.66 seconds. The post-test build
also reports no work and registration remains 314. Complete public/default/
private-helper/protocol/ownership reviews and current named sanitizer/MIPS
checks accept the SQL-log, SQL binding/type, dialect and diagnostic source groups.
Their manifest records identify every source, review, named check and ownership
adaptation. The ledger is 21 accepted / 34 partial, plus two external exclusions
and one reference-only helper. Complete data-service/executor/provider, later
runtime/graph/privacy and target acceptance remain open; no whole-stage or
complete-goal acceptance is inferred from these four source contracts.

## Previous source acceptance checkpoint, 2026-10-08

The final current native consumer refresh finishes 452 tasks, reports no work,
verifies all 375 dependency records and passes full CTest 303/303 in 74.81
seconds. Complete Value/time/enabled-Decimal public/default/trait/protocol/
ownership review and current combined 21/21 plus scalar/memory 13/13 cross
selections now close that source group and Stage 2. The ledger has seventeen
accepted and 38 partial groups, plus two external exclusions and one reference-
only helper. Stages 3–6 and physical WNDR4300 execution remain open. The new
unregistered opaque projection fixture reproduces separate Stage 3 capability
and identity gaps and is not claimed as part of the green native suite.

Independent Stage 3 follow-up reviews all 318 production lines of sql_log.rs.
Eight original opaque-state inputs with a non-Clone payload retain identity,
typed lookup, private diagnostics and readability after dropping the first
wrapper; all 1,008 existing intent observations remain unchanged. The prepared
unregistered native counterpart fails because make/copy eagerly requires the
payload copy constructor, beyond the source contract. A separate scalar-state
Global copy retains value but changes equality. Both open gates are recorded
in data_service_api_contract.md. No production header changes occur during the
final native integration. Source-only fixtures do not accept the log group or
Stage 3, and their unregistered capability is outside the current green suite.

Further enabled Decimal default review reproduces absent Ord::min/max/clamp
entries. Inherent Decimal max keeps self on a numeric tie; inherited Ord max
selects the second operand. C++ now keeps those responsibilities separate as
min/max versus ord_min/ord_max/clamp, including complete raw representation
selection and reversed-bound failure without operand mutation. Six hundred
five direct original calls extend the prior 3,675 unchanged observations to
4,280, all matching live native Rust. Source observations are checked twice.
Named platform builds finish and the thirteen-check selections are running;
the full native build remains live and needs a final header refresh. No
complete source-group/stage acceptance is claimed from the repaired declarations.

Later Decimal follow-up: 238 pointer conversion inputs now have actual 64-bit
and actual 32-bit Rust observations, including original optional/TryFrom errors
and primitive construction. Native C++ and MIPS match their respective live
word widths exactly. A source const-fn compile fixture also reproduces missing
constant evaluation; the allocation-free construction/raw observations/bounds/
serialization entries are restored with unchanged runtime semantics. Both
original Rust word-width builds compile their constant assertions. Fresh Decimal
sanitizer/MIPS selections and a full 644-task native consumer build are running.
The preceding 302/302 remains the source-group acceptance checkpoint; it does
not establish integration of this later header change or completion of the
remaining Value/time/Decimal group. See decimal_api_contract.md.

The fresh Decimal/auxiliary selection then passes 13/13 under ASan/UBSan with
leak scanning disabled (64.18 seconds) and 13/13 on OpenWrt MIPS/QEMU
(35.42 seconds), including exact actual-word-width pointer comparison. Both
selected final build refreshes report no work; all 147 sanitizer and 324 MIPS
compiled dependency records are nonempty. Complete native consumers remain
live. No source-group or stage verdict is inferred from this narrow selection.

Complete source-body, public/trait/protocol/ownership reviews now accept
entity.rs, registry.rs and context/locking.rs. Their contracts retain separate
entity/error/store, decoder/metadata/behavior/module and local/remote locking
responsibilities. Full final native CTest passes 302/302 in 67.69 seconds with
a no-work build and complete nonempty dependency records. Registry composition
checks pass on sanitizer/MIPS, and fresh Entity/locking checks pass 11/11 on both.
Sanitizer leak scanning is disabled in these runs.

The MIPS audit runner repair preserves all 1,421 representable original outputs
and verifies six usize-width count rejections through native preflight and actual
Rust u32 parsing; it changes no recorded privacy output. See the three complete
source contracts for scope and failure/recovery evidence. Sixteen source groups
are accepted and 39 partial, with two external exclusions and one reference-only
helper. Complete stages 2–6 and physical WNDR4300 execution remain open.

## Registry module composition follow-up, 2026-10-08

Registry module composition follow-up: sixteen additional inputs invoke the
actual unchanged original multi-entity `module!` macro and compare ordered
replacement, mixed entity/behavior branches, trailing commas and independent
Clone/into_context snapshots. All 182 prior observations remain unchanged;
all 198 decoder/module inputs match live native Rust. The default-PMR-disabled
ownership contract retains a Global module after actual source User deletion
and runs its typed decoder, preserving distinct descriptor/type keys and
behavior snapshots. Both named checks pass under ASan/UBSan with leak scanning
disabled (0.81 seconds) and MIPS/QEMU (1.26 seconds). Source/stage acceptance
still requires the current whole-native regression and complete surrounding
composition review. See [the registry contract](../registry_api_contract.md).

## Typed standard-time error follow-up, 2026-10-08

The optional reversed-duration result is replaced at the original
`SystemTime::duration_since` entry with a scalar success/error carrier.
`SystemTimeError` preserves the complete positive opposite-direction duration,
Clone, exact message and default absent cause; `elapsed()` uses the same entry.
Typed Result/Error diagnostics and Duration precision, rounding, sign, padding
and Unicode units match 7,056 live original input pairs in twelve Debug modes.
The standalone native contract passes, including full reversed spans, safe
alternative access, formatter boundary cases and current-clock elapsed brackets.
No production dependency or model-generator change is introduced.

See [the detailed arithmetic/error contract](../system_time_arithmetic_contract.md).
Fresh sanitizer/MIPS consumer builds and a complete native regression remain
required after this return-type/header change. Earlier platform selections are
historical evidence. Thirteen source groups remain accepted and 42 partial;
stages 2–6 are still open.

The typed-error refresh subsequently passes all eight named checks on MIPS/QEMU
(41.86 seconds) and ASan/UBSan with leak scanning disabled (59.85 seconds).
Both selected builds finish and report no pending work; all 306 MIPS and 95
sanitizer compiled dependency records are nonempty. A separate core-only
compile fixture reproduces and repairs misplaced SystemTime Debug ownership.
The exact scalar writer moves from data-service into core without changing
metadata output; the expanded 7,056 input pairs still match live Rust.

Stable standard-type review then reproduces the missing associated UNIX_EPOCH
constant. Associated and module constexpr constants now compile with exact
epoch equality. This additional header edit has fresh platform consumer builds
running; the eight-check results above precede it. The stable surface table in
the detailed contract distinguishes the baseline from installed-source unstable
limits/saturating APIs. Whole native integration and stage acceptance stay open.

Post-epoch consumers now pass all eight selected checks on MIPS/QEMU (51.08
seconds) and ASan/UBSan with leak scanning disabled (101.62 seconds). Both final
selected builds report no work; all 306 MIPS and 95 sanitizer compiled dependency
records are nonempty. The native 484-task dependency recovery is complete,
followed by a running latest-header/new-target full refresh. Complete native
regression is still required. The complete original data-service SPI body is
rechecked against its distinct native interfaces and direct default forwarding;
reading those defaults does not accept cancellation/provider/runtime composition.

The independent executor_defaults_scope oracle now calls original inherited
query/stream/mutation/guard observed defaults through mandatory-only providers.
All 144 inputs match live native Rust/C++ dispatch timing, request diagnostics,
chunk size, polled/unpolled alternatives and untouched observer counters.
The named sanitizer and MIPS checks pass; see data_service_root_contract.md for
the fixture's error-payload and composition limits. This strengthens direct
default evidence without accepting the complete source group. Shared native
integration stops at a reference-probe linker failure with later-present
definitions and a recovered Ninja log; a fresh isolated native directory is
used for whole-project acceptance.

## Core field and trait-default checkpoint, 2026-10-08

Registry Debug repair: missing metadata/entity registry writers are now
implemented with the exact ordered source descriptor/name-map shapes. All
eighteen original Default/Clone/Debug/snapshot scripts match live native C++
in nine modes. Default-PMR-disabled native ownership checks pass actual User
deletion and Global diagnostic retention after deleting its formatting User.
The prepared probe is registered. After correcting a sanitizer-detected local
test-string lifetime error, rebuilt selected checks pass 3/3 under ASan/UBSan
with leak scanning disabled and 3/3 on MIPS/QEMU. The full native consumer
refresh remains running; the preceding 301/301 does not validate this
new header/target change. See registry_api_contract.md; source/stage acceptance
remains open.

Current coherent full native consumer checkpoint: build-root-integration
finishes 441 tasks and the final refresh reports no work. All 374 compiled
dependency records are nonempty and complete Debug CTest passes 301/301 in 79.05s.
This closes the earlier interrupted/stale consumer-build gate for the present
source state. Missing registry Debug and remaining requirement-by-requirement
API/default/composition/privacy gates are not closed by those results. Stage
2–6 completion and the overall goal remain unproven.

Current isolated native consumers pass the language/catalog ownership/translation/
diagnostic selection 3/3 and the separate locking/transaction/examples/original
SQL selection 8/8. The latter consists only of completed linked targets; no
additional build runs concurrently in that directory. All 166 currently compiled
dependency records are nonempty. The full build remains live, final dependencies
and current whole-native regression still need acceptance, and all source-group/
stage counts remain unchanged.

Registry/module body review: the complete 590-line source is traced through
separate metadata/entity/behavior/decoder/module interfaces and native ownership
tests. Four decoder modes, independent descriptor-name/type-name keys, snapshot
semantics, observer defaults, policy/before hooks and initial/root registrations
remain distinct. A public compile fixture independently reproduces missing
Debug for both InMemoryMetadataStore and InMemoryEntityRegistry; the native set
must still expose the source's ordered name-to-name map diagnostic. See
[the registry contract](../registry_api_contract.md) for capability/ownership
mapping and explicit open gates. Implementation headers remain unchanged while
the isolated full native integration build runs. The group remains partial.

Eighteen independent registry_debug_scope original-call scripts now preserve
source Default/Clone/Debug in nine modes, replacement/ordering and descriptor
snapshot output. All recorded observations are revalidated by a second live
original execution. No native implementation header changes occur; C++ writers,
probe and parity acceptance are still pending. This source-only corpus cannot
accept a source group or stage.

Telemetry model follow-up: public Debug for RuntimeAttributeValue/RuntimeOperation
is reproduced missing and restored. A distinct MIPS compiler fixture rejects
native std::size_t conversion as ambiguous; the constrained 32-bit constructor
restores source From<usize> and the same fixture then passes. All 110 original
conversion/Clone/PartialEq/Debug inputs in nine modes match live native Rust/C++.
The default-PMR-disabled native contract passes actual source User deletion and
Global retained-operation diagnostics. See
[the telemetry contract](../telemetry_api_contract.md) for exact role and scope
mapping. Named sanitizer/MIPS builds and latest isolated native integration
remain pending; no group or stage is accepted from these model checks.

The selected telemetry ownership/existing-activation/new-model consumers pass
MIPS/QEMU 3/3 (2.53 seconds) and ASan/UBSan with leak scanning disabled 3/3
(3.57 seconds). Final refreshes report no work; all 311 MIPS and 103 sanitizer
compiled dependency records are nonempty. This includes native SIZE_MAX and
actual User deletion. Complete isolated native integration, full runtime/save
call-site acceptance and stages 2–6 remain open.

Language/catalog follow-up: a public runtime-facade compile fixture reproduces
missing Language, BuiltinTranslator and I18nCatalog Debug. Source-shaped writers
restore the variant/translator/private locale/fallback diagnostics. Temporary
builtin grouping uses the explicit output allocator and immutable facts; generated
tables and production translation behavior are unchanged. Sixty original
Default/Debug/Clone inputs in nine modes match live Rust without normalization.
The default-PMR-disabled native ownership contract also passes after actual User
deletion and Global catalog retention. Complete 215/342-line source bodies and
native parse/lookup/render/title/default roles are reviewed in
[the language/catalog contract](../i18n_api_contract.md). Fresh named sanitizer/
MIPS builds and final isolated native consumer refresh remain required. The
source group remains partial; stages 2–6 stay open.

The language repair's selected consumers subsequently pass MIPS/QEMU 3/3 in
21.47 seconds and ASan/UBSan 3/3 in 37.31 seconds with leak scanning disabled.
Both final selected builds report no work; all 309 MIPS and 100 sanitizer
compiled dependency records are nonempty. The unchanged 4,575 translation
corpus, sixty diagnostic inputs and actual User-deletion/Global-retention
ownership checks are included. Complete isolated native refresh/regression
remains required before source-group or stage acceptance.

Complete time.rs review reproduces missing writable public Timestamp storage.
Public signed `millis` now preserves tuple-field mutable borrowing and all
consumers read the same scalar. Native ownership/contracts pass, all 108 live
original observations match, and both unchanged original time tests pass. The
previous integrated probe matches 99/108, failing precisely the nine new cases.

Enabled Decimal review additionally reproduces missing Inv and inherited
Zero/One set_zero/set_one/is_one methods. Inv delegates ONE / self; identity
defaults reset canonical sign/scale or compare numerically with ONE. Native
scalar/default contracts pass with default PMR disabled and all 3,675 live
original foundation observations match. Every earlier 3,495 complete observation
is checked unchanged; 90 reciprocal calls include sixteen source failures and
90 identity sequences exercise ordinary/extended scales and both signs. No
C++ runtime dependency or generator change is introduced.

The final seventeen-check entity/Timestamp/Decimal/locking/original-tail selection
passes 17/17 under ASan/UBSan with leak scanning disabled (11.12 seconds) and
17/17 on MIPS/QEMU (8.53 seconds). This includes inherited identity methods.
The long native build has completed; its final incremental consumer refresh
is running after the shared-header edits and new target registration. Complete
native regression remains pending before acceptance.

Context locking's entire source body, SPI/default/directory roles and two
downstream source tests are reviewed in context_locking_api_contract.md. Current
native named checks pass 2/2 and all 222 observations match live Rust. The
original selection passes both locking tests and one optimistic-conflict test.
Kernel layer verification confirms independent dependencies and no compiled
runtime SQLite C API calls. These are scoped results, not stage acceptance.

Counts remain thirteen accepted and 42 partial, plus two external exclusions
and one reference-only group. Entity/value/locking and stages 2–6 remain open.
Earlier 281/281 native results predate the current shared-header changes.

## Full-range time arithmetic follow-up, 2026-10-08

Current worktree metadata/log fields use the common core signed-seconds and
normalized-nanoseconds scalar; model/log diagnostics and unsigned elapsed
calculations use those parts directly. The representation gap is implemented
as a repair, with coherent consumer acceptance still pending.

A missing native subsecond assignment is reproduced in an existing logging
caller. Checked fractional addition/subtraction and source duration assignment
now preserve carries/borrows, full unsigned spans and unchanged destinations
on failure. Direct standard chrono units and clock points convert before native
nanosecond narrowing; seconds products are checked. All 1,232 actual Rust
checked/assignment observations match current native C++. Native and standalone
ASan/UBSan with leak scanning disabled contracts pass, including full-range
metadata retention through actual User deletion with default PMR disabled.

The [arithmetic contract](../system_time_arithmetic_contract.md) records exact
methods, input boundaries, source observations and pending gates. Seven selected
sanitizer/MIPS targets include these contracts, full-range SystemTime/model
checks and Context logging. Their named execution and the latest native
consumer refresh/regression remain pending. Earlier seventeen-check results
predate the layout repair and are not current whole-consumer evidence.

## Independent scoped model and time-domain evidence, 2026-10-08

The separate data_service facade is checked against all 29 original root names,
including directly declared models/SPIs. Forty independently named model_scope
inputs match live Rust on native and MIPS. Source/private-state/default/Clone
diagnostics and actual User-to-Global retention pass standalone native and
ASan/UBSan with leak scanning disabled. The two named scoped checks additionally
pass 2/2 under ASan/UBSan (7.48 seconds) and MIPS/QEMU (6.01 seconds).

The [root responsibility review](../data_service_root_contract.md) accounts for
the complete 393-line source, its observer defaults, distinct transaction/guard/
schema/ID interfaces and model fields. Full source SystemTime range is now
reproduced as a concrete gap: unchanged Rust metadata accepts +/-10 billion
seconds and both i64-second extremes, while the native metadata time_point has
an i64-nanosecond domain. Exact source observations are committed to the fixture
corpus; the existing separate wide scalar supports the full values. Shared
metadata/log layout, full-span elapsed arithmetic and common-layer ownership
still require repair, without narrowing the source contract or adding a
data-service dependency on runtime. Whole native integration remains pending.

## Schema-index derived capability repair, 2026-10-08

The dialect source exposes SchemaIndexSpec Debug/Clone/PartialEq/Eq independently
of DDL generation. A public-facade compile check reproduces its missing Debug;
the complete four-field writer is added. All existing 1,305 schema inputs and
previous expected fields remain unchanged. Actual original derives add 5,319
diagnostic outputs and Clone equality observations; the standalone native probe
matches every expanded live input. Native ordered-column/equality/domain and
actual User-deletion retention checks pass, and named native integration passes
2/2. Selected ASan/UBSan with leak detection and MIPS integration also pass 2/2;
the MIPS probe matches all 1,305 live original inputs. The
[dialect responsibility mapping](../sql_dialect_api_contract.md) keeps all seven
helpers, forty-three trait methods and remaining source-body/default/override/
composition gates open; a derived model repair does not accept that whole group.

## Full SystemTime representation repair, 2026-10-08

The original Rust accepts times outside the earlier metadata chrono range,
including 10,000,000,000 seconds and the minimum signed Unix second. The existing
cursor carrier is moved to core and shared by metadata, logs and pagination;
the runtime header remains an include adapter. Current time reads, checked
second arithmetic, full unsigned duration subtraction and checked chrono
narrowing retain their separate responsibilities. Metadata/log diagnostics use
stored seconds/nanoseconds, and runtime elapsed calculation no longer narrows.

All 254 full-range original time/metadata inputs match the independent native
probe. Its standalone ASan/UBSan contract passes with leak detection, including
both chrono endpoints and actual User deletion. Context logging preserves all
315 previous inputs/outputs and appends twelve full-range inputs (327 total).
The original logging implementation is unchanged in the oracle.

The fresh native metadata/log/pagination consumer build finishes all 78 tasks,
and the complete nine-check selection passes 9/9 in 20.58 seconds. Live native
probes match all 254 time and 327 Context logging inputs. Sanitizer/MIPS builds
finish successfully and each passes the same nine checks: sanitizer with leak
detection 9/9 in 26.79 seconds and MIPS 9/9 in 121.86 seconds. Live MIPS
time/log comparison output is checked separately. Current full regression
remains required.
The earlier full native build finishes its old 569-task plan; a separate current
599-task consumer refresh is observed live. Final refresh after the new metadata
layout and complete current CTest remain required. The
[time contract](../system_time_contract.md) records these evidence limits.
No source group or stage is accepted from the standalone checks.

## Data-service model diagnostics and lifecycle review, 2026-10-08

The complete pinned data-service production body and C++ model/SPI headers are
reviewed. Missing derived model diagnostics are reproduced and repaired in the
new model_debug.hpp header, preserving every field, recursive statement/mutation,
operation/capability variant and private log-state boundary. Forty original
inputs produce 11,520 complete Debug outputs and actual default/Clone/comment/
trace observations. Native/MIPS match all forty inputs against live Rust.

Native, ASan/UBSan with leak detection and MIPS selections pass 3/3 each,
including existing executor defaults, no-I/O intent and transaction/cancellation
contracts. Explicit Global copies of all scoped model types survive actual User
deletion; checked metadata references reject close, every source block returns,
and nested allocation domains are verified. The new checks add no runtime
dependency. The newly present data_service.hpp facade is retained and exposes
these diagnostics without changing executor responsibilities.

The [complete responsibility review](../data_service_api_contract.md) explicitly
leaves the full original SystemTime representation and generic async/descriptor/
transaction/runtime composition gates open. The tested i64-nanosecond domain is
not the entire Rust platform SystemTime range. Whole-project integration and
stages 2–6 remain open; the group is still partial.

## SQL model API and lifetime repairs, 2026-10-08

The [SQL model source review](../sql_model_api_contract.md) repairs three
reproduced missing capabilities: compile-error default source, SqlBindings Debug
and CompiledQuery Debug. Complete nested log-state diagnostics preserve the
private projection placeholder and redaction count. Error equality now compares
only active variant payloads. Consuming binding inputs transfer same-owner
buffers and explicitly copy foreign data into the destination; allocation
failures restore policy/value alignment without consuming a foreign source.

Forty actual original inputs match native and MIPS, recording 10,710 raw Debug
outputs and 5,760 error comparisons. Source/default/Clone/cause/provenance and
ownership checks pass 2/2 on native, ASan/UBSan with leak detection and MIPS.
The unchanged 1,538 literal/rendering/binding inputs also match live Rust on
native/MIPS. Native dialect scope/failure checks pass; selected sanitizer and
MIPS rendering/dialect checks pass 2/2 each. Final combined selections pass 4/4
on native, ASan/UBSan with leak detection and MIPS, in independent directories
to avoid shared-output races. Whole-project
consumers, complete composition and stage acceptance remain open; the source
group stays partial. No production dependency or generator change is introduced.

## Resumed lifecycle and Entity audit, 2026-10-08

Existing modifications are preserved. A later process audit corrects the initial
missing-handle observation: earlier native and sanitizer Ninja builds were still
running in the same directories. The newly started duplicate processes were
stopped, and the earlier builds were retained. Missing handles alone do not prove
termination. Both exact Arc source bodies and their C++ role/identity/reclamation
adaptations are re-reviewed. Rust passes both tests individually. Five selected
native, ASan/UBSan with leak detection and MIPS tests pass 5/5; current leak
scanning is available after the environment change. Historical disabled-scan
results retain their original limits. The adaptation replaces last-Arc-drop
cleanup with Context close and does not remove weak publication authority,
clone promotion, frozen graph identity or explicit Global-copy requirements.

The complete entity.rs production body and C++ trait/default/boxed/registration/
parsing implementation are reviewed. EntityError's previously missing default
source() is present; its nine original Error observations expand default/dynamic
sequences to 102. Fresh Entity public/ownership/diagnostic integration is running.
Production Entity and runtime/stage gates remain open until the complete evidence
is verified. Current working-tree module counts are thirteen accepted and
42 partial, plus two external exclusions and one reference-only helper.

## Decimal error responsibility repair, 2026-10-08

The enabled locked dependency Error body reveals two gaps: missing default
`source()` and ErrorString Display padding/precision. The first is reproduced
by a failing compile check and repaired with nonthrowing nullptr. Explicit-domain
`format` preserves Unicode character counting, outer padding and the distinction
between ErrorString's `Formatter::pad` and other variants' flag-independent
`write!` output. See [the supported Decimal contract](../decimal_api_contract.md).
The 94 original auxiliary inputs keep every earlier expected field unchanged;
new live observations add six causes, six Display/Clone results and 54 formatted
results per input. Native ownership checks retain all six variants into Global
across actual User destruction with default PMR disabled.

Entity's selected native and MIPS checks pass 10/10; its resumed ASan/UBSan run
with leak detection also passes 10/10 in 5.00 seconds. Subsequent Decimal changes
require current consumer integration. Updated Decimal/auxiliary selections pass
12/12 on ASan/UBSan with leak detection (241.88 seconds) and MIPS (48.49 seconds).
The standalone native contract passes, and native/MIPS probes match all 94 live
original inputs. Stale-archive failures during the first builds are superseded
by these completed incremental builds; those failed builds are not passing
evidence. The original full native plan was interrupted after final edits;
its terminal process was confirmed before starting a 592-task refresh with eight
jobs. A later audit finds concurrent Timestamp public-field changes and a newer
569-task build writing the same native directory. This turn's duplicate native
build is stopped and the newer build is retained. The Decimal selected results
precede that later change. Full current native CTest and stage acceptance remain
open; missing handles or old passing binaries cannot establish final integration.


## Entity error source default repair, 2026-10-08

The complete pinned entity.rs body and C++ entity/default/wrapper/parse bodies
are reviewed. The reproduced missing EntityError source() default is repaired:
nullptr/noexcept preserves original None without a fabricated cause or lifetime.
Nine actual source Error observations retain all previous 93 sequences, for
102 inputs, including raw fields/Display/Debug/Clone and long Unicode/NUL data.
The failed compile fixture now passes; native ownership checks add actual User
deletion and explicit retained Global error rebinding. Full native and selected
sanitizer/MIPS consumer builds are running. Previous 281/281 regression predates
this header change, so the entity group remains partial. Thirteen groups remain
accepted and 42 partial; stages 2–6 stay open.

## Complete annotated input graph accepted, 2026-10-08

The full core entity_graph.rs responsibility group is accepted on its complete
source/default/trait/ownership review. Five node fields, both operations and
all builders preserve descriptor-name provenance, optional comments and ordered
recursive children. All 240 original inputs match live Rust on native/MIPS;
current native, ASan/UBSan and MIPS four-check selections each pass 4/4, and
unchanged source unit tests pass 1/1. The current native 281/281 regression
predates no C++ edit; Ninja is clean. See
[the full source verdict](../core_graph_api_acceptance.md#complete-source-group-verdict-2026-10-08).

Thirteen groups are accepted and 42 remain partial, plus two external exclusions
and one reference-only helper. The next entity.rs review has identified its
EntityError default source() for verification; runtime graph identity/save,
governance/privacy and all stages 2–6 remain open.

## Complete query/row group accepted, 2026-10-08

The entire pinned query.rs responsibility group is accepted: fourteen structs,
two enums, all 105 public methods, every source field/default/trait, helper
validation/ordering, private list recursion, CompactRow/layout and JSON/map
boundaries. See [the complete source review](../query_source_acceptance.md).
Direct method observations expand query/Expr to 1,089 and rows to 401 while
preserving all previous 1,022/356 inputs. Named native/MIPS probes match both
corpora and all 169 row/list and 120 query/Trace Debug inputs against live Rust.
Six unchanged original query unit tests pass 6/6.

Ten native, ASan/UBSan and MIPS selected checks pass 10/10. Current sanitizer
runs disable leak scanning under tracing and retain explicit block-reclamation
assertions. The final full native suite passes 281/281 in 72.92 seconds and
Ninja is clean. Twelve groups are accepted and 43 remain partial, plus two
external exclusions and one reference-only helper. This closes models only;
SQL/runtime cache/pagination/graph/governance/privacy and stages 2–6 stay open.

## Complete expression group accepted, 2026-10-08

The complete pinned expr.rs/safe_expression.rs responsibility group is accepted:
all source variant fields, fifty Expr methods, twenty-four SafeExpression
methods, constants, defaults, derived traits, stronger Clone blocks, exact
logical/subquery behavior and public TeaqlEmpty extension. See the
[complete source contract](../expression_source_acceptance.md). Native/MIPS
match all 1,022 query/Expr, 448 SafeExpression and 120 query/Trace Debug inputs
against live original Rust. Native, ASan/UBSan and MIPS selected checks each
pass 12/12; leak scanning is unavailable under tracing. The final native
consumer refresh is clean and full regression passes 281/281 in 62.11 seconds.

Eleven groups are accepted and 44 remain partial, with two external exclusions
and one reference-only helper. The newly reviewed query.rs group remains
partial: SelectQuery, its helper types, CompactRow/layout and JSON projection
retain their independent acceptance gates. All stages 2–6 remain open. Earlier
expression checkpoints below describe their own verification state.

## Expression streaming and Clone boundaries, 2026-10-08

The complete [two-source review](../expression_source_acceptance.md) now records
all fifty Expr methods, variant fields/traits, twenty-four SafeExpression methods,
source Clone blocks and the public TeaqlEmpty extension point. Four custom
predicate inputs preserve the previous 444 cases, for 448; native/MIPS match
all 448 live original observations. Final ASan/UBSan and MIPS/QEMU selections
pass 12/12 in 14.24/11.53 seconds, including the corrected fixture teardown.
The sanitizer fixture failure is closed by that fresh passing contract, with
leak scanning still disabled under tracing. Full native refresh/regression
remains running and the source group remains partial.


The named MIPS expression selection passes 12/12 in 11.56 seconds and all
1,022 query/Expr, 444 SafeExpression and 120 query/Trace Debug inputs match
live Rust. A review then catches a teardown error in the new budget fixture:
local owning expressions outlive its manual User deletion. The old sanitizer
binary independently reproduces the resulting destructor use-after-free. The
fixture now destroys local expressions before deleting User. Its rebuilt named
MIPS query API check and fresh standalone native contract pass. Final sanitizer
and full native integration remain running. This failure is not counted as a
successful sanitizer result.


A further full-factory review reproduces and repairs generic BETWEEN input
support: scalar bounds failed to compile although the original accepts
Into<Value>. Forty-two additional original-call inputs preserve the earlier 980
cases, totaling 1,022; their 3,444 complete Expr states cover all 50 public
factories, binary operators and subquery guards. The fresh standalone native
probe matches every input against live original Rust. The standalone native
contract passes the bounded logical-stream, owned duplicate consumption,
scalar/string BETWEEN and conversion-order/failure checks. Named/full consumer
integration is still rebuilding; the expression group remains partial.


The full pinned expr.rs/SafeExpression production bodies are reviewed. Two
concrete repairs are in integration: logical range construction now deduplicates
each yielded operand before retaining it, and identity/borrowed-root/entity/list
SafeExpression adapters declare their source Clone boundaries. Before the
repairs, a 1,000-duplicate 8 KiB operand stream exhausted a 64 KiB User budget,
and two independent negative identity/borrowed-constructor assertions failed.
The repaired compile assertions pass; source SafeExpression tests pass 4/4.
See [logical construction](../query_api_acceptance.md#incremental-logical-construction-repair-2026-10-08)
and [Clone capability review](../safe_expression_api_acceptance.md#source-clone-capability-boundaries-2026-10-08).

Full native and selected sanitizer/MIPS rebuilds are running. The previous
281/281 full suite predates these header repairs. Ten groups remain accepted
and 45 partial; complete expression-group and stage acceptance remains open.

## Complete mutation/trace model acceptance, 2026-10-08

The complete pinned mutation.rs/trace.rs production bodies are reviewed and
accepted: all three distinct map roles, six commands, two enum kinds, TraceNode,
every source field/default/method and trait. The review adds actual source
builder and delegated-map calls instead of inferring their behavior from direct
field assignment. All 1,688 API inputs and both unchanged 120-input Debug
corpora match live original Rust on native/MIPS. Explicit-scoped-Clone and
builder ownership checks pass with default PMR disabled and actual User deletion.
See [the complete contract](../mutation_model_api_acceptance.md#complete-source-responsibility-review-2026-10-08).

Eight selected checks pass on native, ASan/UBSan and MIPS/QEMU. Current leak
scanning is unavailable under sandbox tracing; explicit arena-reclamation
assertions remain active. The final full native suite passes 281/281 in 74.57
seconds, with no pending build work. Ten groups are accepted and 45 remain
partial, plus two external exclusions and one reference-only helper. Model
acceptance leaves mutation execution/governance/audit/log/privacy and stages
2–6 open. The following sections retain earlier scoped checkpoints.

## SmartList generic execution and Boolean repair, 2026-10-08

The complete two-file request/dynamic-search responsibility group is now accepted
after the final forwarded-receiver repair and complete public/default/trait/
conversion/protocol/privacy/ownership review. All 174 request/509 search/872 helper
observations match live original Rust on native/MIPS. Six final native, MIPS and
ASan/UBSan selections pass, with current leak scanning explicitly unavailable.
The final complete native rebuild/regression passes 281/281 in 70.86 seconds
and Ninja has no pending work. Nine groups are accepted and 46 remain partial,
plus two external exclusions and one reference-only helper. See
[the full source-group verdict](../request_api_acceptance.md#complete-requestsearch-responsibility-group-acceptance-2026-10-08).
Downstream query/runtime/save/log/privacy and full stages remain separate.

The remaining generic request audit reproduces an EntityReference receiver-category
mismatch: the concept tested a moved receiver while the adapter forwarded its
actual argument. The constraint now follows that exact forwarding, preserving
custom no-Clone owned and explicitly supported borrowed receivers. Positive/
negative compile assertions and a scoped 4 KiB ID transfer check pass within
six native (8.78 seconds) and six MIPS/QEMU (10.35 seconds) selections. The
unchanged original dynamic-search integration file passes 8/8. Refreshed
ASan/UBSan without leak scanning passes 6/6 in 17.73 seconds. The full native
consumer build/regression is running; the preceding
complete native result predates this last public declaration change. See
[the generic boundary review](../request_api_acceptance.md#entityreference-receiver-category-repair-2026-10-08).

The final complete native consumer/probe refresh passes 281/281 in 69.60
seconds, with no pending Ninja work. The complete list.rs responsibility group
is accepted on its explicit 44-method/field/trait/default/protocol/ownership
review, repaired negative capability declarations and current integration.
Eight groups are accepted and 47 remain partial, with two external exclusions
and one reference-only helper. The earlier 280/281 run failed only the known
stale search fixture; the two-task refresh closes that mismatch. Current leak
scanning remains unavailable, explicitly distinguished from successful ASan/
UBSan and arena-reclamation evidence. See
[the list acceptance](../smart_list_api_acceptance.md#complete-list-responsibility-group-acceptance-2026-10-08).
Request/search field probes match all 174/509 live original inputs on native and
MIPS; that field-fixture gate is closed while complete group acceptance remains
separate.

Independent request equality evidence now covers all 43 fields in eleven structs
plus unchanged clones: 54 new observations preserve all 120 earlier model
inputs, giving 174 cases. A fresh standalone native probe matches live original
Rust; named ASan/UBSan without leak scanning passes 2/2 (32.31 seconds) and
MIPS/QEMU passes 2/2 (29.15 seconds). Fourteen further search observations cover
all ten equality fields across warning/filter/order/error and actual warning
serde JSON. The search corpus retains all earlier cases and now has 509 inputs;
standalone native matches live Rust, and two selected sanitizer/MIPS checks pass.
The native full regression is running with a known search-probe fixture refresh
required afterward because that object predates the last test-only edit. No
public header was modified during that build. Whole source/module/stage acceptance
remains separate from the new field observations.

The new whole request/dynamic-search production review also reproduces and
repairs the missing default DynamicSearchError source() capability. Four actual
source Error observations add exact Display/Debug/Clone equality/default cause
to the preserved 491-input search corpus. Eight request/search/helper/diagnostic
checks pass on native (28.66 seconds) and MIPS/QEMU (34.56 seconds); native live
Rust comparisons match 495 search, 120 request API and 872 helper inputs. The
combined sanitizer selection also contains the pending SmartList trait gates.
Its initial 14-check run failed on LeakSanitizer's fatal ptrace/tracing
incompatibility in the restricted environment. A direct helper run completed
872 observations before the same shutdown error. ASan/UBSan is rerunning with
leak scanning disabled and passes all 14 checks in 175.21 seconds; this is not
a passing leak check. Native/MIPS live comparisons match all 495 search, 120
request API and 872 helper observations. The complete native
consumer build/regression remains running. Old tool
handles disappeared after the environment transition, and a process check found
no surviving jobs before resuming builds. These records do not accept the
request/search group without its remaining detailed gates. See
[the whole-source review](../request_api_acceptance.md#complete-requestsearch-source-review-in-progress-2026-10-08).

The complete source-only business_clock integration file is accepted after a
full two-test input/assertion review and fresh unchanged-source Rust 2/2,
native compilation/CTest success, ASan/UBSan with leak detection and MIPS/QEMU
named-check success. Every clock/resource allocation remains explicitly User
scoped with default PMR disabled. This accepts the test-file responsibility
group, not the production clock/runtime module. Seven groups are accepted and
48 remain partial, plus two external exclusions and one reference-only helper.
See [the source-test review](../provider_sql_source_test_acceptance.md#runtime-business-clock-and-permutation-tests).

The complete list source review accounts for all 44 public methods, six fields,
three derives and twelve explicit trait implementations. A negative compiler
probe nevertheless shows that list equality is advertised for elements without
equality; a separately compiled header-copy prototype fixes that capability
boundary and preserves equality-only/bool/float/Value declarations. Production
headers remained stable until the full consumer regression passed 281/281 in
87.32 seconds. Production equality constraints and conditional Clone assertions
were then applied. The latter exposed a false move-only Clone capability through
borrowed-span conversion; the span constructor now follows element domain-copy
capability. After a reproduced native assertion failure and deliberate affected
target-build interruption, corrected native/sanitizer/MIPS selections are
building. These trait gates still require terminal production integration.

The subsequent ownership/diagnostic review repairs unvisited-object cleanup on
consuming map/entity conversion failure and default f32 diagnostics that had
widened to f64. The new corpus has 1,291 inputs: six owned mapper traces, 24
loaded/unloaded entity projection traces and 538 original f32 bit patterns in
nine derived Debug modes supplement the earlier complete states. Eight selected
native checks pass 8/8 in 18.73 seconds; the rebuilt MIPS/QEMU selection passes
8/8 in 37.32 seconds. All 1,291 native and MIPS observations match live original
Rust. The older double Value/format checks pass unchanged. The refreshed
ASan/UBSan selection with leak detection passes 8/8 in 74.90 seconds; the
restarted complete native consumer build/regression is running.
The Boolean full rebuild was explicitly interrupted for these reproduced
repairs; no old running statement is a terminal completion claim. See
[the consumed ownership review](../smart_list_api_acceptance.md#consumed-ownership-and-f32-diagnostics-2026-10-08).

The continued source review repairs owned extension failure prefixes and filter
unwinding, including release before the next callback and preservation of the
untouched tail. The intermediate 595-case selection passes six checks on native,
ASan/UBSan with leak detection and MIPS/QEMU. Independent field/nested/NaN equality
and generic callback failures supplement the earlier iterator observations.

A compile reproduction then exposed the packed `std::vector<bool>` limitation.
Scoped typed Boolean storage now restores actual pointer/slice borrows, copying,
storage transfer and default Debug without a dependency. All 128 new original
Boolean sequences retain construction/metadata/mutation/mapping/grouping and
consumption behavior. The corpus has 723 inputs; every prior 528 input and
expectation is preserved. Six checks pass 6/6 on native, ASan/UBSan with leak
detection and MIPS/QEMU. All 723 native and MIPS observations match the live original
oracle; the complete native consumer rebuild/regression is running.
The earlier full rebuild was explicitly interrupted for this repair batch.
The list group remains partial pending terminal integration and full-surface
acceptance. See [the detailed source review](../smart_list_api_acceptance.md#iterator-callback-and-boolean-storage-review-2026-10-07).

## Core metadata/naming responsibility-group acceptance, 2026-10-07

The complete two-file source surface is accepted in
[metadata acceptance](../meta_api_acceptance.md#complete-source-surface-acceptance-2026-10-07).
All three distinct models, fields, constructors, builders, optional first-match
lookups, writable iteration, scoped Clone, structural equality and raw Debug are
reviewed. The refreshed corpus adds independent field/nested/order equality and
ASCII/Unicode/NUL naming boundaries; all 344 inputs match a freshly rebuilt local
original Rust oracle on native and MIPS. The two named checks pass native 2/2,
ASan/UBSan with leak detection 2/2 and MIPS/QEMU 2/2. The unchanged source's three
builder tests pass 3/3. Ownership checks verify complete long metadata after
actual source User deletion and self-aliased mask replacement. Six groups are
accepted and 49 remain partial. Runtime metadata/schema/privacy and full stages
remain separate; the latest complete native regression remains 281/281.

## Complete Python probe emulator registration, 2026-10-07

Sixty-six further Python CTest registrations now forward the configured emulator.
The isolated log-manager, three-probe log-formatter and dynamic-search comparisons
accept it explicitly; all other affected scripts use the existing shared parser.
Generated MIPS registrations account for all 110 Python C++ probe entries with
`/usr/bin/qemu-mips;-cpu;74Kf`; an empty native prefix preserves direct execution.
Only C++ is emulated; native Rust oracle invocation remains unchanged.

The rebuilt eight-check MIPS smoke selection passes 8/8 in 11.55 seconds.
The initial selection reached seven missing probe binaries, which were then built;
the observer now diagnoses a missing executable before invoking QEMU. The later
formatter run exposed six decimal counts outside 32-bit usize and one text-NaN
payload difference. Exact size_t preflight rejects all six, matching actual Rust
u32 parsing in the host oracle; this is input-boundary evidence, not Rust MIPS
execution. All 921 representable formatter outputs match live original Rust.
The NaN fixture now supplies exact f64 bits; its existing expected output is
unchanged, and production EntityKey still preserves raw bits. No output strings
or privacy fields are rewritten to make comparisons pass.

The final native consumer build and probe refresh pass 281/281 in 73.07 seconds.
An earlier intermediate full run used the old audit probe before its width CLI
refresh and failed only that test; the fresh final run includes every check.
This completes launch configuration, not the complete MIPS suite, every module
or the remaining stage acceptance. The additional sanitizer refresh for the
changed probe tooling completed with 5/5 checks passed in 15.76 seconds, including
leak detection. See tests/parity/README.md.

## Core Business ID responsibility-group acceptance, 2026-10-07

The full production file now has an explicit constant/model/method/trait/interface/
ownership review in [Business ID acceptance](../business_id_api_acceptance.md).
The missing default Error source() API is repaired after a compiler reproduction.
All 1,283 live-original inputs match native and MIPS, adding per-field equality
for every model and all 49 error-code pairs. Four selected native, sanitizer and
MIPS checks each pass. The MIPS test registration now forwards its configured
emulator after the default binfmt CPU raised SIGILL; the same binary/observations
pass with QEMU 74Kf. Complete native consumer build and final probe/CMake refresh
pass 281/281 in 73.07 seconds. The core responsibility group is accepted on the
complete source table and ownership evidence. Five groups are accepted and 50
remain partial. Runtime profiles/allocation/save and complete
stage acceptance remain separate.

## Runtime error acceptance and whole-result completion integration, 2026-10-07

The further audit repairs Context query/list/observe/mutation, entity mutation,
governance/graph-fix and graph/ledger completion adapters. Native execution now
passes 78 concrete noncopyable-error scenarios, including begin/commit/rollback
failure, rollback-error precedence, all five suspended transaction stages and
authoritative ledger reread. See
[the reviewed source and ownership contracts](../runtime_source_test_acceptance.md#whole-result-runtime-completion-integration-2026-10-07).
The entire three-enum runtime error source surface also has an explicit review;
no Clone/Eq/Default/serde traits are invented. The rebuilt MIPS/QEMU selection
passes 11/11 in 2.18 seconds; rebuilt ASan/UBSan with leak detection passes
11/11 in 1.02 seconds. Complete native integration passes 281/281 in 67.65 seconds.
Fresh native/MIPS probes each match all 1174 live-original diagnostic inputs.
The source runtime error group is accepted on its complete requirement table.
Four groups are accepted and 51 remain partial; no stage
is closed by a native fixture or the complete source-disposition ledger.

## Enhanced-query noncopyable ownership integration, 2026-10-07

Actual noncopyable instantiation exposed thirteen copied-error completion sites
in enhanced queries, ID pages, relation execution/hydration and row/aggregate
enhancement. They now transfer completed errors when copying is unavailable,
preserving the concrete executor payload and owner. Twelve native-executor
scenarios cover direct/page/nested/embedded/aggregate/child/group failures,
short circuit, wake forwarding, repeated cancel and User deletion while pending.
The final eight-check selections pass ASan/UBSan 8/8 (0.73 seconds) and MIPS/QEMU
8/8 (1.56 seconds). The expanded native consumer build/regression now passes
280/280 in 66.80 seconds, including the final twelve-scenario fixture. See
[the source and ownership review](../runtime_source_test_acceptance.md#noncopyable-enhanced-query-integration-2026-10-07).
Context/list/observe/mutation and graph/ledger completion paths remain explicit
generic audit work. This does not accept the whole runtime module or stage.

## Presentation and trimming responsibility-group acceptance, 2026-10-07

Full production serde_utils/web/xls bodies have an explicit public/method/default/
trait/JSON/ownership table in
[presentation acceptance](../presentation_api_acceptance.md#complete-source-surface-review-2026-10-07).
The new 145-input trim matrix preserves all 25 Unicode whitespace codepoints,
nine non-whitespace boundaries and optional/null/wrong-type behavior. Six further
source calls exposed and repaired a named owning-iterator projection omission:
noncopyable E&&/E values no longer acquire an extra Clone requirement solely from
an lvalue range. Native and MIPS match all 675 live-original API inputs and the
older 785-input protocol matrix. Five rebuilt sanitizer/MIPS checks each pass;
65 original core tests pass. Complete native/header-refresh integration passes
279/279 in 67.99 seconds; the five native presentation checks pass again. The
three-file responsibility group is accepted on its complete source review.
Three groups are accepted, 52 remain partial, two are external exclusions and
one is reference-only. Stage acceptance remains open. The generator, source pin
and production dependencies are unchanged.

## Explicit noncopyable SQL completion ownership in validation, 2026-10-07

Asynchronous completion and stream-item taking now allow noncopyable transport
errors through actual SQL query, mutation, diagnostic, repeated-query, begin and
terminal adapters. Copyable results preserve their existing child borrows. The
native group passes real query/mutation/begin/transaction-query/commit/rollback/
stream paths, concrete tags, single diagnostic failure and exactly-once cleanup.
See [the ownership contract](../context_lifetime.md#explicit-asynchronous-completion-consumption)
and [runtime acceptance](../runtime_source_test_acceptance.md#noncopyable-sql-completion-integration-checkpoint-2026-10-07).
Pending/duplicate consumption and source stream identity retirement are checked.
Rebuilt five-check ASan/UBSan and MIPS/QEMU selections each pass 5/5 after
correcting the added fixture's local PMR destructor ordering. The merged full
native consumer rebuild subsequently passes 279/279. This integration does not
accept the entire error/runtime responsibility group. Advanced
runtime graph/cache/enhancement generic paths remain a distinct review scope.

## Earlier generic error copy-bound repair checkpoint, 2026-10-07

The later SQL completion integration and complete 279/279 native regression
supersede the pending-build and SQL-adapter findings in this historical section.
They do not accept the full runtime/error responsibility group.

DataServiceError and SqlExecutorError now declare allocator-aware copies only
when their concrete payload is domain-copyable. Real move-only error tests cover
both wrappers, nested Result, strong failure preservation and exactly-once Context
cleanup; the original negative compiler reproduction now succeeds. The default
empty source chain is explicit on all source error carriers. Initial scoped
native/sanitizer/MIPS evidence passes; final selected builds and the 546-task
native consumer rebuild are ongoing. The error group remains partial pending
that final acceptance; the previous diagnosis below is historical.

SQL query/mutation/stream adapters still clone const child completion errors.
Their move-only propagation is a distinct outstanding implementation requirement,
not proved by wrapper construction or String-error corpora. See
[the precise scope](../runtime_source_test_acceptance.md#generic-error-capability-repair-checkpoint-2026-10-07).
The six-stage objective and source baseline are unchanged.

## Entity-status acceptance and error Debug repair, 2026-10-07

The entity-status responsibility group has a full production/trait/default review:
all 28 status/action pairs, every predicate, exact Display/Debug and rejection
text, native enum copy/equality/hash containers and absence of a source serde
protocol. It is accepted separately from runtime/save composition. Structured
Debug for RuntimeError/ContextError/DataServiceError and nested SQL/check errors
has also been restored and exposed through public facades. Native and MIPS match
all 1,174 live-original observations; four scoped checks pass on native, ASan/
UBSan with leak detection and MIPS/QEMU. See
[the complete review](../runtime_source_test_acceptance.md#complete-entity-status-api-and-structured-error-diagnostics-2026-10-07).
Target-width reference inputs normalize only the pinned u64-to-usize index cast.

The final full native Debug regression passes 277/277 in 61.52 seconds;
rebuilt sanitizer/MIPS selections remain the four explicitly named checks.
Two responsibility groups are now accepted; 53 remain partial, plus two excluded
external groups and one reference-only helper. At that checkpoint error group completion was blocked
by a concrete uncorrected generic bound: allocator-aware DataServiceError copying
is declared for noncopyable executor payloads. A compiler reproduction fails the
expected domain_copyable=false assertion. The next error repair must preserve
move-only executor types without adding a Clone requirement to source envelopes.
This is remaining implementation work, not a user-input blocker or complete
stage acceptance. The 441 covered source-test ledger remains a separate gate.

## Complete eval responsibility-group acceptance, 2026-10-07

The full pinned production body has an item-by-item review of both enums, every
variant/field, three public methods, default and derived Clone/Eq/Debug/JSON
capabilities. Native and MIPS match 1,136 live Rust inputs, adding 529 full state-
pair equality observations and independent default checks. Generic move-only
callbacks, non-string codecs, typed payload-decoder failures and foreign nested
payload/missing-path relocation have native ownership evidence. The two checks
pass on native, ASan/UBSan with leak detection and MIPS/QEMU. See
[the full requirement table](../core_api_acceptance.md#complete-eval-module-contract-review-2026-10-07)
for accepted ownership/codec adaptations and evidence limits.

The full native Debug regression passes 276/276 in 56.82 seconds.
The module map now distinguishes an accepted module contract from partial work;
accepted records require explicit pinned source, public/default/trait/protocol/
ownership review and named checks. These fields are evidence locators, not an
automated semantic proof. At that checkpoint one responsibility group was accepted; 54 remained
partial, plus two excluded external groups and one reference-only test helper.
The source-test ledger remains 441 covered and ten exclusions; complete stages
still require the remaining member/default/privacy/composition audits.

## Graph-reference ownership adaptation acceptance, 2026-10-07

The two final source bodies preserve weak publication authority, actual child
clone promotion, shared frozen relation identity, explicit cyclic Global copying
and complete Context teardown. Rust last-Arc-drop timing is explicitly replaced
by Context reclamation under the agreed ownership design. Clone promotion is
observed before freeze; Context-close checks alone are not the proof. See
[runtime source acceptance](../runtime_source_test_acceptance.md) and
[the exact lifetime contract](../context_lifetime.md#graph-reference-lifetime-adaptation-acceptance).
The complete native Debug regression passes 276/276 in 60.80 seconds.
Rebuilt ASan/UBSan and MIPS/QEMU selections each pass 3/3; both unchanged Rust
bodies pass. The ledger is 441 covered, zero pending and ten justified backend/
external exclusions. This completes source-test dispositions, not every public
API, trait/default, protocol, privacy, composition or six-stage acceptance gate.

## Runtime-root memory and Context-tail source acceptance, 2026-10-07

All 75 runtime-root source bodies now have reviewed dispositions. Nine private
cfg(test) memory-service contracts preserve exact U64/Decimal/Soundex outputs
without expanding the published engine or introducing a production backend.
Five final roots preserve schema errors, identity options, local lease expiry,
provider ownership and separate-process optimistic-conflict output capture.
Explicit Global provider ownership and native C++ thread IDs are documented
adaptations. See [runtime source acceptance](../runtime_source_test_acceptance.md)
for exact inputs, selected fixture limits and reproduction commands.

The completed full native Debug regression passes 275/275 in 66.76 seconds.
Rebuilt native, ASan/UBSan with leak detection and MIPS/QEMU selections each
pass 3/3. The unchanged Rust runtime passes 224, with one subprocess child ignored
in the parent. At that checkpoint the ledger was 439 covered, two pending, five backend and five
external exclusions. Graph weak-reference and clone-promotion source contracts
remain pending. Source-body acceptance does not prove complete public APIs,
trait/default dispatch, privacy/provenance, lifetime composition or six stages.

## Runtime-root pagination source acceptance, 2026-10-07

Ten further root contracts retain continuous cursor boundaries, ordered ID-set
reuse/exact totals, limit/store/shape fallback diagnostics, overlapping real
same-key build coalescing, actual one-second TTL expiry, shared-store principal
isolation, empty exact results and disappearing-entity page stability. The 25ms
native timer only signals completion and is joined before referenced storage
reclamation. Explicit Global store/counter sharing replaces Arc ownership; all
Context work remains confined to its owner thread. The native group and all ten
original source names pass. See [runtime source acceptance](../runtime_source_test_acceptance.md)
for exact fixtures, delay/wait and private production-operation boundaries.
At that checkpoint the ledger had 425 covered, 16 pending, five backend and five external exclusions;
14 roots and two graph-reference lifetime adaptations remain. Complete API/
default/privacy/composition/lifecycle and six-stage acceptance remain open.

The completed full native Debug checkpoint passes 272/272 in 59.85 seconds,
including all four source/page/build/store checks. Rebuilt ASan/UBSan and MIPS/
QEMU selections each pass 4/4 with the original delay/expiry timings. Source,
export, layer and formatting gates pass separately; production behavior and
generator remain unchanged.

## Runtime-root aggregate/cache source acceptance, 2026-10-07

Six further root contracts retain dynamic I64/F64 variants and full flattened
JSON, missing relation-count U64 defaults, physical/logical FK mapping, exact
cache/provider telemetry, namespaced write invalidation and root/child cache
propagation. Controlled queue/capture fixtures are shared, while actual kernel
components perform aggregation and caching. Native new/refactored groups and all
six original names pass; see [runtime source acceptance](../runtime_source_test_acceptance.md)
for exact inputs, source interface ownership adaptation and verification scopes.
At this checkpoint the ledger had 415 covered, 26 pending, five backend and five external exclusions;
24 roots and two graph-reference lifetime adaptations remain for source review.
Complete API/default/privacy/composition/lifecycle and six-stage acceptance
remain open.

The completed native Debug checkpoint passes 271/271 in 87.78 seconds, including
all four source/typed/store/enhancement checks. Rebuilt ASan/UBSan and MIPS/QEMU
selections each pass 4/4. Source/export/layer/format gates pass separately;
production behavior, dependencies and generator remain unchanged.

## Runtime-root typed-query source acceptance, 2026-10-07

Six further root contracts retain inverse-many typed List decoding, actual module
identity-graph getters, immutable adjacency/local list copy-on-write, distinct
handwritten/derived scalar mappings and full identity/version projection ordering
on both typed query entries. Private mappings are handwritten and the generator
is unchanged. The native group and all six original names pass; see
[runtime source acceptance](../runtime_source_test_acceptance.md) for every input,
assertion and the explicit User copy adapting borrowed results. At this checkpoint the ledger had
409 covered, 32 pending, five backend and five external exclusions. Remaining
source review is 30 roots and two graph-reference lifetime adaptations; complete
API/default/privacy/composition/lifecycle and six-stage acceptance remain open.

The full native Debug checkpoint passes 270/270 in 57.47 seconds, including all
four source/entity/registry/graph checks. Rebuilt ASan/UBSan and MIPS/QEMU
selections each pass 4/4. Source/export/layer/format gates pass separately;
production kernel/dependencies and generator remain unchanged.

## Runtime-root event/order/relation source acceptance, 2026-10-07

Eight further root tests retain actual four-kind raw/safe mutation events and
optional snapshots, complete FK dependency vectors/cycle errors, behavior relation
plans, ordered U64 identity deduplication, actual hydration/telemetry and TOPN
partition/order/rank/query-count behavior. No test-local event/planner/hydration
implementation replaces a kernel component. The native group and all eight
unchanged Rust source names pass. See
[runtime source acceptance](../runtime_source_test_acceptance.md) for inputs and
the original TOPN fixture's unexercised empty-parent branch. At this checkpoint the ledger had 403
covered, 38 pending, five backend and five external exclusions; 36 root tests and
two graph-reference lifetime adaptations remain. Complete public API/default/
privacy/composition/lifecycle and six-stage acceptance remain open.

The completed native Debug checkpoint passes 269/269 in 66.18 seconds, including
all four source/mutation/ledger/hydration checks. Rebuilt ASan/UBSan and MIPS/QEMU
selections each pass 4/4. Source/export/layer/format gates pass separately;
production behavior and generator remain unchanged.

## Runtime-root validation source acceptance, 2026-10-07

Fifteen further root contracts now review actual checker/metadata/ledger entries:
custom and typed fixes, sparse update versus explicit Null, nested required-create
paths, loaded original-version requirements, generated attachment conflict
preflight, Timestamp type-error recovery, fifteen-language translation and exact
custom-catalog selection. Shared handwritten fixtures preserve production layer
boundaries and keep the generator unchanged. The native new/refactored groups
pass; all fifteen source names pass unchanged Rust. See
[runtime source acceptance](../runtime_source_test_acceptance.md) for exact inputs
and assertions. At this checkpoint the ledger had 395 covered, 46 pending, five backend and five
external exclusions. The pending source gate covers 44 roots and two graph-
reference lifetime adaptations; complete API/default/privacy/composition/lifecycle
and six-stage acceptance remain separate requirements.

Rebuilt validation/service/checker/ledger selections pass 4/4 on native,
ASan/UBSan with leak detection and MIPS/QEMU. Prepared-field assertions safely
check presence and exact Value type; 545 handwritten C++ files pass the pinned
format/check. Production dependencies and generator remain unchanged.

The completed full native Debug checkpoint passes 268/268 in 62.96 seconds
after rebuilding all targets. Inventory/export/layer checks remain locators and
structural evidence, not proof of complete member/default behavior.

## Runtime-root service-boundary source acceptance, 2026-10-07

Eight further root tests retain typed optimistic conflicts, Context telemetry
ordering, named entity-service binding, behavior-before-policy preparation,
every one of the ten original public query rejection calls, exact generated
ID/version variants and Context-aware resource/local reads. The direct group
passes natively with default PMR disabled; all eight original names pass in the
unchanged Rust suite. See [runtime source acceptance](../runtime_source_test_acceptance.md)
for fixtures, scope and reproduction. At this checkpoint the ledger had 380 covered, 61 pending,
five backend and five external exclusions. The remaining source reviews cover
59 runtime roots and two graph-reference lifetime adaptations; this does not
close complete API/default/privacy/lifecycle/composition or six-stage gates.

The complete native Debug regression passes 267/267 in 71.69 seconds. The source
group plus existing query/entity/mutation ownership checks pass 4/4 on native,
ASan/UBSan with leak detection and MIPS/QEMU after rebuilding.
Production behavior/dependencies remain unchanged;
543 handwritten C++ files pass pinned clang-format 18.1.8 checks.

## Runtime-root registration source acceptance, 2026-10-07

Eight runtime-root tests now have reviewed direct ports covering collision-safe
scalar mapping, detached reverse metadata, metadata/module/typed behavior
registration, actual module macro expansion and Context resource/local indexes.
The original synthetic dialect is adapted to SQLite syntax; handwritten models
do not migrate the generator. See [the runtime source acceptance](../runtime_source_test_acceptance.md)
for every fixture/assertion and the three-platform source/ownership selection.
The unchanged original runtime passes 224 tests with one source-ignored test;
all eight reviewed root tests run and pass. At this checkpoint the ledger had 372 covered and 69
pending, plus five backend and five external exclusions. This leaves 67 root
tests and two graph-reference lifetime adaptations for individual source review;
complete public API/default/privacy/lifecycle/composition and six-stage acceptance
remain separate gates.

The completed native Debug regression passes 266/266 in 56.57 seconds. After
rebuilding, the source/registry/resource selection passes 3/3 on native,
ASan/UBSan with leak detection and MIPS/QEMU. Root export and compiled layer
verification remain separate from member/default/behavioral acceptance.

## Repairs identified on 2026-10-07

| Source contract | Current C++ evidence | Required action |
| --- | --- | --- |
| `teaql-core/src/time.rs`: Timestamp derives PartialOrd, Ord and Hash on its i64 millisecond value. | Timestamp now supplies total ordering and standard hash; native container tests compile and pass. | 67 original public-call inputs cover default, sorted values, hash scalar writes and 16,698 pairwise comparisons. |
| `teaql-core/src/meta.rs`: EntityDescriptor's audit_mask_fields builder accepts a runtime Vec of names, including an explicitly declared empty list. | The builder now accepts dynamic string ranges, snapshots into the owner and publishes fields/flag after successful copying. | 192 original public-call inputs cover policy/default/clone results. Native contracts cover failure consistency, aliasing and actual User deletion. |

See [the capability acceptance contract](../core_api_acceptance.md) for the 259
original-call comparisons and ownership boundaries. These repairs do not prove
complete model acceptance. C++ standard hashes need equal-value consistency;
the source's default hash algorithm is not a persistent wire format.

## Mutation and trace repairs

- `mutation.rs` derives PartialEq for the three distinct field-map wrappers and
  all six mutation/batch commands. Their C++ types now supply structural
  comparison. Original-call observations preserve NaN's source non-reflexive
  comparison rather than inventing total Value equality.
- The source provides Record conversions for all three wrappers and borrowed
  and consuming iteration for MutationValues. C++ exposes the map through
  `fields` and now supplies explicit wrapper/Record construction, retained and
  consuming map conversion, and direct MutationValues range iteration. Native
  contracts cover transfer, failed foreign copying and actual User deletion.
  The three roles remain distinct.
- `trace.rs` provides both untyped `TraceNode::new` (Entity kind by default) and
  `typed`. C++ now preserves both constructor flavors and their domain-aware
  copy behavior. BatchUpdateCommand also accepts dynamic field collections.

The [mutation/trace acceptance contract](../mutation_model_api_acceptance.md)
records 1,642 original public-call observations. These repairs are not a complete
list or proof of the remaining API requirements.

## CompactRow repairs

- Original `CompactRow::shared_columns()` always returns its immutable Arc
  layout, including rows built from a map and rows whose shape was changed.
  C++ now provides an owner-bound immutable layout in all published row states,
  with stable snapshots after shape changes. Same-owner copies share layouts;
  foreign copies own destination layouts. Physical-storage null checks have
  been replaced with actual layout/domain checks.
- Original `CompactRow::iter()` exposes paired column/value iteration. C++ has
  keys/values spans and now has a paired reference range, retaining ordering,
  duplicate columns and the row owner's native-borrow contract.
- The source's consuming `into_map` and same-layout Clone responsibilities need
  explicit acceptance alongside C++'s destination-domain copy API. Rvalue
  conversion and Record construction now consume only after complete preparation;
  same-domain nested data transfers, while foreign retention deep-copies.

The [row capability contract](../compact_row_api_acceptance.md) records 356
original-call sequences, including layout identity and every saved snapshot.
Native tests cover failed preparation and actual owner destruction. Complete
query/model API acceptance remains open.

## SmartList repairs

- Owning vector/range construction, push/set, local allocator-aware list moves
  and assignment, and arbitrary-range merge preserve the source's lack of Clone
  requirements on consuming elements. Foreign non-copyable sources are rejected
  explicitly; borrowed inputs snapshot into their owner before mutation.
- Newly produced mapper outputs and ordered/hash keys can be move-only.
  Borrowed callback results still explicitly copy into the destination domain.
  Single-pass input, move-only keys and actual User destruction have native
  contracts rather than being inferred from Value-only compilation.
- Set, keyed replacement and stable retain support non-assignable elements
  through noexcept construction; the source does not require an entity
  assignment operator. A separate construct-only fixture checks these paths.
- Named const/mutable iterators and slices retain borrowing. The complete six
  public fields and every generic/entity method are mapped in
  [the list contract](../smart_list_api_acceptance.md). Its 526 original-call
  inputs retain recursive metadata, optional results, duplicate behavior and
  exact entity-conversion callback order. The two unchanged original list tests
  pass independently. The separate model Debug boundary now supplies structural
  row/list output and borrowed custom element callbacks, with 169 unchanged
  original-call inputs and 3,042 exact formatting outputs.

These repairs do not complete all model or runtime acceptance.

## Entity registration, diagnostics and consuming capabilities

The [registration dispatch audit](../entity_registration_api_acceptance.md)
reproduced a source responsibility: the original macro invokes each entity's
`register_into`, while C++ bypassed its override. The negative control matched
12/42 original calls; an isolated dispatcher/default repair matched 42/42. That
repair is now in the production header, with a direct inherited default to avoid
recursive redispatch and both named checks registered.

The [BaseEntityData diagnostic audit](../base_entity_api_acceptance.md) repairs
optional Value Debug errors that previously used JSON projection. All 1,481
original inputs match native and MIPS, with exact roles, precedence and errors.
The [Entity Debug audit](../entity_debug_acceptance.md) supplies both derived
structures through core.hpp; all 5,202 formatter outputs match native and MIPS.
Their scoped ownership checks pass native, ASan/UBSan and MIPS. Raw core
information remains separate from safe log projections.

The [consuming wrapper audit](../entity_owned_api_acceptance.md) removes extra
Clone/assignment restrictions from Result, Audited and boxed relations. The
original non-Clone fixture executes actual public trait/wrapper methods. Current
production headers pass the new native contract with default PMR disabled,
including registered same-owner movement, rejected foreign non-copyable
retention and Global copying of an Optional<Value> expression after actual User
destruction. Const copy declarations now reflect payload capabilities.

Seven new named checks are registered. The previous Decimal full pipeline
completed successfully (205/205); the new full-current native consumer rebuild
and eleven selected sanitizer/MIPS checks are running. Fresh MIPS acceptance now passes 11/11, including all seven new contracts and
four relevant ownership consumers; rebuilt MIPS probes match the live original
registration, parsing and Debug corpora. Fresh ASan/UBSan with leak detection also passes the same 11/11 selection.
The final full-current native rebuild has now completed with 215/215 checks
passed, and the expanded sanitizer/MIPS selections pass 14/14 each. The [complete Entity method mapping](../entity_api_contract.md)
adds 93 original default/dynamic sequences matching native and MIPS, with every
intermediate state and exact default/no-op behavior. Its sequence probe is now registered and passes named native/sanitizer/MIPS
checks plus live original comparisons. Complete
Entity/API/default/privacy/source-test gates remain separate.

## Native UTC conversion

The [UTC Value capability](../chrono_value_acceptance.md) adapts the original
From<DateTime<Utc>> to an explicit-destination C++ system-clock factory. Negative
fractional milliseconds use floor; seconds bounds are checked before scaling.
All 64 original conversion inputs match native and MIPS, with scoped native,
ASan/UBSan and MIPS ownership checks passing. The new standalone header adds no
runtime dependency. Umbrella and named CTest registration are complete; final native regression
passes 215/215 and selected sanitizer/MIPS acceptance passes 14/14 each.

## DataType independent diagnostics

The [DataType audit](../datatype_debug_acceptance.md) repairs standalone derived
Debug that was previously available only inside TypedNull. Ten original types
compare 180 exact formatter outputs and 100 equality results. Native owner/facade
checks pass with default PMR disabled. TypedNull now delegates without a wrapped
intermediate string. Named checks pass after rebuilding: native full regression 217/217 and seven
selected ASan/UBSan with leak detection/MIPS checks 7/7 in each configuration.
Both named native and MIPS probes match all original scalar outputs. No new variants or serde are introduced.

## Owned Value input repair

The [owned Value audit](../value_owned_api_acceptance.md) restores direct storage
transfer for same-owner String/List/Object rvalues that previously selected a
borrowed copying constructor. Foreign retention still deep-copies and preserves
the source on failure, following the existing domain adaptation. All 171 actual
original constructor observations match standalone native C++. Native contracts
check storage/capacity identity, injected payload allocation failure and nested
Global retention after actual User deletion. Query IN-list factories use the
formal constructor. Named checks are registered. Fresh MIPS/QEMU acceptance passes all ten selected
checks and matches all 171 live original observations. ASan/UBSan with leak
detection also completed all ten selected checks successfully. Full native consumer rebuild and regression completed with 219/219; its
named probe matches all 171 live original owning observations.

## Complete metadata model audit

The [metadata contract](../meta_api_acceptance.md) maps all fields, defaults,
builders, lookups and derived traits of the three original descriptors. New
meta_debug.hpp overloads retain their full raw shapes and inherited formatter
flags. All 160 original public inputs and 11,520 formatting outputs match native
and MIPS. Scoped ownership checks pass native/ASan/UBSan/MIPS; the three unchanged
source tests pass; fresh standalone native, ASan/UBSan with leak detection and
MIPS/QEMU contracts also pass all exact C++ source ports. Named registration and umbrella exposure are complete after the Value
consumer build passed 219/219. Fresh integrated acceptance, including the new qualified-facade assertion,
completed with native 224/224 and ten selected sanitizer/MIPS checks 10/10 each. Runtime privacy/schema gates remain
separate from model presence.

## Value and Timestamp public contract mapping

[Value enum API acceptance](../value_api_acceptance.md) now maps all thirteen
variants, ten TypedNull types, every From responsibility and all eight extractors.
The additional 115 actual original inputs expose optional floating results and
13,225 pairwise equalities, preserving source NaN behavior. Native matches the
corpus; the MIPS rerun applies only the documented width-changing NaN contract.
No production conversion or variant was changed.

[Timestamp API acceptance](../timestamp_api_acceptance.md) adds 99 actual original
calls spanning signed/unsigned wrapping, chrono calendar fallback, Debug flags
and transparent raw JSON type/syntax acceptance. Native/MIPS probes match; both
unchanged source tests pass and their latest ownership/source ports pass native,
ASan/UBSan with leak detection and MIPS/QEMU. The five new metadata/scalar checks are registered. Fresh native full
regression passes 224/224 and ten selected sanitizer/MIPS checks pass 10/10
each. All named native/MIPS probes match live original metadata/scalar inputs. This does not prove generic serde error
wording, complete Decimal or all-stage acceptance.

## Public query and expression diagnostics repair

The [query Debug contract](../query_debug_acceptance.md) closes the previously
reproduced missing public formatter with twenty overloads through core.hpp.
Every Expr/query/Trace type retains its original fields and inherited formatter
options. Private non-pretty cache-key entry points now delegate to the same
kernel; complete old/new key comparison remains a required acceptance gate.
The initial standalone native probe matches all 120 original inputs and 27,441
formatted outputs. Native ownership contracts pass with default PMR disabled,
qualified facade calls and retained nested output after actual User destruction.
The full native regression passes 226/226; final fixture-adjusted native and
ten selected sanitizer/MIPS checks pass 10/10 in each configuration. Fresh named
native/MIPS query Debug probes match every output. Old/new native query probes
match 980 inputs; old/new MIPS key probes match all 451 target-width inputs.
The target-width wrapper preserves every original input and calls unchanged
pinned methods; it is not a Rust MIPS execution claim. Raw diagnostics remain
separate from safe logs.

## Mutation diagnostics implementation

The [mutation Debug contract](../mutation_debug_acceptance.md) supplies all ten
original derived model capabilities: three distinct wrappers, MutationKind and
six commands, with complete fields and inherited formatter options. The correct
negative public-core compile now succeeds. The initial native probe matches 120
actual original inputs and 9,810 exact outputs. Native ownership contracts pass
with default PMR disabled and retained nested batch/snapshot/trace/output data
after actual User deletion. Named full native regression passes 228/228; fresh selected sanitizer/MIPS
checks pass 8/8 each. Named native/MIPS probes match all 120 live original inputs. Source models have no
serde derive; diagnostic observations do not invent a mutation wire protocol.

## Request generic ownership and model repair

Original DateRange<T>::new(start, end) consumes arbitrary T without a Clone bound.
The C++ allocator-aware constructor currently accepts only const T& endpoints
and calls clone_in. The earlier correct public-core construction with a scalar non-copyable
MoveOnly fixture failed inside allocator construction of a const endpoint. The
owning and allocator-aware range move repair is now implemented, with conditional
copy declarations and owner-independent equality. The public request formatter also preserves all twelve derived model
capabilities, including scalar/vector non-Clone endpoints. Preserve explicit allocator
propagation and rejection of unsupported foreign retention while restoring the
same-owner non-Clone capability. The initial native model/ownership checks pass; named sanitizer/MIPS selections
pass 10/10 each. Native full-current model regression completed with 230/230 checks and all
120 live original inputs matched; sanitizer/MIPS selections pass 10/10 each. See
[the complete request mapping](../request_api_acceptance.md). Its additional 872
original helper inputs reproduce lost diagnostic information and Between check
order; an isolated repair matches 872/872. After the 230-check boundary finished,
production diagnostics and sequential Between preparation were repaired. Named full native regression passes 232/232; twelve selected sanitizer/MIPS
checks pass 12/12 each. Fresh named helper probes match 872/872 on native/MIPS. Actual User
deletion and nested Global helper/facet retention have native ownership contracts.

## Source input-graph and presentation model gates

Original entity_graph.rs derives Debug for EntityGraphOperation and EntityGraphNode.
The builder and whole EntityGraph do not derive it; do not invent a source trait
for those wrappers. The previous correct public-core node construction followed by
production debug_string(owner, node) failed at the missing formatter.
[The input graph contract](../core_graph_api_acceptance.md) now provides both
actual derived types and owning mutation forwarding; its initial native probe
matches 240 original inputs and 12,960 exact outputs. The original source test
and exact native port pass, as do ownership contracts after actual User deletion.
Fresh full native regression passes 234/234; ten selected sanitizer/MIPS
checks pass 10/10 each, and named native/MIPS probes match all original outputs. The annotated source input graph remains separate from the runtime
identity graph and saving; node diagnostics cannot establish graph persistence.
The previously reproduced missing WebStyle formatter is repaired by
[the seven-type presentation diagnostic header](../presentation_api_acceptance.md).
All 452 original inputs and 8,127 outputs match initial native comparison. Generic
response projection now accepts non-contiguous records and non-copyable owned
entity arrays; borrowed Clone constraints reflect the original requirement.
Native ownership and source ports pass. Final full native regression passes
236/236; ten selected sanitizer/MIPS checks pass in each configuration and both
named probes match all 524 expanded original inputs.
Web/Xls presentation types retain their original Debug/Clone/equality/serde
responsibilities. Their existing JSON observations do not supply every trait or
complete deserialization/error acceptance. Continue those exact source contracts
under the full six-stage objective.

## Remaining audit scope

The [core query capability audit](../query_api_acceptance.md) restores arbitrary
input ranges for Expr function/logical/IN factories and SelectQuery projection.
Standalone AggregationCacheOptions factories/propagation retain the public
fields with explicit C++ collision renaming. Its 980 unchanged original calls
compare complete query/Expr state and exact thresholds. Native single-pass,
aliasing and actual User-deletion checks complement the source observations.
These repairs do not establish complete query/default/execution acceptance.

Proceed through the existing 58 responsibility groups without narrowing their
contracts. Cross-check each source item against its concrete C++ API and current
original-call/source-test evidence. Trait implementations and defaults require
semantic checks beyond exported names. Serde derives, optional/default fields,
unknown fields, wire errors and destination ownership need explicit protocol
checks wherever the source defines serialization. Source types without serde
must not be assigned an invented serde protocol.

The [extended Decimal arithmetic audit](../decimal_extended_acceptance.md)
adds 8,056 direct original-call fixtures. It repairs result scale, intermediate
panic boundaries, division power-table/constructor failures and buffered
remainder window behavior. The scalar arithmetic implementation adds no runtime
dependency. This matrix does not replace complete Decimal API acceptance.

Remaining Decimal capabilities, complete query and save entry capabilities,
transaction/stream cancellation authority, log provenance/privacy and native
resource ownership remain acceptance gates. The retained legacy SQLite path
now has an explicit opt-in compatibility disposition; the simple examples use
the independent runtime/executor/provider layers. Backend/cloud/GPU implementations
outside the agreed scope and the unchanged generator do not become new work.

Module rows remain partial until their actual requirements are proven. Passing
200 regression checks and removing missing module rows does not prove all six
stages complete.

## Frontend model binding dispatch

The full presentation model checkpoint passes native 236/236 and ten selected
sanitizer/MIPS checks each. The expanded 524-input original-call matrix now
reproduces an additional Style binding defect: direct base writes bypassed
BaseEntity::put_dynamic overrides, matching only 452/524 before repair. The
production Style entry now dispatches the override and matches 524/524 on native.
Action retains the source's direct base append. Original source web tests pass
4/4 and XLS tests pass 2/2 in an isolated target. Final handwritten ports and
owner-retention acceptance pass in the complete native 236/236 regression and
ten selected sanitizer/MIPS checks each; native/MIPS match all 524 live inputs. The six-stage plan has been
restored after discovering it had been overwritten with an API-audit copy.
See presentation_api_acceptance.md for this bounded capability evidence.

## Decimal inline string capability

The locked rust_decimal 1.42.1 public Decimal implements array_string(). A
correct C++20 public-header syntax probe originally failed on this member. The
same probe accepted the existing mantissa(); truncating to_i128() was never a
substitute for that unscaled coefficient capability. The repaired declaration
and implementation now accept the public call.

DecimalInlineString owns 32 inline character bytes and a one-byte length, has no
allocator or pointer to its source, and is trivially copyable/destructible.
Its view() is a borrow of the carrier and is not null-terminated. Copies own
independent bytes, including after actual source User deletion. This scalar
value introduces neither a heap owner nor a third Context allocation domain.

array_string() preserves the original unsigned text: append_sign=false differs
from signed Display/to_string. Retained scale and negative zero are observed
exactly. Source fixed-buffer panics map to std::length_error; successful normal
formatting, copy and move publish no managed ID and perform no heap allocation.
No normal heap fallback or silent truncation is introduced. Existing signed
Display/JSON behavior is unchanged.

The original-call fixture decimal_inline_cases.json.gz records 484 inputs,
including 50 original buffer panics. Seven 96-bit coefficients, both signs,
negative zero, initial scales 0/1/28, explicit rescale through maximum u32 and
seeded combinations retain complete raw coefficients/sign/scale and output or
original panic outcomes. The oracle invokes the actual original method.
Zero rescale follows the original scale-28 clamp; the full 32-character capacity
and following failure are verified using a nonzero fractional value.

All 484 observations match the native production probe, and all ten native
Decimal checks pass, including allocation counters, null default PMR, independent
inline copy addresses and actual User deletion. decimal_inline_contracts and
decimal_inline_rust_parity are registered with the cross emulator forwarded.
The twelve selected ASan/UBSan checks with leak detection and twelve selected
MIPS/QEMU checks have completed successfully. The MIPS probe matches all 484
live original inputs. The full native 238-check consumer rebuild completed successfully; all 238
CTest checks passed and the final native probe matches 484/484 live inputs. This repair
does not prove complete Decimal/source-test/API or all-stage acceptance. Active
feature-gated mathematical APIs must be checked against the pinned features.

## Remaining core diagnostic capability inventory

A correctly mapped C++20 public-header probe checks callable write_debug entries
for original core derived Debug types, with DateRange<int>, SmartList<Value>,
request/dynamic_search namespaces and the separate core input-graph namespace.
It also checks the supported Decimal helper result/error/strategy types. Eighteen
entries are absent: the eight BusinessId models/enums, seven dynamic-search
models/errors, UnpackedDecimal, DecimalError and RoundingStrategy. EntityError
and the earlier model families have entries. This is declaration evidence; a
present overload does not prove complete formatting or ownership semantics.

The actual original core_aux_debug_oracle records 94 inputs and 34216 string
observations for the next grouped repair. These cover all seven BusinessId codes,
all twelve rounding variants, all six Decimal error variants, full declared field
order, nested query and JSON output, optional date text, numeric bounds, four
calendar forms, long/Unicode/control-character strings, nine formatter modes,
source-defined Clone equality and the warning's actual Serialize output.
UnpackedDecimal is obtained through Decimal::unpack and its public fields; it has
no crate-root name reexport. Source files, generator and runtime dependencies
remain unchanged. The three standalone diagnostic headers now supply all eighteen formatter
entries. Native and MIPS probes each match all 94 inputs. Native and ASan/UBSan
with leak detection pass allocator/ownership contracts with null default PMR,
complete nested retained output and actual User deletion. The rebuilt MIPS ownership contract also passes under QEMU. After the 238-check checkpoint finished, core.hpp exposure and the two named
CMake checks were added. Qualified core::write_debug accepts dynamic-search as
well as root core model overloads. The new full native consumer rebuild and all 240 checks pass; nine selected
ASan/UBSan checks with leak detection and nine MIPS/QEMU checks pass. Named
native/MIPS probes match 94/94 live original inputs. All 79 correctly mapped
core/dependency diagnostic declarations are callable in the presence probe;
this declaration check is not full behavioral acceptance. Standalone capability evidence
does not establish that integration or full API/privacy acceptance.

## Per-source-test acceptance gate

The initial source_test_dispositions.json bound the then-indexed 441 tests to an
explicit reviewed disposition. Five OpenTelemetry/OTLP implementation tests are
excluded by the agreed external-adapter scope. Three eval tests were incorrectly
classified as unpublished; the correction below restores their published API.
Six metadata/Timestamp/input-graph tests have been
reviewed assertion-by-assertion against concrete native ports; all three named
native contract checks pass in the current scoped rerun. The remaining 427
entries are pending individual review, even when module-level corpora or source
ports already exist. This count records review work, not 427 proven code defects.

verify_source_test_dispositions.py checks pinned identity, complete/unique test
coverage, explicit exclusion reasons, concrete evidence files and optional CTest
registration metadata. Its --require-complete mode currently fails because the
applicable acceptance audit is incomplete. It never turns a matching name, an
existing file or a passing test total into automatic semantic acceptance.

```sh
ctest --test-dir build --show-only=json-v1 > /tmp/teaql-ctest-registrations.json
python3 scripts/verify_source_test_dispositions.py \
  --ctest-json /tmp/teaql-ctest-registrations.json
python3 scripts/verify_source_test_dispositions.py --require-complete
```

## Latest disposition checkpoint

The ledger now has 45 individually reviewed covered_in_scope tests: the earlier
six model contracts, six query diagnostics, fifteen mutation diagnostics, ten
binding-policy contracts and eight flat-graph/ledger contracts. All original
SQL tests pass (21 diagnostics and 10 binding policies), and original runtime
flat/ledger tests pass 4/4 each. Scoped native/sanitizer/MIPS checks pass.

Review exposed weakened numeric assertions in two C++ source-test ports. These
now preserve exact I64/U64 variants and Some/None presence; a deliberately wrong
flat-graph variant passes the old converter and fails the repaired assertion.
No production implementation needed a change at this checkpoint.

The remaining 388 applicable records need individual review; they are not 388
proven code defects. Five external tests and three unpublished reference tests
retain their explicit exclusions. The strict source-test completion gate still
fails, and the complete six-stage objective remains active.

## Search/provider/business disposition checkpoint

Sixteen more original tests have individual reviewed dispositions: eight core
dynamic-search/privacy cases, two real SQLite integration cases, two business
clock cases and four business-ID permutation cases. Original Rust and scoped
native/sanitizer/MIPS checks pass. Exact I64 row assertions and complete CSV/hex
fixture consumption replace weaker C++ checks. Golden fixtures, source inputs,
all nine guarded actions/rollback and the full 20000-sequence range remain.

That ledger checkpoint had 61 covered_in_scope, 372 pending_review, five excluded
external and three incorrectly excluded eval tests. Pending review is not a claim
of missing implementation. Strict source-test and complete-stage acceptance
remain incomplete.

## Published eval and corrected source-test scope

The current ledger indexes 451 tests after correctly expanding the six masking
macros into twelve runnable tests. It has 103 covered, 341 pending, two named
backend exclusions and five external exclusions. All eval tests are applicable:
the published declaration appears after the root test module. LoadState and
EvalResult<T> now retain the actual five/three variants, generic consuming
callbacks, missing-path propagation and Debug/serde. Both named evaluation
checks pass on native, ASan/UBSan and MIPS/QEMU, and native/MIPS match 607/607
live original inputs. See core_api_acceptance.md for the exact ownership and
unordered-set diagnostic adaptation. Public core module namespace checks and
a negative-control exclusion guard now catch this previously missed boundary.
This implementation repair does not close the other 341 review records or the
full member/default/composition/privacy acceptance gates.

Full native integration for this correction passes 242/242 CTest checks after
rebuilding every consumer. The two scoped sanitizer/MIPS checks remain a
separate selection, not claimed full-platform parity acceptance.

## Core functional source-test checkpoint

Seventeen more applicable tests have assertion-by-assertion reviewed direct
ports: four SafeExpression, two SmartList merge, four trimmed serde helpers,
one WebResponse shape and six root expression/subquery tests. Original literals,
typed numbers, complete vectors, callback counts, optional presence, JSON keys
and AST equality remain intact. The production implementation is unchanged.
All 65 original core library tests pass; the new named C++ group passes after
rebuilding on native, ASan/UBSan and MIPS/QEMU. Details and scoped reproduction
are in core_api_acceptance.md.

The current ledger is 120 covered, 324 pending, two backend exclusions and five
external exclusions. Its strict completion gate still fails. The earlier full
242-check native checkpoint is separate from this subsequent test addition;
passing source fixtures cannot by itself close full API or migration acceptance.

## Handwritten entity and presentation disposition checkpoint

Ten original root-library entity-mapping/dynamic/record tests now have direct
handwritten counterparts; five presentation tests have reviewed existing ports.
Strict scalar ranges, missing/Null defaults, boxed Object serialization, field
metadata, registration, Date/Json/Timestamp and typed dynamic record roundtrips
are retained. The presentation review restores source loaded inputs and checks
the actual serialized facet map, including the mutable accessor. Scope is the
private source fixtures; no automatic generator acceptance is implied.

The rebuilt paired native/sanitizer/MIPS checks pass 2/2 each, and unchanged Rust
passes 65/65 core tests. The ledger is now 135 covered, 309 pending, two backend
and five external exclusions. The strict completion gate remains incomplete;
the original six-stage objective and all unreviewed API gates remain intact.

## Root composition and query disposition checkpoint

Eleven additional source tests retain full query builder composition, typed
entity-list helpers/record consumption, optional expression paths, and query
defaults/validation. All 26 root-library tests and all six query.rs tests now
have individually reviewed covered dispositions. Fresh hard-limit/zero-threshold
inputs match the source; panic cases keep their source reason text through the
C++ exception adaptation. Direct ports use actual kernel APIs and handwritten
fixtures, with default PMR disabled.

The rebuilt named group passes on native, ASan/UBSan and MIPS/QEMU, and unchanged
Rust passes all 65 core tests. The ledger is now 146 covered, 298 pending, two
backend exclusions and five external exclusions. Complete public API/default/
trait/composition and all-stage acceptance remain separate uncompleted gates.

## Core source-test review subset

All thirteen Value tests have exact-input direct ports, preserving numeric
boundaries, accepted/rejected variant rules, Decimal conversion, dates and
timezone normalization. Expected timestamps come from independent standard
calendar arithmetic. All ten TypedNull types have an additional native
projection/extractor contract. The rebuilt core-only target passes on native,
ASan/UBSan and MIPS/QEMU; original Rust passes 13/13 Value and 8/8 dynamic-search
integration tests.

The current ledger is 159 covered, 285 pending, two backend and five external
exclusions. Its core subset is 73/73 individually reviewed: 65 library tests
and eight integration tests. Compiler names confirm the 65 library index;
pending work is now in data-service, SQL, provider and runtime source tests.
This source-test milestone does not imply every core public capability or the
complete six-stage goal is accepted.

Full native integration at this milestone passes 246/246 CTest checks after
the complete consumer build. Layer validation confirms the compiled runtime
has no SQLite C API calls. The targeted sanitizer/MIPS Value checks and prior
core selections remain scoped evidence rather than full-platform acceptance.

## Layer defaults and diagnostic cursor checkpoint

Two data-service source tests, three generic dialect tests and seven diagnostic
stream tests have individual direct-port reviews. Original trace/comment access,
unknown provider intent, relation-sibling exclusion, TypedNull/schema/domain
validation, delivered-row counts, resource release, cancellation/unwind and sink
isolation remain distinct contracts. The dummy dialect adapts to SQLite without
implementing another backend. The cursor fixture invokes the actual wrapper;
its source supplies only rows, errors or pending state.

Rebuilt native/sanitizer/MIPS selections pass 2/2; unchanged Rust passes 2+3+7.
The ledger is now 171 covered, 273 pending, two backend and five external
exclusions. Data-service source tests are covered, but complete SPI, API/default/
composition and six-stage acceptance remain open. Full native 246-check evidence
predates these new registrations; no complete 248-check rerun is claimed.

## Executor cache and partition-probe checkpoint

All twelve executor.rs tests now have individually reviewed direct ports. The
original complete fixtures cover shared descriptor state, privacy-sensitive
cache identity, per-statement numbered bindings, disabled metadata, absent eager
debug expansion, probe versus window selection and guard-preserving rewrite.
Rebinding preserves scalar values, IN cardinalities, complete synthetic array
parameters and all four original complex query shapes. Cached outputs are
compared against direct uncached production compilation.

The array compile hook is a test-only adaptation of the source dummy, with no
new native database backend. Runtime state and result storage have explicit
Global/User owners and default PMR is disabled. Native, ASan/UBSan and MIPS/QEMU
pass the named contract; original Rust passes 12/12. The ledger is 183 covered,
261 pending, two backend and five external exclusions. Public API/default/
composition and overall six-stage acceptance remain open.

The complete native consumer build and full regression at this checkpoint pass
249/249 CTest checks. Compiled-runtime layer validation passes with no SQLite
C API calls. Pinned source verification, source inventory, export locators,
source-test registration and clang-format checks pass; none replaces the
remaining requirement-by-requirement acceptance.

## Entity-status source and diagnostic checkpoint

All sixteen original entity-status tests have individually reviewed direct
ports. The standalone model preserves default New, eleven valid transitions,
seventeen rejected transitions, all persistence/helper flags and every original
lifecycle intermediate state. The production header now exposes the four
EntityAction Display names and preserves the complete original state/action
error text through the existing std::invalid_argument adaptation. A temporary
negative control reverting only that error text fails the port.

The rebuilt named check passes on native, ASan/UBSan with leak detection and
MIPS/QEMU; unchanged Rust passes 16/16. The existing native core-model target is
rebuilt and passes. The ledger is 199 covered, 245 pending, two backend and five
external exclusions. Standalone source-contract acceptance does not replace
remaining runtime composition, trait or complete-stage requirements.

Full native integration passes 250/250 CTest checks after rebuilding all affected
consumers. Compiled-runtime layer validation preserves SQLite independence.
Pinned source, inventory/export locators, registered source dispositions and
524-file formatting checks pass. Full-platform and complete-stage acceptance
remain separate from the selected sanitizer/MIPS status evidence.

## Aggregation-key and relation-planning source checkpoint

Two aggregation-cache key tests and two relation-plan/telemetry tests now have
individually reviewed direct ports. The actual production helpers retain the
original typed Trip filters, canonical predicate ordering, comment/TTL omission
and result-affecting vendor IDs. The Vendor.trips child plan retains all five
original default/provider/explicit-threshold cases, including the zero override.
Telemetry retains exact variants, original counts, all relation/operation
attributes and absent entity IDs. Additional tests cover the helper early exits,
Window zero-probe reporting and explicit Global copies after actual source User
deletion. No replacement algorithm or production dependency is added.

Original Rust passes 2+2 tests; the named check passes after building on native,
ASan/UBSan with leak detection and MIPS/QEMU.
The ledger is 203 covered, 241 pending, two backend and five external exclusions.
The prior full native 250-check result remains separate from this new
registration; full 251-check and complete six-stage acceptance are not claimed.

## In-memory engine source-test checkpoint

All nineteen indexed in-memory engine tests now have individually reviewed
direct ports retaining original row fields, Value variants, ordered sort
vectors, page/projection behavior and expression inputs. Aggregate checks use
exact I64/F64 equality. Private LIKE inputs enter the same production matcher
through the public expression evaluator, with no replacement algorithm.
Default PMR is disabled and actual User deletion returns every engine block.
Unchanged Rust passes 19/19. The source/ownership/differential selection passes
3/3 on native, ASan/UBSan with leak detection and MIPS/QEMU. A rebuilt live Rust
oracle matches 6,532/6,532 observations on native and MIPS. The comparator and
CMake now propagate the configured emulator to both pointer-width detection
and observation execution; an initial direct-launch MIPS run failed with SIGILL
before this runner repair. Production engine semantics remain unchanged.

The ledger is 222 covered, 222 pending, two backend and five external exclusions.
The prior full native 250-check result predates the cache/relation and engine
registrations; no complete 252-check regression is claimed. Complete API/trait/
composition and six-stage acceptance remain open.

## Checker, language and catalog source checkpoint

Five checker, two language and two catalog source tests now have individually
reviewed direct ports. Original locations and all naming profiles, JSON Pointer
escaping, typed/serialized wire aliases and status inference/markers retain
their original inputs and assertions. Byte-identical locale and builtin-catalog
fixtures preserve complete normalization and malformed-template rejection.
The exact minimal English catalog retains French-request fallback to English,
stable missing key and builtin checker translation. Explicit Global wire/catalog
copies survive actual source User deletion with nested destination ownership.

The rebuilt source/ownership selection passes 3/3 on native, ASan/UBSan with leak
detection and MIPS/QEMU; unchanged Rust passes 5+2+2. The ledger is 231 covered,
213 pending, two backend and five external exclusions. Pinned source, inventory/
export locators, registered dispositions and 527-file formatting checks pass.
No production implementation or dependency changed. The prior full native
250-check result is separate from these later registrations; complete 253-check,
full-platform and six-stage acceptance are not claimed.

## Formatter and telemetry source checkpoint

Four formatter and three telemetry source tests now have individually reviewed
direct ports. Original truncation inputs, non-executable marker, UTF-8 boundaries,
both ordinary/sensitive formatter paths and exact raw-audit level gates retain
their source contracts. Native category strings and safe/forbidden operation
attributes are unchanged. Throwing start and activation adapters enter the real
failure-isolating runtime; the actual async business result remains 42 with
exactly one poll. Actual User deletion destroys the delegate once and returns
every block; explicit Global ordinary output still excludes its canary.

The rebuilt source/ownership selection passes 3/3 on native, ASan/UBSan with leak
detection and MIPS/QEMU; unchanged Rust passes 4+3. The ledger is 238 covered,
206 pending, two backend and five external exclusions. Pinned source,
inventory/export locators, registered dispositions and 528-file formatting
checks pass. Production implementation and dependencies are unchanged. The
prior full native 250-check evidence is separate from these later registrations;
complete 254-check, full-platform and six-stage acceptance are not claimed.

## Mutation-policy and graph-source checkpoint

Five mutation-policy and two graph source tests now have individually reviewed
direct ports. Complete original exec-1/Order.saveGraph plan values, policy/
approval identities, warning vectors and first/repeated occurrences are kept.
The real Context review isolates warning Result errors, requires exact approval
fingerprints, and returns typed denial before the source persistence counter.
Both stack/token trace chains retain complete nodes and omit only empty comments.
All six original pushes preserve four ordered batches and all three grouped
counts, with exact 2/2/1/1 batch membership. Explicit Global event/plan copies
survive actual User deletion with nested allocator ownership.

The rebuilt source/ownership selection passes 3/3 on native, ASan/UBSan with leak
detection and MIPS/QEMU; unchanged Rust passes 5+2. The ledger is 245 covered,
199 pending, two backend and five external exclusions. Pinned source,
inventory/export locators, registered dispositions and 529-file formatting
checks pass. Production implementation and dependencies are unchanged. Review
and batching evidence remain separate from transaction/save completion. The
prior full native 250-check evidence is separate from later registrations;
complete 255-check, full-platform and six-stage acceptance are not claimed.

## Log-privacy and isolated file-sink source checkpoint

All five indexed log-privacy contracts now have individually reviewed direct
ports. The source's ignored child runs in three isolated processes, preserving
the original unset/invalid/exact acknowledgement inputs and every normal/
audit-diagnostic/SQL-diagnostic assertion. Ordinary logs retain safe expanded
SQL and authoritative intent while excluding business, credential and inherited
write canaries. Opted-in business diagnostics carry their explicit marker;
credentials remain excluded. The exact gate, unchanged audit inputs and target
trace scrubbing are independently tested. Explicit Global safe-event copies
remain private after actual source User deletion.

The rebuilt source/file/ownership selection passes 3/3 on native, ASan/UBSan with
leak detection and MIPS/QEMU. Unchanged Rust passes four tests with one ignored
child exercised by its parent, rather than five standalone default passes.
The ledger is 250 covered, 194 pending, two backend and five external exclusions.
Pinned source, inventory/export locators, registered dispositions and 530-file
formatting checks pass. Production implementation and dependencies are unchanged;
complete logging/default/composition and six-stage acceptance remain open.

Full native integration at this checkpoint passes 257/257 CTest checks after
requesting the complete consumer build, including every later source-test
registration. Compiled-runtime layer validation passes with no SQLite C API
calls. This does not broaden selected sanitizer/MIPS evidence into a full-platform
claim or close the remaining source/API/default/composition acceptance gates.

## Context source and fresh-ledger factory repair

All six indexed Context tests now have individually reviewed direct ports.
Typed root validation, every SQL switch state, disabled metadata rejection,
complete safe/buffered parameters, structured query/mutation intent and every
multi-level trace assertion retain the original inputs. Review exposed the
missing UserContext::entity_runtime_state public factory. It now owns a lazy
User graph anchor and uses the existing fresh_with_shared_graph primitive;
operation ledgers remain independent while the immutable graph is shared.

Additional checks preserve graph sharing, closed rejection, existing objects
after quota failure, complete reclamation, and explicit Global projected-log/
ledger/graph retention after actual User deletion. The original native source
contract passes, and five selected source/entity/memory/logging checks pass on
rebuilt ASan/UBSan and MIPS/QEMU. The existing SIZE_MAX rejection input uses a
runtime volatile value to avoid GCC's compile-time oversized-allocation warning;
the exact boundary and warning policy remain unchanged. Both selections pass
again after this test-only adjustment. Unchanged Rust passes 1+4+1 tests.

The ledger is 256 covered, 188 pending, two backend and five external exclusions.
Pinned source, inventory/export locators, registered dispositions and 531-file
formatting checks pass at that checkpoint. The complete native consumer rebuild
for the public Context layout change finished and passed 258/258 checks.
Full-platform and six-stage acceptance remain open. No production dependency
or generator changed.

## Entity-runtime source fixture checkpoint

Thirteen original composition/lazy-root tests now have individually reviewed
direct ports preserving every typed key/value, missing/conflicting error,
snapshot version, complete retryable source and relation state. The deterministic
seed, LCG, field schedule and all 512x32 source iterations remain unchanged;
failed composition checks the complete target change set atomically. The
handwritten private TestEntity delegates through the actual public composition
default rather than implementing composition locally.

The rebuilt entity-runtime named check passes under ASan/UBSan with leak detection
and MIPS/QEMU, and unchanged Rust passes fifteen source tests. Two Arc ownership
tests remain pending C++ lifecycle-adaptation review, without exclusion or
replacement by ordinary graph checks. The ledger is 269 covered, 175 pending,
two backend and five external exclusions. The native entity-runtime target
was refreshed after the complete consumer rebuild to include all additions.
The paired entity/Context selection passes 2/2 on native, ASan/UBSan and
MIPS/QEMU; the Context fixture preserves its shared query/mutation timestamps.
The latest complete native regression then passes 258/258. Pinned source,
inventory/export locators, registered dispositions and 532-file formatting
checks pass. Production behavior and dependencies are unchanged by these
fixture additions. Full API/lifecycle/composition and complete-stage acceptance
remain open.


## Root SQL source-test checkpoint

All 29 root SQL tests have individually reviewed dispositions. Twenty-six
applicable generic/SQLite tests retain source inputs and exact SQL, typed bindings,
whole compiled-query/default log equality, identifiers/index/DDL bounds,
partitioning, all guarded mutations, raw/subquery/having/function hooks and
SQLite debug rendering. Three wholly native PostgreSQL/MySQL renderer tests
are excluded by exact source name. Generic projection deduplication, opaque
raw fragments and the synthetic GBK override are retained without a native
SQLite function/backend claim.

The direct compiler target passes after rebuild on native, ASan/UBSan with leak
detection and MIPS/QEMU. Unchanged Rust SQL passes 88/88, including all 29 root
names verified against the ledger. The most recent full native regression passed
259/259 before the test-only scoped-error catch refinement, which subsequently
passes all three selected runs. No production compiler or dependency changed.
The ledger is now 295 covered, 146 pending, five backend and five external
exclusions. See the SQL source-test acceptance note for exact scope/reproduction.
Root reexport presence and these source tests do not close the remaining full
API/default/composition/lifecycle or six-stage acceptance gates.


## Portable-reference source-test checkpoint

Both portable codec source tests have individually reviewed evidence. The direct
codec helper preserves fixed key/clock/nonce inputs, exact golden wire output,
opacity, full claims tuple and type/purpose/final-character tampering rejection.
The existing Context helper retains the original missing-codec error and checked
installation/golden/decode contract, with actual User destruction and explicit
Global retention checked separately. Source names do not broaden their assertions
into unsupported rotation/expiry coverage; broader existing corpora cover those.

Unchanged Rust passes 2/2; the rebuilt native, ASan/UBSan with leak detection
and MIPS/QEMU Context contract each pass 1/1. No production behavior or dependency
changed. The ledger is 297 covered, 144 pending, five backend and five external
exclusions. The 17 round-trip tests and other remaining source/API/default/
lifecycle/composition gates are still pending; complete-stage acceptance is open.


## SQLite provider storage source-test checkpoint

Fifteen provider root source tests have individually reviewed direct C++ ports
preserving SOUNDEX, real native streaming/cursor release, Decimal/temporal
storage, complete mutation/DDL, comment-independent layouts, strict Text/Json,
ID increment/actionable overflow, all three buffered stream shapes and exact
general/private-fast timestamp plus declared-kind inputs. The original fast
parser body is unchanged and now named in the internal detail namespace;
private assertions call the same function used by the actual decoder.

Unchanged Rust passes all 42 provider tests with names verified against the
ledger. The direct group passes native; rebuilt storage/decoding/binding
selections pass 3/3 under ASan/UBSan with leak detection and MIPS/QEMU. The
Context schema boundary and explicit stream cancellation retain the documented
C++ lifecycle adaptations. No dependency or generator changed. The ledger is
312 covered, 129 pending, five backend and five external exclusions; the
remaining 27 provider and other source/API/composition/target gates remain open.


The complete consumer rebuild subsequently passes all 260/260 native Debug
CTest checks (88.64 seconds). The latest full regression therefore includes
the root SQL, portable-reference, memory-fault and provider storage additions.
Selected sanitizer/MIPS evidence remains the explicitly named three provider
checks. Pinned source, module/export locators, registered dispositions and
534-file formatting/whitespace checks pass; the full six-stage audit remains open.


## SQLite schema and boundary source-test checkpoint

Seventeen further provider root tests retain every source model, schema input,
compatibility/evolution diagnostic, FK/index requirement and public/private
bootstrap boundary. Eight independent connections preserve the original barrier
and final column/file assertions. The cfg(test)-only source schema entry maps
to the actual private physical kernel with ordered descriptors; public business
schema still enters UserContext. Passive module installation and exact row
roundtrip remain separate from physical/bootstrap checks.

Unchanged Rust passes 42/42. Full native CTest passes 261/261 (73.17 seconds).
Following the test-only MIPS timing refinement, the three selected schema/storage
checks pass native, ASan/UBSan with leak detection and MIPS/QEMU. Initial MIPS
five-second contention failures were retained, alongside the successful 7.32s
run using a 30s fixture budget. Production remains at 5000ms and all eight
writers/source assertions remain. The standalone SQLite-only reproducer and
measurement record do not establish a library defect or physical-device result.

The ledger is 329 covered, 112 pending, five backend and five external
exclusions. Ten provider root tests still require individual review. No
production behavior, dependency or generator changed. All-stage and full
API/default/privacy/composition/target acceptance remain open.


## SQLite row/query source-test checkpoint

Seven further provider roots retain the complete scalar seed with all seventeen
predicates, eight relation-subquery/NULL cases, exact partition rows, both TOPN
executor paths with complete optional identity/name vectors, every new/legacy
Boolean DDL/variant/typed-mapping case and four independent file-backed ID workers
producing exactly1..100. Handwritten FeatureFlag mapping follows source macro
rules; it does not alter native decoding, Value conversions or the generator.

Unchanged Rust passes 42/42. The rebuilt direct C++ target passes native,
ASan/UBSan with leak detection and MIPS/QEMU. ID contention retains the original
five-second configuration. No production behavior or dependency changed. The
ledger is 336 covered, 105 pending, five backend and five external exclusions.
Three provider roots remain pending for TransactionScope, audited boundaries
and collision hydration; full API/default/privacy/composition/lifecycle/target
and all-stage acceptance remain open.


The complete native Debug regression subsequently passes 262/262 (55.10 seconds).
The same current source names/ledger, module/export locators, compiled layer
separation and 537-file formatting/whitespace checks pass. Original provider
root names match all 42 pinned inventory entries. Selected sanitizer/MIPS proof
remains the named seven-contract group, not a full-platform regression claim.


## Provider root source-test review completion

All 42 indexed provider roots now have individually reviewed dispositions.
The final three direct ports preserve actual raw/typed TransactionScope outcomes,
both authoritative version-one saves, complete committed ID/name tuples,
retryable failed ledger and the physical/logical channel collision's scalar
alias, absent relation and optimistic update/reread. Handwritten source models
reuse allocator-aware entity/ledger/field helpers; callback states only sequence
real runtime operations and forward/cancel children. The User owns the moved
third model instead of weakening the one-shot callback capture contract.

Unchanged Rust passes 42/42 with exact names matched to the pinned inventory.
Native full CTest passes 263/263 in 103.79 seconds; rebuilt final-source and
transaction ownership selections pass 2/2 under ASan/UBSan with leak detection
and MIPS/QEMU. No production behavior/dependency/generator changes are introduced.
The ledger is 339 covered, 102 pending, five backend and five external exclusions.
Source review continues through 75 runtime root tests, 17 round-trip references,
eight entity-save tests and two graph-reference lifetime adaptations. Complete
provider SPI/API/default/privacy and the six-stage audit remain open.


## Entity-save source-test checkpoint

All eight entity_save source contracts now have individually reviewed ports.
They preserve exact Task/scalar/relation/dirty/nested-path inputs, authoritative
version2/tombstone-2 before commit-race visibility, missing-readback rollback,
retryable version, cancelled-new-root rejection and customer denial before any
provider call. The first raw event retains the original generated governance
snapshot. Private snapshot/path/preflight helpers and actual TypedEntityChecker
execute the other cases; handwritten model hooks do not replace runtime logic.

Unchanged Rust passes exactly8/8 under the entity_save filter. The runtime-only
C++ target passes native; full native CTest passes 264/264 in 88.48 seconds.
Rebuilt source/graph-saver/save-preparation selections pass 3/3 under ASan/UBSan
with leak detection and MIPS/QEMU. No production behavior/dependency/generator
changes are introduced. The ledger is 347 covered, 94 pending, five backend and
five external exclusions. Runtime roots, round-trip references, two graph
reference adaptations and full API/default/privacy/composition/target gates
remain open under the unchanged six-stage objective.


## Round-trip reference source-test checkpoint

All 17 round-trip roots now have individually reviewed direct ports preserving
exact vector inputs/golden output, binding/coarse errors, old key rotation,
nonce/expiry, the UserContext boundary, all nine raw-mode rules and ordered safe
startup metadata, default-clock/nonce live authorization, and exact invalid
wire/lifetime/token bounds. The actual Fn-style authorization adapter and real
runtime/codec execute; helpers supply inputs only. Scoped providers/atomic state
replace Arc ownership without extending the User lifetime.

Unchanged Rust passes exactly17/17. Full native CTest passes 265/265 in 62.90
seconds; rebuilt source/Context-ownership selections pass 2/2 under ASan/UBSan
with leak detection and MIPS/QEMU. No production behavior/dependency/generator
changes are introduced. The ledger is 364 covered, 77 pending, five backend and
five external exclusions. The remaining indexed source gate covers75 runtime
root tests and two graph-reference lifetime adaptations. Full API/default/
privacy/composition/lifecycle/target and six-stage acceptance remain open.
