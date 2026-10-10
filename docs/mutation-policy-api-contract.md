# Mutation policy production contract review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete production body is mutation_policy.rs lines 1–545; embedded tests
are additional evidence. This group remains partial until complete installed
integration and policy/save/privacy composition are accepted.

| Source responsibility | C++ mapping and exact boundary |
| --- | --- |
| Warning constants | Preserve MUTATION-POLICY-001 and MUTATION-POLICY-002 independently. Native lowercase constant spellings are also exported through the source facade. |
| Policy identity | Three complete strings; validate blank id, version and fingerprint in that order using Unicode whitespace, retaining original untrimmed values. Clone/equality/order/hash and full derived Debug remain distinct capabilities. |
| Operation/plan | Four operation kinds, complete EntityKey, optional original version, ordered changed values; execution/request/root/audit reason and ordered operations remain separate fields. Value NaNs preserve source PartialEq behavior. |
| Decision | Independent Allow/Deny, optional code/message and ordered field paths. Allow has no code/message/paths; deny validates only blank code and preserves untrimmed code, arbitrary message and duplicate paths. |
| Policy/registry | Independent identity/review and request-key resolution SPIs; scoped callable registry adapter preserves the blanket closure responsibility. Empty registry defaults to no policy. |
| Approval/provider | Complete policy identity, untrimmed approver and SystemTime; constructor rejects blank approver. Independent lookup SPI/callable adapter; default provider returns no approval. |
| Governance models | Source and approval enums, summaries with complete entity keys and ordered changed fields, snapshots with policy/warnings/operations, and events with warning/first-occurrence are independent public models. |
| Governance sink | Independent warning sink and callable adapter. Default prints exactly the source fields only on first occurrence. No Debug is invented for default SPI implementations that do not derive it. |
| Context registration | Separate setters/fluent facades for registry, approval provider and sink; typed checked borrows do not prolong provider lifetime. Defaults behave as source empty registry/no approval/default sink. |
| Review ordering | Resolve once; identity precedes review; denial returns before approval or warnings. Allow checks complete approval identity and nonblank approver, independently of the approval timestamp. |
| Denial diagnostic | Preserve code/message fallback only for absent optionals; empty strings stay empty. Field paths are not substituted into the source diagnostic. |
| Warning delivery | Snapshot summarizes ordered changed-value keys. Dedup key combines request, complete policy identity or `none`, and warning code. Every warning is delivered; first occurrence only controls default printing. Result failure prints a diagnostic and remains fail-open. |
| Async activation | Source task-local snapshot becomes an explicit application-owned activation slot, installed/restored on each owner-thread poll/cancel. Descendant work stops under its snapshot before reclamation; inactive periods retain no task-local snapshot. |
| Execution sequence | Process-local wrapping u64 scalar starts at one and owns no Context payload. Native mutex avoids a new 64-bit atomic requirement on MIPS. Trace prefix and sequence formatting remain exact. |
| Graph plan projection | Require planned root before sequence allocation; omit references; preserve source item order, id fallback, signed version conversion, full create values, selected update fields and empty deletes. Use root entity/comment for request and audit reason. |
| Ledger plan projection | Ordered union of changed/deleted keys; omit newly created-and-deleted keys; deletion wins over creation, original version fallback and changed values remain exact. Prefer ledger comment even when empty, then node comment. |

## Ownership and supported scheduling

Models and nested strings/maps/vectors copy explicitly into the selected Global
or User domain. Resource registration is a checked borrow. Context mutation,
allocation, policy callbacks and activation run on the owner thread; source
Send/Sync is not a promise of arbitrary concurrent Context access. Synchronous
review registers a quiescence guard so a callback cannot reclaim its own active
User stack. Cross-domain callback retention uses the existing explicit scoped
callable contract. Async work joins before arena blocks are returned.

