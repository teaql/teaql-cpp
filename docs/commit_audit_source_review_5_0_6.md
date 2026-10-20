# Complete transaction-local audit queue source review

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This review covers all 232 lines of `teaql-runtime/src/commit_audit.rs`, including
its five original unit tests. It complements the earlier
[implementation checkpoint](commit_audit_5_0_6.md); it does not accept the whole
Context event pipeline, event model, transaction implementation or G17.

Status: the complete source is accepted under CA01-CA10 after installed module
repair, durable exact-source reproduction and the additional C++ controls below.
Source dispositions are **28 accepted / 94 pending / 2 excluded_external**.
All capability statuses and the incomplete P0-P7 objective remain unchanged.
Earlier pending statements describe the preserved temporary checkpoints.

## Responsibility and adaptation map

| ID | Complete source responsibility | Current C++ mapping and boundary |
| --- | --- | --- |
| CA01 | Pending batch and task-local identity | `detail/commit_audit.hpp` retains a distinct operation-owned batch, exact UserContext identity and complete raw facts. GlobalContext holds a temporary borrowed activation pointer only during synchronous callback/child polling. No queue is stored on UserContext or in a hidden singleton/TLS. |
| CA02 | `has_scope` | Match only the exact active Context. A different Context cannot join the queue. Owner-thread execution replaces Tokio task-local scheduling; interleaved operations must activate their own batches and restore the previous pointer at every suspension/return. |
| CA03 | `try_enqueue` | The matching scope appends the entire fact in order. An unmatched call leaves delivery to its caller. The C++ boolean-plus-borrow API preserves the caller's original fact and explicitly copies a buffered fact into its batch; it does not require a separately boxed unscoped event. |
| CA04 | Dynamic enrichment selection and failure | Select the newest fact whose entity matches and whose `values["id"]` converts to the requested u64. Wrong/missing scope, owner or ID returns the distinct `DYNAMIC_FIELD_AUDIT_SCOPE_REQUIRED` error, even for an empty change list. Already prepared C++ spans replace the source IntoIterator input; no iterator side-effect contract is claimed for this different interface. |
| CA05 | Complete change application | Some(new_value) updates current values and an existing new-values snapshot. None does not erase current values or create a missing snapshot. Every change appends its field and full old/new payload in order, including duplicates. Old snapshots, unrelated facts and other event metadata are retained. This is audit enrichment, not a persistence operation. |
| CA06 | `collect` future storage and generic output | Original work is Box-pinned before entering the scope. C++ transaction/graph operation state and ContextOnceFunction hold work outside the public operation's callback interface. A current 64 KiB callback control executes both small and large Context-owned captures, checks fixed carrier size and bounds the actual ScopeExecution carrier, then verifies full User storage reclamation. Non-Clone callback/output contracts remain separate existing transaction evidence. No shared owner extends Context lifetime. |
| CA07 | Scope restoration and queue extraction | `AuditActivation` restores the prior activation through RAII. Manual scope execution and automatic graph Save retain their own batches through suspension and terminal commit. The source collect helper returns work output and pending facts; commit/rollback decisions belong to its callers. Do not attribute persistence or rollback semantics to this helper alone. |
| CA08 | Nested and concurrent operation isolation | Inner scopes own distinct queues and restore the outer activation. An operation on another Context cannot enqueue into the active queue. The original five tests include concurrent Tokio tasks reusing one Context. The current C++ control interleaves two real AsyncOperation instances on that same Context, completes them in reverse order and checks every fact and activation restoration across suspension and completion. This is the approved single-owner cooperative polling adaptation, not unrestricted concurrent Context allocation. |
| CA09 | Delivery and errors | Call direct Context delivery for every fact, retaining the first returned error while attempting later facts. Wrap that error once in AuditAfterCommit. Direct delivery bypasses an enclosing queue; an empty vector succeeds. Raw/safe sink policy and per-event sink ordering remain Context responsibilities. |
| CA10 | Context cleanup and caller integration | Pending facts and nested payloads use their operation Context. ScopeExecution and GraphSaver clear successful ledgers before delivery, discard failed/cancelled queues, and cancel/join children and terminal operations before cleanup. Existing real SQLite tests distinguish commit, rollback, deferred commit failure, cancellation and audit failure after commit. Explicit Global copies are required for retained errors/results. |

The source recovers poisoned event mutex contents. Under the approved C++
owner-thread profile there is no separate poisonable queue mutex; that is an
explicit threading adaptation. It does not establish arbitrary cross-thread
equivalence. Allocation failure and exceptions restore activation through RAII;
they must not be described as a source Result error or a guarantee that committed
business writes rolled back.

