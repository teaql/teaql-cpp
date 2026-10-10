# Typed transaction scope source contract review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Complete original production review covers context/transaction.rs, lines 1–212.
Its scope owns one real provider transaction and borrows UserContext; it is
independent of ambient execution and generic graph transaction boundaries.
Acceptance is recorded separately from the broader graph, governance, privacy
and complete Stage 5 gates.

| Source responsibility | Native mapping and semantic boundary |
| --- | --- |
| Private start_transaction | Require the exact typed registered executor, then await begin before creating the scope; resource/begin errors map to RuntimeError::Transaction. No eager transaction at Context construction. |
| execute_in_transaction | One FnOnce callback and one provider transaction. Callback output T has no Clone requirement. Begin failure prevents callback invocation. |
| Scope context getter | Borrow the UserContext supplying metadata, policy, telemetry and audit configuration. Application code does not receive Global through this getter. |
| entity_data_service | Check entity registration before returning a service bound to the transaction executor; missing entity remains ContextError::MissingEntityDataService. Ambient services remain independent. |
| query/mutate | Use transaction-owned observed execution with select/mutation observer gates. Map provider errors to Transaction and record successful metadata once. Requests use distinct query and mutation types. |
| save_audited | Use the transaction-bound audited ledger save. Remember distinct ledger identities without clearing them yet; preserve returned entity and pending intent until enclosing commit. |
| execute success | Take/consume the transaction, commit, then drain and clear remembered ledgers only on success. Return callback ownership directly; commit failure retains pending ledgers. |
| execute error | Take/consume and rollback. Successful rollback returns the original business error; failed rollback combines both original error displays into the exact Transaction diagnostic. |
| Completion misuse | Reject repeated completion and later provider work. A kept wrapper never restores consumed transaction access. |
| Cancellation/close | Cancel/join child work and release the acquired native transaction before arena reclamation. Provider RAII performs actual rollback/cleanup. |

## C++ ownership adaptation

TransactionScope, callbacks, requests, asynchronous states and outputs belong
to the explicit User domain. Checked native borrows and quiescence prevent
in-flight provider work from using reclaimed host memory. Cross-domain callback
operations are rejected. Rust's drop of the owned future is expressed by explicit
cancel or owning Context close; an abandoned C++ borrow does not own or prolong
its Context. Allocation/poll/close remain owner-thread operations.

A copyable completed result can remain available as a native result borrow.
For a noncopyable completed result, ownership must pass through `take_result`,
which invalidates that child result borrow without polling/cancelling completed
work. Neither path invents a Clone bound absent from the original generic API.
Provider errors likewise have no original Clone requirement; Rust's io::Error
fixture is already non-Clone.

## Installed generic ownership repairs

The original callback output and provider error types have no Clone bound.
Compiler fixtures exposed unwanted native copies; installed adapters now copy
only when domain-copy capability exists, otherwise take the completed value.
The callback value remains owned across suspended commit. Begin, query,
mutation, commit and rollback errors retain their exact source diagnostics.
AuditedLedgerResult advertises allocator-copying only when its entity supports
it, allowing the same completion adapters to transfer a noncopyable entity.

The original non-Clone callback fixture covers sixteen combinations of begin,
body, commit and rollback outcomes and verifies ownership through Drop counters.
The separate original non-Clone entity fixture executes all 356 audited-save
inputs. Its dynamic timestamp projection is explicit observation scaffolding;
it does not establish complete event diagnostics or privacy acceptance.

Native contracts disable default PMR allocation and verify suspended commit,
explicit result taking, exactly one callback and error destruction, cancellation
and actual User deletion returning all arena blocks. Real SQLite contracts
verify audited and raw writes share one physical transaction, transaction-owned
reads see pending writes, rollback retains retryable ledger intent, successful
commit clears the same ledger identity, and explicit Global copies survive User
destruction. Repeated saves deduplicate ledger identities; callback, commit and
rollback errors retain pending intent. Post-BEGIN allocation failure releases
the acquired native transaction without requiring another allocation.

Returned foreign callback work is cancelled before rejection. Native transaction
tracking validates provider-returned work; cancellation joins child and terminal
work before native transaction cleanup. Callback exceptions follow the same
cleanup path. Private start remains an implementation responsibility expressed
by ExecuteInTransaction; application code receives UserContext through the scope.

Historical staged compiler/target evidence remains in
[transaction ownership review](../measurements/parity/2026-10-08-transaction-move-review/README.md),
[noncopyable ledger review](../measurements/parity/2026-10-08-ledger-move-review/README.md)
and [returned-work review](../measurements/parity/2026-10-09-runtime-foreign-work-review/README.md).
Those dated records describe their own snapshots.

## Complete source reconciliation, 2026-10-09

All 212 original production lines were compared with the installed scope,
request/save, execution and begin adapters. The table above covers every public
entry and private completion responsibility. TransactionScope derives no Default,
Clone, Eq, Hash, Debug or serde contract. Its configured select/mutation observer
flags come from UserContext; source Mutex/Send/Sync scheduling adapts to explicit
Context-owner execution. No implicit shared lifetime or serialization is added.

The latest complete native regression passes 369/369 after all Context layout
changes. Fresh transaction-specific native checks pass 11/11. A freshly rebuilt
locked, offline original oracle agrees with all 1,288 transaction observations,
16 non-Clone callback observations and 356 non-Clone entity-save observations.
Fresh ASan/UBSan and MIPS/QEMU selections each pass 11/11 after their 22-task
consumer builds finish, with no-work before/after testing. Sanitizer leak
scanning is disabled. Native/MIPS comparisons agree with the live original
oracles. Complete source acceptance is recorded in
[the current evidence](../measurements/parity/2026-10-09-transaction-source-acceptance/README.md).
Broader graph, governance/privacy and Stage 5 acceptance remain independent.

Physical WNDR4300 testing is deferred at the user's explicit request. Current
target acceptance uses cross-compilation and QEMU; no hardware run is claimed.