Source Result sink errors are fail-open. Native callback exceptions remain
separate from Result errors; neither the original policy denial order nor Rust
panic behavior is replaced by a blanket success fallback. Warning deduplication
is per User, and sink failure does not refund its first-occurrence record.
Raw model diagnostics expose the source fields and remain separate from safe
audit/log projections; this review does not accept full privacy/provenance.

## Reproduced model capability omissions

A public umbrella compile reproduces missing Debug for public policy models and
missing Hash for MutationPolicyIdentity. The staged repair restores all thirteen
source-derived diagnostics, including nested plan/entity/value/approval/snapshot/
event fields and inherited formatting options. The hash specialization includes
all three equality fields. Native hash bytes and Rust DefaultHasher bytes are
not a portable identity protocol; hash-set equality behavior is the contract.

The actual original oracle records 177 twice-agreeing inputs for constructors,
validation order, Clone/equality/order/hash-set capability, default registry/
approval provider and nine Debug modes. They cover full-width values, NaNs,
Unicode whitespace/data, preserved input spaces, negative/fractional/extended
SystemTime, every operation/source/approval state and empty/nested collections.
Existing separate 276 public review and 2,129 graph/ledger plan observations
remain independent execution evidence; these model cases do not replace them.

A standalone lifecycle test disables default PMR allocation, uses a native hash
set, explicitly retains nested plans/events/policies in Global, actually deletes
the source User, verifies destination-owned maps/values/strings and formats through
a new User. Stale references reject before hashing or formatting.

The repair is staged outside the current native331 inputs. Standalone native/
sanitizer/MIPS validation, production/CMake consumers and complete source/save/
privacy acceptance remain independent gates. Physical-router execution remains
unverified.

## Installed full-body reconciliation, 2026-10-09

All 545 production lines were compared with the installed model/diagnostic,
review, warning, activation and graph/ledger projection implementations. Model
constructors retain validation order and original inputs. Default and callable
providers remain distinct SPIs; no source serde is invented. Review resolves
once, reads identity before decision and returns denial before approval/warning.
Approval compares every identity field and nonblank approver without timestamp
expiry. Warning keys preserve source delimiter construction; every occurrence
is delivered and Result delivery failure is fail-open without refunding dedup.
Graph and ledger projections preserve ordering, version/id fallbacks, references,
cancelled creates and empty present audit comments. Sequence allocation follows
successful projection. Explicit owner-thread activation restores nested snapshots
on poll, exceptions and cancellation, with no active snapshot between polls.

The model repair is installed and covered by the earlier native369 checkpoint.
Original checker and mutation-policy unit suites freshly pass 5/5 each against
the unchanged baseline. Current native/sanitizer/MIPS consumer validation is
active after the separate direct-checker lifecycle repair; policy source
acceptance and complete graph/privacy/stage acceptance remain independent.

Current MIPS probes additionally agree with freshly rebuilt live original Rust:
220/220 checker models, 177/177 policy models, 1,362/1,362 public checker/runtime
observations and 276/276 policy reviews. These comparisons preserve the complete
existing corpora; no normalization or fixture omissions were introduced. Native
full and sanitizer composition validation remain separate unfinished gates.

Current ASan/UBSan composition validation also passes 31/31 in 25.46 seconds,
with leak scanning disabled and no-work before/after testing. All 69 initial
and six scope build/link tasks finish successfully. Whole native integration
remains active; complete source acceptance awaits that final current checkpoint.

## Complete current source acceptance, 2026-10-09

Final native integration passes 371/371 in 210.67 seconds after its 656-task
consumer rebuild, with 418 nonempty compiler dependency records and no-work
before/after testing. All corresponding fresh sanitizer/MIPS consumer selections
pass; complete source/body/helper/input/assertion or API/default/trait/protocol/
ownership review is recorded above. The reviewed source groups in this document
are accepted within their explicit C++ ownership and SQLite scope. This does
not accept broader Context/services, graph/event/log privacy, runtime-root or
complete stages 4–6. Current full output is retained in
measurements/parity/2026-10-09-checker-direct-callback-review.
