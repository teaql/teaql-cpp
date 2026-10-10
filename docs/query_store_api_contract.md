# Aggregation cache and pagination source contract review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Complete source review covers data_service/cache.rs (89 production lines) and
context/pagination.rs (252 lines), including its Context methods and private
ID-set build registry. This source group is accepted within the explicit Context ownership,
owner-thread scheduling and Linux/OpenWrt diagnostic profiles below.

| Responsibility | C++ mapping and exact boundary |
| --- | --- |
| Aggregation cache backend | Independent namespace/get/put/invalidate SPI. Reads copy rows into the result Context; puts copy nested rows into the store Context. No external cache dependency. |
| Cache default and namespace factory | Empty namespace/map by default; explicit namespace construction. The reserved source method name `namespace` is represented by `name_space`. |
| Cache expiration | Monotonic stored time, unlimited age for zero, inclusive maximum age, and future stored times treated as fresh. Expired reads do not remove entries. Replacement starts age after the explicit ownership copy. |
| Cache invalidation/clear | Remove only keys starting with the exact namespace followed by `::`; clear removes every logical entry. Arena capacity remains owned until Context close. |
| Private cache entry | Original stored Instant followed by full CompactRow vector. Entry Clone responsibility is expressed by allocator-aware row copies on reads and writes. |
| ContinuousPageCursor | All eight fields, explicit-domain Clone and fieldwise PartialEq, including Value NaNs and distinct null variants. Full source-derived Debug retains nested boundary and SystemTime seconds/nanoseconds. |
| Continuous cursor store | Separate lazy async get/put/invalidate SPI. Composite key is query key, colon, target/next offset. Expiration is inclusive (`expires_at <= now`) and removes the requested expired cursor. |
| Cursor default/eviction | 4,096 entries. At capacity, evict a minimum-expiry entry before inserting, including replacement. Equal-expiry HashMap tie order is not a source guarantee. |
| RetainedIdSet | Query key, complete u64 ID vector and expiration. Source Clone borrows immutable Arc payload; native retention explicitly deep-copies into the destination scope. No source PartialEq or store Debug is invented. |
| ID-set store | Separate lazy async get/put/invalidate SPI. Get removes expired requested sets. Put first rejects an oversized incoming set, prunes expired entries, then evicts earliest expiry until count and combined logical ID bytes fit. Invalidation is exact-key removal. |
| ID-set defaults | 64 entries, 256 MiB of logical ID payload. Saturating per-vector byte accounting and combined comparison preserve native-width source profiles; this is not a ceiling on all arena allocations. |
| Context observations | Independent continuous-plan/cursor and ID-plan/count getters and observers. Initial values remain `DISABLED`/None and `ID_SET_DISABLED`/None. Installed stores are checked borrows, selected through their independent SPI. |
| Build lock registry | Source process-static Weak registry becomes explicit application Global-owned registry reached through UserContext. Same live key shares a logical lock, dead leases prune on lookup, and guards/waiters preserve cancellation/wake/reuse responsibilities. |

## Ownership and scheduling adaptation

Source Mutex/Send/Sync and Arc ownership do not authorize arbitrary concurrent
Context access in C++. Allocation, store mutation, polling and close follow the
Context owner-thread contract. Cache diagnostics preserve the source Mutex
structure with `poisoned: false` for this supported profile; the cache does not
have a separately poisonable native mutex. This adaptation must remain explicit
in complete group acceptance, rather than claim general cross-thread parity.

Async inputs snapshot strings, cursor fields and nested ID vectors into the
operation Context. No store operation executes before its first poll; cancellation
before polling does not write. Reads and retention create destination-owned
values. Borrowed store access rejects destroyed owners before dispatch, and
pending operations quiesce before arena reclamation. Allocation failure is a
recoverable C++ ownership concern; source HashMap/Vec OOM behavior must not be
claimed as a transaction rollback guarantee. Logical eviction does not return
individual arena bytes to the OS.

