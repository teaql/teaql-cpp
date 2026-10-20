# Entity runtime production API review

This is the historical 5.0.5 acceptance record. The active reference is the
5.0.6 commit in `upstream.lock.json`; the
[complete 5.0.6 review](entity_runtime_source_review_5_0_6.md) now accepts that
source with its explicit Context adaptations. The
[consuming loaded-view continuation](loaded_view_move_5_0_6.md) records the
earlier receiver-copy difference and its focused repair.

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
entity_runtime.rs production lines 1–1035. Embedded source tests are separate
acceptance evidence. The source group is accepted within its explicit Context ownership/owner-thread
adaptations after the final integration evidence below.

## Responsibility mapping

| Source responsibility | Native mapping and review boundary |
| --- | --- |
| LoadedRelation / RelationHandle | Loaded, Empty and NotLoaded remain distinct; handles borrow values, never trigger I/O. Loaded-empty lists contain a value; null to-one relations do not. Both source types derive Debug/Clone/Copy; only the state enum derives equality. |
| EntityKey | Public entity/id plus a private immutable identity snapshot. Nonnegative signed IDs compare as unsigned; float identity preserves bits; TypedNull shares Null identity; other categories preserve source order. Public id mutation does not recompute the private key, including on Clone. Static versus owned name construction has equivalent observable key behavior under explicit native ownership. |
| EntityGraphBuilder / FrozenEntityGraph | Separate typed entity tables and owner/relation/type-erased values; replacement, counts and one-time consuming freeze. Source diagnostics reveal three counts, not payload internals or native addresses. |
| EntityChangeSet | Ordered key-to-mutation maps, empty/default/clone/equality, set/get, remove-one-entity and field-name queries. Original Value and key semantics remain distinct. |
| ChangeSetStack | Lazy current frame, push/pop, newest-first reads, current/all-entity clearing and changed-field union; independent default/clone/equality and source stack diagnostics. |
| EntityMutationLedger / OriginalVersions | Deferred ledger allocation, mutation frames, comment, deleted/new keys, original snapshot, trace chains, first/overflow versions, deferred composition errors and new-state flag. These remain independent fields and responsibilities. |
| EntityRuntimeState | Shared lazy ledger anchor and immutable graph anchor, plus per-view loaded snapshot. Clone, fresh graph-sharing views and graph-preserving/rebinding views remain distinct; source equality follows shared mutation identity. |
| Strong / weak graph roles | Graph-owned entity views remain weak; clone promotes an escaping view. Native roles use Context-owned slots and never extend lifetime beyond close. Publication requires an owning view; graph reads remain explicit. |
| Checked composition | Preserve the receiving save boundary, retain source intent for retry, check every original version before merging, and preserve source merge precedence. Hidden void adoption records its error for later saving. |
| Snapshot and version APIs | Materialized ledger snapshots and compact loaded per-view snapshots remain separate; loaded versions are a fallback only for the matching entity/id. Read-only calls must not allocate a mutation ledger. |
| Relation resolution | Exact owner canonicalization and distinct list/optional typed payloads; absence returns NotLoaded, wrong payload type preserves the source failure boundary. |
| Mutation APIs | Independent frame operations, new/delete flags, changed-field queries, snapshots, versions and trace access. Successful committed-state clearing remains separate from graph storage and failed-save retention. |
| LedgerCompositionError / LedgerEntity | Three distinct error variants, Clone/equality/Debug/Display and empty Error cause; separate entity trait and default target-before-source state checks followed by checked composition. |

Rust Any/Arc/Weak/Mutex/OnceLock ownership maps to allocator-aware type erasure,
explicit scoped graph/ledger slots, owner-thread calls and deferred construction.
Cross-domain retention requires a deep copy and preserves cycles within that
copy. Noncopyable payloads remain valid within their owner domain; their explicit
cross-domain retention must reject rather than introduce hidden shared ownership.
No source serde/hash capability is invented for the public graph/state models.

## Reproduced protocol gaps

A public runtime umbrella compile fails to find complete structural writers for
LoadedRelation, EntityKey, EntityGraphBuilder, EntityChangeSet, ChangeSetStack,
EntityRuntimeState and LedgerCompositionError. A separate generic relation-handle
fixture reproduces missing RelationHandle Debug. Existing EntityKey.debug only
supports its compact default; it does not establish inherited formatting modes
or the surrounding model derives.

LedgerCompositionError also needs its empty Error cause adaptation. Its native
conflict payload already has a public field named source, so a C++ source()
method cannot coexist under that spelling. The implementation must preserve both
the payload and error-cause responsibilities with an explicit naming adaptation;
removing the payload or treating it as a cause would change source semantics.