## Direct source verification

Private helper verification uses a temporary view of the complete runtime crate.
Every original source and original test body is copied byte-for-byte. An observer
test module is appended after the original commit-audit module's exact bytes;
it calls those actual private functions and the actual Context implementation.
There is no substitute Context, copied queue algorithm or callback shell.

The temporary manifest derives normal dependencies and their features from the
real crate metadata. It retains the macro dev dependency, excludes the disabled
external-exporter test dependencies, and starts with the frozen reference lock.
Registry name/version/source/checksum identities are checked before the locked
offline test build. The original source hashes, complete manifest/lock, appended
observer and terminal commands are retained in the
[source replay receipt](parity/commit_audit_source_replay_5_0_6.json.gz).
Its `complete` field distinguishes an ongoing replay from terminal success.

The initial temporary view omitted two directories used by compile-time test
fixtures. Its [compiler failure](parity/commit_audit_source_initial_fixture_failure_5_0_6.json.gz)
is preserved separately. The continuation copies the original `tests` and
`test-vectors` directories and verifies their bytes. No original test or runtime
algorithm is removed to make that compilation succeed.

The finite enrichment controls distinguish no scope, exact scope and another
Context; present/absent new-values snapshots; matching, negative and missing IDs;
and empty, removal, NULL, Unicode/embedded-zero and repeated changes. Multiple
facts check reverse selection and preservation of a different entity's fact.
Delivery controls cover all eight error masks across three facts plus an empty
batch, including delivery while an enclosing collection scope is active.
Complete event Debug/pretty output and structured error outcomes are compared,
not just counts or boolean success.

The five original tests and the added observer test now pass against the exact
reference dependencies. All **99/99** complete actual-source/header observations
match: 90 enrichment cases and nine direct-delivery cases. Source algorithms and
original test bodies remain unchanged. The existing real SQLite imported
transaction-audit contract also passes independently.

The existing complete C++ commit-audit body also verifies explicit activation,
nested delivery, suspended terminal commit, cancellation, failed commit,
continued fan-out and recursive Global error retention after actual User
deletion. The real SQLite body independently checks physical autocommit, visible
committed rows, ledger clearing, rollback and failure boundaries. Current
canonical imported coverage is inspected independently of source behavior.

## Acceptance boundary

During the initial review, execution inputs remained frozen for the complete
native epoch. Temporary source views, consumers and compressed receipts did not
modify its sources, producer, SDKs or module configuration. A missing module export/registration found by
the canonical complete test must be retained as a gap and repaired after the
epoch is terminal; a header fallback is not imported acceptance. At that
checkpoint formal source promotion and durable reproduction remained pending.
The installed continuation below supplies that integration step. No
new sanitizer, MIPS, external exporter or physical-router result is claimed.

## Reproduced module gap and verified export-only repair

At the gap checkpoint, the canonical complete kernel test failed to compile because five
existing declarations are not visible through imports: `PendingAuditBatch`,
`AuditActivation`, `has_audit_scope`, `enrich_dynamic_audit` and
`deliver_committed_audits`. The current module registry also omits that complete
test. Its unchanged body passes through the header frontend. The source replay
receipt preserves the failed import compiler output rather than reporting the
header result as module acceptance.

An isolated temporary runtime facade exports the complete existing helper set:

```cpp
using ::teaql::detail::AuditActivation;
using ::teaql::detail::ContextAuditAccess;
using ::teaql::detail::PendingAuditBatch;
using ::teaql::detail::deliver_committed_audits;
using ::teaql::detail::enqueue_audit;
using ::teaql::detail::enrich_dynamic_audit;
using ::teaql::detail::has_audit_scope;
```

The temporary runtime and downstream SQLite facades compile successfully against
the existing package implementations. The entire canonical kernel test passes
through those imported facades, and all **99/99** original observations match a
separate imported observer. Runtime/provider implementation and header bytes are
unchanged; the downstream provider facade is byte-identical. The
[shadow verification receipt](parity/commit_audit_module_shadow_5_0_6.json.gz)
retains the exact seven-alias patch, commands, outputs and source/body proof.
At that checkpoint the repair was verified only in a temporary copy. The later
installed result is recorded separately below.

