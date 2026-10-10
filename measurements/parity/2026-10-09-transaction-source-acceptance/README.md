# TransactionScope source acceptance evidence

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete 212-line `context/transaction.rs` production source is reviewed in
[the transaction contract](../../../docs/transaction_scope_api_contract.md).
This directory records current installed consumers after the Context layout
changes, rather than earlier staged standalone executables.

The latest complete native regression passes 369/369 in 80.91 seconds; its
622-task build and 418 nonempty compiled dependency records are retained in
[Context integration evidence](../2026-10-09-context-dispatch-follow-up/README.md).
The transaction selection passes 11/11 on native in 7.87 seconds and MIPS/QEMU
in 9.92 seconds. Native refresh and target pre/post-test builds have no work.
The target selection first completed all 22 build/link tasks successfully.
The ASan/UBSan selection passes 11/11 in 12.19 seconds after all 22 build/link
tasks complete, with no-work before/after testing. Leak scanning is disabled.
This accepts the complete TransactionScope source group within the documented
C++ ownership adaptation; it does not close Stage 5.

Fresh locked, offline original oracles and installed native/MIPS probes agree
on all 1,288 transaction observations, 16 non-Clone callback observations and
356 non-Clone entity-save observations. The last fixture projects dynamic audit
timestamps; it is not proof of complete event diagnostics or privacy parity.

The eleven named checks cover typed scope behavior, noncopyable output/provider
errors/entities, actual SQLite transaction behavior and source provider/SQL
transaction contracts. They include default-PMR-disabled suspension, explicit
result taking, callback-once behavior, returned foreign-work cancellation,
post-BEGIN allocation failure, actual User reclamation, native cleanup, repeated
ledger identity and commit-only ledger clearing. Their distinct responsibilities
remain in the production runtime, SQL executor, transport and SQLite provider.

Physical WNDR4300 execution is explicitly deferred by the user. Current target
acceptance uses cross-compilation and QEMU. No physical-device run is claimed.
Complete graph saving, governance/privacy and stages 4–6 remain independent.
