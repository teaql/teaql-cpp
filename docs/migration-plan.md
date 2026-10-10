# TEAQL C++ six-stage migration plan

The only baseline is Rust `v5.0.5`, commit `62940ae2b88e7acc32763853b8b4685a01c7e90a`, pinned in `upstream.lock.json`. Verify it with `scripts/verify-upstream.py`.

**Preserve every core concept, responsibility, and abstraction. C++ optimization concerns memory and resource lifetimes.** SQLite is the only database implementation. Fewer dependencies must not mean fewer design elements. The generator remains outside the current work scope. See the complete [design parity contract](design-parity.md).

| Stage | Goal | Final status | Acceptance evidence |
| --- | --- | --- | --- |
| 1 | Align the complete core design | Complete: exact baseline, all concepts/roles, independent layers and explicit ownership adaptations. | design-parity.md; 82 source files in 58 contracts; complete implementing reviews. |
| 2 | Core models and two explicit lifetimes | Complete: all thirteen assigned groups and the memory kernel. | Complete APIs/defaults/protocols; checked IDs/bitmaps/limits; destination copies and cleanup; final three-profile integration. |
| 3 | Separate data-service, SQL executor, transport and SQLite provider | Complete: all eight assigned groups; compatibility remains opt-in. | Complete SPI/default/driver/protocol/ownership reviews; compiled independent layers; matched SQLite3.53.2 target contracts. |
| 4 | Query and entity runtime | Complete: all fourteen assigned groups and service composition. | Public/private query, relations, identity, pagination/cache, schema/IDs, reference, telemetry and Context contracts. |
| 5 | Mutation governance, audit and logs | Complete: all six assigned groups and graph/ledger saving composition. | Full source ordering, policy/checker/approval/transaction/events/privacy; actual-source comparisons and real SQLite rollback/trigger/foreign-key contracts. |
| 6 | Examples and complete parity acceptance | Complete within the explicit supported scope. | Complete runtime root/all75 tests; all451 dispositions; native388/388, sanitizer388/388, MIPS387/387; 2/2 live examples per profile; 22-requirement final audit. |

Accepted on 2026-10-10. The ledger is **55 accepted / 0 partial / 2 external
exclusions / 1 reference-only helper**. All three profiles have 427 nonempty
compiled dependency records and no-work builds before/after testing. The root
review reconciles complete production exports, all published module bindings
and all 75 original fixture/assertion contracts; declaration scans alone are
not acceptance.

Final ASan/UBSan leak scanning is disabled. The one native64-only raw FromSql
Debug comparison has explicit platform scope; structured errors/provider checks
remain on MIPS. Physical WNDR4300 execution is deferred by the user. Generator
changes, GPU adapters and external/non-SQLite integrations remain outside scope.
C++ ownership and owner-thread/protocol adaptations are documented in each
accepted contract and do not merge source responsibilities.

See [the final evidence and requirement audit](../measurements/parity/2026-10-10-stage6-final-review/README.md),
[the API audit](parity/api-audit.md) and [the module map](parity/cpp-module-map.md).
Historical counts below describe their recorded snapshots.

## Historical checkpoints

The checkpoints below describe their original source snapshots; their open
gates and totals do not override the current stage table or remaining-gate
document.

The current SQL model review adds complete binding/query/log Debug, default
compile-error causes, active-variant equality and consuming binding input.
Forty original-call cases and explicit lifecycle checks pass native/sanitizer/
MIPS selections; the existing 1,538 SQLite rendering/binding cases also match
live original Rust on native/MIPS. See [the detailed contract](sql-model-api-contract.md).
The combined four-check selections now pass native, ASan/UBSan with leak
detection and MIPS. Final whole-project consumer/composition acceptance remains
required. The SQL source group and stages 2–6 remain open.

An earlier data-service model checkpoint preserves complete source diagnostics and
records default/Clone/comment/trace behavior across forty original inputs.
Native, ASan/UBSan with leak detection and MIPS selections each pass 3/3,
including existing executor SPI contracts. See
[data-service-api-contract.md](data-service-api-contract.md). Full original
SystemTime range, generic async/descriptor/transaction composition and current
whole-project integration were independent gates at that checkpoint. The current
complete data-service source acceptance is recorded below.

The later [SystemTime repair](system-time-contract.md) shares the existing full
Unix carrier from core across metadata, runtime logs and cursor deadlines. Its
254-input standalone native comparison and independent sanitizer-with-leak-
detection contract pass. Context logging retains its old 315 observations and
adds twelve full-range inputs. Fresh consumer builds, log/pagination checks and
final whole-project ABI regression remain required; stages 2–6 stay open.

## Work order and outstanding differences

1. **Core models.** Value variants/conversions/JSON projection and BusinessId interfaces are present. Core BusinessId, metadata/naming, presentation/trim and evaluation groups are accepted. Decimal arithmetic, ordering, scale/significant-digit rounding, and binary encoding have differential evidence. Exact/radix parsing and numeric conversion matrices are implemented. Audit the complete supported reexport contract and remaining model/ownership/default contracts, including Value/time and entity models; CompactRow/query models are now accepted. SmartList and mutation/trace are now accepted source groups. Entity/Audited/default hooks/input graphs and entity-list conversion are present; presence alone is not full model acceptance.
2. **Query models.** Expr, SelectQuery, and request builders retain their fields/conversions. Dynamic search preserves validation/warnings/trusted binding. Preserve optional comments, comment traces, NOT IN subquery non-null constraints, and recursive relation/enhancement limits. Purpose and execution-time policy require runtime call sites.
3. **Runtime layering and query execution.** Keep data-service, SQL executor, transport, provider, and runtime responsibilities independent. SQL compilation/execution, diagnostics, real provider codecs/streams/transactions, ID/schema services, metadata, graphs, telemetry, audit primitives, query adapters, relation execution/hydration/enhancement, and cache/pagination stores have the evidence listed above. Same-key build coordination now has 144 comparisons against the original registry and actual asynchronous mutex; Context close joins native build work before releasing its guard. Canonical aggregation and pagination key helpers now have 451 original-method comparisons. Miss/lock/recheck/build, pagination/cache orchestration and the simple runtime examples are connected; audit their complete public entry/default/cancellation contracts. Preserve existing regressions throughout the transition.
4. **Save execution.** Validate graph, ledger, and TransactionScope paths separately. Approval occurs before the graph saver starts a transaction; authoritative ledger rereads occur before commit. Deliver audit events at the source call sites rather than inventing uniform post-commit publication.

This order does not narrow the migration. The objective remains every core contract in the module map. Preserve interfaces whose external implementation is out of scope; an empty implementation cannot establish support.

Follow dependencies: implement required model contracts, then their interfaces/execution adapters, then query/entity runtime, mutation/audit/log integration, and complete parity acceptance. Cross-stage tests may be introduced early but cannot prove an unimplemented later stage. Keep the reference worktree unchanged and never use its moving active branch as the baseline.

## Stage 2 completion audit, accepted 2026-10-08

The final current native refresh completes 452 tasks, reports no remaining work,
verifies all 375 compiler dependency records and passes full CTest 303/303 in
74.81 seconds. Fresh combined models and separate scalar/memory selections pass
on sanitizer and MIPS. The last Value/time/enabled-Decimal source group is now
accepted after its complete body/default/trait/protocol/ownership review.
All thirteen assigned Stage 2 source groups and the memory kernel have explicit
accepted contracts. This closes Stage 2; it does not close stages 3–6, arbitrary
external integrations, prepared unregistered Stage 3 fixtures or hardware testing.

| Requirement | Authoritative evidence | Current gate |
| --- | --- | --- |
| Every core model/concept/abstraction | Accepted complete contracts for metadata/naming, expression/safe-expression, query, list, mutation/trace, Entity, input graph, request/search, business IDs, presentation/serde helpers, evaluation and entity status. Each module-map acceptance names its full public/default/trait/protocol/ownership review and direct checks. | Value/time/Decimal is also accepted after complete review and final current integration. All thirteen assigned source groups have explicit accepted contracts. |
| Complete Value/time and enabled Decimal | Full Value/time production-body review; constructor/extractor/derived-trait/projection/owning-input/UTC/Timestamp source observations; enabled Decimal factories, arithmetic, casts, inherited defaults, diagnostics and binary/default JSON/format protocols. New Ord, pointer-width, const-fn and negative-clock corrections retain the source behavior. | Combined consumers pass 21/21 under ASan/UBSan with leak scanning disabled and 21/21 on MIPS/QEMU. Final current native suite passes 303/303. |
| Explicit Global/User lifetime and allocation sites | context-memory-implementation.md; growing multi-block arenas, checked IDs/independent bitmaps, bounds, explicit destination copies, nested allocators, scoped native resources and stop/join before reclamation. Application business access goes through UserContext, including Global delegation. | Thirteen-check scalar/DataType/memory selection passes 13/13 on sanitizer (2.63 seconds, leak scanning disabled) and 13/13 on MIPS (4.24 seconds). Final native integration passes 303/303. |
| Preserve mutation/snapshot/generated values and query roles | Complete accepted model source contracts retain distinct types and behavior; runtime saving/execution layers have separate later-stage gates. | Stage 2 model acceptance does not accept graph saving, provider/runtime policy or audit/log privacy. |
| Baseline, compact dependency scope and code style | verify-upstream.py, inventory-upstream.py --check, pinned clang-format checks and whitespace review. Actual Rust word-width oracle dependencies are test-only and match existing lock versions/checksums. | No model-generator or C++ runtime dependency change. Physical WNDR4300 execution remains a separate target gate. |

The current results above close these Stage 2 gates. The full six-stage objective is unchanged;
stages 3–6 retain their independent requirements.

## Stage 3 completion audit, accepted 2026-10-08

All eight assigned source groups are accepted after complete public/default/
trait/protocol/ownership reviews: data-service API/log, SQL binding/types,
dialect, executor/transport/mutation execution, diagnostics, root exports and
SQLite provider. Independent layers and typed interfaces remain intact; the
compiled runtime still has no SQLite C API calls or native backend link edge.

The latest full native 315/315 regression and no-work build cover current sources.
Executor selections pass 15/15 on sanitizer/MIPS; fresh provider builds precede
32/32 sanitizer (leak scanning disabled) and 31/31 same-engine MIPS checks. Live
source comparisons and unchanged verification fingerprints are retained in
[the provider checkpoint](../measurements/parity/2026-10-08-sqlite-provider-acceptance/README.md).
The matched MIPS profile uses SQLite 3.53.2. Older engine numeric/diagnostic
limitations, native-width raw Debug and physical-router limits remain explicit;
no tolerance or omitted core responsibility is used as an acceptance shortcut.

Stage 3 closes provider/query/mutation/guard/stream/transaction/schema/ID resource
contracts. It does not accept full entity-runtime semantics, graph/governance/
audit/privacy, final source-test/example acceptance or physical-device execution.
Stages 4–6 and the complete six-stage goal remain open.

