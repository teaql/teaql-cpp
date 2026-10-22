# SQLite ledger ID allocation transaction gap (Rust 5.0.6)

Status: the original SLAT03 lease defect is fixed and the focused repair
checkpoint is qualified. This was an implementation gap, not an approved
compatibility exception. Whole-source qualification remains incomplete.

## Reproduction and current boundary

`teaql_runtime/tests/sqlite_ledger_allocated_trace_tests.cpp --slat03` installs
an observing wrapper over the actual `SqliteIdSpaceGenerator`, reserves root
floor 500 and child floor 800, then saves a new typed root with ID 0. The wrapper
checks physical SQLite autocommit is disabled and no safe audit was delivered
before delegating ID allocation. No substitute in-process counter is used.

The save begins its native transaction before calling the generator, as required.
Generation then fails with:

```text
id generation error: rusqlite connection lock error: connection is borrowed by an open SQLite stream; use the pollable transport
```

The stream wording is shared by the connection-borrow diagnostic; this failure
is caused by the active transaction lease, not a fixture stream. The historical full CTest
entry included SLAT03 and failed at this point before the repair below. Case
selectors permit independent evidence for SLAT01 and SLAT02; they do not disable
SLAT03 in the default entry or turn its failure into an expected-pass test.

## Confirmed implementation path

- `detail::GraphSaver` starts the actual executor transaction before constructing
  `LedgerSaveExecution`.
- Ledger preparation calls `UserContext::next_id` through the configured
  `InternalIdGenerator`, retaining application customization and failure policy.
- `SqliteIdSpaceGenerator::next_id` first ensures its ID-space table through the
  ordinary provider entry and then uses `SqliteIdAccess` for the read/CAS loop.
- These synchronous operations acquire a lease without the native transaction
  identity. `SqliteLease::acquire` correctly rejects that lease while another
  transaction owns the connection.

The existing provider ID tests deliberately reject direct parent-generator
access during an asynchronously leased transaction. That isolation contract
must continue to hold; merely allowing every operation on the same connection
or in the same Context would weaken it.

## Required repair and gates

Preserve the runtime, data-service transaction, SQL adapter and provider layers.
Introduce or reuse an explicit transaction-bound access scope for synchronous
runtime services such as the configured ID generator. Its lifetime must cover
only the relevant poll/call, with restoration on pending return, success,
exception and cancellation. No TLS, singleton, implicit ownership extension or
unrestricted ambient connection bypass is permitted.

The scope must reach the actual native transaction through adapters and observer
wrappers. The provider must validate transaction identity, owning Context and
active state; unrelated root calls, streams and other Contexts stay excluded.
Both ID-table initialization and ID-space read/CAS/floor operations must use the
same authorized transaction when bound. Keep the custom generator invocation
and original error propagation; never fall back to a process ID after failure.

Qualify automatic ledger save and explicit transaction save, physical floor
rollback/retry, overflow, graph failure and cancellation/Context close. Preserve
existing ID-generator lease-rejection tests. Use focused native and module gates,
plus ASan/UBSan for the new scope lifetime and cleanup. Revisit the full source
acceptance only after SLAT01-SLAT15 and helper responsibilities pass.

The conditions above guided the repair. The next section records the implemented
scope and its bounded qualification evidence; broader source gates remain open.


## Qualified runtime repair checkpoint

`TransactionResourceScope` is a move-only stack borrow returned by the transaction
SPI's `bind_transaction_resources`. The data-service and SQL transaction bases
check active state and allocation domain before invoking the provider hook. SQL
adapters and the source observer forward the scope to the native transaction.
Default hooks are inert, preserving non-SQL executors and in-process generators.
The type is exported through the existing data-service module and isolated
lookup catalog; no new package or dependency was introduced.

Ledger preparation binds the scope only around the synchronous configured
`next_id` calls. The scope is released before command execution can suspend,
and on allocation failure or an exception. Automatic and explicit transaction
saves use that shared ledger path; no alternate generator or generated adapter
implements the fix.

SQLite records a temporary native transaction identity and owning Memory domain
on its explicitly owned connection. A nesting count permits same-transaction
nested scopes. Only ID-space table initialization and ID read/CAS/floor access
consult this binding. Ordinary root SQL, other Contexts and active statement
leases remain excluded. Cancellation invalidates the binding immediately;
subsequent ID access through a still-live cancelled scope returns an error,
instead of silently allocating outside the transaction. Scope destruction then
clears the binding. These guards must never survive a poll return or Context
close and do not own or extend transaction/Context lifetime.

The original native SLAT03 and unchanged provider ID tests passed after the
first implementation. Expanded tests now cover scope nesting/move/exception
restoration, wrong Context, ordinary SQL exclusion, active cursor exclusion,
rollback floor reuse, cancellation, consumed handles and Context teardown after
scope release. The source fixture also adds SLAT11 explicit commit and explicit
business-rejection/suspension-cancellation controls. The
[repair receipt](parity/sqlite_ledger_id_scope_repair_5_0_6.json.gz) retains current
sources, build logs and passing JUnit results: native 8/8, imported modules 4/4,
ASan/UBSan 3/3 with leak scanning disabled. Canonical bodies 1,136, exports 1,520,
pinned formatting 1,800 and active paths pass. No MIPS result is claimed.

SLAT01/02/03/11 are qualified at this checkpoint. Source-specific overflow,
partial graph failure/retry, child allocation variants and local-reason vectors
remain pending under SLAT04-SLAT10 and SLAT12-SLAT15; these broader requirements
are not proven by the focused scope controls. No source or phase is promoted.
