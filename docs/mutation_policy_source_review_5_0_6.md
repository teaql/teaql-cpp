# Complete mutation policy source review: Rust 5.0.6

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: all 739 lines of `teaql-runtime/src/mutation_policy.rs`, including its
six embedded tests. Status: current original tests pass; isolated source/imported
reconciliation is separate from permanent installation, formal source disposition
and complete current native integration. No production function changes here.

The [historical review](mutation_policy_api_contract.md) retains its original
5.0.5 scope. The complete source diff has two production changes: governance work
is boxed before entering its task-local scope, and ledger plans include dynamic
keys and dynamic mutation audit values. A sixth source test checks the wrapper's
size independently of a 64-KiB child future. These changes are reviewed below;
historical acceptance is not relabeled.

## Complete responsibility reconciliation

| ID | Locked responsibility | Current C++ mapping and boundary |
| --- | --- | --- |
| MP01 | Independent warning constants | Preserve `MUTATION-POLICY-001` and `MUTATION-POLICY-002`; absence of policy and absence of exact approval remain different conditions. |
| MP02 | Policy identity validation | Validate id/version/fingerprint in source order using Unicode whitespace; retain original untrimmed strings. Clone does not revalidate public fields. |
| MP03 | Identity traits | Full Debug, Clone, equality, ordering and hash-set equivalence include all three identity fields. Portable numeric hash bytes are not promised. |
| MP04 | Operation and immutable review plan | Preserve four kinds, complete EntityKey, optional original version and ordered changed values; execution/request/root/reason and ordered operations are separate. Value NaNs retain PartialEq behavior. |
| MP05 | Allow/deny decisions | Allow has no code/message/paths. Deny validates only its code, preserves whitespace and arbitrary message, and retains path order/duplicates. `is_allowed` reads the verdict. |
| MP06 | Policy and registry protocols | Identity/review, request resolution and callable registry adaptation remain independent. Empty default/new resolution returns no policy. Source Send/Sync/static ownership adapts to checked Context owner borrows. |
| MP07 | Approval model | Complete identity, untrimmed nonblank approver and SystemTime retain Clone/equality/Debug. No expiry or current-time approval rule is invented. |
| MP08 | Approval provider | Independent lookup and callable adapter; default provider returns no approval. |
| MP09 | Governance models | Source/approval enums, summaries, snapshots and warning events retain all independent fields and declared Clone/equality/Debug capabilities. No serde or extra Default protocols are invented. |
| MP10 | Governance sink | Separate warning SPI/callable adapter. Default prints only on first occurrence, with exact request/source/approval fields, and returns success. |
| MP11 | Context installation | Preserve setters and builder responsibilities for registry, approval provider and sink. C++ checked borrows and explicit scopes replace implicit Arc lifetime extension. |
| MP12 | Review order | Resolve once, then identity, then review. Explicit denial returns before approval lookup, warning collection or business execution. |
| MP13 | Denial diagnostic | Fallback code/message apply only to absent optionals. Present empty strings remain empty; field paths are not substituted into the error message. |
| MP14 | Exact approval decision | Compare complete identity and require a nonblank approver. Timestamp does not affect approval. Missing/mismatched approval warns while an allowed policy remains allowed. |
| MP15 | Default policy behavior | Missing policy produces GeneratedDefault/NotApplicable and the missing-policy warning. It does not automatically deny. |
| MP16 | Snapshot summary | Copy execution/request, policy, warning codes and ordered operation summaries. Changed fields come from actual ordered changed-value keys; private values/reason are not invented summary fields. |
| MP17 | Warning identity and delivery | Deduplicate by exact request, complete identity or `none`, and warning code. Deliver every occurrence; first occurrence controls default printing. The source poisoned-lock fallback is a synchronization boundary; C++ owner-thread state has no exposed poisoned mutex. |
| MP18 | Failure ordering | Record dedup before calling the sink. Result delivery failure prints the source diagnostic and remains fail-open; native exceptions remain separate and propagate without refunding dedup. |
| MP19 | Async governance activation | The original boxes the child before task-local capture. C++ `GovernedOperation<T>` stores a child-operation pointer and its scoped snapshot. Install/restore activation around each owner-thread poll/cancel; no snapshot remains active between polls. |
| MP20 | Cancellation and reclamation | Pending descendants stop under their snapshot before arena reclamation. Callback quiescence rejects reentrant User reclamation; ordinary errors/cancellation restore the previous activation. This retains the explicit application-Global activation adaptation instead of TLS. |
| MP21 | Execution identity | Successful plan projection allocates the next process-local wrapping scalar and exact trace prefix. It owns no Context data; a native mutex replaces the source atomic to avoid a new MIPS atomic dependency. Missing graph root fails before sequence allocation. |
| MP22 | Graph plan projection | Omit references, retain item order and ID fallback, convert signed versions through the source Value conversion, preserve full create values, selected update fields and empty delete changes. Use the planned root's entity/comment. |
| MP23 | Ledger key union and classification | Ordered union includes ordinary changes, dynamic-only keys and deleted keys. Omit newly created-and-deleted entities; deletion wins over creation. Retain original versions and exact complete keys. |
| MP24 | Dynamic audit values | Merge each dynamic mutation under `#<code>` with the original `mutation_audit_value(..., None)` behavior. This can replace an identically named ordinary entry; no old value is inferred. |
| MP25 | Ledger request/reason | Use node entity for request/root; prefer a present ledger comment even when empty, then the node comment. Allocate execution identity after projection. |
| MP26 | Complete source tests | Retain all five original review/approval/warning/denial tests and the new thin-wrapper test without altering assertions. Broader graph Save, protected output and provider persistence remain separate. |

