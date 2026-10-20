# Document and round-trip reference source acceptance

Status: qualified SQ08 protocol group with the explicit C++ adaptations below.
The [acceptance receipt](parity/document_reference_acceptance_5_0_6.json.gz)
joins whole-source review, actual-source observations and terminal gates.
The acceptance scope is the whole
production responsibility of `teaql-runtime/src/document_round_trip.rs` and
`teaql-runtime/src/round_trip_reference.rs`, plus the independent
`teaql-runtime/src/entity_reference.rs`, at Rust 5.0.6 commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. This is the SQ08 protocol group;
G13, other source dispositions and full P0-P7 integration remain independent.

## Complete responsibility review

| ID | Source responsibility | C++ implementation and evidence |
| --- | --- | --- |
| DP01 | Document constants and enum protocols | `document_round_trip.hpp` and `document_snapshot_codec.cpp` retain `tqd1`, version 1, all eight limits/prefix constants, three ordered mutation variants and two completeness variants. Wire values remain 1/2/3 and 1/2; unknown values reject. The model, malformed-payload and maximum-boundary matrices cover their diagnostics and wire behavior. |
| DP02 | Entity snapshot construction and validation | Ordered sets deduplicate loaded/writable fields and mutation kinds. Row/type text, nonzero ID, nonnegative version, field bounds, writable subset and nonempty mutation kinds retain source error codes and ordering. `DocumentEntitySnapshot::create` validates; allocator construction permits direct public assembly. |
| DP03 | Relation snapshots | Ordered row vectors, duplicate-row rejection, completeness, writable flag, allowed mutations, readonly restrictions and maximum rows remain distinct. The model matrix compares every field; the malformed/limit matrices exercise wire and cardinality failures. |
| DP04 | Aggregate snapshots and lookup | Fingerprint/business/scope/aggregate identity stay separate. Validation checks text, collection limits, aggregate/scope equality, unique entity row keys and relation membership. Empty entities and repeated relation names retain source behavior; lookup returns the first matching item. No extra persistence authority is inferred from valid metadata. |
| DP05 | All nine document model/request/result structures | `ContextualDocument`, submitted/open/accept/accepted/verified values and three snapshot types retain all original fields. Only source-defined factories validate. Complete Debug, equality, explicit copies and 47 field mutations are qualified by the 192-case model matrix. JSON bodies retain source Value semantics. |
| DP06 | Document errors | Nine factories, static code, Display, empty Error source, Clone/equality/Debug and `From<RoundTripReferenceError>` mapping retain exact codes. All nine service error results and all five custom reference-provider errors propagate without reclassification. |
| DP07 | Complete document payload/AAD codec | Source field order, big-endian signed/unsigned integers, four-byte text/count framing, UTF-8 validation, sorted sets, ordered relation rows, duplicate rejection, checked reads, exact trailing-byte rejection and validated decoded reconstruction remain intact. The 661-case corpus includes 453 independently authenticated malformed stimuli; exact forged token bytes are compared too. |
| DP08 | Document issuance and consumption | Principal/snapshot/lifetime validation, aggregate-then-member authorization, current-key lookup, clock/expiry, actor fingerprint, distinct document HKDF label, AAD and nonce retain source order. Consume preserves key-by-ID/decrypt/decode/time/binding/revalidation/reauthorization order. The 154 custom-provider cases compare complete callbacks, exact tokens, all five provider errors, old-key retention, missing/wrong keys, expiry/future boundaries and Context replacement. |
| DP09 | Asynchronous service/resource protocol | The service remains an application-owned open/accept SPI with the complete UserContext and trusted principal. No default document projection or Save is invented. The 432-case source comparison covers lazy lookup, complete arguments/results, all nine errors, typed resources, replacement between operations, pending/wake/completion/drop and actual native Context close. |
| DP10 | Entire pre-existing round-trip reference responsibility | The prior [complete source contract](reference_api_contract.md) accounts for every model/trait/default, error, provider, wire codec, raw/governed profile, startup notice, issuance/consumption helper and Context resource boundary. All 933 reference cases and 229 model cases now match actual frozen source with exact reference dependency versions/checksums. Current native/imported contracts exercise lifetime, reentrant guards and all seventeen directly ported original tests. |
| DP11 | Every 5.0.6 reference-source change | The exact old/new diff changes visibility for principal validation, key-secret access and derivation, and adds eight direct internal access/delegation methods for document execution. It changes no existing model, wire or execution body. Native document code uses its corresponding scoped fields/providers and existing derivation/fingerprint helpers; DP08 compares their actual invocation order and values. The original current-source unit filters pass document **3/3** and reference **17/17** without modifying frozen source. |
| DP12 | Ownership and frontend integration | User/Global arenas, checked provider borrows, explicit nested copies, actual User destruction, zero retained User bytes and cancellation/quiescence remain the approved C++ adaptation. Module consumers execute canonical bodies. Targeted sanitizer evidence combines the new provider/dispatch/Context checks with the unchanged model-copy and reference ownership receipts. |
| DP13 | Independent older entity-reference codec | Its complete source contract remains distinct: seven-field claims, separate errors/SPI, numeric active/retained keys, fixed AAD, codec-specific validation and DateTime overflow behavior, claims decoding and UserContext installation. The file is byte-identical across 5.0.5/5.0.6; current exact-dependency source replay matches all **548** retained inputs. Both actual original tests pass **2/2**. Current native/imported lifetime/reentrant checks and native NIST vectors retain their separate scope. |