## Memory constraints

An explicit application-owned GlobalContext manages global objects; UserContext exclusively owns request data. `global.make<T>()`, `context.make<T>()`, and corresponding resource factories expose the lifetime at allocation sites. Arenas grow across multiple blocks. Object destruction and native-resource cleanup precede reclamation of every block. Borrows/results/views become invalid on close; cross-domain retention requires an explicit deep copy. See [context-lifetime.md](context-lifetime.md).

## Validation

```sh
python3 scripts/verify-upstream.py
python3 scripts/inventory-upstream.py --check
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
python3 scripts/check-header-dependencies.py --build-dir build
ctest --test-dir build --output-on-failure
```

Lifetime changes additionally require ASan/UBSan. The 330 source declarations and 451 source tests are an omission index, not an automatic public-API or compatibility verdict. Review public functions, reexports, serialization, conditional configuration, trait defaults, and composition semantics by module.

## Context API follow-up, 2026-10-08

Stage 4 review reproduces missing Context root/fix model Debug and RootError's
empty cause, then restores their full source protocols. All 234 actual original
model inputs match native, including Clone/equality/validation and nine formatter
modes. A separate controlled DataStore child reproduces missing cancellation on
allocation-domain rejection; cancellation now precedes discarding its pointer.
Native model/store checks pass 4/4 and all 251 old store observations remain
unchanged. Fresh selected ASan/UBSan (leak scanning disabled) and MIPS checks
pass 4/4; all 234 MIPS model inputs additionally match live original Rust. The
complete 552-task native consumer refresh remains running. The last complete 315/315 native acceptance predates these
Context changes. The ledger remains 25 accepted / 30 partial; Stage 4 and the
complete Context group remain open. See [the review](context-api-contract.md).
The schema returned-work follow-up additionally cancels invalid provider and
bootstrap children before releasing UserContext. New schema checks pass 2/2
under sanitizer/MIPS, preserving bootstrap cancellation mode and pending-borrow
release. The fresh MIPS probe matches all 384 live original schema observations;
native resource/event/ID/business comparisons also match. Nineteen broader
composition checks pass 23/23 on sanitizer/MIPS. Telemetry factory cleanup and
an extra completion Clone restriction are also repaired; its three-check
selections pass on both configurations, preserving all original observations.
The initial 552-task native pass finishes; a final 294-task schema/telemetry/
template consumer refresh now runs before complete 317-check regression.

## Current checkpoint, 2026-10-09

The latest complete native regression passes **387/387** in 79.85 seconds with four CTest workers
after the 344-task graph/save consumer rebuild and two-task ownership-fixture refresh.
No-work builds precede/follow testing; all **426** compiled object dependency records
are nonempty. The ledger is **54 accepted / 1 partial**, with two external exclusions
and one reference-only helper. Stages 2–5 are accepted; Stage 6 remains open.

Complete graph/save source responsibility is now accepted after all four production
files, models/defaults/traits/protocols, public/private entry boundaries and source
ordering/cleanup are reconciled. Fresh ASan/UBSan and MIPS/QEMU selections each
pass 44/44 with post-test no-work; sanitizer leak scanning is disabled. Evidence is
stored in measurements/parity/2026-10-10-runtime-graph-source-acceptance. Runtime-root
and final Stage 6 source-test/example/target acceptance remain independent.

The current runtime-service review repairs unconditional cursor next-offset
rejection in Release. Six of 16 actual original boundary observations disagree
on MIPS before repair, while all 165 existing query observations remain equal.
The new comparison retains Debug overflow and Release wrapping by build profile.
The complete six-file runtime service group is now accepted after full body/
API/default/trait/protocol/ownership reconciliation, including the restored
private typed-aggregate responsibility. Current sanitizer/MIPS selections each
pass 56/56 plus independent base/stream 3/3, with completed builds and no-work.
Thirteen live original matrices match on both profiles, including 165 public
entity-query, 48 private helper and 1502 mutation observations. All fourteen
assigned Stage 4 groups are accepted. Evidence is recorded in
measurements/parity/2026-10-09-runtime-service-source-acceptance.

Graph/save and runtime-root remain partial. All ten graph-model Debug writers
are installed; 128 original observations in nine modes match, with four focused
checks passing on native, ASan/UBSan and MIPS/QEMU. The restored repository
extraction member matches 141 live original observations on all three profiles,
with actual source-Context destruction and explicit Global copies. Full native
integration passes 381/381 after a completed 340-task consumer rebuild. Retained
recursive graph execution and internal save adapters remain open. Evidence is
stored in measurements/parity/2026-10-09-runtime-graph-model-review and
measurements/parity/2026-10-09-runtime-graph-repository-review. Generator/dependency
scope and physical-device deferral are unchanged.


Context source responsibility is now accepted after complete production review
and repair of generated bootstrap active-root rejection inside ensure_schema.
The registered negative control fails before repair and actual original Rust
accepts the operation. Fresh ASan/UBSan and MIPS/QEMU selections each pass 64/64;
both native and MIPS match all 2,384 live original Context observations. Evidence
is retained in measurements/parity/2026-10-09-bootstrap-root-context-review.
Sanitizer leak scanning is disabled. No whole-stage completion follows.

The direct audit-event fan-out repair is fully integrated. Each distinct borrowed
Context/owner/event/result arena has a stack-only synchronous guard; ordinary
exceptions detach guards before actual User reclamation. All four reentrant-close
checks pass. Recorded event/save/privacy selections pass **20/20** on ASan/UBSan
and MIPS/QEMU, with no-work before/after. Leak scanning is disabled.

Complete event source responsibility is accepted after all 572 production lines,
public/private factories, model traits/diagnostics, safe projection and sink
contracts are reviewed. Fresh live original comparisons match native112/1427/1088
and MIPS112/1421/1088; six MIPS native-width count inputs explicitly reject before
execution and match Rust u32 parsing. No corpus is weakened to match.

Logging/privacy/formatter source responsibility is now accepted after full
body review, three 16/16 selections and live original comparisons. Evidence is
recorded in measurements/parity/2026-10-09-log-source-acceptance. Remaining groups
are graph/save execution and the runtime root.
All 451 source-test dispositions verify against current CTest registration;
bookkeeping alone is not semantic
proof. Formatting checks pass 674 handwritten C++ files and the unchanged Rust
reference verifies at v5.0.5. Generator/dependency scope is unchanged. Physical
WNDR4300 execution remains explicitly deferred by the user; current target
acceptance uses cross-compilation and QEMU. Earlier counts are historical.

Stage 2 is complete after final-state native 303/303 acceptance and current
sanitizer/MIPS model/lifetime selections. The Value/time/enabled-Decimal source
group is accepted, bringing the ledger to seventeen accepted and 38 partial
groups, with two external exclusions and one reference-only helper. Complete
stages 3–6 and physical WNDR4300 execution remain open. The independent SQL-log
review reproduced non-Clone projection-state creation/copy and retained identity
gaps. Both are now repaired and the registered eight-input counterpart matches
live original Rust, alongside all 1,008 existing intent observations. Initial
four-check native/sanitizer/MIPS selections pass. The arena layout change requires
the running memory and whole-project consumer refresh; 303/303 is the previous
accepted checkpoint rather than a claim about the new headers. The SQL-log group
remains partial.
The following dated paragraphs retain earlier implementation/integration history.

Further complete default review restores distinct Ord min/max/clamp operations.
All 605 new direct original calls match native and MIPS, with all 3,675 previous
outputs unchanged (4,280 total). Fresh thirteen-check Decimal selections pass
13/13 on ASan/UBSan with leak scanning disabled (76.28 seconds) and 13/13 on
MIPS/QEMU (45.67 seconds). The Timestamp system-clock read and bracket now floor
negative Unix milliseconds as the source does. Complete Value/time production
bodies and enabled Decimal public/default/protocol responsibilities are reviewed;
combined consumers and current full native integration remain running, including
a required final refresh for the later header and clock edits. Group/stage
acceptance is still open for Value/time/Decimal.

The latest Decimal follow-up records 238 actual pointer-conversion inputs at
both Rust word widths. Native C++ matches live 64-bit Rust; MIPS matches actual
32-bit ARMv7 Rust under QEMU without output rewriting. Allocation-free source
const-fn construction, raw reads, bounds and serialization now support C++
constant evaluation. Source and native constant assertions compile. Fresh
Decimal selections pass 13/13 under ASan/UBSan with leak scanning disabled
(64.18 seconds) and 13/13 on MIPS/QEMU (35.42 seconds). The latest whole-native
644-task header consumer rebuild is running; the preceding 302/302 is historical
until the new final regression completes. This does not accept the remaining
Value/time/Decimal group or complete stages 2–6.

The complete Entity, registry/module and Context locking source contracts are
accepted after their entire source-body/public/trait/protocol/ownership reviews.
All independent abstractions remain intact. The final native suite passes
302/302 in 67.69 seconds after a no-work refresh; all 375 compiled dependency
records are nonempty. Fresh Entity/locking selections pass 11/11 on sanitizer
and MIPS/QEMU. The broader registry composition selection passes 20/20 on both;
its target-width recovery verifies six rejected counts and all 1,421
representable original audit outputs. Sanitizer leak scanning is disabled.

Sixteen of 58 source groups are accepted, 39 remain partial, two are external
exclusions and one is reference-only. This closes these source-group gates;
complete stages 2–6 and physical WNDR4300 execution remain open. Detailed scope
and recovery records are in entity-api-contract.md, registry-api-contract.md and
context-locking-api-contract.md. The following paragraphs retain the preceding
implementation/integration history.

Latest registry capability repair: both missing Debug writers are restored with
exact source ordered map shapes, including name-to-name formatting over native
set storage. All 18 original Default/Clone/Debug/replacement/snapshot scripts
match live native; default-PMR-disabled retained diagnostics survive actual
source and formatting User deletion with explicit Global copies. The prepared
probe is registered. The first sanitizer run found local test PMR strings
outliving their User; the fixture now destroys them before deleting the User.
Rebuilt registry selections pass 3/3 under ASan/UBSan with leak scanning disabled
and 3/3 on MIPS/QEMU. The full native build remains running.
The previous 301/301 checkpoint predates these headers/targets and remains
historical evidence until the new consumer regression finishes.

The subsequent module composition review adds sixteen actual Rust macro
observations for multiple entities, duplicate keys, mixed behavior entries,
trailing commas and independent installed module snapshots. All 198
decoder/module inputs match live native Rust, with the prior 182 unchanged.
A retained Global module decodes after its source User is deleted; empty-module
replacement preserves independent registry responsibilities. The two selected
checks pass under ASan/UBSan with leak scanning disabled and on MIPS/QEMU.
The full native build is still running, and source/stage acceptance stays open.

