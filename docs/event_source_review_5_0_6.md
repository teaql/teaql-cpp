# Complete audit event source review: Rust 5.0.6

Current source disposition: **accepted** at the [installed cohort checkpoint](source_cohort_acceptance_5_0_6.md). Earlier staged/pending paragraphs retain their historical scope.

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: all 607 lines of `teaql-runtime/src/event.rs`. Status: complete body
reconciliation and permanent native/imported source gates pass after the
preceding 784-test native epoch. Formal source disposition remains separate.
No runtime or generated implementation changes here.

The [historical 5.0.5 review](audit_event_api_contract.md) remains historical.
Its 572-line source and completed profiles do not establish acceptance of the
new source. The [5.0.6 provenance repair](audit_event_provenance_5_0_6.md) adds
safe target identity/bootstrap and raw invocation provenance; the
[dynamic projection continuation](dynamic_audit_projection_5_0_6.md) retains its
own complete matrix. Their responsibilities are reconciled below.

## Complete source responsibilities

| ID | Locked responsibility | Current C++ mapping and boundary |
| --- | --- | --- |
| EV01 | Eight event kinds | `RawAuditEventKind` preserves four mutation and four schema/bootstrap variants, equality/copy and exact derived Debug names. |
| EV02 | Property-change construction | `EntityPropertyChange` keeps field and independent optional old/new Value. Missing and Some(Null) remain different; all nested values copy into the explicit destination. |
| EV03 | Complete raw model | Preserve kind/entity/current values/updated fields/old/new records/changes/trace/bootstrap/governance/private diagnostic redactions as distinct fields. No snapshot or provenance field is inferred from another. |
| EV04 | Created factory | Clone current values into the raw/new records and emit absent-old/present-new changes in source record order. Updated-field list remains empty. |
| EV05 | Updated factories | Ordinary updated derives fields from the complete record. With-old retains independent supplied records, field order, duplicates and missing-value optionals. |
| EV06 | Deleted factories | Request values contain ID and optional expected version. Ordinary deletion has no snapshots/changes; with-old emits old-to-None changes and retains its supplied old snapshot. |
| EV07 | Recovered factories | Keep the requested expected version distinct from the new recovered version. With-old constructs an independent record with ID and `1 - expected_version`, and only a version change. Debug overflow and Release wrapping retain their existing explicit profile boundary. |
| EV08 | Bootstrap identity | Actor/category/reason, optional resulting version and full u64 timestamp retain Clone/equality/Debug. Event constructors leave attribution absent. |
| EV09 | Schema created/verified | Preserve table and native-width field count as record values and changes; do not invent a new-values snapshot or target ID. Kind is the only created/verified distinction. |
| EV10 | Field added/data seeded | Keep field name and separate inserted/updated counts. Preserve source record ordering and native-width integer-cast semantics. |
| EV11 | Private change helper | Lookup each requested field independently in optional old/new records; clone only present values and retain duplicate field entries. |
| EV12 | Invocation secret collection | Collect inherited diagnostic secrets under the current permission, then credentials, configured masks, dynamic `#` fields and nested credentials from both old/new values. Sort longest byte strings first. |
| EV13 | Safe field selection/rendering | Collect secrets before skipping `_` fields. Stringify new Text/I64/U64 directly and other values through actual Value Debug; absence is not NULL. |
| EV14 | Safe field masking and truncation | Exact field matching and `#` rules use the source acknowledgement gate; credential names always mask. Apply mask before length limiting. Preserve independent raw/output scalar counts, flags and reason strings. |
| EV15 | Nested-credential override | Credential-containing old or new values force the complete redaction sentinel, masked true, fixed output length and truncated false, independent of an earlier limit. |
| EV16 | Post-projection scrubbing | Scrub masked fields and ordinary fields except unmasked id/version using collected secrets; recompute output scalar length without inventing new truncation metadata. |
| EV17 | Trace and bootstrap projection | Add current request-ID strings to intent secrets. Scrub cloned trace and bootstrap actor/reason; preserve category/version/time and original raw payload. Governance is copied as the source model. |
| EV18 | Positive target identity | Safe `entity_id` comes only from positive U64/I64 in current raw values. No text/float/bool coercion, old-snapshot fallback or parent-lineage inference is allowed. |
| EV19 | Unicode value limiting | Within the limit, retain the original. Limits zero through three use stars when truncating; larger limits retain the source head/ellipsis/tail split over Unicode scalar values. |
| EV20 | Business masking helper | Short strings and ASCII digits mask completely. Other strings retain two scalar values at each end. This is distinct from credential redaction. |
| EV21 | Separate raw/safe sink SPIs | Preserve the supplied Context/event and Result errors. C++ adds the explicit result allocator. Source Send/Sync/static ownership adapts to checked Context owner borrows; no implicit shared lifetime or thread-transfer promise is introduced. |
| EV22 | In-memory raw registry | Empty default/new, registration order, duplicates, consuming builder semantics, Clone and first-error short circuit remain separate responsibilities. C++ builder chaining and explicit-destination copies retain checked sink borrows. |
| EV23 | Callback cleanup | Synchronous fan-out guards each distinct Context/registry/event/result arena. Reentrant reclamation terminates before freeing borrowed data; ordinary exceptions detach guards and propagate once. This is the existing C++ ownership adaptation, not an asynchronous sink contract. |
| EV24 | Public traits and diagnostics | Preserve all declared Clone/PartialEq and actual Debug field order, including private provenance; kind and bootstrap additionally retain their declared equality. Do not invent serde/Hash/Default or sink Debug protocols. Full raw diagnostics and safe projection remain separate APIs. |

