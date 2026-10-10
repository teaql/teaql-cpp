# Context memory and string storage design

Status: the selected core design is implemented and validated. This document
extends the [Context API review](context_design_review.md). It records the agreed
ownership, bitmap, allocation-limit, and string-storage direction, and recommends
mechanisms from existing implementations. The
[implementation contract](context_memory_implementation.md) records the selected
APIs, limits, and mechanisms. This work does not change the Rust v5.0.5 baseline
or establish completion of later migration stages.

The [current lifetime contract](context_lifetime.md) remains the description of
implemented behavior. Arenas still use `std::pmr::monotonic_buffer_resource`;
checked factories now coexist with temporary kernel borrows. The implementation
adds quotas, exact object bitmaps, integer reference resolution, and explicit
string interning. Optional automatic admission and custom string layouts remain
research work.

## Agreed direction and proposed mechanisms

| Area | Agreed direction | Proposed mechanism or remaining choice |
| --- | --- | --- |
| Ownership | Application-owned GlobalContext and UserContext are the only two lifetimes. | Keep the existing growing arenas and destructor registration. |
| Application boundary | Business code sees UserContext; Global allocation is delegated explicitly through it. | Named checked factories and rebinding are implemented; raw kernel borrows remain temporary. |
| Object validity | Global and each UserContext have separate exact bitmaps; one bit represents one independently managed object allocation. | Bounded, paged slots and typed integer references are implemented. |
| Allocation limits | Application scenarios set allocation-count limits; exhaustion makes further allocation fail. | Per-domain reservations, bytes, cumulative Context identities, and aggregate User bytes are implemented. |
| Internal storage | Character arrays, byte buffers, and container backing storage do not consume additional object IDs. | Use the owning Context's allocator and charge all storage to its byte budget. |
| String sharing | Admission should depend on reuse benefit, not length alone. | Separate ordinary string storage from optional interning; begin with explicit interning for known repeated strings. |

The rationale below records the original design discussion. The implementation
contract is the reference for selected API names, encoding, and defaults;
remaining optimization candidates are not implemented guarantees.

## Existing designs to reuse

These are references for mechanisms, not additional runtime dependencies. Use
C++20 facilities and the existing PMR implementation where they fit. Preserve
TEAQL entity, query, mutation, logging, and execution-layer responsibilities.

