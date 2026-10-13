# Sample save failure and pending-operation qualification

Status: partial G12/P6 qualification against runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and producer
`45e1572c4956ca963410d8ea436ead3c8857087a`.
This extends the generated sample execution checkpoint, without changing the
frozen producer, production default limits or generator artifacts.

## Original failure behavior

The unchanged original generated entry supplies 28 observations over 42
invocations. Identical explicit SQLite test schemas and triggers reject insert
IDs 1, 3 or 20 for Device, Profile or School. Missing-root cases cover optional
Device references; additional cases drop the root table to fail the reference
query. Source fixture/model hashes and the runtime lock are checked before replay.

All 28 original/C++ observations match without normalizing errors, reports or
rows. The result includes each invocation's complete persisted table contents,
ordered IDs, names, foreign keys and versions. Errors retain the original
`graph write error: Transport error: sample fixture write failed` message.

A failed save stops the current invocation. Earlier successful saves remain
committed; the failing insert and later generation do not persist. For example,
a failure at Profile ID 3 leaves 20 Devices, two Profiles and no Schools. A
second invocation succeeds and adds another business batch, including another
20 Devices, rather than silently deduplicating the earlier batch. ID 3 stays
absent; allocation occurred before the failed insert. No partial success report
is returned for the failing invocation.

A failed root query follows the source's `unwrap_or_default`: the reference pool
is empty. Optional Device references remain absent, while Profile and School
produce the exact required-dependency skip records. An active Context root alone
does not replace the sample reference pool.

## Pending audited saves and Context cleanup

The native deferred adapter delays the existing graph-saver boundary. Releasing
it delegates to the original registered saver, which still performs governance,
transactions, SQL mutation, readback and ledger cleanup. The adapter does not
replace the saved result, implement a substitute data service, or fabricate
successful rows.

Twelve lifecycle scenarios suspend at save ordinals 1, 23, 41 and 60, then either
resume, explicitly cancel, or actually delete UserContext. They verify that:

- Repeated pending polls do not recreate the entity or allocate a second ID.
- Only earlier completed saves have reached the database while work is pending.
- The save wake reaches the enclosing model/sequence through the existing async
  adapters, and resumption commits all 60 rows with three per-model counts of 20.
- Cancellation detaches the pending callback and checks its 32 KiB Context-owned
  host buffer while the arena remains alive. The pending work stops once; earlier
  completed work stays completed and later generation never starts.
- Successful completion leaves no pending work for actual Context destruction.

The test disables the default PMR resource and uses Context-scoped delegated
resources and graph/ledger state. The deferred stimulus is an owner-thread gate;
it does not claim qualification of a native worker thread or router execution.

## Remaining scope

Source and Context lifetime evidence is scoped to these fixtures. Complete
scalar/temporal/JSON/optional seeding, constant/candidate/inherited reference
topology, sample progress logs, full Assist/extension contracts and final P7
integration remain open. The ordinary 18-case sample execution corpus is retained
separately; adding fault stimuli does not relabel that earlier evidence.

## Completed gates

- Live original/C++ replay: 28/28 observations, 42 invocations.
- Native CTest: 2/2 (failure replay and the 12-scenario save lifecycle contract).
- ASan/UBSan: 1/1 expanded save lifecycle contract, leak scanning disabled.
- Pinned formatting/check: 952 C++ files.
- Runtime lock, generator capability ledger, module catalog/path checks and
  whitespace validation pass. No generator mutation or new dependency was
  required; Java/artifact reproduction and MIPS/QEMU were not repeated.

G12, P6 and the overall goal remain active and unqualified.

Further scalar/temporal/JSON and reference-pool evidence is recorded in
[generated_sample_fields_5_0_6.md](generated_sample_fields_5_0_6.md), including
explicit preservation of the complete original JSON-literal compiler rejection.