Implementation remains in `mutation_policy.hpp`, `mutation_governance.hpp`,
`mutation_policy_debug.hpp`, the policy/context/plan source files and
`detail/governance_scope.hpp`. All nested model data copies explicitly into its
destination scope. Source-owned Arc relationships do not extend C++ Contexts.

## Current original execution

The actual locked source's `mutation_policy::tests` selection passes **6/6**,
including the 64-KiB wrapper-size regression. Its complete output is retained
with the [isolated replay](parity/policy_source_staged_replay_5_0_6.json.gz).
The original source diff and all test bodies remain unchanged.

The isolated public observer project uses exact reference registry versions and
unchanged model/review drivers. The private plan observer is appended to a
byte-checked disposable runtime view. Only fixture imports, comment placement
and fixture-file paths adapt; graph/ledger projection functions and all original
production/test bodies remain untouched. The retained 2,129 inputs execute in
their original order with no new output normalization.

Current source matrices are 177 model/derive cases, 276 public review cases,
2,129 graph/ledger plan cases and 24 real dynamic-governance cases. The latter
uses the already locked exact-reference dynamic-protocol workspace and actual
SQLite provider; the denying policy prevents persistence after plan inspection.
Their gates and source/consumer installation remain independently recorded.

## Terminal staged qualification

The [initial source replay](parity/policy_source_staged_replay_5_0_6.json.gz)
executes all **2,129** private graph/ledger observations through the unchanged
original functions and exact reference dependency versions. Its later module
build requests a nonexistent `teaql_module_runtime_completion_tests` target and
fails. The actual native test is `runtime_generic_completion_contracts`; no
imported registration exists yet. That unsuccessful selection is retained.

The [corrected replay](parity/policy_source_staged_replay_final_5_0_6.json.gz)
reuses those completed original private observations only after checking the
receipt hash, complete original/copied source hashes, appended observer text,
reference identity and unchanged expected corpus. Public model/review and
dynamic-governance source observations execute afresh. No failed build is
relabeled as passing.

The corrected gate is terminal and passes **2,606/2,606** original/imported
observations across all four complete matrices, **4/4** existing imported
contract/comparator checks and the complete staged model/completion programs.
The direct original embedded tests separately pass **6/6**. Every captured
execution input and both references remain unchanged. All five staged consumers
retain their original function bodies and link without SQLite or its provider;
the already registered dynamic-governance consumer keeps its provider binding.

The complete generic-completion program retains Base/Context/Entity entry
families, nested governance activation, pending/error/cancel behavior and graph/
ledger/transaction cleanup. It is not replaced by a thin-wrapper size assertion.
The model program retains native hash containers, deep Global retention, actual
source User deletion and stale-reference rejection.

The [staged installation archive](parity/policy_source_staged_installation_5_0_6.json.gz)
retains the portable public observer workspace, all proposed module/preset
registrations, complete original/candidate metadata files, private fixture inputs/
observer/controllers and five complete consumer bodies. Its metadata additions
must be merged with the separate Event candidate instead of overwriting it.
Nothing is installed into frozen execution inputs. A portable installed helper,
both permanent frontends and formal source acceptance remain required.

## Portable helper and merged installation candidate

The [portable helper replay](parity/policy_source_portable_helper_5_0_6.json.gz)
is terminal and passes its requested imported profile. It freshly runs all six
original embedded tests plus the appended private-plan fixture in one byte-checked
runtime view, then replays all **2,606** original/imported outcomes. The four
registered module checks, both complete staged programs and final no-work build
pass. This replay does not reuse a previous private observation: the fixture
executes again over all 2,129 retained inputs. Its `all_installed_frontends` is
false because the permanent consumers/native frontend are not qualified here.

The [merged Event/Policy installation archive](parity/event_policy_merged_staged_installation_5_0_6.json.gz)
preserves twelve complete before/after files and proof hashes. It merges the
two candidates' shared CMake, preset and consumer-catalog additions structurally,
so neither candidate overwrites the other's registrations. The package adds six
runtime-only consumers and four missing comparator registrations, preserving all
original C++ function bodies. It contains both tested portable helpers and exact
observer workspaces. Installation and both default frontend gates remain pending
the active native epoch's terminal state.

An incomplete receipt is not a successful gate. Complete callback, nested
activation, pending cancellation, explicit Global retention and actual User
deletion contracts are kept in the complete C++ bodies; no assertion is replaced
with a miniature observer. No new sanitizer, MIPS, physical-device, complete
source disposition, whole G17 or P0-P7 result is claimed here.