| Reference | Documented mechanism | Proposed TEAQL adaptation |
| --- | --- | --- |
| [LLVM BumpPtrAllocator](https://llvm.org/doxygen/classllvm_1_1BumpPtrAllocatorImpl.html) | Allocates sequentially across growing slabs and sends allocations above a threshold to their own slabs. | Retain Context arenas; consider a dedicated block path for large payloads after measuring existing PMR behavior. |
| [LLVM StringSaver](https://llvm.org/doxygen/classllvm_1_1StringSaver.html) and [UniqueStringSaver](https://llvm.org/doxygen/classllvm_1_1UniqueStringSaver.html) | Stable string storage and deduplicated storage are separate components. UniqueStringSaver adds lookup work, avoids duplicate storage, and has no individual reference-counted deletion. | Separate saving from interning, keep character addresses stable, and reclaim both with their Context. |
| [SQLite Lookaside](https://www.sqlite.org/malloc.html) | Each connection has a pool of fixed-size slots for small allocations; larger requests or pool exhaustion use the general allocator. | Consider a Context-local small-storage pool. Every fallback must remain under the Context byte budget; lookaside alone is not a hard memory limit. |
| [EnTT entity identifiers](https://github.com/skypjack/entt/wiki/Entity-Component-System#the-registry-the-entity-and-the-component) | Identifiers contain an entity number and version; releasing an identifier normally changes its version, and the registry validates IDs. | Separate identity from address. Prefer bounded non-reused slots initially; require generation checks if reuse is introduced. |
| [Caffeine Window TinyLFU](https://github.com/ben-manes/caffeine/wiki/Efficiency#window-tinylfu) | Uses a compact probabilistic frequency sketch to guide cache admission. | Consider approximate statistics for string admission only. Object validity must remain exact, and cache eviction must not invalidate borrowed characters. |

LLVM's arena reset behavior is not our shutdown contract. TEAQL must still stop
native work, destroy registered objects and resources, and return every block.
SQLite's internal allocation mechanisms also do not automatically account for
its library heap within our Context budget.

## Ownership and reference identity

Global owns its object bitmap, object records, strings, and resource cleanup.
Each User owns its own bitmap, records, strings, and cleanup. A User bitmap may
be reclaimed with its arena; it must not become permanent Global storage merely
to diagnose expired references.

Global retains a bounded directory of Context identity and state, containing
small runtime-owned records rather than retained User object pointers. A closed
record can retain a Context number and a limited diagnostic location. It must
not retain User strings, log buffers, callbacks, or object tables. Any diagnostic
text retained in Global requires an explicit Global copy and budget charge.

A User-facing reference needs the following logical information:

```text
Runtime identity
Access Context identity
Object owner domain: Global or User
Object slot
```

The access Context determines whether the business reference is still usable.
The owner domain determines which object bitmap and table to consult. A Global
resource can remain alive after User close, while access obtained through that
User expires. Another User must obtain its own access to the resource.

An opaque `std::uint64_t` is the proposed ID representation, rather than
platform-dependent `long`. Typed wrappers prevent accidental type mixing:

```cpp
template <class T>
struct Ref {
    std::uint64_t id;
};
```

The implementation uses 23 runtime bits, 20 access-Context bits, one owner bit,
and 20 slot bits. Configured limits must fit the encoding, and another Global's
namespace is rejected even when local slots coincide. Invalid configurations
fail initialization; IDs never silently truncate or wrap. These are process-local
memory identities, not an unrestricted or persistent 64-bit address space.

Reference values do not own objects or extend Context lifetimes. Copies preserve
identity. Hash collisions must never establish reference validity or string
equality. Any resolver must itself be accessed through a live runtime and
UserContext; an ID does not make invoking a destroyed Context safe.

## Exact object bitmaps

A set bit means the corresponding successfully published object is live. An
unset bit means the object is unavailable. The bitmap is an allocation-validity
table, not an OS page map or free-space map. Object addresses and types require
separate records; the bitmap cannot resolve them on its own.

For `N` object slots, byte-packed storage needs `ceil(N / 8)` bytes. Storage in
64-bit words needs `ceil(N / 64) * 8` bytes. Both give 125,000 bytes, about
122.1 KiB, for 1,000,000 slots. This is the bitmap cost alone; object records,
Context records, indexes, alignment, and cleanup metadata are additional costs.

The proposed resolver checks in this order:

1. Confirm that the invoking UserContext is open and the runtime identity matches.
2. Validate the reference's access Context identity using live Global metadata.
   Reject a closed, foreign, unissued, or mismatched Context before touching User
   storage.
3. Select the Global table or the invoking User's table from the owner domain.
4. Validate the slot against its issued range before reading the bitmap.
5. Check its live bit and expected type, then resolve the object's address.

This ordering permits User tables and bitmaps to disappear completely on close.
A stale reference is rejected using metadata whose lifetime has not ended.
Global teardown still requires all dependent Users to end first; diagnostics
are not promised after Global itself has been destroyed.

Sorted closed-ID tables are an alternative discussed earlier. Dense, bounded
IDs make exact bitmaps simpler for the selected direction. Bloom filters and
frequency sketches are not substitutes for exact validity checks.

### Slot reuse and allocation failures

The recommended first implementation does not reuse a published object slot
within its ownership domain. User slot numbers may repeat in another User only
because its access Context identity differs. Context identities are likewise
not reused within one Global lifetime. Reaching either identity limit fails
explicitly, rather than recycling an old identity.

If slots or Context records are later reused, generation must be part of
validation, with defined exhaustion behavior. A live bit alone cannot distinguish
an old reference from a new object occupying the same slot.

Construction must complete and cleanup registration must succeed before the
object's live bit and reference become visible. Failed construction publishes no
valid ID. Existing successful objects remain usable after a later allocation
fails. Handling an unpublished slot reservation is an implementation decision;
it must not roll back identities already published by nested construction.

Early destruction clears the bit before destroying the object and detaches its
registered cleanup so Context close cannot destroy it twice. It does not imply
individual reclamation of monotonic arena bytes.

## Allocation limits and byte accounting

An object allocation is an independently managed object with its own identity.
Members, character storage, container capacity, and byte buffers are payloads,
not extra referenceable objects. A separately exposed string or buffer with its
own lifetime identity consumes an object slot; its internal storage does not.

Recommended budgets are separate and explicit:

| Budget | What it bounds | When capacity can be returned |
| --- | --- | --- |
| Context identities | Cumulative UserContexts created during one Global lifetime. | Not returned under the initial non-reuse policy. |
| Global object identities | Global reservations during one Global lifetime, including failed construction. | Not returned under the non-reuse policy. |
| User object identities | Object reservations within that UserContext, including failed construction. | Not returned during that User's lifetime; another User has a separate namespace. |
| Domain bytes | Retained arena block capacities and other owned storage, including tables, bitmaps, pool indexes, and cleanup metadata. | Only when the owner actually returns storage; individual logical destruction is insufficient. |
| String pool entries and bytes | Shared character storage and its index. | Reclaimed with the owner; admission may stop earlier. |

Internal allocations are exempt from the object-ID count, not the byte limit.
Small allocations must not bypass accounting through a default allocator or a
hidden global pool. Nested PMR strings and containers use their owner's resource.
Native SQLite, driver, and GPU allocations require separate accounting contracts;
do not claim that a host arena limit bounds all process or device memory.

Use checked arithmetic for counters, alignment, bitmap sizing, and requested
block capacity. Charge a whole backing block before admitting it, including
unused capacity. Refuse a growth block that exceeds the remaining budget, or
choose a smaller valid block under a documented growth policy. Failed allocation
must leave counters and previously published records consistent.

Bitmap and shutdown-record capacity should be reserved at owner creation, or
reserved in bounded segments before publishing the identities they cover.
Creating a Context may fail when this reservation cannot be satisfied. Closing
an existing Context must not need a fresh allocation to mark it invalid or retain
the minimum diagnostic record.

The implementation reports MemoryLimitError, derived from std::bad_alloc. Exhaustion must
not become a null dereference, partial live object, or silent fallback beyond the
hard budget. Reaching a limit prevents new allocations but must preserve the
ability to stop work, destroy existing objects, and close the Context.

### OS allocation granularity

Linux virtual and physical memory management uses pages; page size depends on
the target configuration. Anonymous mappings can acquire physical pages lazily.
Query the target page size rather than assuming 4096 bytes from its CPU model.
See the [Linux memory concepts](https://docs.kernel.org/admin-guide/mm/concepts.html)
and [sysconf page-size interface](https://man7.org/linux/man-pages/man3/sysconf.3.html).

A small `malloc` request can be served from an allocator's existing heap space,
while new `mmap` mappings operate at page granularity. Neither implies one
additional OS page for every managed object. See the Linux
[malloc](https://man7.org/linux/man-pages/man3/malloc.3.html) and
[mmap](https://man7.org/linux/man-pages/man2/mmap.2.html) documentation.

Our enforceable byte budget initially covers memory ownership at the configured
upstream boundary, not exact physical RSS or the upstream allocator's private
overhead. Returning blocks to that upstream does not promise immediate OS memory
release. Any direct page-backed arena would be a separate measured adaptation.

## Strings and byte storage

There are three separate storage responsibilities: inline content, ordinary
Context payload storage, and shared immutable content. All belong to an existing
Global or User domain. Immutability does not create a third lifetime or promote
User data to Global.

| Content | Proposed handling |
| --- | --- |
| Source string literal | Borrow static storage when the API permits it; preserve the User access rules for a Context-derived handle. |
| Short owned string | Prefer existing short-string storage or a measured inline Value representation. |
| Ordinary string or bytes | Allocate payload through the owning Context resource without another object ID. |
| Large content with little reuse | Use ordinary payload storage or a dedicated Context-owned block. Do not insert merely because it is long. |
| Known repeated immutable content | Explicitly intern within the selected owner's bounded string pool. |
| Dynamic content with uncertain reuse | Keep ordinary storage initially; optional later admission may use bounded frequency statistics. |

An 8-byte inline threshold was discussed, not selected. Measure total Value
layout, existing PMR string behavior, alignment, and copy cost on both native and
32-bit MIPS targets before fixing a threshold. String lengths and limits are in
bytes, not Unicode character counts. These storage adaptations must preserve
every Rust Value variant and its string semantics.

### Stable saving and optional interning

Following LLVM's separation, a saver appends characters to stable storage, while
an interner additionally checks an index for identical content. Grow by adding
blocks; do not reallocate a single character vector and invalidate existing
views. Pool indexes and saved characters both use the owner's allocator and
budget. Individual shared strings need no reference-counted lifetime extension.

UserContext exposes explicit saving and interning through `save_string` and
`intern_string`, and explicit Global delegation through `save_global_string` and
`intern_global_string`. These return scoped `StringRef` values resolved through
`user.get()`. Stable addresses are valid only until the owner closes; they do
not make arbitrary escaped `string_view` values safe.

When an internal string is exposed through a checked view, bind it to an existing
owner reference and resolve its content during access. Owner liveness alone does
not validate an old address after mutation or buffer growth. Mutable slices need
bounds and invalidation rules or a storage version. A caller requesting an
independent referenceable buffer must allocate an object identity for it.

### Admission based on reuse

For `R` copies of `L` content bytes in the same lifetime, deduplication can avoid
approximately `(R - 1) * L` character bytes, before index, terminator, reference,
and alignment costs. This estimate is a design model, not measured TEAQL savings.
Frequency, length, expected retention, and hash/comparison cost all matter. Long
strings seen once can occupy substantial memory without a sharing benefit.

Start with known repeated metadata and explicit interning. If automatic admission
is justified by workloads, borrow TinyLFU's bounded frequency-estimation idea,
not its full cache eviction machinery. Approximation may change the decision to
share a string; it must not change string contents or reference validity.
Deduplication always verifies actual bytes after hash lookup.

Pool admission has its own entry and byte limits. When the pool cannot accept a
new string, ordinary Context storage is an optional fallback only if the owner's
hard byte budget still permits it. Existing borrowed characters cannot be evicted
while their owner is open. An arena-backed index eviction would not reclaim
characters by itself and must not be advertised as memory recovery.

Admission affects future saves. It cannot automatically redirect raw views into
an earlier private copy, and adding a shared copy does not free that private
arena storage. Any promotion must preserve existing references. Global interning
of User input requires an explicit deep copy into Global, including index keys;
no process-wide hidden cache may retain User content.

## Shutdown and diagnostics

The implemented public state sequence is `Open -> Closing -> Closed`. Once closing
begins, invalidate User-facing access in the Global Context record before any
User table or characters can disappear. Stop and join registered asynchronous
work while the objects it borrows remain alive. Then invalidate object access,
run registered destructors in reverse order, and return User tables, bitmaps,
payload blocks, and arena blocks. Preserve idempotent close and resource cleanup.

The arena allows trusted final diagnostics to allocate during quiescence. Public
reference access and new business work are rejected as soon as closing begins.
Trusted shutdown paths use precharged storage first; larger diagnostics may use
remaining normal capacity under the same byte budgets and may fail. Cleanup must
handle diagnostic failure and finish stopping work. This internal path preserves
the cleanup responsibility and Rust log semantics; it does not permit applications
to use closing Contexts.

All reclamation and reference resolution remain confined to the owner thread.
Atomically changing a bit alone cannot make a simultaneous dereference and close
safe. Concurrent access would require a separate synchronization contract that
keeps host memory valid until readers and native work have stopped.

Diagnostics may record Context identity, object slot, expected type, and closing
and access locations. `std::source_location` can capture callers of explicit
access functions with default location arguments; transparent `operator->`
access does not automatically provide that information. Keep diagnostic records
bounded and alive in Global; do not dereference User memory to report its expiry.

Changing an internal pointer cannot rewrite all copied native pointers. Checked
handles and IDs only protect access through their resolver. Optional Linux page
protection can aid debugging, but requires dedicated pages and address quarantine;
it is not the selected portable validity mechanism.

## Proposed implementation sequence

These were the implementation work packages within the existing migration plan.
The implementation contract records completed mechanisms and remaining optional
optimization work; memory implementation is not proof of a later migration stage.

1. Specify the application-facing UserContext factories, budget units, allocation
   errors, runtime identity, and opaque reference encoding. Keep existing core
   responsibilities and low-level allocator-only operations intact.
2. Add bounded identity records and exact bitmaps. Implement typed resolution,
   non-reuse limits, and shutdown invalidation before broad API migration.
3. Add byte accounting around existing arena upstream allocations and reserve
   shutdown metadata. Integrate native quiescence and final diagnostic behavior.
4. Add ordinary stable string saving and explicit bounded interning using the
   existing Context resources. Measure before introducing extra small-slot pools,
   dedicated large-block policies, or a custom short-string representation.
5. Evaluate automatic admission only when measured string workloads justify its
   CPU and metadata costs. Keep approximate policy statistics separate from
   exact lifetime checks.

The generator remains untouched. Runtime examples and tests should drive API
integration without rewriting TEAQL behavior locally.

## Acceptance evidence required

Implementation must validate relevant contracts through CMake and CTest, with
ASan and UBSan for lifetime work. Current acceptance evidence is recorded in the
implementation contract; the following requirements remain the design checklist:

- Copied IDs reject access after close and after destruction of their User owner,
  while the Global diagnostic owner remains alive.
- Equal slot numbers in different Users and Globals never validate foreign IDs.
- User-facing access to a surviving Global resource expires with its User.
- Bounds, type, unpublished IDs, early destruction, and identity exhaustion are
  checked before object memory is accessed; published identities never wrap.
- Constructor and upstream allocation failures preserve existing live objects,
  cleanup registration, counters, and idempotent shutdown.
- Full ID tables and byte budgets still permit asynchronous stop/join, resource
  destruction, and all-block reclamation without new shutdown bookkeeping.
- Payload and container growth consume bytes without consuming object IDs;
  independent buffer objects consume IDs. Nested allocators stay in their domain.
- Bitmap arithmetic, metadata cost, and counter overflow are checked at limits.
- String saving keeps addresses stable across growth; interning compares bytes,
  respects budgets, and preserves explicit cross-domain copies.
- Mutable views have defined invalidation and bounds rules. Pool admission or
  index eviction never invalidates already borrowed immutable content.
- Measure repeated and distinct strings, short and long payloads, index overhead,
  allocation latency, total retained blocks, and close latency. Include native
  builds and the MIPS target; emulator results are not physical-device evidence.

## Remaining policy and integration work

- Tune budget and pool defaults for measured application workloads; account for
  external library and device heaps separately.
- Extend application adoption of checked references while preserving explicitly
  temporary native borrows and their mutation/invalidation contracts.
- Measure before adding custom inline-string layouts, small-slot pools,
  large-allocation policies, or automatic frequency-based admission.
- Any future slot or Context identity reuse requires generation validation and
  explicit exhaustion behavior. The current implementation never reuses them.