The first imported-observer adaptation correctly failed its canonical-body
check because it removed the quoted fixture include position. The
[initial prologue rejection](parity/commit_audit_module_shadow_initial_prologue_failure_5_0_6.json.gz)
is preserved. The corrected adaptation retains that include at its original
position and passes the unchanged canonical check. The two successful facade
builds and complete kernel test are reused only after verifying all five
completed object/PCM/executable hashes; only the corrected observer needs a new
compile and comparison. This failure concerns observer preparation, not the
export-only repair or runtime execution.

After the native epoch is terminal, install the verified aliases and register
the complete canonical kernel test, refresh module export/coverage records and
move the reference-locked reproduction into durable repository fixtures. Review
the new imported results and current native integration before changing the
source disposition. Source status remains pending in this checkpoint.

## Installed module continuation

After the frozen native epoch became terminal, the seven verified aliases were
installed in teaql_runtime's existing facade. The isolated lookup and export
catalog were updated together, and the complete existing commit_audit_tests body
was added to the canonical registry and module preset. There are no runtime
function/header or generated SDK changes. The current catalog verifies 1,513
declarations and the canonical checker verifies 882 consumer/fixture bodies.
The installed build and three selected imported tests pass, recorded in the
[installed receipt](parity/commit_audit_module_installed_5_0_6.json.gz); its complete
flag and terminal commands retain their actual success independently. The original
temporary source and gap evidence remain unchanged.

## Durable source qualification

The [repository helper](../scripts/verify_commit_audit_source_5_0_6.py) now
recreates the private-source view from the actual frozen crate. Every original
source, test and test-vector file is copied and checked byte-for-byte. The
[unchanged observer module](../tests/parity/commit_audit_source_observer_5_0_6.rs)
is appended after the original commit_audit.rs bytes. Normal dependency features
come from the actual crate metadata; only disabled external-exporter test
dependencies are omitted. All registry versions/checksums and local package
versions match the fixed reference lock before the locked test build.

The [installed replay](parity/commit_audit_source_installed_replay_5_0_6.json.gz)
passes all **five original tests plus one observer**, then compares all
**99/99** actual original outcomes with retained expectations, native C++ and
imported C++. No algorithm, Context shell, output normalization or test-body
replacement stands in for the source implementation. The new
[permanent corpus](../tests/parity/commit_audit_source_5_0_6_cases.json.gz) retains
the earlier 90 enrichment and nine delivery inputs/outcomes in their original
order. The native comparator is now registered in CTest, bringing the native
registry to **738**; this does not claim a full 738-test pass.

The [installed gates](parity/commit_audit_source_installed_gates_5_0_6.json.gz)
pass **native 3/3** and **imported 2/2**, including the full existing kernel and
physical SQLite transaction-audit bodies. Two further C++ controls extend the
kernel without replacing any previous test: CA08's same-Context cooperative
interleaving and CA06's 64 KiB captured work. Both extended canonical bodies pass
**native 1/1** and **imported 1/1** in the
[follow-up](parity/commit_audit_scope_controls_5_0_6.json.gz). The
[before archive](parity/commit_audit_scope_controls_before_5_0_6.json.gz) retains
the complete earlier C++ body. Default PMR allocation is disabled in the new
controls; actual User deletion returns all User bytes. Full format/check passes
across 1,676 handwritten C++ files and all 883 current canonical bodies verify.

No audit runtime function/header or generated SDK changes enter this repair.
The seven existing declaration exports and full canonical registration close the
module gap. These source, ownership and import gates accept this file's complete
CA01-CA10 responsibilities, while its callers, event/privacy pipeline and complete
G13/G17/P0-P7 remain separate obligations. No additional sanitizer/MIPS profile,
external exporter, GPU or physical-router result is claimed.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_commit_audit_tests \
    teaql_commit_audit_source_probe teaql_commit_audit_transaction_tests
cmake --build --preset modules_native --target teaql_module_commit_audit_tests \
    teaql_module_commit_audit_source_probe teaql_module_commit_audit_transaction_tests
ctest --preset native -R '^(commit_audit_contracts|commit_audit_source_rust_parity|commit_audit_transaction_contracts)$'
ctest --preset modules_native -R '^module_commit_audit(_transaction)?_contracts$'
python3 scripts/verify_commit_audit_source_5_0_6.py \
    --module-build-dir build_modules_native --record /tmp/commit_audit_source_fresh.json.gz
```

The [terminal acceptance receipt](parity/commit_audit_source_acceptance_5_0_6.json.gz)
joins the complete responsibility review, actual-source gates, installed module
repair, scoped C++ adaptations and preserved before-state. Earlier failed or
temporary checkpoints are not relabeled.
