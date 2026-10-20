# Context API and lifecycle review

The [complete 5.0.6 Context review](context_source_review_5_0_6.md) now accepts
UC01-UC25 with its own current-source, native/imported and reused lifecycle
evidence, including the separately qualified
[dynamic schema repair](context_dynamic_schema_lifecycle_5_0_6.md). The review
below remains a historical 5.0.5 checkpoint; its evidence is not relabeled.

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-runtime/src/context.rs`. The complete Context source group is accepted
after the final review and validation recorded below. Earlier partial/running
paragraphs retain the repair history.
Stage 4 retains the full source responsibilities; its API cannot be reduced to
an allocator or SQLite session. Stage 5 policies/privacy and final Stage 6
acceptance retain their independent gates.

## Production-body responsibility review

The review reads production lines 1–83 and 103–1266, excluding the embedded active
root test module and the later logging/reuse test modules. Models, private
schema-invocation proof and callbacks, default Context state, DataStore, typed
resource/metadata/service installation, root/identity graph, references, IDs,
language/checkers/fix evidence, event routing and store delegation remain distinct.

| Source responsibility | Native mapping and remaining verification |
| --- | --- |
| ContextEntityRef / ContextRootError | Full fields, allocator-aware Clone, equality, raw Debug, exact Display and empty error cause. Active-root assignment/canonical matching and bootstrap idempotence remain Context methods. |
| FixEvidenceSource / FixEvidence | Separate Clock/Context provenance, all fields/Clone/equality/raw Debug and constructor validation ordering. Public-field mutation and subsequent Clone do not rerun constructor validation. |
| Default/new UserContext | Explicit Global owner replaces standalone implicit owners. Empty stores/registries/graphs, logging switches, identity strings, clocks, English language, builtin catalog and Noop telemetry preserve their semantic defaults. Native thread identity differs from Rust private ThreadId numbering. |
| Typed/named resources and locals | Distinct registration/lookup/require and Value locals/removal. Origins are explicit; retained nested values require destination copies. Arena replacement does not imply early resource destruction. |
| Runtime state and metadata | Shared immutable graph anchor with fresh mutation state, separate root/initial graph storage, descriptor and registry access, four decoder delegations and behavior/request-policy registration. |
| Schema and ID services | Private SchemaInvocation, ordered provider/business-schema/generated bootstrap futures, floor reservation before mark_as_new, optional ID generation and explicit Global fallback. |
| References | Independent codec, round-trip runtime and trusted principal resources; require configuration, enforce mode before wire serialization and delegate issue/consume through the installed runtime. |
| Language/check/fix | Separate catalog/language settings, checker-first fixes and metadata-required checks, translated errors, operation-local fix time/evidence and independent last-evidence snapshots. |
| Telemetry/event/log routing | Explicit telemetry registration, failure-isolated operations, governance/bootstrap attachment, ordered raw/safe/LogManager routes and completion classification. Full policy/privacy acceptance remains Stage 5. |
| DataStore | Separate get/put/remove futures and default in-memory TTL semantics; Context lazily selects the resource and treats absent stores as no-ops. Nested future ownership and cancellation remain explicit. |

Context-owned boxes/Arc/OnceLock/locks become explicit scoped objects, checked
resource borrows and owner-thread operations. Quiescence protects schema/native
work before arena reclamation. Runtime activation pointers borrow only during
poll/cancel; no singleton, thread-local cache or shared owner extends User data.
Application business access receives only UserContext and delegates Global
allocation through it. These adaptations preserve separate service interfaces.

## Reproduced model protocol gaps

A public umbrella compile check reproduces missing Debug for ContextEntityRef,
ContextRootError and FixEvidence, plus the missing ContextRootError source()
method. context_model_debug.hpp now supplies complete structural writers for
these models and FixEvidenceSource. ContextRootError inherits the original
empty cause contract through a nonthrowing nullptr accessor. Raw Debug is not a
privacy projection or a new serialization protocol.

The independent oracle calls actual pinned constructors/derives/Error methods.
All 234 inputs agree across two original executions and match current native
C++: nine formatter modes, root None/Some/full-width IDs, Clone/equality, exact
Display/source, evidence sources and fields, Unicode/control/long strings,
validation precedence and public-field mutation followed by Clone. The corpus
is deterministic compressed JSON; no source behavior algorithm is duplicated.

Native lifecycle contracts disable default PMR, retain complete models and raw
diagnostics through User's Global proxy, close/delete User, reject a stale root
reference and inspect every nested destination allocator and retained output.
Local owning strings are registered in User before close. This exercises actual
cleanup and explicit retention, not merely a text comparison.

## Reproduced DataStore foreign-work cleanup gap

ContextDataStoreOperation previously discarded a returned child pointer when
its resource differed from the caller's Context. The child could still hold a
borrowed key from User. A controlled provider returns Global work retaining that
key; the negative control fails the required pre-reclamation cancellation check.
The fixture explicitly cancels the failed negative control before unwinding,
so the reproduction itself does not read freed User memory.

The adapter now cancels returned foreign work before discarding its pointer and
throwing. Cleanup observes the live key, the child becomes Cancelled exactly
once, and actual User deletion leaves no deferred borrowed access. The unchanged
normal pending/wake/cancel and public TTL/no-op semantics remain separate tests.

## Schema foreign-child cleanup follow-up

The same returned-work review reproduces a missing cleanup in SchemaOperation:
its provider/bootstrap factory result is checked for the correct Context, but
an invalid child was previously discarded before cancellation. Controlled
Global children retain the original UserContext, and the negative control fails
the required live-owner cancellation assertion. The fixture explicitly cleans
up its failed negative control before unwinding.

check_child now cancels the returned work before discarding it and throwing.
Provider cancellation runs in ordinary mode; bootstrap cancellation runs within
the already active bootstrap mode. Both then restore activation and release the
pending schema borrow, so provider replacement and actual User deletion remain
possible. This does not cancel a caller-owned operation passed to an unrelated
scope wrapper; it cleans up invalid work returned by the schema factories.
Fresh schema lifecycle/source-parity selections pass ASan/UBSan 2/2 in
0.97 seconds with leak scanning disabled and MIPS/QEMU 2/2 in 0.58 seconds.
Both selected builds finish before CTest; the original 384-input corpus remains
unchanged alongside the new foreign provider/bootstrap cancellation cases.
The complete native build will be refreshed after its running pass finishes.

## Current verification and remaining gates

The current native four-check model/store selection passes 4/4 in 1.06 seconds,
including all 234 new model observations and all 251 existing store observations.
Fresh selected builds precede ASan/UBSan 4/4 in 5.51 seconds (leak scanning
disabled) and MIPS/QEMU 4/4 in 3.78 seconds. All 234 MIPS model inputs also match
live original Rust. The complete 552-task native consumer refresh remains running.
The earlier 315/315 result predates the Context header and cleanup changes and
cannot establish their final integration. Complete Context source-group and
Stage 4 acceptance remain open; later service/reference/schema/checker/telemetry/
event composition must retain their full independent contracts.

## Live composition evidence after the schema follow-up

The fresh MIPS schema probe matches all 384 live original observations after
the cancellation repair. The unchanged native Context resource probe matches
410 live original scripts; event routing matches 1,088; internal IDs match 171;
and business clock/service installation matches 859. These observations invoke
actual pinned methods rather than copied orchestration algorithms.

Nineteen selected composition targets are being rebuilt in sanitizer/MIPS
directories, covering resources, schema, event routing, internal/business IDs,
original Context/tail contracts, decoder delegation, graph-fix scope and language.
The complete native consumer pass remains active and must be followed by a
final schema refresh and full regression. No complete Context/Stage 4 acceptance
is inferred from the live comparisons or focused cleanup tests alone.

The broader nineteen-target builds finish before both twenty-three-check
composition selections: ASan/UBSan passes 23/23 in 45.25 seconds with leak scanning
disabled, and MIPS/QEMU passes 23/23 in 28.07 seconds. These include resources,
schema, event routing, IDs, original Context/tail, decoding, fix scope and
language, plus the new model/store checks. Telemetry's separate factory/output
capability follow-up remains under verification; complete native regression and
Context source acceptance remain open.

The initial 552-task complete native pass finishes successfully. A final
294-task refresh is now running for the schema/telemetry implementation, its
noncopyable-output template and affected consumers. Complete regression remains
pending until that pass is terminal and a follow-up build confirms stability.
The telemetry follow-up's three-check selections pass sanitizer/MIPS and all
1,032 activation plus 110 model observations match live original Rust on MIPS.

The final native consumer refresh completes, followed by a four-task purpose
wrapper refresh and a terminal no-work build. All 318 registered native checks
pass in 133.86 seconds against this current state. All 386 compiled dependency
records are nonempty. Context composition selections remain 23/23 on
sanitizer/MIPS; telemetry and purpose support have their own completed reviews.
The complete Context source group stays partial pending its remaining
reference/service/default integration audit. Earlier running paragraphs describe
intermediate checkpoints.

## Default trace, timezone and module facade gaps, 2026-10-08

The production default review finds three additional gaps. The default trace
suffix used epoch nanoseconds rather than the source's hexadecimal microseconds.
An independent before/after microsecond clock bracket fails before the repair;
the portable full-width word calculation now multiplies seconds by one million
and adds truncated subsecond microseconds. Negative epoch time still maps to zero,
as the source's failed duration_since fallback does.

Public timezone(), set_timezone() and with_timezone() were absent; a direct
compile reproduces all three omissions. The restored getter defaults to Some
UTC, setters copy exact text into User, and the fluent builder returns the same
User. Empty, whitespace and arbitrary timezone strings are retained without
new validation. Getter/setter access uses the Context lifetime gates. Explicit
Global retention remains an application choice.

The public with_module() facade was also absent despite the separate module
installation being present. Its native counterpart delegates RuntimeModule.apply_to
and returns the invoking User; it preserves independent descriptor/entity/behavior/
checker/sink/decoder/bootstrap and graph snapshots. A source User can be deleted
after installation into a different User; no source module borrow is retained.

The new independent context_defaults oracle observes actual original public
construction, getters, setters and with_module. All 75 inputs agree across two
runs, including the microsecond clock bracket, PID/thread consistency, initial
empty stores/graphs/logs, English language, Noop telemetry, default UTC, optional
identifiers, exact timezone/trace replacement and module entity/language/service
installation. Absolute timestamps are not captured or normalized; native thread
identity retains the documented platform adaptation. Native ownership tests
add mutable-source snapshots, closed access rejection, actual User destruction
and explicit Global timezone retention with default PMR disabled.

Current selected sanitizer/MIPS consumers are rebuilding. The complete native
reference consumer pass remains active and needs a final refresh for these
Context changes and the corrected reference probe before full 322-check CTest.
The earlier 318/318 and reference-only cross results predate this ABI/header
change; complete Context/reference source-group acceptance remains open.

The changing Context layout invalidated the in-flight reference consumer pass.
That pass was deliberately interrupted at its known live handle; one selected
link failure also exposed its stale pre-facade schema object. After all affected
passes are terminal, the Context/identity headers and schema implementation are
invalidated and fresh stable native/sanitizer/MIPS builds start. These include
all reference consumers affected by the ABI, plus defaults, resource/model/store
and original Context-tail targets. No final acceptance is inferred from the
superseded mixed build. Keep the source snapshot stable until builds/tests end.

The stable MIPS defaults targets and their linked runtime finish before the
focused two-check selection: both ownership and the 75-input default corpus
pass. All 75 inputs also match live original Rust, including the independent
microsecond bracket and restored module facade. The broader current reference/
resource/store/Context-tail consumers remain building; this early focused result
does not substitute for their final selection or full native regression.

The stable sanitizer defaults targets also finish before their focused checks;
both ownership and the 75-input source corpus pass 2/2 in 0.29 seconds with leak
scanning disabled. The broader MIPS consumer build finishes all 68 tasks and its
repeat reports no work; its current twenty-one-check selection is running.
Sanitizer composition and full native integration remain pending.

The broader stable MIPS selection completes 21/21 in 13.22 seconds. It includes
all reference model/codec/runtime consumers, both new Context defaults checks,
resources, root/fix models, DataStore and the original Context-tail contracts.
The no-work build precedes these checks; fresh sanitizer composition and full
native regression remain required before any complete Context/reference verdict.

Fresh stable native defaults/reference-model targets finish before their four
focused checks; all pass 4/4 in 1.07 seconds. This includes the 75 default and
229 reference model comparisons plus both ownership contracts. The complete
602-task native pass remains active, so its full 322-check regression is still
pending. The seventeen sanitizer consumer targets now finish all 68 tasks;
repeat no-work verification precedes the current twenty-one-check selection.

The stable sanitizer selection also completes 21/21 in 17.05 seconds with
leak scanning disabled. Both stable seventeen-target builds finish and report
no work before the selections; source fingerprints remain unchanged throughout.
These results revalidate reference/resource/model/store/Context-tail paths after
the new Context layout. Whole-native 602-task work and its full 322-check CTest
remain active/pending. Complete Context/reference and Stage 4 acceptance stay open.

## Checked clock, registry and schema-provider dispatch

Installed-native reproductions confirm dead virtual dispatch after early
destruction of each of these three managed provider types. Registration alone
was insufficient: later calls used raw pointers. All three child processes
terminated with signal 6 instead of ReferenceError::Dead.

The repair now retains typed checked ResourceBorrow values and validates them
before virtual dispatch. Explicit date overrides retain precedence over clock
access. Clock and entity-registry callbacks hold synchronous quiescence guards;
existing shutdown callbacks can still read live resources while closing. Schema
work retains its existing asynchronous quiescence and invocation boundaries.
Unmanaged providers cannot bypass explicit allocation lifetimes. No shared owner
extends a User or provider lifetime.

Six new contracts cover stale local/Global providers, date override precedence,
replacement, shutdown reads and reentrant callbacks. Four existing ID/policy/
checker reentrant checks now reject ordinary caught exceptions rather than
accept any terminate path. The new header changes UserContext layout, so all
consumers must rebuild. Pre-repair full native 343/343 passes; the installed
349-test native snapshot and selected sanitizer/MIPS consumers are building.
Complete Context/default/service/privacy acceptance remains open.

The first full provider snapshot passed347/349, revealing changed setter errors
during closing/closed access. Restoring ensure_mutable before checked public
validation preserves the previous exception contract. Corrected focus passes
10/10 native; corrected provider/buffer selections pass42/42 sanitizer/MIPS.
Whole native351 consumers are still building. Complete Context acceptance
remains open; the failures and correction are retained in the measurements.

## Synchronous callback shutdown follow-up, 2026-10-09

Complete native 353/353 and fresh ID/relation sanitizer/MIPS26 selections pass
after the installed clock/provider and returned-work repairs. A separate
public-entry audit now reproduces missing quiescence during behavior-registry
lookup, relation-load selection, and BusinessIdService slot/key/allocator
callbacks. All five controlled callbacks close User while the outer call is
still active. The negative controls exit immediately without subsequent access.

Registry dispatch, relation-load selection and the entire Business ID ensure
sequence now hold synchronous guards. Existing read-only registry callbacks
during an already active shutdown retain their previous access boundary;
normal exceptions remove the guards before subsequent close. The new focused
contracts and complete 361-consumer validation remain pending. Full Context
acceptance cannot be inferred from the preceding 353 snapshot.

## Remaining provider-dispatch follow-up

The wider audit independently reproduces five more public Context boundaries
that let a provider close User during synchronous dispatch: metadata entity
lookup/listing, checker lookup, and telemetry inspection/scope creation. These
controlled callbacks exit immediately after close and never access reclaimed
objects. An independent telemetry getter fixture also destroys a registered
Global telemetry object through User's proxy; runtime_telemetry still returns
the dead raw alias instead of ReferenceError::Dead. It performs no virtual
dispatch or field access after destruction.

These are Context integration responsibilities in pinned context.rs, separate
from the accepted concrete language catalog and telemetry model/SPI source
groups. Telemetry registration needs a checked ResourceBorrow as well as the
new synchronous boundaries; the borrow changes UserContext layout. Evidence
is saved in measurements/parity/2026-10-09-context-dispatch-follow-up. These
repairs remain outside the active361 snapshot until its compiler/linker work
finishes; full Context/source/stage acceptance remains open.

The dispatch/checked-telemetry follow-up is installed after stable native361.
Eight new contracts pass on native, ASan/UBSan and MIPS/QEMU, including trusted
shutdown reads, ordinary exception release, default/replacement behavior and
expired local/Global telemetry. All consumers must rebuild for the changed
UserContext layout; complete369 and wider50-check integration remain active.
Physical-router tests are deferred at the user's request; current target
validation uses cross-compilation and QEMU. No Context/Stage4 acceptance is
inferred from these focused checks.


## Complete production reconciliation, 2026-10-09

The complete production source (lines1–83 and103–1266) was reconciled with
context.hpp, resources.hpp, context_root.cpp, schema.cpp, the separate Context
entity/reference/graph/checker/fix/event/store adapters and the previously
accepted locking/pagination/transaction/logging/registry/ID/telemetry groups.
The embedded tests keep their independently reviewed source dispositions.

Root canonicalization validates blank type before zero ID and rejects repeated
ordinary assignment; generated assignment is idempotent only for the same
canonical type/ID. Root errors retain exact fields/Display/empty cause. Fix
constructor checks preserve ordering and ASCII-sensitive label rejection;
allocator-aware copying preserves public-field edits without revalidation.
SchemaInvocation remains privately constructible. Provider, optional business
schema contributor and ordered bootstrap futures keep distinct phases. Provider
failure skips later phases; bootstrap activation exists only during poll/cancel,
and foreign returned children are cancelled before disposal. ID floor reservation
precedes mark_as_new. Explicit Global-local ID fallback replaces the source
process singleton without changing optional generation versus required next_id.

Default construction preserves empty stores/registries/graphs/evidence, disabled
pagination observations, independent logging flags, Noop telemetry, English
catalog, Some UTC and identity/trace shape. Source lazy concrete defaults are
allocated only when used. Native thread identity is the documented OS/thread
adaptation; trace suffix is hexadecimal microseconds. Immutable graph identity
is shared inside the owning User, while each entity_runtime_state result has
fresh mutable ledger state. RuntimeModule installs independent destination
registries, descriptors, decoder borrows, callbacks and graph snapshots.

Typed and named resource lookup/type mismatch/require remain separate from
Value locals and asynchronous DataStore. Resource origins are checked before
retention, managed slots are checked before dispatch and explicit destination
copies own nested values. Removed locals retain the invoking User lifetime.
Store lookup happens on first poll, absent stores are no-ops, expiry compares
strictly greater than its monotonic deadline, overflow poisons the write state,
and foreign child cancellation preserves live borrowed keys. Replacing a
registered resource does not free its arena-owned predecessor early.

Codec and round-trip runtime configuration remain distinct. Missing runtime
precedes missing principal; mode gating precedes wire serialization and runtime
issue/consume owns authorization. Business ID infrastructure retains the same
allocator through two separate allocation/schema interfaces; key replacement
copies the service explicitly and preserves missing-resource error precedence.
Metadata/registry/behavior/checker/clock/telemetry callbacks use their installed
checked borrows and synchronous guards. Public access rejects a closed User;
trusted teardown reads remain limited to the existing cleanup boundary.

Checker status is captured before custom fixes. Custom checker runs before
metadata-required checks; create missing and explicit-null nonnullable values
fail, runtime-managed version is excluded and matching Required/location results
are not duplicated. Sparse create validation avoids repeating Fix. Existing
messages are retained; only missing messages consult the catalog. Operation-local
Fix time/evidence remain separate from independent last-evidence snapshots.

Event routing overwrites governance with the active scope, attaches generated
bootstrap identity only to Created/Updated events, prefers present new version
over old values, and sends raw then metadata-projected safe events before ordinary
LogManager publication. First sink failure stops later routes; telemetry success
or audit_error classification follows the result. This preserves the previously
accepted event/log contracts without merging their responsibilities.

The current375/375 complete native snapshot is unchanged. A fresh64-check
Context composition selection passes native64/64 in17.95 seconds. Forty selected
consumer targets are being rebuilt for ASan/UBSan and MIPS/QEMU before their
corresponding selections. Live original defaults/models/store/resources/schema/
ID/business-service comparisons and final cross results are required before
recording complete Context acceptance. Physical-router testing remains deferred.


## Generated bootstrap active-root correction

Fresh pre-repair composition passes64/64 native/ASan-UBSan/MIPS and all seven
live original Context probe groups. Complete body review nevertheless finds an
uncovered schema composition: generated bootstrap root installation. The
set_generated_bootstrap_active_root helper used ensure_mutable, which rejects
every pending ensure_schema borrow. The locked Rust helper writes its OnceLock
through an immutable Context borrow; locked generated runtime assemblies call
it during bootstrap.

A controlled native reproduction exits2 with the pending-schema error; an
independent actual-source Rust pipeline succeeds, preserves idempotent repeated
assignment and canonical conflict/invalid-input errors, and exposes Tenant42
after completion. The new registered schema fixture fails before repair.
The C++ helper now requires a live, nonclosing Context without rejecting its
active schema borrow. It cannot replace providers or other borrowed resources;
ordinary root assignment and resource mutation retain their previous gates.
The fixture additionally repeats ensure_schema and checks closed rejection.

Evidence is recorded in measurements/parity/2026-10-09-bootstrap-root-context-review.
Complete native and64-check sanitizer/MIPS repair validation are running.
The earlier375/375 and pre-repair64/64 checkpoints do not accept this repair;
complete Context/stage acceptance remains open until final validation finishes.


## Complete Context source acceptance

The complete production source ranges1–83 and103–1266 and each responsibility
in the reconciliation above are accepted. The bootstrap correction is fully
integrated after a successful262-task complete native rebuild, no-work refresh,
focused schema check, full375/375 regression in76.30 seconds with four workers,
418 nonempty compiled dependency records and a post-test no-work build.
Fresh repaired43-task selected builds precede ASan/UBSan64/64 in33.16 seconds
(leak scanning disabled) and MIPS/QEMU64/64 in24.00 seconds; each has no-work
before/after. The independent native reproduction now exits0.

Both native and MIPS match all2,384 live original defaults/models/store/resources/
schema/internal-ID/business-service observations (75/234/251/410/384/171/859).
The separate original bootstrap pipeline confirms root installation, same-value
idempotence, canonical conflict and invalid-input precedence. The registered
fixture also preserves ordinary pending mutation rejection and closed access.
All451 source-test dispositions verify against the current375 registrations;
those locators remain bookkeeping, not a substitute for complete body review.

The evidence is retained in measurements/parity/2026-10-09-bootstrap-root-context-review.
This closes only Context's source responsibility under the documented explicit
Global/User, checked-borrow and owner-thread adaptations. Complete runtime
services, graph/save execution and runtime-root acceptance remain independent.
Stages4–6 are still open. Physical WNDR4300 execution remains deferred.