These compile failures describe the pre-repair source snapshot. The installed
implementation and current target evidence are recorded below; complete native
integration remains the final independent gate for this source group.

## Independent original observations

The entity_runtime_model oracle calls actual original constructors, Clone,
PartialEq/Ord, custom/derived Debug, Error methods and public graph/ledger state
transitions. The 252-input deterministic corpus agrees across two executions in
nine formatting modes. Inputs cover full-width and cross-kind key identities,
NaN/negative-zero keys, private identity preservation after public id mutation,
ordered nested mutations and frames, every composition error, deferred/materialized
ledger state, original version first/overflow storage, compact snapshots,
consuming graph publication and all list/optional relation load states.

Graph fixtures install ordinary String/i64 payloads and invoke the actual source
freeze/relation functions; they do not implement the graph or formatting protocol.
State observations also include Clone and fresh graph-sharing views. The recorded
private OnceLock/Mutex/graph/ledger diagnostics are source observations, not a new
application serialization format. Recording these inputs originally identified
the gap; current native counterparts and passing target comparisons are
separate evidence below.

The previously recorded behavioral corpora and direct source-test ports remain
separate evidence for operations and native Context ownership. Current full
public/default/trait/protocol/ownership acceptance still requires the diagnostic
repair, differential native consumers, scoped retention/nonmaterialization
contracts and final native/sanitizer/MIPS integration. Stages 4–6 remain open.

## Installed diagnostic repair, 2026-10-08

The isolated staged implementation matches all 252 original observations in
nine modes. Its native and ASan/UBSan-with-leak-scanning-disabled contracts pass
without default PMR, including read-only deferred state, cached key identity,
weak-role borrowing versus actual Clone promotion, complete deep Global graph/
ledger/key/error retention and stale access after actual User deletion.

After the complete current 322-check regression ends, the repair is installed.
entity_runtime_debug.hpp exposes structural writers for all reviewed public
models. Out-of-line writers borrow private graph/stack/state/key data directly;
they never call the lazy ledger constructor. Legacy key.debug delegates to the
same writer. Complete private versions/snapshots/traces/composition errors and
frozen counts retain their source shape; all formatter scratch uses the caller's
selected resource. Source Mutex/OnceLock diagnostic wrappers describe the
native owner-thread/deferred state adaptation and do not introduce a mutex or
shared owner. Arbitrary concurrent or poisoned native Context access is not
claimed. Raw diagnostics remain separate from privacy projections.

LedgerCompositionError.error_source() returns nullptr, preserving the source
empty Error cause while its public source conflict-version payload stays intact.
This spelling is the explicit C++ field/method collision adaptation.

Two current model/lifetime checks are registered. Selected sanitizer/MIPS
consumers finish and pass; the full native build and corrected probe refresh
are terminal, followed by an active complete 324-check regression. The source group and stages 4–6 stay partial until the complete
public/default/trait/protocol/ownership and target evidence is accepted.

## Current target evidence and NaN input correction

The installed targets finish before final six-check selections: ASan/UBSan
passes 6/6 in 21.17 seconds with leak scanning disabled; MIPS/QEMU passes 6/6
in 7.21 seconds. Both include original incremental-ledger/graph-lifetime tests,
existing behavioral parity and new model/ownership contracts. Final repeats
report no work, and all 252 model inputs match live original Rust on MIPS.

The initial MIPS model selection differs on nine NaN-key inputs because parsing
NaN text yields different target and host IEEE payload bits. Entity identity
correctly preserves actual bits. The key fixture therefore supplies explicit
IEEE bits on both sides before the real constructor, leaving production identity
and all diagnostic outputs unmodified. The original oracle is rerun twice;
recording remains actual original behavior for the now identical inputs.

The complete native consumer build is terminal-successful. A final two-task
refresh rebuilds the corrected probe, and the following build reports no work.
The complete current 324-check regression is active. No complete source/stage
acceptance is inferred before that regression finishes.

## Complete source acceptance, 2026-10-08

The installed source group is accepted after complete production/public/default/
trait/protocol/ownership review. Final native integration passes 324/324 in
151.69 seconds after the corrected probe refresh, followed by no pending build
work; all 392 compiled dependency records are valid and nonempty. All 252
current model observations match live original Rust on native and MIPS.
Selected current sanitizer/MIPS consumers each pass six checks, including
original ledger/graph-lifetime, behavioral, model and ownership contracts.

This accepts the complete entity-runtime responsibilities above within explicit
scoped deep retention, noncopyable-payload rejection on retention, owner-thread
access and the error_source naming adaptation. It does not accept the wider
Context/service, graph saving/governance/privacy, physical-router execution or
stages 4–6. Earlier partial/staged paragraphs retain their historical checkpoints.
