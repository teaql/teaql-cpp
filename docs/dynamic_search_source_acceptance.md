# Dynamic search and guarded mutation source-test review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete original bodies and helper fixtures were compared directly on
2026-10-09 with their C++ ports. This records source-test responsibilities;
it does not establish complete runtime composition, privacy or stage acceptance.
Current consumers require validation after the direct-checker header repair.

## Core dynamic search: all eight original tests

The School model retains seven typed fields and its Platform relation; Platform
retains its name field. The complete unknown-clause JSON preserves all three
unknown filter paths, the unknown order field and submitted canary values.
Accepted filters are name/platform.name, accepted order is id and all four
warnings retain their source code and structured fieldPath without values.

The child process preserves both canaries and the returned untrusted field path.
The Python parent captures both stdout and stderr, requires the child success
and original run marker, rejects both canaries in either stream and requires
<omitted> in stderr. The explicit CMake emulator is forwarded on MIPS.
The parent/child split adapts Rust's test-harness subprocess using the C++ CLI;
no production log output is rewritten to pass the assertion.

All twelve fatal inputs remain byte-identical: malformed/root/trailing JSON,
unrecognized tenant key, integer type/fraction errors, nonfinite number,
invalid leap date, unknown operator, malformed IN, unsafe constructor path,
empty relation segment and invalid order direction. Each fails without warnings.
The six-field valid input retains boolean, exact decimal text, leap date,
timestamp, floating integral ID and null. Limits retain two clauses under a
limit of one, the 1,001-element IN list and missing Platform metadata.

Composition retains tenant_id=7, descending id, limit=2, hard_limit=3 and the
original comment. It preserves the immutable base, combines the filter, appends
ascending name after descending id and publishes exactly one warning. Both late
normalization and trusted-binding failures retain deferred warning publication.

## SQLite dynamic search: the complete original integration test

Both descriptors, logical/physical table names, two platforms and all six
schools remain unchanged. Physical schema uses UserContext.ensure_schema;
compiled inserts seed actual SQLite. Search metadata retains both models.
The immutable base retains outer tenant_id=7, id/name projections, descending
id, slice/hard_limit=2, comment and Purpose trace. Trusted bindings compile
platform.name into the original relation subquery with its own tenant_id=7.
The complete submitted filter/order JSON retains all ignored paths and values.
The real SQL executor/provider must return I64 IDs [3, 2] in order and four
warnings. The C++ port additionally checks warning serialization omits secrets.
No handwritten fixture substitutes for SQL compilation or query execution.

## SQLite guarded mutation: the complete original integration test

The Order descriptor and actual three seeded rows retain their tenant, version
and name values. All nine original guarded calls remain ordered: denied tenant-1
update of tenant-2 row 2, successful row-1 update, denied row-2 soft delete,
successful tenant-2 soft delete, denied recover, successful recover, denied
row-3 hard delete, successful hard delete, and denied row-2 transaction update
followed by rollback. Every affected-row assertion is preserved. Actual rereads
retain row 2 as (tenant=2, version=3, name=tenant-two), row 1 name=updated and
zero remaining rows for id=3. Transaction capability recovery preserves the
separate GuardedMutationExecutor interface instead of broadening every generic
transaction or simulating a guard with a pre-read.

The supplementary native write-boundary check changes the tenant immediately
before UPDATE. The guarded statement itself must reject the write, return no
foreign snapshot, and preserve the original name/version. This strengthens
atomicity evidence without replacing any of the original nine calls.

## Ownership and validation boundary

Fixtures select explicit arenas/UserContext and disable default PMR allocation.
Nested models/requests/results use that scope; SQLite connection resources clean
up before the arena. Standalone test-helper JSON is an observation envelope,
not a new public model protocol. The reviewed source-file groups contain tests
and helper fixtures rather than an additional production API/default/trait.
Original source runs and current native/sanitizer/MIPS validation will be retained
before recording source acceptance. Physical WNDR4300 execution is deferred by
the user; target acceptance uses cross-compilation and QEMU.

Fresh unchanged locked/offline original integration tests pass core 8/8 and
SQLite provider 1/1 dynamic search plus 1/1 guarded mutation. The current C++
consumer gate remains pending; these source results are not a C++ pass.

The current OpenWrt MIPS/QEMU selection passes all four direct source checks in
1.21 seconds after its six build/link tasks complete. The subprocess canary
forwards the explicit emulator. Post-test build reports no work. Current native
full and sanitizer consumer validation remain pending; source groups stay partial.

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