The isolated native latest-source build is terminal-successful at 441/441 tasks.
Its final incremental refresh reports no work, all 374 compiled objects have
nonempty dependency records, and the complete current Debug suite passes
301/301 in 79.05 seconds with four workers. This includes repaired time,
language/catalog, telemetry and source-contract consumers. Baseline/inventory
and whitespace checks pass. Registry diagnostics remain a reproduced missing
public capability; the prepared eighteen-case source/native fixture is still
unregistered. The full six-stage completion audit remains open; this whole-native
checkpoint does not accept unknown or absent capabilities from green tests.

The isolated full native refresh stops at 150/550 on a SQLite cause-view
incomplete-type compile error. Current source inspection finds the table
definition already moved outside the class; both exact failing storage/schema
translation units pass standalone syntax checks against that current header.
The corrected source is preserved and the full isolated build resumes after
terminal confirmation. The earlier scoped 3/3 language and 8/8 consumer results
remain their bounded checkpoints. No complete current native regression or
stage/source-group acceptance is claimed from the interrupted build.

Isolated native consumer progress: the completed language/catalog consumers pass
3/3 (10.45 seconds). A separate completed eight-check selection for locking,
transaction scopes, runtime examples and original SQL defaults/query/mutation
passes 8/8 (8.07 seconds). Current recorded dependencies are nonempty for the
166 already compiled objects; the directory is still building and this is not
final object/whole-suite acceptance. Implementation headers remain stable while
the full latest-source build runs. Thirteen groups remain accepted and 42 partial;
stages 2–6 stay open.

Registry/module review preserves all separate decoder, metadata, entity,
behavior, policy, module and bootstrap responsibilities across the complete
590-line source. A public compile fixture reproduces missing metadata/entity
registry Debug. The exact source map shapes and remaining integration gates are
recorded in [registry-api-contract.md](registry-api-contract.md). No shared
implementation header changes are made during the current full native build;
this review does not accept the source group or stages 2–6.

Latest telemetry model repair: missing public Debug and an ambiguous MIPS
std::size_t conversion are reproduced and restored. All 110 original model
conversion/Clone/PartialEq/Debug inputs match live native in nine modes;
default-PMR-disabled retained-operation diagnostics survive actual User deletion.
See [telemetry-api-contract.md](telemetry-api-contract.md). Selected platform
builds remain running. The isolated full native build stops on missing new
i18n_debug.cpp linkage because it started with an older CMake graph; after
terminal confirmation, the latest graph/source/header refresh is now running.
Full regression and stages 2–6 remain open.

Telemetry selected consumer acceptance subsequently passes MIPS/QEMU 3/3 in
2.53 seconds and ASan/UBSan with leak scanning disabled 3/3 in 3.57 seconds,
including unchanged activation and new model inputs plus ownership checks.
Selected final refreshes report no work with nonempty compiler dependencies.
The isolated latest-source native refresh remains running; thirteen source
groups are accepted and 42 partial, and stages 2–6 stay open.

Latest language/catalog capability repair: missing public Debug derives for
Language, BuiltinTranslator and I18nCatalog are reproduced and restored with
complete source locale maps/private fallback. All sixty live original
Default/Clone/Debug inputs match native in nine modes; native retained-catalog
diagnostics pass with default PMR disabled after actual User deletion. See
[i18n-api-contract.md](i18n-api-contract.md) for complete source responsibility
review. Named sanitizer/MIPS builds and full isolated native consumer refresh
are running/pending. No source group or stage is accepted from this scoped proof.

The repair's fresh translation/ownership/diagnostic selection passes MIPS/QEMU
3/3 (21.47 seconds) and ASan/UBSan with leak scanning disabled 3/3 (37.31 seconds).
Final selected refreshes report no work and compiled dependency records are
nonempty. The isolated whole-native build remains running and requires the
latest source/header/CMake refresh before complete regression. Counts remain
thirteen accepted and 42 partial; stages 2–6 stay open.

Latest executor-default evidence: 144 direct original query/stream/mutation/guard
observed-default inputs match live native C++, including dispatch-before-poll,
complete received requests, chunk sizes, unpolled results and absent observer
notifications. The named ASan/UBSan (leak scanning disabled) and MIPS checks pass.
This proves these four defaults; broader error/provider/transaction/cancellation
composition remains open. The shared native full refresh stops on a reference
probe link failure with subsequently present definitions and a recovered Ninja
log. A fresh isolated native build-root-integration directory is now building
for whole-project acceptance. Thirteen groups remain accepted and 42 partial;
stages 2–6 stay open.

Latest standard-time capability repair: duration_since now exposes typed
success/error alternatives under the original name; elapsed retains its reversed
error duration, and scalar errors preserve Clone/message/default cause.
Result/Error/Duration diagnostics match all 7,056 live original input pairs,
including twelve formatter modes, ties-to-even precision, unsigned rounding
overflow and Unicode units. Standalone native contracts pass with default PMR
disabled. Fresh sanitizer/MIPS consumer builds and full native integration are
pending after these headers and the return type change. See
[the arithmetic/error contract](system-time-arithmetic-contract.md). No new
source group or stage is accepted from this scoped result.

The typed-error/time/model/logging selection subsequently passes MIPS/QEMU
8/8 (41.86 seconds) and ASan/UBSan 8/8 (59.85 seconds), with leak scanning
disabled. The core-only formatter compile capability is restored without
changing output. Stable-source review additionally restores associated/module
constexpr UNIX_EPOCH values; public compile-time equality now passes. Fresh
epoch-header platform consumer builds are running. The preceding eight-check
results do not substitute for the final native regression or stage acceptance.

Post-epoch selected consumer acceptance: MIPS/QEMU passes 8/8 in 51.08 seconds
and ASan/UBSan with leak scanning disabled passes 8/8 in 101.62 seconds. Each
selected final refresh reports no work, with complete nonempty compiled
dependency records. Native's 484-task recovery is complete; its latest full
header/target refresh is running before complete regression. Source groups
remain thirteen accepted and 42 partial; stages 2–6 stay open.

EntityError's inherited source default is repaired and its 102-input corpus
matches live Rust. Complete Value/time review repairs writable Timestamp
storage: public signed millis retains mutable borrowing, with 108/108 live
observations and both original time tests passing. Enabled Decimal Inv and
inherited Zero/One set_zero/set_one/is_one are also reproduced missing and
restored through the existing division kernel and canonical scalar identities.
Native scalar/default contracts pass; all 3,675 foundation observations match
live original Rust, preserving all earlier 3,495 observations unchanged.

The final seventeen-check entity/Timestamp/Decimal/locking/original-tail selection
passes ASan/UBSan with leak scanning disabled and MIPS/QEMU, including inherited
identity defaults. Two additional scoped data-service checks pass on sanitizer
and MIPS; their forty model inputs match live original Rust. All 29 data-service
root names are checked. The long native build has completed, and its final
incremental consumer refresh is running before complete regression. Earlier
281/281 results predate these changes; no source group/stage is accepted from
intermediate builds.

The reproduced metadata SystemTime range gap now has an implementation repair:
metadata/log fields use the common core signed-seconds/nanoseconds scalar,
direct diagnostics and unsigned full-span elapsed arithmetic. The missing
subsecond assignment is restored; checked carry/borrow and direct native chrono
unit/clock-point adapters preserve the full source domain without overflowing
intermediate nanosecond products. All 1,232 actual Rust arithmetic observations
match native C++; native and standalone sanitizer lifetime/overflow contracts
pass. Seven selected cross targets and coherent native consumer integration
remain pending. See system-time-arithmetic-contract.md; no source group/stage
is accepted from old-layout or incomplete consumer builds.

Context local/remote locking's complete body, SPI/directory/default roles and
downstream source tests are reviewed in context-locking-api-contract.md. Its
current native named checks pass 2/2 and all 222 observations match live Rust.
Independent kernel dependencies and absence of compiled runtime SQLite C API
calls are verified. Thirteen groups are accepted and 42 remain partial;
entity/value/locking and stages 2–6 remain open.

### Earlier core graph/query checkpoints


Resume audit: current source is pinned and existing modifications are preserved.
The first interrupted handles are missing, but a subsequent process audit finds
earlier native/sanitizer Ninja builds still running. Duplicate builds started on
resume are stopped; the earlier processes are retained. Missing handles alone do
not prove termination. Both Arc lifetime source tests are
reviewed against their unchanged Rust bodies, C++ fixture and implementation.
Each original Rust test freshly passes. Five selected native, ASan/UBSan with
leak detection, and MIPS/QEMU checks pass 5/5 (1.15/2.41/1.65 seconds).
LeakSanitizer now runs under the updated environment; earlier tracing-limited
results remain historical. Context close replaces last-Arc-drop reclamation,
while weak publication authority, clone promotion, frozen identity, explicit
Global copies and exactly-once cleanup are preserved. The two adaptation gates
are closed; production runtime/API/composition and complete stages remain open.

Entity's ten selected checks now pass on native, ASan/UBSan with leak detection
and MIPS. A subsequent supported Decimal review reproduces and repairs its
missing Error cause default and ErrorString Display formatting; the new 94-input
live oracle retains every prior observation. See
[decimal-api-contract.md](decimal-api-contract.md). The updated standalone native
contract and native/MIPS live comparisons pass; sanitizer with leak detection
and MIPS each pass all twelve Decimal/auxiliary checks. Current full native
consumer integration remains required; stages 2–6 are still open.



The complete core annotated entity-graph group is accepted after full source,
field/default/trait, builder, diagnostic and ownership review. All 240 inputs
match live Rust on native/MIPS, the exact original source test passes 1/1, and
current native/ASan-UBSan/MIPS selections pass 4/4. The current native 281/281
regression has no intervening C++ edit and Ninja is clean. Thirteen groups are
accepted and 42 partial, plus two external exclusions and one reference-only
helper. Entity.rs is next; runtime identity/save/governance/privacy and stages
2–6 remain open.

### Earlier query/expression checkpoints


The complete query.rs group is accepted after reviewing all structs/enums,
105 public methods, defaults/traits, helper validation, list recursion, row
layouts and JSON projections. Native/MIPS match all 1,089 query/Expr, 401 row,
169 row/list Debug and 120 query/Trace Debug inputs. Ten selected native,
ASan/UBSan and MIPS checks pass 10/10; current leak scanning is unavailable.
The final full native suite passes 281/281 in 72.92 seconds and the build is
clean. Twelve groups are accepted and 43 remain partial, plus two external
exclusions and one reference-only helper. Model acceptance leaves SQL/runtime,
caching/pagination/graph/save/governance/privacy and all stages 2–6 open.

### Earlier expression checkpoints


The complete Expr/SafeExpression group is accepted against its full source,
method, field/default/trait, protocol/diagnostic and ownership review. Native/
MIPS match all 1,022 query/Expr, 448 SafeExpression and 120 query/Trace Debug
inputs. Native, ASan/UBSan and MIPS selections pass 12/12; current leak scanning
remains unavailable. The final native consumer refresh is clean and full
regression passes 281/281 in 62.11 seconds. Eleven groups are accepted and
44 remain partial, plus two external exclusions and one reference-only helper.
The query.rs review is next; its models/rows and downstream execution, runtime,
governance/privacy and all stages 2–6 remain open.

