# Flat identity graph and incremental ledger source-test review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete 113-line flat_identity_graph.rs and 393-line test_incremental_ledger.rs
were compared directly with their C++ ports on 2026-10-09. All eight original
bodies and helper fixtures retain their source responsibilities. Current native
full and fresh sanitizer/MIPS validation remain required for source acceptance.

## Flat graph: all four original contracts

Separate first/second roots share the frozen Vendor(7, Vendor A) address while
retaining independent mutation ledgers. NycYellowTrip#1 total_amount remains
exact I64(100), not merely a convertible numeric value; the second root has no
such change. The same numeric ID 7 retains distinct Vendor and String payloads
with their complete original names. The frozen Vendor#7 trips list retains IDs
11/12 and first/second names, length two, the second element and stable list
address; String lookup for that list remains absent. Garage#7 primary_vehicle
retains a loaded-null optional view, while backup_vehicle remains uninstalled.

The handwritten Vendor uses allocator-aware String and an explicit destination
copy. This is fixture ownership adaptation, not a new production entity API.
Graph builder, frozen graph and mutation ledger remain separate runtime objects.

## Incremental ledger: all four original contracts

The source-specific SequentialIdGenerator returns fetch_add itself, starting at
one, with sequentially consistent ordering. It remains distinct from the core
counter provider's incremented return. The full observability fixture retains
Task IDs1/2/99/100/101, all name/status values, overwritten name, version999,
comments, deleted IDs200/201 and separate original versions3/5/2/1/1. Field-
signature groups retain name:[99], name/status:[1,2,101] and name/version:[100].
These observations are not SQL batching or a persistence implementation.

Commit cleanup retains both source entity keys, the new marker, deleted marker,
original version and comment; all are removed before an independent UserAccount#3
change. The later change set contains exactly that key and one entry.

The randomized fixture retains all eight original seeds, unsigned wrapping
multiplier6364136223846793005/increment1442695040888963407, 2,048 steps per seed
and three independent roots. Root/entity/id/operation/field/version selectors
match source bit shifts and moduli. Each operation retains its independent
standard-container model. Every step compares all three roots' complete changed
fields, new/deleted sets and recorded original versions, then checks the same
numeric ID under the other entity name is unequal. Actual values remain exact
U64 before conversion to the independent expected map.

The nested fixture retains seed0x4348414e47455345, wrapping multiplier
2862933555777941757/increment3037000493, all 4,096 steps, the original entity/id/
field selectors and depth limit eight. Push/pop/clear mirror the independent
stack; each popped map retains full keys and exact U64 fields. Latest-value
lookup checks presence separately from value type and payload, preserving None
versus a present nonnumeric value. Neither iteration range is shortened.

## Scope and validation boundary

Standard maps/sets/strings are independent test expectations, not runtime Context
owners. Runtime roots, typed graph payloads and change-set snapshots use explicit
UserContext allocation. This test-only source group adds no additional public
production defaults/traits/wire protocol. Original source runs and current
consumer output will be retained before recording source acceptance. Broader
graph save, governance, privacy and Stage 6 remain separate. Physical-router
execution is explicitly deferred by the user; target acceptance uses QEMU.

Fresh original suites each pass4/4. Current ASan/UBSan and MIPS/QEMU selections
each pass2/2 after their four-task consumer builds, followed by no-work checks.
Leak scanning is disabled for sanitizer. Native full integration remains active;
current original/target output is retained in
measurements/parity/2026-10-09-flat-ledger-source-review.

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
