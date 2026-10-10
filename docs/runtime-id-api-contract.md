# Runtime Business ID and internal ID source review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Reviewed complete production files: teaql-runtime/src/business_id.rs (563 lines)
and id.rs (187 lines). The core Business ID model group has its own accepted
review in business-id-api-acceptance.md; it does not accept these runtime bodies.

## Complete source acceptance, 2026-10-09

This source group is accepted after complete public/body/default/trait/protocol/
ownership review of business_id.rs 1–563 and id.rs 1–187. The checked clock and
service callback repairs are installed. Current native 361/361 passes in 75.15
seconds with no-work before/after testing and 417 nonempty compiler dependencies.
Fresh sanitizer/MIPS callback/ID/relation/service selections each pass 40/40;
focused callback checks pass 8/8 on all three profiles. Leak scanning is disabled.
Live original comparisons retain 82/82 ID models in their Debug/Release profiles,
859/859 service/clock calls, 171/171 internal IDs and the independently accepted
full 20,000-sequence permutation source tests. Prior 2,150 strategy and 384 schema
observations remain in the current registered selection.

The tables and complete body review below define the accepted scope and explicit
C++ ownership adaptations. Context metadata/checker/telemetry dispatch, broader
service/relation composition, graph/governance/privacy and complete Stage 4 retain
independent gates. The new Context dispatch patch is staged and not part of this
accepted snapshot. Earlier staged/pending paragraphs record historical evidence.

## Responsibilities

| Source surface | Native mapping and semantic boundary |
| --- | --- |
| BusinessClock / system/fixed clocks | Independent business time and inherited business-date default; operational duration/log clocks remain separate. System clock is default/copy/clone/unit Debug; fixed clock preserves its private UTC value and derived Debug. |
| BusinessDate | Separate override value, Copy/Clone/equality/tuple Debug and today factory; Context checks the installed override before the business clock. |
| Schema contributor/service | Passive provider registration, separate contribution SPI and scoped contributor wrapper, invocation only through the ordered Context schema path. Constructing a Context performs no DDL. |
| Legacy daily profile | Independent plan/format/validate, scope and optional date, exact definition/range/error order and fixed digit formatting. Validation checks actual calendar dates and ASCII sequence text without inventing other definition rules. |
| Encoding key / static provider | Positive u32 version, all 32 private bytes, Clone/equality and version accessor. Provider returns an equivalent copied key; neither key nor provider derives Debug, so diagnostics must not expose key material. |
| V1 permutation | Exact public alphabet/width and eight-round HMAC/cycle walking, source tweak framing/field order/key version, big-endian bytes and six-character base36. Allocation and cryptographic derivation remain separate responsibilities. |
| Permuted daily profile | Required date splitting/six digits, initial zero, exact domain ceiling and scope, allocate/format error order, period requirement and uppercase ASCII code validation. The source derives Clone without Debug/default. |
| In-memory allocator | Plan-range check and canonical scope before locking; existing level overrides initial sequence; maximum/overflow failures precede insertion. Mutex poison and exact errors remain observable. Native allocation is owner-thread confined; this is not a claim of arbitrary cross-thread Context equivalence. |
| BusinessIdService | Allocator/key-provider assembly, explicit-domain Clone and passive installation. Select/fetch profile/key before inspecting slot; validate existing nonempty ID; enforce immutability; clock, plan, allocate, format, assign and return in source order. |
| InternalIdGenerator | Separate generate/floor SPI with exact default unsupported-floor error. Native facade/protected virtual adapter avoids colliding with SQLite's inherent error-returning method. |
| Atomic counter | Default 1000/start factory, relaxed generate, maximum-floor reservation and source Debug. Debug overflow wraps the atomic increment before the checked addition failure; release behavior remains explicit. |
| Snowflake | Both ID bounds and validation order, epoch/shifts/masks, default zero worker/datacenter/state and complete derived Debug. Rollback and sequence wrap wait for the next millisecond; pre-epoch and poison errors retain source text and state order. |
| Canonical entity names / fallback | Exact ASCII uppercase splitting while preserving Unicode bytes. The source process fallback becomes an explicit Global-owned generator shared by its Users, without a hidden singleton owning Context data. |
| Context integration | Checked clock/generator/allocator/provider borrows, passive Business ID infrastructure assembly, explicit schema contribution, optional ID generation and floor-before-mark-as-new bootstrap. |

Source Arc/mutex/static ownership adaptations are explicit: callbacks/providers
must outlive their Context borrows; retention copies nested values into the
selected scope. Pure atomic synchronization remains separate from owner-thread
Context storage. Existing native tests cover early provider destruction,
reentrant close, deep retention, poisoning/overflow, controlled clock rollback
and real SQLite adaptations. Runtime save/audit integration remains Stage 5.

## Reproduced diagnostics omissions

A public runtime umbrella compile fails to find source Debug for
SystemBusinessClock, FixedBusinessClock, BusinessDate, DailySequenceBusinessIdProfile,
InMemoryBusinessIdAllocator, AtomicCounterIdGenerator and SnowflakeIdGenerator.
These are seven actual source derives. The scalar encoding key, key provider,
permuted profile and service do not have a Debug derive; no secret-bearing
formatter should be invented for them.