### Earlier expression integration checkpoints


The expression source review is complete and includes the application-defined
TeaqlEmpty extension contract. All 448 SafeExpression inputs match live Rust
on native/MIPS, retaining the earlier 444 observations. Final ASan/UBSan and
MIPS/QEMU selections pass 12/12 in 14.24/11.53 seconds, including the corrected
fixture destruction order. Current leak scanning is unavailable under tracing.
The full native build is still running; its final refresh/regression is required
before expression-group acceptance. Ten groups remain accepted and 45 partial;
all stages 2–6 remain open.


The full Expr factory review additionally repairs generic BETWEEN bounds.
Forty-two new original-call inputs retain the old 980 cases, for 1,022 total,
including complete subquery/operation/equality states. The fresh standalone
native contract passes bounded logical construction and BETWEEN ownership/
conversion-order/failure checks; its probe matches every input against live
original Rust. Final named/full consumer refreshes are still running and must
include the latest header/probe edits before acceptance. Ten groups remain
accepted and 45 partial; stages 2–6 stay open.


Expression integration is running after two reproduced repairs: incremental
AND/OR range deduplication avoids retaining every duplicate in a bounded User
arena, and SafeExpression identity/borrowed-root/entity/list constraints match
actual source Clone responsibilities. New compile contracts pass and unchanged
source SafeExpression tests pass 4/4. The native full and sanitizer/MIPS selected
builds are running; earlier full-suite results predate these header changes.
Ten groups remain accepted and 45 partial. Complete expression-group and stages
2–6 acceptance remain open.

### Completed mutation/trace checkpoint


The complete mutation/trace model group is accepted after a full source field,
method, default, trait, diagnostic and ownership review. Actual original builder
and delegated-map calls expand the model corpus to 1,688 inputs, all matching
live Rust on native/MIPS alongside the unchanged Debug corpora. Eight selected
native, ASan/UBSan and MIPS/QEMU checks pass; current leak scanning is unavailable
under sandbox tracing. The final full native suite passes 281/281 in 74.57
seconds and the build has no pending work. Ten groups are accepted and 45 remain
partial, with two external exclusions and one reference-only helper. Mutation
execution/governance/audit/log/privacy and stages 2–6 remain open.

### Earlier request/list checkpoints (superseded by the verdict above)


The complete request/dynamic-search responsibility group is accepted on its full
source/default/trait/conversion/protocol/privacy/ownership review and final
forwarded-receiver repair. Native/MIPS match all 174 request/509 search/872 helper
observations; current native, MIPS and ASan/UBSan selections pass, with leak
scanning unavailable under tracing constraints. Final full native consumer
build/regression passes 281/281 in 70.86 seconds with no pending Ninja work.
Nine groups are accepted and 46 remain partial. The downstream runtime/query/
save/log/privacy and full stage acceptance remain open; this source group cannot
close those responsibilities.

The final generic request review aligns EntityReference constraints with the
adapter's actual forwarded receiver, repairing a reproduced false lvalue
capability. Compile/runtime ownership checks pass within six native and six
MIPS/QEMU selections; original Rust dynamic-search tests pass 8/8. Refreshed
ASan/UBSan without leak scanning passes 6/6 in 17.73 seconds. The full native
consumer rebuild/regression is running. The previous 281/281 native checkpoint predates this declaration
change; complete request/search and stage acceptance remain open.

The final complete native consumer/probe refresh and regression passes 281/281
in 69.60 seconds; Ninja has no pending work. The complete list.rs responsibility
group is accepted, including all fields/methods/traits, negative equality/Clone
declarations and ownership/diagnostic repairs. Eight groups are accepted and
47 remain partial. The earlier 280/281 stale search-probe failure is closed by
the explicit two-task refresh. All 174 request and 509 search inputs match live
original Rust on native/MIPS; complete request/search acceptance remains open.
Current leak scanning is unavailable under tracing constraints, while ASan/
UBSan and explicit arena-reclamation checks pass. No stage is accepted merely
from the complete native suite or the list module verdict.

Independent equality/Debug observations cover all 43 request-struct fields and
ten search equality fields, including actual camelCase warning JSON. The current
request/search corpora have 174/509 inputs, preserving the earlier cases. Native
standalone live comparisons and named sanitizer/MIPS selections pass; current
leak scanning remains unavailable. The full native regression is running; the
search probe object needs a post-run refresh for its latest test-only fixture
edit before final integration is claimed. These are field-acceptance observations,
not complete module or stage acceptance.

The entire request/dynamic-search source bodies are reviewed; the missing
DynamicSearchError default source() is reproduced and repaired. Four actual
source Error observations retain the earlier 491 search inputs. Eight selected
native and MIPS/QEMU checks pass; native live Rust comparisons match all 495
search, 120 request API and 872 helper inputs. The combined sanitizer selection
also verifies the corrected SmartList traits. Its initial 14-check run failed
because LeakSanitizer cannot run under the new sandbox's tracing constraints.
ASan/UBSan is rerunning with leak scanning disabled and explicit arena-reclamation
assertions intact and passes all 14 checks in 175.21 seconds; this is not
successful leak-scan evidence. Native/MIPS live original comparisons match
all 495 search, 120 model and 872 helper observations. The complete native
consumer rebuild/regression is running. No request/search group or complete
stage is accepted from source reading or these selections alone.

The complete SmartList source table is reviewed, but a newly reproduced negative
equality capability boundary remains open. An isolated constrained-header
prototype passes positive/negative compilation; the production header remains
stable until the full consumer regression passed 281/281 in 87.32 seconds.
Production equality constraints and conditional Clone assertions were then
applied. A false move-only Clone declaration through span conversion was
reproduced and repaired by constraining the borrowed constructor; corrected
native/sanitizer/MIPS selections are building. The source-only business
clock test file is also reviewed in full, with fresh Rust 2/2, native compile/
CTest success and rebuilt sanitizer/MIPS named-check success. This complete
test-file group is accepted; the production clock/runtime module and complete
stages remain open. Seven groups are accepted and 48 remain partial. The
SmartList negative trait integration remains pending terminal verification.

The further SmartList review repairs consuming map/entity cleanup on exception
and binary32 Debug widening. All 1,291 native observations match live original
Rust, including no-Clone cleanup traces and nine f32 formatting modes. Eight
native selections and eight rebuilt MIPS/QEMU selections pass 8/8, retaining
existing double Value/format checks. All 1,291 native and MIPS observations
match live original Rust. Refreshed ASan/UBSan with leak detection passes 8/8
in 74.90 seconds. The restarted complete native consumer build/regression is
running. The preceding Boolean full rebuild was explicitly
interrupted for this batch. Complete module/stage acceptance remains open.

The continued SmartList review repairs owned iterator failure prefixes and
filter unwind/cleanup behavior, with isolated equality and callback observations.
The intermediate 595-case selection passes six native/sanitizer/MIPS checks.
Scoped typed Boolean storage then restores pointer/slice capabilities missing
from packed standard C++ vectors. All earlier observations are preserved in the
723-input corpus, including 128 new Boolean sequences. Six refreshed checks
pass 6/6 on native, ASan/UBSan with leak detection and MIPS/QEMU; all 723 native
and MIPS observations match live original Rust. The complete native consumer rebuild/
regression is running. The earlier full rebuild was explicitly stopped
to batch these changes. The list group remains partial pending terminal
integration and full-surface acceptance.