## Custom provider matrix

The 154 inputs exercise direct runtime and UserContext entries. Errors are
injected at construction, issue-key lookup, consume-key lookup and each of the
three ordered authorization positions, using every reference error code. Other
controls cover absent/mismatched/wrong-master keys, current-key rotation with
retained decode-only keys, changing nonces including byte wraparound, custom
nanosecond clocks and expiry/future boundaries. Context tests cover missing
runtime/principal precedence, fluent/setter registration and replacement of the
runtime namespace and principal.

Callbacks record every principal, scope and entity-identity field. Results retain
the exact token, complete decoded snapshot Debug, snapshot equality, selected key
and issue/expiry timestamps. Observers implement only injected provider behavior
and controlled inputs. Production cryptography, codecs, policy sequencing and
Context delegation remain the observed implementations. No token decoder or
alternate authorization orchestrator is copied into the new observer.

## Source/dependency provenance

The [provider comparison](parity/document_providers_initial_comparison_5_0_6.json.gz)
retains all 154 actual original/native outputs and hashes. Its independent
workspace has exact reference registry versions and checksums. The old model
observer workspace has dependency drift; its old output is retained rather than
treated as sufficient provenance. An independent exact-dependency workspace
replays its unchanged driver and all 229 unchanged inputs. All outputs agree.
The 933-case reference workspace already has exact dependency identities; its
unchanged producer is verified separately.

The [source replay](parity/document_reference_source_replay_final_5_0_6.json.gz)
compares both complete corpora against actual frozen Rust and current C++ without
normalization or case exclusion. Old corpus bytes and historical acceptance
labels are preserved. No new runtime dependency or generator change is needed.
The independent [codec replay](parity/document_reference_codec_source_replay_5_0_6.json.gz)
likewise retains all 548 source/native outcomes without rewriting its corpus.

## Source-supported behavior and explicit C++ adaptations

- Valid snapshots may issue tokens larger than the consumer's 256,000-byte cap.
  The issuer has no corresponding size rejection; maximum legal collections
  therefore need not produce consumable tokens. The 21-case boundary checkpoint
  preserves this source asymmetry.
- Document snapshot validity does not establish database authorization, current
  versions or permission to save presentation JSON. Application services supply
  projection, editing rules and governed persistence.
- Raw Debug exposes original fields, including tokens and JSON bodies. It is
  separate from safe diagnostic publication and sensitive-log permissions.
- Rust Arc/Send/Sync and borrowed futures map to checked Context borrows,
  explicit copies and owner-thread operation polling. Pending work is cancelled
  before reclamation; resource replacement does not promise identical physical
  destructor timing. Raw C++ borrows remain invalid after Context close.
- C++ strings are byte containers; protocol text corresponds to Rust's valid
  UTF-8 String domain. Extra invalid-byte states or invalid enum casts are not
  invented Rust inputs. The wire reader rejects invalid UTF-8.
- Native wiping of key/plaintext storage and scoped RAII cleanup do not alter
  protocol bytes or provide cryptographic certification/side-channel guarantees.

## Current validation and environment recovery

The combined document gate passes native **7/7**, imported **6/6** and targeted
ASan/UBSan **3/3**. These cover all five document corpora and existing Context/
reference ownership contracts. The separate complete reference selection passes
native **7/7** and imported **7/7**; one native Context check overlaps. Original
Rust filters independently pass **3/3**, **17/17** and **2/2**. The older codec/
crypto selection passes native **5/5** and imported **4/4**. Together these are
**18 distinct native checks**, **17 imported checks** and **3 sanitizer checks**.
All **833** canonical consumer/fixture bodies and **1,503** exports pass their
verifiers; formatting/check passes for **1,653** handwritten C++ files.

NIST vectors run through the registered native contract. An attempted imported
NIST target lookup failed because that target is not registered; its output is
preserved and no imported NIST pass is claimed. Imported protocol tests link the
same production crypto implementation and compare actual source tokens.

Three pre-existing reference model/source-test consumers had no canonical module
entry. They now execute their unchanged bodies through imports, including the
229-case model comparator, Global-copy test and seventeen original source tests.
This changes registration, not their functions or production exports.

The first isolated original-unit build failed with ENOSPC. Its compiler log and
failed-build disposition are retained. Only its fresh temporary output and the
rebuildable shared Rust incremental cache were removed; four existing observer
binary hashes were verified unchanged. Reusing the existing dependency target
then compiled and passed the original tests. Source, dependency archives,
linked artifacts, migration edits and evidence were preserved.

This accepts three complete source dispositions, bringing the active source
ledger to **8 accepted / 116 pending**. It does not accept the rest of
`context.rs`, logging/privacy/governance, generated application document services
or every runtime file merely because this group uses their interfaces.

Full native P7 integration, other source groups, G08/G12 pending behavior
decisions and G14/G17 remain open. Final leak scanning is disabled. This review
adds no MIPS, physical-device, GPU or external-service acceptance.
