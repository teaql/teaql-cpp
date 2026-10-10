# Context provider-dispatch lifetime follow-up

Baseline: Rust v5.0.5 at 62940ae2b88e7acc32763853b8b4685a01c7e90a.

While the installed callback-scope repair rebuilds its 361 consumers, controlled
public Context calls independently confirm five remaining synchronous lifetime
gaps: metadata entity lookup, metadata listing, checker registry lookup,
telemetry is_noop inspection, and telemetry scope creation. Each provider closes
its captured User and immediately exits1; no result or destructor accesses
reclaimed memory. Exit 0 is reserved for explicit fail-fast termination, and
ordinary caught exceptions exit 2. All five actual native executions exit 1.

The existing language catalog is a concrete model, not a replaceable provider
callback; it is not inferred to have this dispatch gap. Event, mutation-review,
reference and checker execution boundaries already have explicit guards.
These successful boundaries do not establish safety of separate public lookups.

This is a follow-up gate for the partial Context/checker integration source
contracts. No production input in the active 361 snapshot is modified by these
reproductions. The review also checks Context telemetry's raw registration
pointer independently of callback reentrancy. Installed repair and complete
consumer/native/sanitizer/MIPS validation remain required for this follow-up.

An independent telemetry getter reproduction also fails: a Global telemetry
object is allocated through User's proxy, registered, then explicitly destroyed.
The getter still returns its raw alias instead of ReferenceError::Dead. The
fixture does not invoke any virtual method or inspect fields after destruction.
Its exit 1 and false result demonstrate registration alone does not keep the
reference checked. An initial fixture used unavailable Global get/destroy
methods and did not compile; the corrected execution uses the public User
allocation/access/destruction proxy. A typed checked borrow is required in
addition to the callback guards; that change affects UserContext layout.

A reviewable follow-up patch is staged in staged-implementation.patch. It adds
checked telemetry storage and guards around the five Context provider calls.
It changes UserContext layout and is not installed while current361 consumers
compile. Syntax-only compilation checks the staged translation units and both
fixtures; no staged executable is linked against libraries with the old layout.
Installed native/sanitizer/MIPS behavior and lifecycle checks remain required.

Staged syntax-only compilation completes successfully before formatting the
three files with pinned clang-format 18.1.8. The patch remains uninstalled;
these checks do not establish ABI-compatible executable behavior.

## Installed integration

The staged patch is installed after the complete native361 checkpoint finishes
with no-work verification. Telemetry storage is now a typed checked borrow;
metadata/checker/telemetry dispatch has synchronous quiescence. Eight new CTest
checks cover the five negative callback boundaries plus telemetry lifetime/
replacement/origin constraints, ordinary exception cleanup, and trusted
metadata/telemetry shutdown reads. The native registration becomes369.
Pinned formatting/check passes667 handwritten files. Focused builds across
native, ASan/UBSan and MIPS are active; no execution result is claimed yet.

Physical WNDR4300 testing is deferred at the user's explicit request on
2026-10-09 because the device is remote. Current target acceptance uses
cross-compilation and QEMU; no hardware execution is claimed or required
for this requested validation phase.

All focused compiler/linker processes finish successfully; no-work refreshes
precede execution. The eight installed contracts pass on native (0.03 seconds),
ASan/UBSan (0.30 seconds, leak scanning disabled), and MIPS/QEMU (0.33 seconds).
This includes destroyed local/Global telemetry, replacement, sibling/unmanaged
rejection, exact ordinary exceptions, fail-open telemetry creation and trusted
metadata/telemetry shutdown reads with zero live User bytes afterward. Complete
369-consumer/native integration and wider50-check cross selections remain active.

The current MIPS build finishes 67 tasks before the additional renderer link
and CTest run. All 51 callback/Context/ID/relation/service/query/example and
SQL diagnostic/renderer checks pass in 16.68 seconds. No pending compiler or
linker output is executed. Sanitizer/native 369 integration remains active;
physical-router testing is explicitly deferred by the user.

The post-test MIPS build reports no work. Current installed relation models
match all162 live original observations with the new Context layout. Native369
and sanitizer51 consumers remain active; no complete source/stage acceptance
is inferred from this target checkpoint alone.

The matching sanitizer build finishes 67 tasks before its additional renderer
link and CTest run. All 51 current Context/runtime/SQL checks pass under ASan/
UBSan in 17.83 seconds, with leak scanning disabled. The post-test build reports
no work. Complete native 369 and recorded source acceptance remain pending.

Current full native 369/369 passes in 80.91 seconds after the complete622-task
consumer build. No-work refreshes precede/follow testing;418 compiled object
dependency records are nonempty. Both current sanitizer/MIPS selections pass
51/51. This closes the installed Context layout integration checkpoint;
complete Context and broader stage acceptance remain separate requirements.