The native repair is staged separately in a reviewable patch under
measurements/parity/2026-10-08-runtime-id-model-review. Its standalone Debug
probe matches all 82 original observations; its lifecycle executable proves
non-mutating counter/allocator diagnostics, actual User block reclamation,
explicit Global retention of diagnostic text and rejection of expired counter
references. No current production header or implementation is changed while
the entity-runtime consumer rebuild is active. Complete public/default/trait/
protocol/ownership and CMake integration acceptance remain open.

## Independent original evidence

The runtime_id_model oracle calls the original public factories, Clone/equality,
Error/overflow behavior, fixed-clock getters and actual counter/allocator
transitions. Eighty-two inputs agree across two executions with diagnostics in
nine modes. They include UTC fractions/leap/extended years, key version/byte
identity without key Debug, default/custom counters, floors, full-width overflow
and post-failure counter state, Snowflake constructor bounds and exact private
empty state, and allocator range/exhaustion/overflow/continued levels.

Source HashMap multi-entry formatting order is not deterministic or a persisted
wire contract. Exact allocator diagnostic observations therefore use one scope;
this does not reduce the allocator's production scope support. Existing separate
2,150 strategy/permutation, 859 clock/service and 171 ID observations plus schema
and ownership tests retain the wider behavioral evidence. The new model corpus has a passing staged native counterpart. Original Debug
and optimized Release executions are recorded independently, twice each: nine
counter overflow observations differ between profiles, and the comparer checks
the selected original profile without rewriting native results. Standalone
sanitizer and MIPS builds finish successfully; both lifetime tests pass and all
82 observations match live original Rust for the corresponding build profile; final current CMake/native integration and
complete source acceptance remain required.

## Staged Mutex and ownership adaptation

The staged formatters borrow private state rather than fabricate a second model.
Counter diagnostics load the atomic without incrementing it. Mutex diagnostics
use try-lock and report the source `<locked>` placeholder instead of blocking.
The allocator retains its existing owner-thread contract. Snowflake's poison
flag becomes atomic, allowing safe observation when try-lock fails; its actual
state/sequence and generation order stay protected by the existing mutex.
Output and temporary formatting storage use the requesting Context's allocator.
Final standalone lifetime repeats disable default PMR allocation and pass on
native, ASan/UBSan and MIPS/QEMU. The production/CMake integration patch is
still staged; these results do not claim full consumer or Stage 4 acceptance.

## Installed integration checkpoint, 2026-10-08

The combined runtime-ID/cache/typed-transaction repair is installed. Its full
native regression passes 331/331 in 72.65 seconds, with no pending build work
before or after testing. The selected installed sanitizer and MIPS checks each
pass 17/17; those results are retained in the corresponding measurements.
Earlier staged statements describe historical evidence. Complete source and
stage acceptance remain independent. The subsequent six-repair integration
registers 343 checks and is now building; no passing result is claimed for it.

## Complete body comparison follow-up

The complete pinned business_id.rs (1–563) and id.rs (1–187) were compared
again with the installed profiles, clocks, services and generators. The review
includes the eight-round HMAC framing/cycle walk, exact error precedence,
calendar parsing, allocator continuation and checked overflow, service key
selection before slot inspection, immutable-slot rejection, and passive schema
contribution. Context clock/date override and resource installation, fallback
counter use and floor-before-bootstrap ordering were inspected in their actual
Context methods.

The derived diagnostic writers borrow actual private state and keep source
fields/order; encoding keys, key providers, permuted profiles and services gain
no secret-bearing Debug absent from Rust. Complete source acceptance remains
pending the subsequent installed integration checkpoint and final recorded
acceptance audit. This follow-up does not accept the independent schema/Context,
save, graph or privacy source groups.

## Reproduced clock lifetime gap

The follow-up finds a concrete remaining ownership gap in Context clock storage.
Registration validates a managed clock, but subsequent time/date access uses a
raw pointer. An isolated installed native reproducer destroys FixedBusinessClock
then calls business_time; it terminates in dead virtual dispatch rather than
ReferenceError::Dead. The checked-borrow/quiescence repair is staged under
measurements/parity/2026-10-08-business-clock-borrow-review, with six future
clock/registry/schema contracts. Full consumers must rebuild because Context layout changes. This
gate prevents complete clock/ID and Context source acceptance until repaired
and verified; current343 compiled inputs are preserved.

## Current callback lifetime gate, 2026-10-09

The clock repair is installed and the complete native regression subsequently
passes 353/353. Fresh ID/relation selections pass 26/26 on sanitizer and MIPS;
82 ID models match live original Debug/Release, and service/internal-ID public
calls match 859/859 and 171/171. The complete production bodies, public methods,
defaults/traits, private key handling and diagnostic fields were compared again.

That final ownership audit finds a separate gap in public BusinessIdService
ensure: its slot, key-provider and allocator callbacks could close User while
the service still holds the definition, result domain and Context borrows.
Controlled callbacks confirm all three closes return. They exit immediately
without accessing reclaimed data, so the negative controls do not depend on
undefined behavior to demonstrate the missing boundary.

The installed repair guards the entire ensure sequence, preserving the source
profile/key/slot/date/plan/allocation/format/assignment order. Reentrant close
fails fast before arena reclamation; ordinary callback exceptions remove the
guard so subsequent User close can reclaim blocks. The eight new runtime
callback contracts and current 361-consumer validation remain pending. This
gap prevents source acceptance despite the earlier passing ID comparisons.
See measurements/parity/2026-10-09-runtime-callback-scope-review.
