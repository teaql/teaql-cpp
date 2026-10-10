# Context locking source responsibility review

Baseline: Rust v5.0.5 commit `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-runtime/src/context/locking.rs` (137 production lines). This review
accounts for every method and private directory role. It does not accept the
module from declaration presence or historical test counts.

## Final source-group acceptance, 2026-10-08

The complete current native suite passes 302/302 in 67.69 seconds following a
no-work final build and nonempty dependency records for all 375 compiled objects.
Fresh Entity/locking/original-tail selections pass 11/11 under ASan/UBSan with
leak scanning disabled (2.43 seconds) and 11/11 on OpenWrt MIPS/QEMU
(2.03 seconds). They include both direct lock checks and the unchanged source
local/remote ownership and expiry assertions. The previously reviewed 222
original-call scripts and all 137 source lines are part of this acceptance.

The source group is accepted within the explicit application Global namespace,
owner-thread Context and bounded-memory adaptations described below. Process
static storage does not own Context data. Optional remote providers remain
provider-neutral; this adds no remote/cloud implementation. Provider policy,
SQLite coordination, TransactionScope and the remaining runtime/stage gates
retain their own responsibilities.

| Source responsibility | C++ implementation and evidence |
| --- | --- |
| LocalLockEntry owner/optional monotonic expiry | LocalLocks::Entry retains owner and optional LockInstant. Keys are independent Global-owned strings. |
| ProcessLocalLocks mutex/map/condition variable | Separate LocalLocks resource retains directory, mutex and change notification. C++ application Global ownership replaces the source process-static table under the explicit lifetime contract. |
| next_local_lock_owner / per-Context identity | Existing non-reused runtime/User identities supply a stable owner. User recreation cannot inherit an abandoned lease. No User pointer is retained by an entry. |
| try_local_lock free/reentrant/expired branch | Same-owner reentry replaces expiry, without a recursion count. Zero expiry removes expiry; expired foreign ownership may be replaced. |
| try_local_lock contention/deadline/wakeup | Zero timeout makes one attempt; positive timeout waits until the earlier expiry/deadline and rechecks after every wake. u64 milliseconds avoid narrowing to signed native nanoseconds. |
| unlock_local | Only current-owner removal notifies waiters. Missing/wrong-owner release leaves entries unchanged, and does not allocate a new directory. User destruction does not release leases implicitly. |
| RemoteLockProvider try_remote_lock / unlock_remote | Separate asynchronous provider-neutral SPI retains key, owner token, timeout/expiry and boolean result. No remote/cloud implementation is introduced. |
| UserContext try_remote_lock / unlock_remote | Missing providers return true. First poll selects the current registered provider and passes the stable Context owner token; resource expiry is checked before each virtual use. |
| Awaiting provider work | Parent operations forward child wakes and cancel/join child work before arena reclamation. Inputs are copied into their selected scope; provider borrows do not extend provider lifetime. |

There is no source Clone/Debug/serde/default contract for the public provider
trait. Private LocalLockEntry's scalar Copy/Clone and ProcessLocalLocks' default
empty directory are separate responsibilities. User ownership tokens do not
depend on mutable business identifiers. Provider expiry/reentry policy belongs
to the provider; Context does not invent an unlock on close.

All 222 existing differential scripts call the actual original public Context
methods. The provider fixture implements only its SPI, not the Context's local
lease algorithm. Unique Rust key prefixes isolate the process-static table.
Only opaque owner spelling is normalized; owner equality, provider replacement
and User recreation remain observable. The native ownership fixture separately
checks long key copies, wrong-owner release, expiry, maximum u64 leases, Global
quota failure, wrong-thread rejection, lazy provider selection, destroyed
providers, child wake/cancellation and actual User deletion.

The scope adaptation is explicit: independent C++ GlobalContext instances
have independent local namespaces, and Context access is thread confined.
These are required ownership choices already documented in design_parity.md;
tests do not establish source process-wide exclusion across independent Globals
or concurrent access to a User. The source responsibilities remain separate
from query-build coordination, SQLite locks and TransactionScope.

The complete original body, C++ directory/operations and 222-script oracle
are reviewed. Current native/sanitizer/MIPS consumer integration is still
pending after shared Entity/Timestamp header changes. Fresh named checks and
full native regression remain required before changing this group's status.

The two downstream source tests in runtime lib.rs are additionally reviewed
against context_tail.cpp: every 50ms/60ms local ownership/expiry assertion and
1,000ms remote owner-specific release/optional-provider assertion is retained.
Owner token initialization is traced through Context construction and preserves
PID, non-reused owner identity and creation-time nonce roles.

Fresh locked offline original selection passes 3/3: both reviewed local/remote
locking tests plus the optimistic conflict test selected by the same name filter.
The extra result does not expand this source-group review.

Current native named locking integration passes 2/2 in 1.09 seconds. Both
selected binaries finish building before execution. The complete native
consumer rebuild and selected cross-platform refresh remain pending; this
result alone does not change module or stage acceptance.

The final seventeen-check entity/Timestamp/Decimal/locking/original-tail
selection passes 17/17 under ASan/UBSan with leak scanning disabled (11.12 seconds)
and 17/17 on MIPS/QEMU (8.53 seconds). Both original local/remote source ports
and generic lock contracts are included. Complete native consumer refresh and
regression remain pending before changing the source group's status.