The complete core metadata/naming responsibility group is now accepted. All 344
inputs match the pinned original Rust oracle on native/MIPS, including isolated
field equality and naming boundaries. Two named checks pass 2/2 on native,
ASan/UBSan with leak detection and MIPS/QEMU; three unchanged original builder
tests pass. Long scoped copies remain valid after actual User deletion, and
self-aliased mask replacement retains the input. Six groups are accepted and 49
remain partial. Runtime metadata/schema/policy/privacy and stage acceptance remain
open. See [metadata acceptance](meta-api-acceptance.md#complete-source-surface-acceptance-2026-10-07).

All 110 Python C++ probe registrations now forward the configured emulator;
native commands have an empty prefix and MIPS commands explicitly select QEMU
74Kf. Eight rebuilt MIPS launch/format/value/search/runtime checks pass 8/8.
Audit-count inputs outside 32-bit usize have explicit rejection checks; exact
NaN bits preserve EntityKey input identity without altering production behavior.
The complete final native consumer/probe refresh passes 281/281 in 73.07 seconds.
The additional sanitizer refresh for changed probe tooling passed 5/5 in 15.76
seconds; complete MIPS and all-stage acceptance remain separate gates.

The complete core Business ID source review repairs the missing default Error
source() and adds per-field equality plus all error-code pairs. Native/MIPS
match 1,283 live-original inputs; four selected native/sanitizer/MIPS checks each
pass. The MIPS differential registration now forwards its configured emulator.
Its complete native consumer build and final probe/CMake refresh now pass
281/281 in 73.07 seconds. The core group is accepted; five groups are accepted
and 50 remain partial.
The preceding 281/281 runtime-error checkpoint below predates this header repair.

Whole-result completion adapters now preserve noncopyable errors through Context
query/list/observe/mutation, entity mutation, governance/graph-fix and graph/ledger
paths. The native 78-scenario fixture passes actual failure, cancellation and
transaction/authoritative-read cases. The runtime error module has a complete
source-surface review and accepted final integration. The rebuilt MIPS/QEMU
selection and ASan/UBSan with leak detection each pass 11/11; complete native
integration passes 281/281 in 67.65 seconds. Four groups are accepted and 51
remain partial. No stage is accepted solely from these new tests.

Enhanced-query/ID-page/relation adapters now preserve noncopyable executor errors
through thirteen completed-error transfers. The twelve-scenario native executor
fixture and seven related checks pass 8/8 under both ASan/UBSan with leak detection
and MIPS/QEMU. That full native consumer rebuild and regression subsequently
passes 280/280 in 66.80 seconds. The later whole-result changes above require
their own final verification. See the current runtime source acceptance and API
audit. No stage is closed by this integration.

The agreed memory implementation is complete within its kernel/Context scope.
The latest complete native consumer rebuild passes 279/279 checks in 67.99
seconds. The refreshed seven-check memory/lifetime selection passes on native,
ASan/UBSan with leak detection, and OpenWrt MIPS/QEMU. Application examples now
show checked User references and explicit Global delegation through UserContext.
See [the memory acceptance](context-memory-implementation.md#current-memory-verification-2026-10-07).
This completes the previously pending native refresh described below; it does
not complete the remaining module or stage audits. Earlier checkpoints below
retain their historical verification scope.

Full trim/Web/Xls source-surface review adds 145 Unicode trim cases and repairs
named owning-range projection without a Clone bound. Native/MIPS match 675 live
API inputs and the older 785 protocols; five selected sanitizer/MIPS checks pass.
Final native consumer and header-refresh verification passes 279/279 in 67.99
seconds. The full three-file source review is accepted; the module map now has
three accepted groups and 52 partial groups, plus two external exclusions and
one reference-only helper. Complete stage acceptance is still open.


Explicit future/stream completion taking and conditional adapter transfer now
remove blanket error-copy requirements from real SQL query/mutation/transaction/
stream paths. Native noncopyable integration tests pass, including diagnostic
ordering and exactly-once cleanup. Selected sanitizer/MIPS checks pass 5/5 after fixing the new fixture's local PMR
lifetimes. The merged full native rebuild is in progress. The earlier build was deliberately interrupted
to batch the foundational ownership fixes. Advanced runtime generic composition
and complete stage acceptance remain open.


Generic error carriers now retain move-only executor/transport payloads and
accurately declare conditional copies; source empty cause chains are explicit.
Initial scoped native/SAN/MIPS checks pass. Final selected builds and the full
546-task native consumer rebuild are in progress; no new full regression is
claimed yet. SQL adapters still clone const child error completions and require
separate consuming-path work. Complete stage acceptance remains open.


The preceding full native Debug checkpoint passed 277/277 in 61.52 seconds.
Entity-status now has complete public/default/Copy/Eq/Hash/Display/Debug acceptance.
Runtime/Context/data-service error Debug and nested SQL/check diagnostics are
restored; 1174 live-original inputs match native and MIPS. Four scoped checks pass
on native, ASan/UBSan and MIPS/QEMU. The error group remains partial because its
copy capability declaration does not follow noncopyable executor payloads.
Two groups are accepted, 53 remain partial, plus two external exclusions and
one test-only reference helper. Complete stage acceptance remains open.


The preceding full native Debug checkpoint passed 276/276 in 56.82 seconds.
The complete eval responsibility group now has accepted public/default/trait/
JSON/ownership review, with 1136 live-original inputs including all 529 state
pairs and generic native callback/codec/domain tests. Native/SAN/MIPS checks
pass. At that checkpoint accepted module status was distinct from stage acceptance: 54 groups remained
partial, two are excluded external adapters and one is a reference-only helper.
Continue the complete requirement audit; a passing source-test ledger alone
cannot close the stages.


The preceding full native Debug checkpoint passed 276/276 in 60.80 seconds.
The final two graph-reference source dispositions accept the documented Context
ownership adaptation with direct weak-authority/clone-promotion/identity/cyclic-
copy and complete teardown tests. The ledger is now 441 covered, zero pending,
five backend and five external exclusions. Source-test omission acceptance is
complete; complete APIs, defaults, privacy/provenance, lifetime composition and
six-stage acceptance remain open. See runtime-source-test-acceptance.md.


The preceding full native Debug checkpoint passed 275/275 in 66.76 seconds.
All 75 runtime-root source bodies now have reviewed dispositions. The final
fourteen preserve private test-helper aggregate/Soundex semantics, typed query
and optimistic mutation results, Context schema/identity/lease/provider contracts
and isolated-process output capture. Their three-check selection passes 3/3 on
native, ASan/UBSan with leak detection and MIPS/QEMU. Private helper coverage does
not expand the production in-memory engine. At that checkpoint the ledger was 439 covered,
two pending, five backend and five external exclusions. The remaining source
cases concern graph-owned weak references and clone promotion. Full API/default/
privacy/composition/lifecycle and complete stage acceptance remain open.


The preceding completed full native Debug checkpoint passed 272/272 checks.
Ten additional pagination contracts retain cursor boundaries, ordered ID reuse,
exact counts, fallbacks, overlapping build coalescing, real TTL expiry, shared-
store principal isolation and disappearing-entity page stability. Their source/
page/build/store selection passes 4/4 under ASan/UBSan with leak detection and
MIPS/QEMU; the full native run includes all four checks. At that checkpoint there were 61 reviewed
runtime roots and 14 root tests still pending, plus two graph-reference lifetime
adaptations. Native25ms work only signals readiness and is joined before teardown.
Six additional aggregate/cache contracts retain dynamic Value/JSON projection,
relation-count defaults, storage/property aliases, telemetry, namespaced write
invalidation and cache propagation. Their source/typed/store/enhancement selection
passes 4/4 under ASan/UBSan with leak detection and MIPS/QEMU; the full native run
includes all four checks. At that checkpoint, 51 runtime roots were reviewed and
24 root tests remained, plus two graph-reference lifetime adaptations.
Six additional typed-query contracts retain inverse-many Lists, actual module
identity-graph getters, immutable adjacency/local copy-on-write and exact identity/
version projection restoration. Their source/entity/registry/graph selection
passes 4/4 under ASan/UBSan with leak detection and MIPS/QEMU; the full native run
includes all four checks. At that checkpoint, 45 runtime roots were reviewed and
30 root tests remained, plus two graph-reference lifetime adaptations.
Eight additional event/order/relation contracts retain raw/safe mutation payloads,
complete FK ordering, required-cycle rejection, relation identity deduplication,
hydration telemetry and TOPN query shape. Their source/mutation/ledger/hydration
selection passes 4/4 under ASan/UBSan with leak detection and MIPS/QEMU; the full
native run includes all four checks. At that checkpoint, 39 runtime roots were
reviewed and 36 root tests remained, plus two graph-reference lifetime adaptations.
Eight further runtime-root service contracts cover typed optimistic conflicts,
Context telemetry, named entity binding, behavior-before-policy preparation,
all ten original public query rejection calls and exact generated/contextual
values. Fifteen additional validation contracts retain checker fixes, sparse
updates, nested required-field paths, loaded-version checks, attachment preflight
and language/catalog behavior. At that checkpoint, 31 runtime roots were reviewed
and 44 root tests remained. The validation/service/checker/ledger selection passes
4/4 on native, ASan/UBSan with leak detection and MIPS/QEMU after rebuilding.
The source/query/entity/mutation selection passes 4/4 on native, ASan/UBSan with
leak detection and MIPS/QEMU after rebuilding.
Eight runtime-root registration/resource source contracts now have direct reviewed
ports; the source group and existing registry/resource ownership checks pass 3/3
on native, ASan/UBSan and MIPS/QEMU. The unchanged original runtime passes 224
tests, with its subprocess-only file-log child fixture ignored in the parent run.
All 42 provider roots, eight entity-save and 17 round-trip-reference source
tests have individual reviewed dispositions. Reference source/ownership checks
pass 2/2 under ASan/UBSan and MIPS/QEMU. Four-writer ID tests retain 5000ms; the separate
eight-writer schema MIPS fixture uses its documented 30s test budget. Production
retains the source 5000ms default. Runtime/API/privacy/lifecycle/composition and
complete-stage acceptance remain separate.
The seven dedicated memory/lifetime checks pass on native, ASan/UBSan with leak
detection, and OpenWrt MIPS/QEMU. Checked references, exact independent bitmaps,
finite allocation/byte budgets, explicit Global delegation and payload storage
are implemented; see [memory acceptance](context-memory-implementation.md).

Core model audits now cover owning and borrowed generic capabilities, allocator
propagation, derived diagnostics, defaults, comparison and serialized models.
Runtime work includes query/cache/pagination orchestration, language/checker
interfaces, input and identity graphs, graph/ledger saving, mutation governance,
TransactionScope and audited-save call sites. The module map and API audit
record the exact evidence; implementation presence does not close their gates.

The original six stages above remain the objective. Continue requirement-by-
requirement acceptance across all 58 responsibility groups, including source
trait defaults and override dispatch, supported reexports, serializer behavior,
resource cancellation and end-to-end privacy/provenance. Do not change the
generator or add out-of-scope database, cloud or GPU implementations.

The per-source-test gate is now explicit in
[the disposition ledger](parity/source-test-dispositions.json), checked by
verify-source-test-dispositions.py. The latest ledger has 441 covered tests,
zero awaiting individual review, five backend exclusions and five external
exclusions. The strict source-test disposition check passes. This verifies
reviewed source-test coverage; it does not close the broader API/behavior audit. The six-stage
objective remains unchanged. The checkpoints below record the earlier counts.

On the next complete native consumer checkpoint, all 238 Debug checks passed
and the inline Decimal probe matched 484/484 live original inputs. The twelve
selected ASan/UBSan and MIPS/QEMU checks each passed. The grouped eighteen-type
core diagnostic repair has standalone native/MIPS 94/94 evidence and scoped
ownership acceptance in all three configurations. Its public facade and named
CTest checks are now integrated; full native acceptance passes 240/240 and the nine selected sanitizer/MIPS
checks pass in each configuration. Named native/MIPS probes match 94/94 live
original inputs. These counts do not close the remaining stage gates.

The subsequent per-test checkpoint has 45 reviewed source contracts and 388
applicable records awaiting individual review. SQL query/mutation diagnostics,
binding-policy tests and flat-graph/ledger tests have original and three-platform
scoped acceptance. The graph/ledger ports now retain strict Value variants and
optional presence rather than weakening source equality through converters.
This was the disposition count at that checkpoint.
Strict source-test completion and all-stage acceptance remain open.

The subsequent disposition review covered 61 original tests, leaving 372 applicable
records for individual review. Core dynamic search, real SQLite tenant search
and guarded mutation, business-clock override and business-ID permutation
contracts retain original fixtures and pass scoped three-platform checks.
Test assertions now preserve exact integer variants and complete fixture
consumption. The production implementation is unchanged at this checkpoint.
The full six-stage completion audit remains open.

The current disposition review covers 100 original tests, with 341 applicable
records still pending, two named backend exclusions, five external exclusions
and three unpublished reference tests. The inventory is corrected from 441 to
451 because six masking macro invocations expand into twelve runnable tests,
rather than the two templates previously indexed. The compiler's 35 masking
test names match the corrected index exactly. All 34 SQLite masking contracts
and the SQLite portions of five shared renderer tests have individual reviews;
the entirely PostgreSQL renderer/array tests remain outside database scope.
The rebuilt two-group selection passes on native, ASan/UBSan and MIPS/QEMU.
Strict source-test completion still fails on the 341 pending records; these
are uncompleted reviews, not an inferred count of implementation defects.

The next core audit found that `lib.rs` publicly declares eval after its test
body. Its former reference-only exclusion was incorrect. LoadState and generic
EvalResult are now implemented, preserving all variants, short-circuit callbacks,
path diagnostics, Debug/serde and explicit Context ownership. Native and MIPS
match 607/607 live original inputs, and both named checks pass on native,
ASan/UBSan and MIPS/QEMU. The three original eval tests now have reviewed
covered dispositions. The current ledger is 103 covered, 341 pending, two
backend exclusions and five external exclusions, with no unpublished test
exclusions. Strict source-test and six-stage completion remain open.

The subsequent complete native consumer rebuild passed 242/242 CTest checks
with the new public eval module integrated. Its two selected checks passed on
ASan/UBSan with leak detection and MIPS/QEMU. All six public core namespaces are
checked explicitly; compiled layer independence, pinned reference and formatting
checks also pass. Physical-device execution is still unverified.

The latest per-test checkpoint covers 120 source tests and leaves 324 pending,
with two backend and five external exclusions. Seventeen direct ports preserve
the original SafeExpression callback/default assertions, complete SmartList
merge vectors, trimmed optional serialization, WebResponse constructor/JSON
shape and six root expression/subquery contracts. The newly rebuilt named check
passes on native, ASan/UBSan with leak detection and MIPS/QEMU; unchanged Rust
passes all 65 core library tests. Production implementation and generator are
unchanged in this checkpoint. The full native 242-check result predates this
new test registration; no complete 243-check run is claimed here. All-stage
and strict source-test acceptance remain open.

The next entity/presentation checkpoint covers 135 source tests, leaving 309
pending, two backend and five external exclusions. Ten direct handwritten
mapping/dynamic/record fixtures and five existing presentation fixture reviews
retain original range errors, defaults, relation Object serialization, metadata,
registration, typed values and JSON. Presentation checks now preserve original
loaded inputs and validate serialized facets as well as the mutable map.
The paired checks pass after rebuilding on native, ASan/UBSan and MIPS/QEMU;
unchanged Rust passes 65/65 core tests. No generator or production dependency
changed. The prior full native 242-check result remains separate from these
later test additions; full 244-check and all-stage acceptance are not claimed.

The latest composition checkpoint covers 146 source tests, leaving 298 pending,
two backend and five external exclusions. All 26 root core lib.rs tests now
have individually reviewed dispositions, and all six query.rs validation tests
retain their original inputs and outcomes. The remaining root query/list/
expression composition fixtures invoke the actual production models and
adapters, preserve complete ordered outputs and optional/type distinctions,
and pass after rebuilding on native, ASan/UBSan and MIPS/QEMU. Unchanged Rust
passes 65/65 core tests. This is source-fixture acceptance, not complete core
API or stage acceptance. No full native 245-check rerun is claimed here.

The next scalar checkpoint covers 159 source tests, leaving 285 pending, two
backend and five external exclusions. All 73 indexed core tests now have
individual reviewed covered dispositions: 65 library and eight dynamic-search
integration tests. The new thirteen-test Value port preserves original numeric
boundaries, failed conversions, Decimal rounding, date/time formats and offset
normalization. Its native/sanitizer/MIPS check passes, and unchanged Rust passes
13/13 Value plus 8/8 dynamic-search integration tests. This closes the core
source-test review subset; public API/reexport/default/composition and overall
six-stage acceptance remain open.

The subsequent complete native consumer build and full regression passed
246/246 CTest checks, including all newly registered core source groups.
The new Value check passes separately under ASan/UBSan with leak detection and
MIPS/QEMU; compiled runtime/layer checks retain SQLite independence. The core
source-test subset is accepted, but remaining stage/API/default/privacy and
physical-device acceptance are still unproven.

The next layer-default/cursor checkpoint covers 171 source tests, leaving 273
pending, two backend and five external exclusions. Two data-service defaults,
three generic schema/dialect and seven diagnostic-stream fixtures now retain
original inputs, errors, terminal counts, cursor-release ordering, sink isolation
and consumer-unwind behavior. Generic dialect helpers adapt to SQLite; no
PostgreSQL implementation is implied. The rebuilt paired checks pass on native,
ASan/UBSan and MIPS/QEMU; unchanged Rust passes 2+3+7 tests. Data-service's two
indexed tests are reviewed, while its full SPI/default/composition and all-stage
acceptance remain separate gates. The prior full native 246-check result is
separate from these later registrations; no full 248-check run is claimed.

The executor-source checkpoint covers 183 tests, leaving 261 pending, two
backend and five external exclusions. All twelve indexed executor tests have
individually reviewed direct ports for descriptor-cache sharing, binding-policy
identity, statement metadata, private diagnostics, partition-probe selection
and scalar/list/complex-shape rebinding. The test-only array dialect retains the
original generic extension hook without implementing another native backend.
The rebuilt named contract passes on native, ASan/UBSan with leak detection and
MIPS/QEMU; unchanged Rust passes 12/12. Complete API/default/composition and
six-stage acceptance remain open.

Full native integration at the executor-source checkpoint passes 249/249 CTest
checks after requesting the complete consumer build. Layer validation confirms
the compiled runtime still has no SQLite C API calls. The executor's sanitizer
and MIPS results are selected checks, not full-platform regression evidence.

The entity-status checkpoint covers 199 source tests, leaving 245 pending,
two backend and five external exclusions. All sixteen original status tests
have individually reviewed direct ports. Action Display names and the complete
invalid-transition reason text are repaired; all valid/rejected transitions,
persistence/helper flags and lifecycle intermediate states retain their source
contracts. Restoring the old generic error fails a native negative control.
The new check passes after building on native, ASan/UBSan with leak detection
and MIPS/QEMU; unchanged Rust passes 16/16. Existing native core-model contracts
are rebuilt and pass. Complete stage/API/composition acceptance remains open.

Full native integration at the entity-status checkpoint passes 250/250 CTest
checks after rebuilding all affected consumers. The compiled runtime remains
independent of SQLite C API calls. Pinned-source, inventory/export-locator,
source-test registration and 524-file formatting checks pass. The sanitizer
and MIPS status checks are selected evidence, not full-platform acceptance.

The cache/relation checkpoint covers 203 source tests, leaving 241 pending,
two backend and five external exclusions. The four original aggregation-key
and relation top-N/telemetry tests have individually reviewed direct ports.
Exact typed predicates, predicate orders/comments/TTLs, result-affecting values,
provider-default and explicit-threshold behavior, safe attribute variants and
counts retain their source contracts. Explicit Global telemetry/key copies
survive actual source User deletion. Production implementation and dependencies
are unchanged. The prior full native 250-check result predates this added test
registration; no full 251-check regression is claimed here.

The cache/relation contract passes after rebuilding on native, ASan/UBSan with
leak detection and OpenWrt MIPS/QEMU. Unchanged Rust passes both helper and both
planner tests. Pinned source, inventory/export locators, registered source
dispositions and 525-file formatting checks pass; complete stage acceptance
remains open.

The in-memory source checkpoint covers 222 source tests, leaving 222 pending,
two backend and five external exclusions. All nineteen original engine tests
now have individual reviewed direct ports. Complete original sample rows,
exact aggregate Value variants, sort vectors, page/projection behavior and
expression inputs are preserved. Private LIKE tests use the production public
evaluator path. The default PMR resource is disabled and actual source User
deletion returns every engine allocation. The named native check and unchanged
Rust 19/19 pass. Production implementation and dependencies are unchanged;
complete stage acceptance remains open.

The in-memory source/ownership/differential selection passes 3/3 on native,
ASan/UBSan with leak detection and OpenWrt MIPS/QEMU. Rebuilt live Rust matches
6,532/6,532 observations on native and MIPS. Verification exposed and repaired
the Python runner's missing CMake emulator propagation; both pointer-width
detection and observation execution now use the configured CPU command.
The prior full native 250-check result is separate from these later test
registrations; no complete 252-check regression is claimed.

The checker/locale checkpoint covers 231 source tests, leaving 213 pending,
two backend and five external exclusions. Nine original tests now have
individually reviewed direct ports for ObjectLocation paths/escaping, wire
alias/JSON projection, status markers, language normalization and catalog
template validation/fallback. Both source JSON fixtures are byte-identical;
all fifteen canonical codes, twenty-four aliases and four unsupported codes
are checked. Explicit Global wire/catalog copies survive actual source User
deletion. The source/ownership selection passes 3/3 after building on native,
ASan/UBSan with leak detection and MIPS/QEMU; unchanged Rust passes 5+2+2 tests.
Production implementation, dependencies and generator are unchanged. The full
native 250-check checkpoint remains separate from these later registrations;
no complete 253-check regression or all-stage acceptance is claimed.

The formatter/telemetry checkpoint covers 238 source tests, leaving 206 pending,
two backend and five external exclusions. Seven original tests now have
individually reviewed direct ports preserving diagnostic/audit UTF-8 truncation,
ordinary/sensitive disclosure, raw-audit level gates, native type categories,
forbidden ID filtering and telemetry failure isolation. Actual asynchronous
business execution retains result 42 with one poll despite context activation
failure. User deletion releases every block and destroys the delegate once;
explicit Global ordinary output remains safe. The rebuilt source/ownership
selection passes 3/3 on native, ASan/UBSan with leak detection and MIPS/QEMU;
unchanged Rust passes 4+3. No production dependency or generator changed.
The full native 250-check evidence predates these later registrations; no
complete 254-check regression or all-stage acceptance is claimed.

The policy/graph-source checkpoint covers 245 source tests, leaving 199 pending,
two backend and five external exclusions. Seven original tests now have
individually reviewed direct ports preserving complete mutation-plan and policy
identity inputs, exact/mismatched approval, warning Result isolation and
deduplication, review denial, trace chains, ordered batches and grouped counts.
Explicit Global event/plan copies survive actual source User deletion. The
review persistence-counter gate is not transactional graph-save evidence.
The source/ownership selection passes 3/3 after building on native, ASan/UBSan
with leak detection and MIPS/QEMU; unchanged Rust passes 5+2. Production
implementation and dependencies are unchanged. The prior full native 250-check
result remains separate from these later registrations; complete 255-check,
full-platform and all-stage acceptance are not claimed.

The log-privacy source checkpoint covers 250 source tests, leaving 194 pending,
two backend and five external exclusions. Five indexed contracts now have
individually reviewed direct ports, including the ignored file child executed
by its parent in three fresh processes. Exact acknowledgement, immutable raw
events, credential exclusion, all mutation target traces and retained-readback
intent downgrade keep their original inputs/assertions. Explicit Global safe
events survive actual User deletion. The source/isolated-file/manager ownership
selection passes 3/3 on native, ASan/UBSan with leak detection and MIPS/QEMU;
unchanged Rust passes four tests with the child exercised by the parent.
Production implementation, dependencies and generator are unchanged. Complete
API/default/composition and all-stage acceptance remain open.

Full native integration at the log-privacy checkpoint passes 257/257 CTest
checks after requesting the complete consumer build. This includes every newly
registered source-contract group since the prior 250-check checkpoint. Compiled
runtime/layer validation still confirms no SQLite C API calls in the runtime.
Sanitizer and MIPS privacy results remain selected evidence; full-platform and
complete six-stage acceptance remain separate uncompleted gates.

The Context-source checkpoint covers 256 tests, leaving 188 pending, two backend
and five external exclusions. Six original tests now have individually reviewed
direct ports for typed roots, independent SQL switches, disabled recording,
complete projected/buffered SQL, multi-level intent traces and fresh ledgers on
Context reuse. Review found and repaired the missing public entity_runtime_state
factory. Its lazy User-owned graph anchor shares immutable identity data while
each call receives a fresh ledger. Closed/quota behavior and explicit Global
state/log retention after actual User deletion are checked. The native source
contract passes; five selected source/entity/memory/logging checks pass after
rebuilding on ASan/UBSan and MIPS/QEMU. Unchanged Rust passes 1+4+1. An existing
SIZE_MAX rejection test now feeds the same bound at runtime, avoiding a GCC
compile-time allocation-size warning without suppressing warnings.

The complete native consumer rebuild for the Context layout change finished
and passed 258/258 checks. Full-platform and all-stage acceptance remain open.
No production dependency or generator changed.

The entity-runtime source checkpoint covers 269 tests, leaving 175 pending,
two backend and five external exclusions. Thirteen original composition/lazy
tests now have individually reviewed direct ports with exact typed keys/values,
complete error payloads, all 512x32 randomized trials, retryable source intent,
lazy allocation and every loaded/empty/unloaded relation state. At that historical checkpoint, two Arc ownership tests were pending review.
Their current adaptation verdict is recorded in the resume checkpoint above. The named target passes after rebuilding under ASan/UBSan and MIPS/QEMU;
unchanged Rust passes all fifteen source tests. These fixtures add no production
runtime behavior, dependency or generator changes.

The native entity-runtime target was refreshed after the complete consumer
rebuild, including all fixture additions. The paired entity/Context selection
passes 2/2 on native, ASan/UBSan and MIPS/QEMU. The Context fixture also retains
the source's shared query/mutation timestamps. The latest complete native
regression then passes 258/258. Full API/lifecycle/composition and stage gates
remain open. The two Arc adaptations were subsequently reviewed and are
revalidated in the current resume checkpoint above.

## Header dependency coherence checkpoint, 2026-10-08

The named seven-check MIPS wide-time/logging selection passes 7/7. Its sanitizer
counterpart passes six but detects a concrete stale-layout write in Context
logging: an 800-byte SqlLogEntry clone targets a 784-byte compiled stack object.
The old audit_privacy object has zero compiler dependency records; ordinary
incremental rebuilding reports no work despite newer model headers. A clean
sanitizer rebuild is running and must pass logging again before acceptance.

The new dependency checker also locates six compiled native objects and one
MIPS object with empty dependency records. These must be regenerated after the
live native build is terminal, without restarting or mutating its active outputs.
A clean dependency verdict is an artifact-coherence gate, not semantic parity.
All source modifications remain preserved. Full stage acceptance is unchanged.

The clean sanitizer recovery passes all seven selected wide-time/logging checks
(6.43 seconds) with nonempty dependency records for all 94 compiled objects.
MIPS passes seven checks, matches all 1,232 live original arithmetic inputs and
regenerates its one empty-record object; its recovered entity-expression check
also passes. Native refresh is still live; regenerate its six empty-record
objects after completion, then run current complete regression. Standard-time
hashing is implemented; typed reversed-duration error payload mapping remains
an explicit source capability gap. Stage acceptance remains open.

## Reference API follow-up, 2026-10-08

The complete portable/context-bound reference source review restores missing
model Debug and both empty Error causes. Dedicated writers preserve all source
fields/variants, redacted key output, stateless wire Debug and UTC fractions,
leap seconds and extended years. The same umbrella compile succeeds after its
negative reproduction. All 229 new original model inputs agree across two
runs; existing 548/933 semantic corpora remain unchanged. New scoped retention
contracts test actual User deletion, stale principal rejection and nested
Global allocator propagation with default PMR disabled. A complete native
599-task consumer pass and fresh sanitizer/MIPS reference targets are running.
Current whole-native regression is pending; 318/318 remains the earlier full
checkpoint. The reference group and stages 4–6 remain open.

The fresh reference selections finish and pass ASan/UBSan 12/12 in 16.33 seconds
with leak scanning disabled and MIPS/QEMU 12/12 in 9.21 seconds. All 229 new
model observations and unchanged 548/933 token/runtime observations match live
original Rust on MIPS. A fixture number-reader mismatch is corrected before
final checks. Native model ownership passes 1/1; whole-native consumer work
remains running and requires a final probe refresh and 320-check regression.
No source group or stage is accepted from the selections alone.

## Context default/facade follow-up, 2026-10-08

Direct source review reproduces a nanosecond/microsecond mismatch in default
trace IDs, omitted timezone get/set/builder methods and the omitted with_module
Context facade. The repairs retain original units/defaults/unvalidated text and
delegate to the existing independent module installer. Seventy-five direct
original public default/setter/module inputs agree across repeated executions;
new ownership checks include mutable timezone source snapshots, closed access,
explicit Global retention and module copies surviving source User deletion.

The old consumer pass is superseded and explicitly interrupted after the
Context layout changes. Fresh stable native and selected sanitizer/MIPS builds
are now active, with all reference consumers included. Complete native 322-check
integration and the reference/Context source gates remain pending. Source status
is still 28 accepted / 27 partial; stages 4–6 remain open.

The stable seventeen-target builds finish on both cross profiles and repeat
no-work checks precede ASan/UBSan 21/21 (17.05 seconds, leak scanning disabled)
and MIPS/QEMU 21/21 (13.22 seconds). They include all current Context defaults,
resource/model/store/tail and reference consumers. Native defaults/reference
models pass 4/4 in 1.07 seconds. The stable 602-task whole-native build remains
active, so its full 322-check regression and source/stage acceptance stay open.

## Language and entity-runtime review follow-up, 2026-10-08

Fresh current language/catalog consumers pass 4/4 on sanitizer (11.10 seconds,
leak scanning disabled) and MIPS/QEMU (7.20 seconds), including original locale/
checker source tests. All 4,575 translation and sixty diagnostic MIPS observations
match live original Rust; repeat builds report no work and source hashes match.
Complete current whole-native regression remains pending, so the source group
stays partial.

The complete entity-runtime production review through line 1035 reproduces
missing structural diagnostics on keys, relations, graph builders, change sets,
state and composition errors. An actual original oracle records 252 repeated,
nine-format model/state observations, including public key mutation with its
private identity snapshot preserved. Diagnostic implementation and native
counterparts remain required. No running native-build input is modified while
collecting this evidence. The ledger remains 28 accepted / 27 partial; stages
4–6 and complete Context/reference acceptance remain open.

## Reference/language acceptance and entity-model repair, 2026-10-08

The stable complete consumer build finishes all 602 tasks, reports no work and
passes 322/322 in 119.11 seconds with 390 nonempty dependency records. Complete
reference and language/catalog source contracts are accepted after their full
production/default/trait/protocol/ownership reviews and current sanitizer/MIPS
selections/live original observations. The ledger is 30 accepted / 25 partial.
Complete Context and stages 4–6 remain open.

The entity-runtime diagnostics repair is first compiled in an isolated staged
tree while that full build runs. All 252 original model observations match in
nine modes; lifetime/nonmaterialization contracts pass native and ASan/UBSan
with leak scanning disabled. It preserves cached key identity, complete graph/
ledger/private state structure, weak roles and explicit Global copies. The new
error_source() adapter retains the conflict payload field source.

After the 322-check regression is terminal, the staged repair is installed and
formatted. New model/protocol checks are registered; complete native consumer
and current sanitizer/MIPS builds begin from this stable source snapshot. Full
324-check regression remains pending, so the entity-runtime group stays partial.

The installed entity-runtime selections pass 6/6 on sanitizer (21.17 seconds,
leak scanning disabled) and MIPS/QEMU (7.21 seconds), including original ledger/
graph-lifetime tests and existing behavior. All 252 live original MIPS model
comparisons match after the fixture supplies identical NaN IEEE bits; native
identity/output semantics are unchanged. Whole-native work and a final corrected
probe refresh/full 324-check regression remain pending.

The complete runtime Business ID/internal ID production review additionally
reproduces seven missing Debug derives and records 82 twice-agreeing actual
original model/transition observations. Native diagnostic repair is pending;
no running header/implementation input is changed during this collection.
See runtime-id-api-contract.md. The ledger remains 30 accepted / 25 partial.

## Runtime ID staged diagnostic repair, 2026-10-08

The separate runtime ID model repair now matches 82/82 live original Debug
observations and passes its standalone lifecycle executable. A reviewable patch
is saved with the evidence; running production build inputs are unchanged.
Original Debug and Release models are independently observed twice, with nine
counter-overflow cases differing by source build profile. Standalone sanitizer
and MIPS checks finish successfully: lifecycle contracts pass and both probes
match 82/82 live original cases for their corresponding profiles. Leak scanning
is disabled for ASan/UBSan; physical-router execution is unverified.
Production/CMake integration and source acceptance
remain open; the ledger remains 30 accepted / 25 partial.

## Cache and pagination staged diagnostic repair, 2026-10-08

Complete cache/pagination production review reproduces three missing Debug
derives. A separately staged repair preserves every cursor/retained-ID field,
private cache timestamp and actual rows. Native, ASan/UBSan and MIPS/QEMU each
match 74/74 live original cases and pass default-PMR-disabled ownership/clock
contracts. Populated cache timestamps use actual monotonic put brackets, with
no substitution or normalization. Evidence and a reviewable integration patch
are saved in measurements/parity/2026-10-08-query-store-model-review.
Production/CMake integration and complete source acceptance remain open; the
ledger remains 30 accepted / 25 partial.

## Typed transaction staged ownership repair, 2026-10-08

Complete transaction source review reproduces accidental copy requirements for
callback outputs and provider errors. The staged repair accepts move-only outputs
through suspended commit and errors through all five execution phases, without
changing copyable result borrows. Native, ASan/UBSan and MIPS/QEMU each match
16/16 actual original non-Clone calls and pass output/error cleanup contracts
with default PMR disabled. The expanded original oracle preserves all 1,288
existing transaction observations against current native production; a broader
staged native probe is being refreshed. Current entity-runtime whole build is
terminal-successful; final corrected-probe/no-work/full-324-check regression
is active. Production/CMake repairs and complete source acceptance stay open;
the ledger remains 30 accepted / 25 partial.

## Entity-runtime source acceptance, 2026-10-08

Final native integration passes 324/324 in 151.69 seconds after the corrected
NaN probe refresh and post-test no-work; all 392 dependency records are valid
and nonempty. Complete entity-runtime source review accepts all public/default/
trait/protocol/ownership responsibilities within explicit Context adaptations.
Current sanitizer/MIPS six-check selections pass and all 252 current models
match live original Rust on native/MIPS. The ledger is31 accepted / 24 partial.
Stages 4–6 remain open. The runtime-ID, cache/pagination and typed-transaction
repairs are validated separately and are next for production/CMake integration.

## Registered MIPS batch and noncopyable ledger review, 2026-10-08

The installed runtime-ID/cache/transaction batch finishes its selected MIPS
build, reports no work on refresh, and passes 17/17 registered checks in 12.20
seconds. Live MIPS models match82/82 runtime IDs,74/74 query stores and 16/16
noncopyable transaction outcomes. Native331 and registered sanitizer consumers
remain active. A separate actual noncopyable LedgerEntity compile identifies
an unconstrained audited result copy constructor. A staged capability constraint
matches356/356 live original non-Clone saves and passes default-PMR-disabled
SQLite commit/rollback/ledger/User-reclamation contracts. Its cross-target and
installed integration remain pending; the ledger stays31 accepted / 24 partial.

## Installed cross-target batch validation, 2026-10-08

Registered sanitizer and MIPS selections pass17/17 in13.69/12.20 seconds,
respectively, after no-work refreshes. The sanitizer profile disables leak
scanning and keeps UBSan halt-on-error. All1,288 existing transaction cases
also match live original Rust on MIPS. Full native331 integration remains
active. The additional staged noncopyable ledger capability constraint now
matches356/356 original saves and passes real SQLite commit/rollback/lifetime
contracts on native, sanitizer and MIPS; production/CMake integration is pending.
The acceptance ledger remains31 accepted /24 partial.

## Mutation policy staged model repair, 2026-10-08

Complete production review reproduces thirteen missing Debug derives and policy
identity Hash. The separate staged repair preserves all source model fields,
validation/ordering/hash equality behavior and raw diagnostic options. Native,
ASan/UBSan and MIPS/QEMU each match 177/177 live original inputs and pass nested
Global retention/hash/stale-reference contracts with default PMR disabled.
The reviewed patch remains outside the active native331 inputs. Production/
CMake and full policy/save/privacy acceptance remain open; the ledger stays
31 accepted / 24 partial.

## Raw and safe event staged diagnostics, 2026-10-08

Complete event source review reproduces six missing derived diagnostics. A
separate staged repair preserves all raw/safe fields and nested trace/bootstrap/
governance members, reusing the staged policy diagnostics. Original factories
and actual safe projection yield 112 twice-agreeing cases in nine modes; native,
ASan/UBSan and MIPS/QEMU each match112/112 live observations and pass independent
raw/safe Global retention, credential-redaction and stale-reference contracts
with default PMR disabled. Production/CMake/full save/privacy acceptance remains
open; the current native331 build continues, and the ledger stays31 accepted
/24 partial.

## Checker model staged diagnostic and wire repair, 2026-10-08

Complete checker production review reproduces five missing derived diagnostics.
A staged repair supplies status/rule/naming and complete wire segment/result
writers while preserving existing path/serialization/typed-checker code.
Native, ASan/UBSan and MIPS/QEMU each match 220/220 live original model and
serde-wire observations, and pass explicit Global nested Value/JSON/location
retention and stale-reference contracts with default PMR disabled. The original
typed adapter's Clone requirement stays intact. Production/CMake/full Context/
checker/save acceptance remains open; the ledger remains 31 accepted /24 partial.

## Recursive relation plan staged model repair, 2026-10-08

A public source-model review reproduces missing RelationLoadPlan Debug. The
staged writer preserves all nine fields, full optional SelectQuery and child
plans. Native, ASan/UBSan and MIPS/QEMU each match 162/162 live original query/
Clone/PartialEq/field-difference observations in nine modes and pass explicit
Global recursive query/value retention and stale-reference contracts with
default PMR disabled. Larger relation execution/body-to-body/telemetry/query-
privacy and production/CMake acceptance remain separate; the native331 build
continues and the ledger stays 31 accepted / 24 partial.

## Runtime log model staged diagnostic repair, 2026-10-08

A public compile reproduces seven missing source-derived diagnostics. The
staged repair preserves complete SQL provenance/params/times/duration/result
data and both unified payload variants. Native, ASan/UBSan and MIPS/QEMU each
match 172/172 live original models in nine modes and pass default-PMR-disabled
Global buffer/nested ownership/stale-reference tests. Raw Debug and safe/sensitive
publication remain independent; projection/routing are unchanged. Complete
privacy-body/call-site and production/CMake acceptance remain open. The native
331 build continues; the ledger remains 31 accepted / 24 partial.


## Stage 4 completion audit, accepted 2026-10-09

All fourteen assigned source groups have accepted full public/private/default/
trait/protocol/ownership reviews: Context, registry/module, six-file runtime
services, relations, cache/pagination, Context locking, entity runtime, errors,
in-memory engine, generated support, business/ID services, references, language/
catalog and telemetry. Declaration/test ledgers remain omission indices; they
were not used as a completion shortcut. The API audit and per-source contracts
retain the actual source responsibilities and documented C++ adaptations.

The final six-file service review closes its distinct base/Context/entity/metadata
interfaces, query preparation, basic/enhanced/stream/aggregate execution,
cache/cursor/ID-set coordination, recursive identity hydration, concrete generic
error/output propagation, foreign-child cleanup and mutation publication. The
cursor profile and missing private typed-aggregate reproductions are repaired.
No enhanced-query behavior replaces the narrower typed-aggregate helper.

Native378/378 follows338 final consumer tasks with420 nonempty dependency
records and no-work before/after. ASan/UBSan and MIPS/QEMU each pass56/56 after
62-task selected builds, plus independent base/stream3/3. Leak scanning is
disabled. Thirteen live original matrices agree on both profiles, including
1502 mutation,193 stream,154 enhanced and48 exact private helper observations.
The key fixture explicitly adapts native-sized input fields to32 bits; it does
not claim Rust execution on the router. Controlled helper rows are separate
from actual public row-runtime comparisons. Actual User reclamation and explicit
Global retention preserve every nested allocator and checked expiry boundary.

These results close Stage4 within the explicit Context/owner-thread adaptation
contract. Full graph/save models and execution, governance/audit saving integration,
runtime-root production/test support and final examples/target acceptance remain
Stages5–6 responsibilities. Physical WNDR4300 execution is explicitly deferred;
no generator or non-SQLite/runtime dependency expansion is included.


## Retained save adapter follow-up, 2026-10-10

Six original internal typed/core-graph save entry boundaries are now restored
without changing the entity generator or dependencies. Status-only skipping,
commented status persistence, deleted child clearing, explicit Create/comment
ordering and core graph conversion remain distinct. Two unchanged original runs
agree on 541 complete getter/result/provider/query/event observations; native,
ASan/UBSan and MIPS/QEMU match all 541 live. The three relevant checks pass on
each profile, with default-PMR-disabled lifecycle contracts, actual User close,
pending wake/cancel forwarding and explicitly retained Global snapshots.
Post-test selected builds report no work; sanitizer leak scanning is disabled.

The earlier full native 381-check checkpoint predates these edits. 383 checks
are registered; complete current consumer regression is pending. The ledger
stays 53 accepted / 2 partial. Stage 5 still needs retained recursive insert/
upsert/reference/remove/update/deletion source responsibilities and complete
saving composition; Stage 6 retains its independent final gates. Evidence is
stored in measurements/parity/2026-10-10-graph-save-adapter-review. Physical-device
execution remains explicitly deferred.


## Graph node boundary follow-up, 2026-10-10

The subsequent node-command follow-up restores reference/remove validation,
checker update construction and scoped deletion. All 1620 live original inputs
match on native, ASan/UBSan and MIPS/QEMU; each scoped selection passes 4/4 with
post-test no-work. Lifecycle contracts destroy a foreign parent-scope Context,
retain copied provenance, quiesce pending reads on actual User close and rebind
explicit Global authoritative references. Recursive insert/upsert and final
composition remain open. 385 tests are registered; the earlier full 381-check
snapshot predates these follow-ups. Evidence is stored in
measurements/parity/2026-10-10-graph-node-command-review.

The ledger stays 53 accepted / 2 partial; stages 5–6 remain open. The production
generator and dependencies are unchanged; physical-device testing stays deferred.


## Retained recursive write follow-up, 2026-10-10

The recursive write follow-up implements both retained insert/upsert routes.
792 live original observations match on native, ASan/UBSan and MIPS/QEMU; each
five-check selection passes after correcting a test-fixture ownership error.
Real SQLite foreign keys/triggers prove write order, authoritative reads and
complete caller rollback. Pending nested work quiesces before actual User
reclamation; explicit Global recursive copies survive. All listed graph/save
entry boundaries are implemented. Complete native 387-check consumers and a
44-check sanitizer/MIPS graph/source-composition selection are in progress;
53 accepted / 2 partial and stages 5–6 remain unchanged until final acceptance.
Evidence: measurements/parity/2026-10-10-recursive-graph-write-review.


## Stage 5 completion audit, accepted 2026-10-10

All six assigned source groups are accepted: checker/results/locations,
runtime policy/approval/governance, Context transaction scopes, raw/safe events/
sinks, logging/privacy/formatters and complete graph/save models/execution. Current graph/source composition preserves separate graph/ledger/
transaction, generated/snapshot/mutation, checker/fix/policy and raw/safe log roles.
Every retained repository entry is restored; event publication remains at source
call sites, approval precedes saver transaction begin and authoritative reads occur
inside the transaction before commit. Caller-owned saving does not implicitly
commit/rollback/clear intent. Native resources stop/join before User reclamation.

The final native 344-task build plus two-task ownership-fixture refresh passes
387/387 at 79.85 seconds, with 426 nonempty object dependency records and no-work
before/after testing. ASan/UBSan and MIPS/QEMU each pass 44/44 after completed
61-task selected builds and fixture refreshes, with post-test no-work. ASan leak
scanning is disabled. New actual source matrices128/141/541/1620/792 and older
plan/execution/ledger/saver/audited/fix/privacy observations remain covered.
Real SQLite foreign-key/trigger ordering, dirty/reference/version behavior,
authoritative rereads, full rollback, pending cancellation and explicit Global
reconstruction after actual User deletion are asserted. Before-ASan/QEMU fixture
failures remain recorded; source outputs are not weakened to hide them.

See measurements/parity/2026-10-10-runtime-graph-source-acceptance and the complete
runtime-graph-save API contract. This closes Stage 5 and leaves the ledger at
54 accepted / 1 partial. Stage 6 still requires complete runtime-root/final test/
example/target acceptance. The generator and dependencies remain unchanged;
physical WNDR4300 execution remains explicitly deferred.


## Runtime root final review, 2026-10-10

The runtime-root final review restores four missing published module namespaces
with all member bindings and extends the export verifier. The current native
namespace refresh passes 388/388 after 44 consumer build tasks, with 427 nonempty
dependency records and no-work before/after. All75 original root tests have
reviewed direct ports; unchanged Rust and live native examples remain green.
Full sanitizer and matched SQLite3.53.2 MIPS/QEMU validation is running. Stage6
and the final runtime-root group remain open until that evidence completes.