Implementation remains in `event.hpp`, `event_debug.hpp`, `event.cpp` and the
shared `audit_privacy.cpp`. Explicit destination copies propagate allocators
through nested records, Values, trace, governance, bootstrap and provenance.
Checked registry borrows do not keep the source UserContext alive. The event
file itself defines no embedded tests.

## Exact dependency replay and staged installation boundary

The [staged replay](parity/event_source_staged_replay_5_0_6.json.gz) checks the
exact reference registry identities and uses a separate portable observer
workspace. The older `rust_field_layout` lock has **46** registry identities
outside the exact reference set. It is preserved; matching observations from
that project alone are not used as exact-dependency acceptance.

Four complete unchanged observers/corpora cover 112 model/derive cases, 104
identity/bootstrap/provenance cases, 3,968 dynamic projections and 1,427 audit/
Unicode/privacy cases. The new workspace imports the actual original modules;
it does not extract or reimplement masking, factories or projection logic.
Existing human-log time removal remains the explicit standing adapter in the
audit observer; the other three families compare complete outputs unchanged.

Five additional caller tests exercise the actual public original registry:
default/new empty success, ordered duplicate registration, first Result error,
registry Clone/vector independence and panic propagation before later sinks.
These are new source caller fixtures, not tests embedded in the original file.
The corresponding complete imported C++ audit body retains existing success,
first-error/duplicate/borrow and callback exception/deletion assertions.

The staged source gate is terminal and passes all **5,611/5,611** observations
through both actual original and imported implementations, all **9/9** selected
module contracts/comparators and all **5/5** new original-public caller tests.
All **112** registry identities retain exact original versions/checksums. Every
captured execution input and both reference identities remain unchanged.

The current module catalog lacks the model-probe consumer. The isolated stage
therefore creates an import consumer with exactly the same original C++ body,
reusing the canonical prologue/fixtures and completed module compile/link flags.
Its body hash is checked explicitly. This staged consumer is not a permanent
CMake registration or complete installed-source acceptance.

A separate [runtime-only candidate](parity/event_source_runtime_only_candidate_5_0_6.json.gz)
imports only `teaql_runtime`, preserves the same complete model-probe body and
passes all **112/112** model observations. Its actual link command contains
neither SQLite nor its provider. The proposed permanent registration uses that
qualified runtime-only binding.

The receipt preserves complete workspace files, source/driver/corpus guards,
commands, output, imported JUnit and frozen execution input/reference checks.
Only terminal passing evidence supplies a successful scoped result. Permanent
workspace/helper/consumer installation and its installed gates remain required
before formal source promotion. The running full native epoch is independent.

The [initial staged installation archive](parity/event_source_staged_installation_5_0_6.json.gz)
retains eight complete before/after files: the portable workspace, dependency/
corpus contract, verification helper and module/preset registrations. It is not
installed. The helper's initial CLI run passes its requested CTest selection but
uses a relative JUnit path resolved by CTest under the build directory; the
[unsuccessful receipt](parity/event_source_portable_helper_candidate_5_0_6.json.gz)
preserves that path error. The corrected helper resolves its output path before
running CTest and is requalified separately; no source or assertion changes are
needed to fix the harness path.

The [corrected portable helper replay](parity/event_source_portable_helper_final_5_0_6.json.gz)
is terminal and passes all 5,611 original/imported outcomes, five public-source
caller tests, nine selected imported checks and a final no-work build. This
requested-profile replay uses the explicit staged runtime-only model consumer;
its `all_installed_frontends` is false. It does not qualify a missing permanent
consumer or an omitted native frontend.

The [final staged installation archive](parity/event_source_staged_installation_final_5_0_6.json.gz)
retains the corrected tested helper and complete eight-file patch, each existing
file's unchanged before hash, candidate after hashes and proof hashes. It
proposes a canonical runtime-only model consumer and its missing comparator,
without changing original probe/function bodies. This supersedes the initial
candidate patch for later guarded installation; neither patch is installed.
The reference source, drivers, corpora and active full-native inputs are unchanged.

The later [merged Event/Policy archive](parity/event_policy_merged_staged_installation_5_0_6.json.gz)
combines this tested candidate with the separately qualified policy observers and
consumers. Its shared metadata preserves both additions. It is the reviewable
installation candidate; the individual archives remain historical evidence.
It was held outside the frozen full native epoch before the later installed
checkpoint below.

## Installed native and imported source gates

The [cohort controller](parity/event_policy_installed_cohort_5_0_6.json.gz)
verifies the complete **784/784** native terminal, no-work rebuild, frozen
execution inputs/references and all proposed before hashes, then installs the
merged twelve-file package. Both configurations and portable helpers pass.

The [installed Event gate](parity/event_sources_installed_5_0_6.json.gz)
freshly executes all **5,611** original observations and matches them on native
and imported frontends. All five original-public caller fixtures pass; these
are additional caller tests, not embedded Event source tests. Native **10/10**
and imported **10/10** registered checks pass with exact JUnit case sets and
no skips, and final selected builds report no work. The new complete model
consumer imports and links runtime without SQLite or its provider. Source,
observer, corpus, execution inputs and references retain their guarded hashes.
The portable helper is now installed; formal source/capability promotion remains
an explicit separate audit.

Whole Context delivery, post-commit audit failure, graph Save, mutation policy,
generated model configurations, durable dynamic storage, arbitrary sinks and
complete G14/G17/P0-P7 remain outside this file's acceptance. No new sanitizer,
MIPS or physical-device success is claimed here. The preceding full native
checkpoint does not relabel subsequent scoped installation gates.
