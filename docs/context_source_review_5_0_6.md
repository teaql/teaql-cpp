# Complete UserContext source review

UC01-UC25 qualify the complete named source under the established C++ Context,
owner-thread and explicit-retention adaptations. Fresh native and imported
selections each pass 17 core and 23 delegated checks, including all 3,362 retained
core observations and 1,088 freshly compiled original audit-dispatch outcomes.
Whole capability, child-module and P0-P7 acceptance remain independent.

The source is all 1,631 lines of Rust 5.0.6
`teaql-runtime/src/context.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. This includes its models, defaults,
interfaces, implementation bodies and six embedded tests. Child JSON, locking,
logging, pagination and transaction files keep their own source responsibilities.

## Complete responsibility mapping

| ID | Source responsibility | Mapping and evidence |
| --- | --- | --- |
| UC01 | Root-reference models and errors | ContextEntityRef and ContextRootError retain every field, equality, Clone, Debug, exact Display and empty cause. The 234 model observations and complete destination-copy contract cover these alongside FixEvidence. |
| UC02 | Fix evidence | Clock/Context provenance, constructor validation order, ASCII label exclusions, public-field mutation and nonvalidating Clone remain distinct. Complete raw formatter modes are compared; Debug is not safe-log projection. |
| UC03 | Defaults and construction | Global-owned application scope replaces the implicit standalone owner. Empty stores/graphs/registries/history, English/builtin catalog, UTC, independent enabled SQL switches, Noop telemetry, default governance and fresh identity tokens retain their responsibilities. The 75 default cases use independent clock/PID/shape assertions, not absolute-time normalization. |
| UC04 | Context identity and time zone | Preserve optional user identity, exact timezone and trace getters/setters/fluent APIs. Arbitrary text is not newly validated. Native thread identity and scoped owner tokens retain the established platform/Global-domain adaptations. |
| UC05 | Active-root assignment | Canonical entity space, nonblank/positive validation, ordinary one-assignment behavior, bootstrap idempotence/conflict and typed missing/mismatched errors remain independent. The original root test, model observations and qualified bootstrap evidence cover these paths. |
| UC06 | Runtime module assembly | with_module delegates to RuntimeModule installation, preserving distinct metadata, entity/behavior/checker/decoder registries, sinks, callbacks, graph snapshots and optional language. Destination copies retain the two Context domains. The default/ownership contract and existing G12 assembly evidence remain separate from accepting the registry implementation. |
| UC07 | Metadata and service membership | Install/check the metadata resource, exact entity/all-entity/require lookup and registry-or-metadata service membership. Missing metadata remains empty/missing rather than an invented descriptor. Borrowed results cannot become Global owners implicitly. |
| UC08 | Runtime state and graph roots | Context anchors the immutable identity graph; every returned EntityRuntimeState has a fresh mutation ledger. Initial/root graph getters and replacing setters remain distinct. Both original state reuse and complete native root/tail contracts exercise these responsibilities; the accepted runtime-state bridge supplies graph attachment evidence. |
| UC09 | Graph decoder forwarding | Preserve membership, JSON capability and compact scalar/list/batch/SmartList/optional delegation, including owner identity, relation and complete SmartList metadata. C++ ContextEntityAccess is the internal facade over the same independent decoder registry. This accepts forwarding responsibility, not every implementation in the separate pending registry/JSON sources. |
| UC10 | Behavior and query policy | Preserve separate behavior registry and request-policy installation, clear and lookup. Expired/native callbacks are checked before dispatch; synchronous reentrant close is rejected. Underlying service execution and protected publication retain their own exits. |
| UC11 | Checker registry | Separate installation, membership and checker-first dispatch. The 1,362 complete original Checker observations and unchanged native contract cover typed/custom registry behavior, sparse values, required findings and explicit retention. All three existing reentrant controls now also run through imports. |
| UC12 | Required-value validation | Capture status before custom Fix, run the custom checker first, then collect metadata requirements. Skip nullable/version fields; require omitted create fields and explicit plain NULL; suppress only an already-required identical location. Translation fills absent messages. Required-create-payload validation remains metadata-only and never reruns Fix. Source-body review joins the qualified Checker and governed-create evidence. |
| UC13 | Fix scope and evidence snapshots | fix_time and record_fix_evidence delegate to the active graph scope; outside-scope behavior remains separate. Last-evidence replacement/lookup returns independent snapshots. Qualified Checker/Behavior and runtime-state/ledger evidence retain deterministic time and operation-local provenance. |
| UC14 | Language and I18n | Preserve locale parsing errors, language/catalog installation and fill-only-missing translation, including existing empty messages. The accepted language/catalog source review includes actual Context translation, expiry and explicit destination retention. |
| UC15 | Internal IDs and bootstrap entities | Optional generation, required next_id, canonical ID space, generator errors and ID-floor-before-mark_as_new remain independent. The 171 exact internal-ID observations exercise actual Context entries. Full ID algorithms retain their own source disposition. |
| UC16 | Business clock and services | BusinessDate resource overrides the installed clock's derived date. Business-ID service/key installation and one allocator's separate allocation/schema interfaces preserve their ordering and errors. The 859 complete service observations include real Context installation. Full Business-ID algorithms remain separate. |
| UC17 | Schema provider and invocation proof | SchemaInvocation remains private and unforgeable through the supported API. Provider installation and generated callback installation remain separate from invoking them. Context waits for ordinary, dynamic, Business-ID and scoped bootstrap phases in order; the separately qualified dynamic-schema repair retains all 1,248 source outcomes and current lifecycle/sanitizer evidence. |
| UC18 | Typed/named resources | Preserve distinct keys, replacement, lookup and require errors, including wrong-type shadowing. The 410 scripts compare actual public calls. C++ managed-resource origins, native type labels and deferred arena replacement cleanup remain explicit adaptations; no resource lifetime extends implicitly. |
| UC19 | Locals | Put/get/remove preserve arbitrary keys, exact Value kinds and absence versus NULL. Locals remain separate from services and typed/named resources. The same 410 scripts retain nested payloads and independent Context state. |
| UC20 | Entity reference codec | Installation and encode/decode delegate to the separate trusted codec, preserving missing-codec failure before invocation. The accepted entity/document/reference source gate includes actual Context resource paths and custom-provider lifetime checks. |
| UC21 | Round-trip references | Runtime/principal resource wrappers, configuration-before-authorization errors, mode/startup notice, issue/consume and mode-checked wire conversion remain distinct. The accepted complete reference protocol supplies unchanged current source comparisons and original tests. |
| UC22 | Context-bound documents | Service/principal ordering, open/accept, snapshot issue/consume and complete arguments/results remain delegated to application-owned services and the trusted reference runtime. The accepted 432-case service matrix includes real Context dispatch, errors, pending work and teardown. No default document projection/save is invented. |
| UC23 | Audit delivery | Attach current governance and generated-bootstrap identity; select a present new version before current values, without falling back after failed integer conversion. Enqueue in an active commit scope; otherwise deliver raw/custom-safe/LogManager in order. Telemetry success/failure wraps delivery. Separate accepted audit/privacy/commit sources and original Context event observations preserve these responsibilities; entire protected caller/provider publication remains open. |
| UC24 | DataStore | Retain get/put/remove SPI, explicit installation and absent-store no-op behavior. InMemoryDataStore clones returned values, uses TTL and does not erase expiry merely on read. The 251 actual source observations and unchanged native pending/foreign/retention contract qualify this implementation and Context delegation. It is distinct from pagination stores and external cache integrations. |
| UC25 | Original tests and ownership | Execute the exact six tests embedded in this file and their complete canonical C++ root/tail ports. New module registrations reuse every old test/probe body. All Context results remain checked/raw borrows under the established owner-thread, arena, explicit-copy and joined-operation adaptation. No global cache, shared owner or third allocation lifetime is introduced. |

The table reviews production bodies and their behavior, not just names or file
counts. Its delegated responsibilities do not merge Context with data services,
SQL, transport, providers, JSON decoders or application policy.

## Fresh compilation and retained observations

The new independent Context observer project compiles seven retained drivers
directly through manifest paths. All registry versions/checksums match the
reference; old projects and corpora stay untouched. The 3,362 observations cover
defaults75, models234, resources/locals410, Checker1362, DataStore251,
business service859 and internal ID171. No runtime algorithm is copied into a
fixture, and output comparison performs no normalization.

The actual original Context tests run in a complete byte-checked runtime view.
All 67 original source/test/fixture files retain their bodies; no observer is
appended for this gate. The root file's active-root1, logging4 and ledger-reuse1
tests are selected explicitly. Child-module tests are not counted as those six.
The complete original library test registry has 273 entries and no generated
support observer names.

An initial run revealed a source-view cache defect. `copytree` preserved original
source mtimes, allowing Cargo to reuse a newer root-package binary compiled
with another view's appended observer. Its cached diagnostics referred to
generated-support lines absent from this view. That run also selected child
modules (22 tests) while the driver expected eight. The
[failed run](parity/context_source_initial_driver_failure_5_0_6.json.gz) and
[helper/controller snapshot](parity/context_source_initial_driver_failure_details_5_0_6.json.gz)
remain excluded from fresh compilation evidence.

ReferenceRuntimeView now uses fresh copy mtimes while verifying identical bytes.
This forces the current view to compile; explicit test listing confirms 273
original entries without appended observers. This is a development harness
repair and does not change frozen source or runtime behavior. The later
[missing module target](parity/context_source_initial_module_target_failure_5_0_6.json.gz)
and [test-selector failure](parity/context_source_initial_selector_failure_5_0_6.json.gz)
are also retained: the first omitted internal_id_probe registration; the second
required the existing module tail test's authentic name.

## Independent evidence joins and boundaries

The named source review joins these complete, separately scoped responsibilities:

- [Context schema lifecycle](context_dynamic_schema_lifecycle_5_0_6.md).
- [Checker/Behavior and Fix](checker_behavior_acceptance_5_0_6.md).
- [Runtime state bridge](runtime_state_bridge_5_0_6.md).
- [JSON graph decoder boundary](json_graph_decoder_5_0_6.md).
- [Language/I18n](i18n_language_source_review_5_0_6.md).
- [Context SQL logging](context_logging_source_review_5_0_6.md).
- [Telemetry](telemetry_source_acceptance_5_0_6.md).
- [Commit audit](commit_audit_source_review_5_0_6.md).
- [Document and reference protocols](document_reference_acceptance_5_0_6.md).

The [current execution record](parity/context_source_execution_5_0_6.json.gz)
binds exact source compilation, observations, named tests and stable selected
builds. Its `complete` field records that gate, rather than whole-source signoff.
The [delegation gate](parity/context_source_joins_5_0_6.json.gz) additionally builds
all 22 selected targets per frontend before executing the 23 named checks. It
rebuilds the original Context event observer from the complete frozen archive
with two explicit private-access bridges, verifies dependency identities, and
compares all 1,088 event outcomes through each frontend. The unchanged private
commit-audit probe independently matches its 99 retained outcomes.

The [reuse record](parity/context_source_reuse_5_0_6.json.gz) checks the relevant
current implementation hashes against the separate scoped source receipts.
Changed historical package/test registrations are identified explicitly; no
older SDK, configuration, integration or capability claim is refreshed merely
because an implementation body is unchanged. The prior schema gate retains
its original native/imported/sanitizer lifecycle scope, with all eight source
and controller hashes still matching.

The [preservation proof](parity/context_source_preservation_5_0_6.json.gz) retains
all 3,457 files in 78 SDKs, 2,201 protected reference artifacts and all 34 archived
C++/Rust implementation/observer files. Ten additional canonical module entries
import existing test/probe bodies; package interfaces and exports are unchanged.
Current mechanical checks verify 976 canonical bodies and 1,516 exports. These
counts establish neither execution nor semantic acceptance by themselves.

The separate [whole-source qualification](parity/context_source_qualification_5_0_6.json.gz)
joins UC01-UC25, source bodies, current execution and the explicit reused evidence.
Only `context.rs` advances: **54 accepted / 68 pending / 2 excluded_external**.
All seventeen capability verdicts remain unchanged. G12's invalid naming-profile
decision, G08/G15 quoted-FK outcomes, protected publication, child-source reviews
and full current P0-P7 integration retain their independent requirements.
Historical failed whole-suite results are unchanged.

No production C++ function/header, generator or generated artifact changes in
this continuation. Existing lifecycle/sanitizer evidence retains its own scope;
the source/provenance and test-registration work does not rerun sanitizer,
MIPS or physical-router profiles.

## Reproduce

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_context_source_5_0_6.py --modules \
    --record /tmp/teaql_context_source_fresh_5_0_6.json.gz
python3 scripts/verify_context_source_joins_5_0_6.py \
    --record /tmp/teaql_context_source_joins_fresh_5_0_6.json.gz
```