## Diagnostic omissions and staged repair

A public umbrella compile reproduces missing original derives for
ContinuousPageCursor, RetainedIdSet and InMemoryAggregationCache. A separate
staged repair restores their complete field order, values and inherited Debug
options. Private populated entries include the actual stored monotonic Instant
and actual CompactRows; no constant timestamp or row summary replaces them.
Linux native/OpenWrt monotonic time uses the source `Instant { tv_sec, tv_nsec }`
profile. Other operating systems require their own platform protocol review.

The actual original public oracle has 74 twice-agreeing observations: complete
cursor fields/Clone/PartialEq in nine modes, all individual field differences,
NaN/null/nested boundaries, retained ID vectors and expiration/Clone, and cache
defaults/namespaces. Its live-cache case checks an actual put timestamp against
before/after Instant brackets, full row Debug, and unchanged timestamps after
read. Native checks use their own actual monotonic brackets, never fixed clock
substitution or normalization of diagnostic outputs. Exact corpus cases use
empty cache maps because multiple-entry HashMap order and live timestamps are
not deterministic wire representations.

The standalone native lifecycle test disables default PMR allocation, checks
populated cache timestamp/rows, explicitly retains cursor strings/boundaries,
ID vector and cached rows in Global, deletes the source User completely, and
accesses copies through a new User. Expired source references reject before
formatting/copying. Source behavior is otherwise covered by the independent
query-store/ID-set-build-lock scripts and original pagination/cache source tests;
these new observations do not replace their broader execution coverage.

Standalone model/lifecycle builds finish on native, sanitizer and MIPS/QEMU.
All three model probes match 74/74 live original cases; all three ownership/
monotonic-clock executables pass with default PMR allocation disabled.
ASan/UBSan uses leak scanning disabled. The production header/CMake repair remains staged while current
entity-runtime native consumers compile. Full native integration and complete
source/stage acceptance remain open; physical-router execution is unverified.

## Installed integration checkpoint, 2026-10-08

The combined runtime-ID/cache/typed-transaction repair is installed. Its full
native regression passes 331/331 in 72.65 seconds, with no pending build work
before or after testing. The selected installed sanitizer and MIPS checks each
pass 17/17; those results are retained in the corresponding measurements.
Earlier staged statements describe historical evidence. Complete source and
stage acceptance remain independent. The subsequent six-repair integration
registers 343 checks and is now building; no passing result is claimed for it.

## Complete implementation follow-up

The cache, pagination models/stores, Context observation methods and full
ID-set build registry/waiter/guard implementation have been inspected again.
Defaults, native-width accounting, exact-key versus namespace invalidation,
minimum-expiry eviction, lazy input snapshots and checked store dispatch retain
the source responsibilities. The Global registry holds copied keys and logical
leases; User waiters and guards release before their owner's arena is returned.
The diagnostic writer reads actual stored timestamps and rows; source map
iteration order is not a stable wire protocol. Current full native integration
and refreshed complete store/build-lock selections are required before source
acceptance. Surrounding entity query/service behavior remains an independent
Stage 4 gate.

## Complete source acceptance, 2026-10-09

Both complete production files and the native registry/waiter/guard implementation
have full public/default/trait/protocol/ownership reviews. Current complete native
integration passes351/351 in76.06 seconds, with no-work verification before and
after testing and415 valid/nonempty dependency records. Corrected installed
store/build-lock/model checks pass within the42-check sanitizer/MIPS selections;
the independent complete eight-check selections also pass. Fresh live original
comparisons retain259 store,144 build-lock and74 model observations.

This accepts the cache/pagination source group within its stated C++ allocation,
thread and platform adaptations. It does not accept all surrounding Context,
entity/service/query behavior, foreign factory-work cleanup or Stage4 as a whole.
Early staged/pending paragraphs are historical checkpoints. Physical-router
execution remains unverified.
