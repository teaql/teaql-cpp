# Context memory implementation

The kernel implements the bounded identity, bitmap, budget, and string-storage
direction in [the memory design](context-memory-design.md). The APIs below retain
the Rust v5.0.5 concepts and layer responsibilities. Business code receives
UserContext; GlobalContext is owned by runtime bootstrap and is not exposed by
a User `global()` accessor.

## Current migration integration, 2026-10-10

The complete native suite passes 388/388, including managed object/byte limits,
independent live bitmaps, actual Context deletion, nested allocator propagation,
explicit destination copies and native asynchronous cleanup. The final full
ASan/UBSan suite also passes 388/388 and MIPS/QEMU passes 387/387; results are
recorded in the Stage 6 final-review measurements. Final sanitizer leak scanning
is disabled; earlier scoped leak checks below retain their own historical scope.

Memory-kernel acceptance does not intercept arbitrary raw-pointer access after
close. Checked references resolve through Context identity/live state; pointers
and PMR stack objects remain explicit borrows. Unmanaged stack PMR objects must
be destroyed before their owning Context. Context factories register destruction
and clean managed objects/resources before returning arena blocks. QEMU evidence
does not establish physical-router behavior or native GPU adapter support.

## Logical payload identity across explicit copies

Opaque SQL projection state additionally uses a comparison token formed from a
32-bit arena namespace and the existing non-reused object slot. All arenas,
including standalone kernel arenas, obtain distinct namespaces from one bounded
scalar atomic issuer; namespaces never wrap or reuse, and exhaustion throws
`std::overflow_error`. The issuer retains no objects, Context pointers or caches.
A 32-bit atomic keeps the MIPS implementation independent of a 64-bit atomic
library. The token is not the public `Ref<T>` encoding and grants no access.

Explicit deep copies preserve this origin token and allocate the concrete
payload and nested strings in the destination. Thus copies retain logical
identity after source Context deletion without extending source ownership.
Separately allocated equal values remain distinct even when a later arena
reuses the same address. Same-domain projection wrappers borrow their payload
without requiring it to be copyable. Noncopyable cross-domain retention fails
explicitly; all wrapper borrows remain invalid after their owner closes.

Private shutdown diagnostics follow the existing `Arena::make` closing path.
Their origin tokens use a disjoint non-reused range above the public object-slot
limit, within the same arena namespace. Shutdown payloads use precharged reserve
storage and registered cleanup; they publish no bitmap bits or checked references
and cannot start new asynchronous work. Token exhaustion throws without wrapping.

This adds a four-byte namespace and four-byte shutdown counter per arena, and an
eight-byte token per projection wrapper, subject to layout padding. It does not add object IDs for
container or character backing allocations or change checked-reference quotas.
The final ownership/parity selections pass native, ASan/UBSan with leak
scanning disabled and MIPS/QEMU. The subsequent complete native consumer
regression passes 314/314 in 74.66 seconds, and a post-test build reports no
pending work. Earlier verification totals below describe their historical
checkpoints.

## Implementation acceptance

The agreed memory mechanisms are implemented. This completion applies to the
memory kernel and its Context integration; it does not complete the broader
Rust runtime migration.

### Current memory verification, 2026-10-07

The current working tree passes all seven memory/lifetime checks on native
Debug, ASan/UBSan with leak detection, and OpenWrt MIPS32r2/O32 under QEMU.
Selected executable targets were built before each run. The selection covers
actual User deletion, stale-reference diagnostics, the million-object bitmap,
allocation-failure recovery, resource registration, logging, and asynchronous
stop/join ordering. No production dependency was added.

| Configuration | Memory/lifetime result | Selected CTest wall time |
| --- | --- | ---: |
| Native Debug | 7/7 passed. | 0.77 seconds. |
| ASan/UBSan with leak detection | 7/7 passed. | 1.34 seconds. |
| OpenWrt MIPS32r2/O32, QEMU 74Kf | 7/7 passed. | 2.82 seconds. |

These timings describe the complete seven-check selection with two CTest workers,
not isolated allocation latency, peak process memory, or physical-router results.

The latest complete native consumer rebuild and Debug regression passed 279/279
registered checks in 67.99 seconds with four workers. Ninja subsequently reported
no pending build work. This includes the asynchronous completion ownership and
owning-range integration changes in the current working tree; the seven-check
sanitizer and MIPS selections above remain specifically memory/lifetime evidence.

An earlier complete native Debug CTest regression passed 265/265 checks
in 93.17 seconds with two CTest workers. That total describes its historical
checkpoint; sanitizer and MIPS evidence above remains the seven-check
memory/lifetime selection.

The reference contracts additionally inject upstream allocation failure while
crossing the 64-object bitmap-page boundary and while allocating resource storage
after reserving an ID. A failed attempt publishes no live bit or destructor for
an unconstructed object. Prior references remain usable, consumed IDs are never
reused, retry publishes a complete object, and early destruction plus actual User
deletion releases each successfully constructed resource exactly once. Every
backing block is returned when its owner ends.

The million-object test verifies a 125,000-byte bitmap on both native and MIPS;
this is the bitmap alone, not the total object-table or arena footprint. Business
code in the runnable example receives only UserContext and explicitly delegates
Global allocation, copies retained data, and rebinds surviving Global references.

Reproduce the selection with the appropriate build directory:

```sh
cmake --build build --parallel 4 --target \
  teaql_context_memory_tests teaql_context_reference_tests teaql_async_tests \
  teaql_runtime_context_tests teaql_context_logging_tests teaql_context_lifetime
ctest --test-dir build --output-on-failure --parallel 2 \
  -R '^(context_memory_contracts|context_reference_contracts|context_early_global_close|async_context_contracts|runtime_context_resource_contracts|context_logging_contracts|example_context_lifetime)$'
```

Use `build-sanitize` with
`ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1` for CTest, or
`build-wndr4300-memory`, whose tests use the configured QEMU emulator.
Rust v5.0.5 lock verification and formatting/checking of 562 handwritten C++
files with clang-format 18.1.8 passed.

These checks establish the selected managed-memory contract. Native pointers
obtained from `get()` remain temporary borrows; arbitrary later dereferences are
not intercepted. Physical-router execution, native-library/device heap
accounting, automatic string admission, and complete runtime parity remain
separate acceptance scopes. The sections below retain historical evidence;
old full-suite totals describe their own checkpoints.

### Implemented contracts and earlier checkpoints

| Agreed contract | Implemented mechanism |
| --- | --- |
| Exactly two explicit ownership lifetimes | Global and User arenas; business-facing Global allocation is delegated through UserContext. |
| Growing blocks with complete teardown | Monotonic multi-block storage, reverse-order cleanup, then release of all backing blocks. |
| Explicit object identity | Typed eight-byte `Ref<T>` and a checked UserContext resolver. |
| Independent Global and User allocation tables | Paged exact live bitmaps with constant-time slot validation. |
| Finite allocations and bounded memory | Non-reused identities, per-domain object/byte limits, and an aggregate live User byte limit. |
| Strings and bytes do not consume an ID per payload allocation | Stable owner-local storage and immutable scoped views; independent managed objects still consume IDs. |
| Sharing follows actual reuse intent | Explicit bounded interning; ordinary saving for unshared content. |
| Close remains possible after exhaustion | Precharged shutdown storage and stop/join before object destruction and block reclamation. |

An earlier completed full native regression checkpoint on 2026-10-07 passed
224/224 CTest checks after owning Value construction, metadata/scalar
registration and the public facade change. The ten selected metadata/scalar
sanitizer/MIPS checks each pass 10/10; those remain a separate selection from
the dedicated memory/lifetime evidence below. The dedicated memory/lifetime selection below remains
scoped evidence rather than completion of every runtime migration gate.

The subsequent callable ownership acceptance rebuilt six selected checks in
both ASan/UBSan and OpenWrt MIPS configurations and passed 6/6 in each. This
selection includes Context memory/reference contracts, core collection and
entity expression contracts, plus the two new SafeExpression checks. All 444
SafeExpression observations match live original Rust on native and MIPS. See
[the expression ownership audit](safe-expression-api-acceptance.md) for the
reproduction commands and the non-copyable-state boundary.

Earlier regression checkpoint on 2026-10-07: rebuilding every native consumer and running
the complete Debug suite passed 184/184 checks. The seven memory/lifetime checks
also passed under ASan/UBSan with leak detection and the OpenWrt MIPS cross-build
under QEMU, including the updated runnable example. Rust v5.0.5 lock verification,
clang-format 18.1.8 and `git diff --check` passed. The million-object bitmap is
125,000 bytes; this is not the total object-table or arena footprint.

Latest scoped memory acceptance on 2026-10-07, including the example's explicit
budgets, early destruction, non-reused IDs, and zero retained User byte charge:

The final rerun passed 7/7 in each configuration. The sanitizer and MIPS targets
were rebuilt successfully before running this selection. Native reran the
existing executables while its full consumer rebuild continued; the full native
regression after the owning Value repair subsequently completed with 219/219.
The newer metadata/scalar public-facade change subsequently passed the full
native 224/224 regression and ten selected sanitizer/MIPS checks 10/10 each. In addition, all ten
selected owning Value integration checks passed under ASan/UBSan and MIPS/QEMU,
including nested allocator propagation, failure recovery and cross-domain
retention after actual User destruction. See
[owning Value acceptance](value-owned-api-acceptance.md).

| Configuration | Result |
| --- | --- |
| Native Debug | 7/7 memory and lifetime checks passed. |
| ASan/UBSan, leak detection enabled | The same 7/7 checks passed. |
| OpenWrt MIPS cross-build and QEMU 74Kf | The same 7/7 checks passed. |

The recorded earlier three-configuration selected builds completed successfully
before their tests ran. This scoped acceptance covers the memory kernel and Context integration, not the entire
migration or execution on the physical router. The selection is reproduced by
the sanitizer command below; use the other build directories without sanitizer
environment variables for native and MIPS checks. No new production dependency
was introduced.

On 2026-10-06, the final memory acceptance selection passed six native checks,
the same six ASan/UBSan checks with leak detection, and four OpenWrt MIPS checks
under QEMU. After rebuilding all consumers, the full native Debug regression
suite passed 155/155 tests. Pinned Rust v5.0.5 verification and clang-format
18.1.8 checks passed. These include the exact 125,000-byte bitmap for one million object
allocations. The reference tests also inject failure at each upstream allocation
during User initialization, on both sides of Context identity registration:
every User block is returned, already-issued identities remain consumed,
unpublished Context access is rejected, and a subsequent User can still access
the original Global objects. Global diagnostic metadata is retained only within
its configured budget and is reclaimed with Global.

Custom short-string layouts, automatic frequency-based admission, and additional
small-object pools remain optional optimizations. The implementation uses C++20
standard facilities and adds no production dependency. Native pointers and views
returned by resolution remain temporary borrows; checks apply at resolution,
not to arbitrary later C++ dereferences. Physical MIPS-device execution remains
unverified.

### Closing-state acceptance

The closing-state contract is also checked inside a registered stop/join callback,
before the User arena is reclaimed. Eighteen business operations must reject
access with `ReferenceError::Kind::Closed`: resolving objects and payloads,
allocating in either domain, saving and interning characters, saving bytes,
borrowing literals, rebinding Global access, destroying an object, and registering
a resource. The test disables upstream allocation during close and verifies that
none of these operations attempts an allocation or changes the Global object
count or retained bytes. All User bytes are returned; another User can still
rebind the surviving Global object after actual destruction of the old User.
Trusted shutdown diagnostics retain their separate precharged-storage contract.

The final memory review on 2026-10-06 passed the full native Debug suite
(161/161), seven ASan/UBSan checks with leak detection and five OpenWrt
MIPS/QEMU checks, including this closing-state test,
the million-object bitmap, resource cleanup, asynchronous quiescence, the
parent-lifetime guard and the runnable lifetime example. See
[the reference contracts](../tests/context_reference_tests.cpp) for the test.

Reproduce the selected sanitizer acceptance after building its targets:

```sh
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build-sanitize --output-on-failure -j 2 \
  -R '^(context_memory_contracts|context_reference_contracts|context_early_global_close|async_context_contracts|runtime_context_resource_contracts|context_logging_contracts|example_context_lifetime)$'
```

### Allocation-failure acceptance on 2026-10-07

The memory kernel and Context integration passed all seven selected checks in
each configuration: native Debug, ASan/UBSan with leak detection, and the
OpenWrt MIPS cross-build under QEMU. The selection covers checked references,
arena lifetimes, early Global close rejection, resource registration,
asynchronous shutdown, Context logging, and the runnable lifetime example.
Pinned Rust v5.0.5 verification and clang-format 18.1.8 checks also passed.

The reference contracts now inject upstream allocation failure at successive
points in Global string-pool initialization, character storage, and index growth.
Previously published Global and User object references remain usable after a
failed attempt. Retrying preserves exact deduplication, and character/index
storage consumes no additional object identities. After actual destruction of
the source User, its aggregate byte charge is zero and only Global-owned blocks
remain. A subsequent User explicitly rebinds and reads the complete retained
string. Global destruction returns every successful and abandoned pool block.
These checks exercise allocation recovery and explicit cross-domain retention;
they do not establish execution on the physical router.

## User-owned entity graph anchor

`UserContext::entity_runtime_state()` retains the original public factory
responsibility: each call returns a fresh mutation ledger and shares only that
User's immutable identity-graph anchor. The anchor is created lazily in User
storage; it is not a Global cache or a default resource. Failed anchor construction
does not publish the anchor pointer; later fresh-state failure may retain a valid
anchor until close. The factory validates an open User before
touching storage and returns the existing Context-bound state carrier.

Normal same-domain state copies may share their ledger according to the existing
EntityRuntimeState contract; repeated factory calls must not do so. Cross-domain
retention requires the existing explicit deep copy, including graph payloads and
ledger values. A User close reclaims both anchor and all operation states. This
does not add an ownership lifetime or make escaped native state borrows safe.
The direct source and ownership tests are in
`tests/parity/teaql-runtime/context.cpp`; current verification is recorded in the
runtime source-test acceptance checkpoint.

## Checked objects

```cpp
auto value = user.make_ref<int>(42);
auto copy = value;
user.get(copy) = 43;
user.close();
// user.get(value) throws ReferenceError before reading object storage.
```

`Ref<T>` occupies eight bytes. Its opaque `std::uint64_t` is a process-local
memory reference, not a persistent entity ID, address, or security credential.
Resolution checks runtime identity, access Context, slot range, live bit, and
exact type. It requires a live UserContext object; calling a destroyed Context
itself remains invalid C++.

| Encoded field | Bits | Meaning |
| --- | ---: | --- |
| Runtime | 23 | Nonzero namespace, never reused in the running kernel instance. |
| Access Context | 20 | Nonzero for business access; zero for a bootstrap Global reference. |
| Owner | 1 | Global or User object table. |
| Object slot | 20 | Nonzero, never reused within its owner. |

A saturating process-wide atomic scalar issues runtime namespaces. It owns no
Context storage and retains no Context pointers. IDs must not be persisted
across process runs. Global keeps bounded, paged Context state and static closing
locations, not User object tables or payloads. Old IDs can safely be rejected by
a later User after their original User object has been destroyed.

`ReferenceError` contains the reason, ID, current access location, and original
Context closing location. Repeated close preserves the original location.

The [runnable lifetime example](../examples/context_lifetime.cpp) passes only
UserContext into business code. It allocates User rows, explicitly deep-copies
them into Global through that User, and interns repeated Global text. After
actually deleting the original UserContext, a new User rejects the copied User
reference with `ReferenceError::Kind::Closed`. Explicit Global rebinding then
resolves both the retained rows and shared text. No old User storage is accessed
to diagnose expiry.

The example also sets explicit workload budgets: 2 MiB Global storage, 256 KiB
per User, 512 KiB aggregate User storage, and two object reservations per User.
It checks that saving character payloads consumes no additional object identity,
that early destruction rejects the old reference, and that a destroyed object's
ID is not reused to bypass the reservation limit. After reaching that limit,
close still succeeds; actual User destruction leaves zero charged User bytes.
These limits demonstrate the contract and are not a measured deployment profile
for the router.

```sh
cmake --build build --target teaql_context_lifetime
ctest --test-dir build --output-on-failure -R '^example_context_lifetime$'
```

| API | Result |
| --- | --- |
| `user.make_ref<T>(...)` | User-owned managed object, returned as `Ref<T>`. |
| `user.make_resource_ref<T>(...)` | User-owned external RAII wrapper, returned as `Ref<T>`. |
| `user.make_global<T>(...)` | Global-owned managed object with access bound to this User. |
| `user.make_global_resource<T>(...)` | Global-owned external RAII wrapper with User access. |
| `user.get(ref)` | Validates and returns a temporary native borrow. |
| `user.destroy(ref)` | Invalidates and performs exactly-once early destruction. |
| `user.rebind_global(ref)` | Acquires current-User access to a live Global object. |
| `global.make_ref<T>(...)` / `make_resource_ref<T>(...)` | Bootstrap references that require explicit User rebinding. |

Global resources survive User close; old User access expires. Rebinding rejects
foreign runtimes, User objects, unissued source Contexts, unavailable slots, and
wrong types. Allocator-aware copies propagate the destination allocator into
nested strings and containers. Custom objects must still implement correct
copies: arbitrary pointer graphs and callback captures cannot be inspected.

The existing `make`, `make_resource`, and `Memory` APIs remain trusted kernel
and native-integration interfaces. Their normal managed construction still
charges object reservations and bytes, but returned raw references are temporary
borrows. Escaped pointers do not become checked references.

## Tables and allocation failures

Each arena has its own object table, grown in pages of 64 records with one
64-bit validity word. Records contain address, type, cleanup, and extent.
Checked resolution is constant-time. A User table and bitmap are reclaimed
entirely on close. One million slots use exactly 125,000 bitmap bytes; records,
directories, objects, unused capacity, and cleanup are additional costs.

Construction reserves an identity before executing the constructor and publishes
the live bit only after successful construction and cleanup registration.
Failed reservations consume the finite namespace and remain unavailable. Nested
successful construction is never undone by rolling the parent's counter back.
Abandoned monotonic bytes remain charged until close.

Early destruction clears validity before calling the destructor and disables
the registered cleanup, preventing double destruction on close. The object
record retains its address extent until arena reclamation. Recreating a borrow
or registering a base/member view of that dead extent checks its validity first;
it cannot silently become an unmanaged external resource. This tombstone uses
the existing record and adds no allocation or public identity. Published slots
and Context identities are not reused; generation-based reuse is not implemented.

## Budgets

`ContextLimits` configures Global and default User `MemoryLimits`, cumulative
Context creation, and aggregate live User bytes. A User can override its limits
at creation. Applications should configure their workload explicitly.

| Budget | Default | Accounting |
| --- | ---: | --- |
| Object reservations per domain | 1,000,000 | Includes failed reservations; early destruction does not refund IDs. |
| Arena bytes per domain | 64 MiB | Retained backing blocks, metadata, payloads, unused capacity, and shutdown reserve. |
| Cumulative User Context identities | 1,000,000 | Includes identities whose initialization later fails; never refunded. |
| Aggregate live User arena bytes | 64 MiB | Each User backing block counted once; actual release returns capacity. |
| Shutdown reserve per arena | 4 KiB | Preallocated and included in the owner byte limit. |
| Interned entries per owner | 4,096 | Stops new unique-content admission. |
| String pool allocation bytes | 1 MiB | Character and index allocations, including abandoned monotonic storage. |

The limits use checked arithmetic and reject an upstream block before exceeding
its cap. `MemoryLimitError`, derived from `std::bad_alloc`, identifies the exhausted
budget. Ordinary upstream allocation failure propagates as `std::bad_alloc`.
The aggregate resource accounts for bytes without owning or caching User blocks.
Global and aggregate User caps are separate. Native SQLite/driver/GPU heaps,
application-owned wrapper allocations, allocator overhead, RSS, and device memory
are outside managed-arena accounting.

An illustrative configuration, not a universal router recommendation:

```cpp
teaql::ContextLimits limits;
limits.global.max_bytes = 8 * 1024 * 1024;
limits.user.max_bytes = 4 * 1024 * 1024;
limits.user.max_objects = 8192;
limits.max_live_user_bytes = 16 * 1024 * 1024;
limits.max_user_contexts = 100000;
teaql::GlobalContext runtime(std::pmr::new_delete_resource(), 4096, limits);
auto user = runtime.make_user_context();
```

## Strings and payload views

Internal characters, byte arrays, container capacity, and bookkeeping consume
bytes, not additional object IDs. An independently managed string or buffer
object consumes one ID; its internal storage does not consume further IDs.

| API | Behavior |
| --- | --- |
| `save_string(text)` | Stable, null-terminated User copy without interning. |
| `intern_string(text)` | Reuses byte-identical User content admitted to its bounded pool. |
| `save_global_string(text)` / `intern_global_string(text)` | Explicit Global saving or sharing. |
| `literal_string(StaticString)` | Borrows compile-time literal storage without allocating characters. |
| `save_bytes(bytes)` / `save_global_bytes(bytes)` | Immutable copy into the selected owner. |
| `get(StringRef)` / `get(BytesRef)` | Checks access scope before returning a temporary view. |

`StringRef` and `BytesRef` contain a scope token, pointer, and length; they are
payload views, not eight-byte independent object IDs. Their constructors are
private. Copies preserve scope, and Global payloads can be explicitly rebound.
Saving and interning follow LLVM's separate responsibilities using PMR storage
and an owner-local PMR index. Addresses remain stable across block growth.
Interning verifies bytes after hashing and preserves embedded zero bytes.
Lengths are byte counts; statistical counters saturate instead of wrapping.

Pool limits stop admission without evicting borrowed characters. Ordinary-storage
fallback still obeys the hard owner budget. If index admission fails after saving
characters, the same stable copy is returned without duplication or false memory
recovery. All allocations remain charged until close.

Explicit interning represents known reuse intent. Automatic TinyLFU admission,
extra small-slot pools, and a fixed 8-byte string layout remain optional measured
optimizations. Existing PMR short-string behavior and Rust Value variants remain
unchanged. Consteval `StaticString` conversion rejects runtime-temporary literals;
the originating code module must outlive the Global runtime borrowing its literals.

## Resources and native access

`insert_resource(ref)` and `insert_named_resource(name, ref)` delegate ownership
registration without exposing Global. Origin-explicit kernel registration remains
available. Managed object extents allow tracking base-class and member borrows.
Typed and named resource lookups reject early-destroyed objects.

ResourceBorrow checks stable destination Context state before touching its arena,
so an inline borrow can reject access after User destruction while Global remains
alive. Copying an expired borrow is rejected. Legacy address-based registration
can scan the object table at registration or initial borrow construction; known
slots avoid rescanning during required-resource lookup. Subsequent live checks
and checked ID resolution remain constant-time.

Unmanaged external objects and mutable container interiors still require the
caller's lifetime and invalidation contract. Returning `T&`, `string_view`, or
`span` permits a native borrow to escape; it must not be used after close,
early destruction, or invalidating mutation. Checks do not intercept arbitrary
C++ dereferences or manual deletion of managed objects.

## Closing and asynchronous work

Close is terminal, idempotent, and confined to the owner thread:

1. Mark the User access record Closing before reclaiming any User table.
2. Stop and join registered work while objects and host memory remain alive.
   Trusted kernel borrows may inspect still-live objects during quiescence.
3. Reject allocation, clear object validity, and run reverse-order destructors,
   skipping early-destroyed objects.
4. Return table, payload, arena, and shutdown-reserve blocks.
5. Mark the record Closed; retain bounded diagnostic metadata in Global.

Quiescence allocation first uses the precharged buffer without calling upstream
or consuming new public identities. Larger final diagnostics may use remaining
normal storage under the same owner and aggregate caps, preserving existing log
behavior. They can still fail; native noexcept cleanup must handle that failure
and finish stopping work. Exhausted normal quotas cannot prevent shutdown
bookkeeping or reserve-sized cleanup from completing.

Allocation and resolution enforce owner-thread use. Close from another thread,
an active managed constructor/destructor, or an upstream allocation fails fast.
Global reclamation with live Users also fails fast, including through an Arena
pointer. Native completion threads retain only their registered host-memory
borrow and notification contract. This is not a concurrent reader lease, scheduler,
or GPU synchronization implementation.

## Evidence and validation

- [Budgets](../include/teaql/memory/limits.hpp) and
  [reference types](../include/teaql/memory/reference.hpp)
- [Arena construction](../include/teaql/memory/arena.hpp) and
  [cleanup](../src/memory/arena.cpp)
- [String storage](../include/teaql/memory/strings.hpp)
- [Context API](../include/teaql/runtime/context.hpp) and
  [resource borrowing](../include/teaql/runtime/resources.hpp)
- [New memory contracts](../tests/context_reference_tests.cpp),
  [existing lifetime tests](../tests/context_memory_tests.cpp), and
  [checked lifetime example](../examples/context_lifetime.cpp)

Initial memory implementation checkpoint on 2026-10-06:

| Configuration | Result | Scope |
| --- | --- | --- |
| Native Debug CMake build and CTest | 111/111 passed | Full regression suite at this checkpoint, including fixed Rust semantic observations and new memory contracts. |
| ASan and UBSan with leak detection | 10/10 passed | References, resource registration, asynchronous shutdown, Context memory/logging, expression copies, query stores, build coordination, and lifetime example. |
| OpenWrt GCC 13.3.0 MIPS cross-build and QEMU 74Kf | 7/7 passed | Memory/reference contracts, parent-close guard, asynchronous cleanup, lifetime example, and both SQLite examples. |
| Pinned upstream and C++ formatting | Passed | Rust v5.0.5 lock verification and clang-format 18.1.8. |

The million-object test validates the exact 125,000-byte bitmap on both native
and MIPS builds. Full-count/byte exhaustion, upstream failure, and oversized
diagnostic failure still allow registered cleanup to complete. Native test
workers resolve borrows on their owner before launch and retain their host
borrow until stop/join; they do not resolve Context data on completion threads.

Native regression command:

```sh
cmake --build build
ctest --test-dir build --output-on-failure -j 4
```

Sanitizer testing uses `-fsanitize=address,undefined -fno-omit-frame-pointer`,
`ASAN_OPTIONS=detect_leaks=1`, and `UBSAN_OPTIONS=halt_on_error=1`. The expanded
selection includes every contract that required shutdown or native-borrow
integration changes, rather than just allocation primitives.

The MIPS build is separate from the original size experiment, in
`build-wndr4300-memory/`. It retains MIPS32r2/O32, big-endian, soft-float code,
uses the OpenWrt 24.10.4 ath79/nand toolchain and SQLite 3.40.1, and runs through
the configured QEMU emulator with CTest. Physical-device execution remains
pending. The historical [linkage measurements](../measurements/README.md) still
describe their recorded source checkpoint and have not been overwritten.

Memory work does not complete graph save, mutation governance, full audit/log
parity, or GPU adapters.

### Memory acceptance recheck

On 2026-10-06, the current working-tree implementation passed the full native
Debug suite (122/122). ASan and UBSan with leak detection passed all six selected
memory integration checks: checked references, arena lifetimes, early Global
close rejection, resource registration, asynchronous shutdown, and the runnable
Context lifetime example. The OpenWrt MIPS build passed four checks under QEMU:
checked references, arena lifetimes, early Global close rejection, and the same
example. Physical-device execution remains pending.

The reference contracts additionally cover a failed parent reservation at slot
64 that successfully constructs a child at slot 65. The failed parent remains
dead, the child remains resolvable, and both reservations count against the
configured limit. Separate Global and User exhaustion, independent validity
bits, User bitmap reclamation, and rebinding the remaining Global object into a
new User are verified together. These checks exercise table growth and failure
ordering without introducing another owner or reusing expired identities.

The sanitizer selection is reproducible after building its matching targets:

```sh
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
    ctest --test-dir build-sanitize --output-on-failure \
    -R '^(context_reference_contracts|context_memory_contracts|context_early_global_close|example_context_lifetime|async_context_contracts|runtime_context_resource_contracts)$'
```

Pinned Rust baseline verification and clang-format 18.1.8 formatting checks also
passed. The checked-reference contract guards resolution; an escaped native
pointer or view still follows the caller's lifetime contract.


## Query-path integration checkpoint

The subsequent public entity-query integration uses the same ownership model.
Operations snapshot input queries; rows, errors, request provenance, and nested
enhancements belong to the requesting User. A Global aggregation cache owns
explicit copies and copies hits back into the next User. Native provider work
is cancelled during User close before arena reclamation. Lazy execution rejects
an early-destroyed executor with `ReferenceError::Dead` before dereferencing it.

On 2026-10-06, the expanded native suite passed 116/116 tests, including real
SQLite query/enhancement execution and these new lifetime cases. The public
query oracle matched 165/165 observations through the locked Rust v5.0.5
EntityDataService. OpenWrt GCC 13.3.0 built the query probe and entity-service
contracts for big-endian MIPS32r2/O32; QEMU passed the entity-service contracts
and the same 165/165 public-query observations. Physical-device execution
remains pending. The original linkage-size measurements remain unchanged.

ASan/UBSan with leak detection passed 7/7 selected contracts: references,
Context memory, asynchronous shutdown, public entity services, recursive
hydration/enhancement, and public-query differential observations. This expands
the initial memory checkpoint with actual query-path evidence; it does not
complete the six-stage migration.


## Scoped callback and dead-extent checkpoint

The current implementation passes the full native Debug CTest suite (134/134).
ASan and UBSan with leak detection pass seven selected checks: policy resource
ownership, reentrant-close rejection, original public review comparisons,
checked references, arena lifetimes, early Global-close rejection and the
runnable Context lifetime example. The OpenWrt MIPS build passes six ownership
and memory checks under QEMU 74Kf; its review probe also matches all 276 live
Rust v5.0.5 observations. Physical-device execution remains pending.

Early-destroyed managed objects retain their address extent in the existing
object record. Address discovery checks the dead bit before constructing a new
borrow or asking a resource for its allocator. Tests cover the original object,
a member view, repeated registration and a polymorphic policy. Previously,
rediscovery could misclassify that dead address as an unmanaged external object
and permit a virtual call. The correction adds no table, allocation or identity
and preserves native borrow access during registered shutdown quiescence.

Policy registry, approval-provider and warning-sink registrations use the same
managed resource guards. Synchronous review rejects reentrant close before its
active callbacks or stack result carriers can be freed. Explicit Global copies
of nested snapshots and events survive actual User destruction. This integration
retains the two allocation domains and does not implement graph-save transactions
or GPU adapters. Native pointers and mutable container interiors continue to
require their caller's lifetime and invalidation contract.

```sh
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
    ctest --test-dir build-sanitize --output-on-failure \
    -R '^(mutation_policy_context_contracts|mutation_policy_reentrant_close|mutation_policy_rust_parity|context_reference_contracts|context_memory_contracts|context_early_global_close|example_context_lifetime)$'
ctest --test-dir build-wndr4300-memory --output-on-failure \
    -R '^(mutation_policy_context_contracts|mutation_policy_reentrant_close|context_reference_contracts|context_memory_contracts|context_early_global_close|example_context_lifetime)$'
```

Build the matching targets before running these selections. MIPS Python parity
comparison additionally needs a wrapper that invokes the cross-built probe
through QEMU; CTest automatically emulates the native C++ test targets.


## Graph-planning integration checkpoint

Public graph planning uses stable User-owned traversal frames, nested query
operations and copied trace links. Global plan copies retain nested values and
trace ancestry after User destruction. Context close cancels outstanding query
work before reclaiming those frames. Metadata owner validity is rechecked after
a pending query. Provider/resource mutation that invalidates an existing native
borrow still requires the caller to cancel and join that work first.

ID-generator registration now stores a checked resource borrow. User-only
`set_internal_id_generator(ref)` supports generators allocated through the
Global proxy. Generation and bootstrap floor reservation reject early-destroyed
generators before virtual dispatch; reentrant close inside synchronous generator
callbacks fails before User storage can be reclaimed.

The rebuilt native suite passes 139/139. ASan/UBSan with leak detection passes
six selected graph-plan/policy-plan/ID checks. MIPS/QEMU passes four native
ownership/ID checks and matches 704 public planner plus 2,129 private policy-plan
observations against live original Rust. This proves planning/ownership, not
transactional graph save. Physical-device execution remains pending.


## Runtime mutation callback integration

Mutation adapters snapshot commands and propagate User allocators through batch
values, optional old snapshots, lineage and events. Explicit Global event copies
survive User destruction. Pending native mutation work forwards wake signals and
is cancelled/joined during Context close before arena reclamation.

Request policies, behavior registries and raw/safe event sinks are registered
as checked resource borrows. Their original managed slot is checked before
virtual dispatch after early destruction. Command preparation and synchronous
event dispatch register teardown guards, rejecting reentrant Context close while
stack-owned result carriers or active callbacks remain in use. Native pointers
and externally invalidated provider borrows retain their caller's contract.

Mutation infrastructure remains separate from transaction and graph-save
completion; sequential batches preserve partial failures instead of implicitly
creating another ownership or transaction layer.


The mutation checkpoint passes native 143/143 after rebuilding all consumers,
ASan/UBSan 4/4 with leak detection, and MIPS/QEMU 3/3 native contracts plus
1,502/1,502 live original-body observations. It includes actual SQLite writes,
stale-version conflict, cancellation, early resource destruction, reentrant
close rejection and retained Global audit copies. Physical-device execution
and graph/ledger transactional save remain pending.

## Current memory acceptance

The selected memory design is implemented in the current working tree. The
following are acceptance requirements, rather than optional optimizations:

- Separate Global and User arenas, object tables, and exact validity bitmaps.
- Explicit User allocation and Global delegation, with checked 64-bit references
  and User-bound access to Global objects.
- Non-reused object and Context identities, bounded object reservations, whole
  backing-block byte accounting, and aggregate live User byte limits.
- Stable string/byte storage without an object identity for every payload;
  bounded, explicit string interning with ordinary-storage fallback.
- Exactly-once resource destruction, stop/join before reclamation, and a
  precharged shutdown reserve that remains usable after normal quota exhaustion.

After rebuilding all native consumers, the full Debug suite passes 145/145.
The latest matching ASan/UBSan and MIPS/QEMU builds each pass five selected
contracts: checked references, arena memory, early Global-close rejection,
runtime resource registration, and the Context lifetime example. ASan/UBSan
also passes the asynchronous stop/join contract, for six selected checks in
total. Leak detection is enabled in the sanitizer runs. The reference test
allocates one million objects and verifies the exact 125,000-byte bitmap on
both targets.

```sh
ctest --test-dir build-wndr4300-memory --output-on-failure \
    -R '^(context_reference_contracts|context_memory_contracts|context_early_global_close|runtime_context_resource_contracts|example_context_lifetime)$'
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
    ctest --test-dir build-sanitize --output-on-failure \
    -R '^(context_reference_contracts|context_memory_contracts|context_early_global_close|runtime_context_resource_contracts|example_context_lifetime)$'
```

These checks establish the managed-memory contract. Native borrows still expire
with their owner; arbitrary raw-pointer dereferences are not intercepted.
Physical-router execution, native-library/device heap accounting, automatic
string admission, and the remaining migration stages are separate work.

### Graph-execution ownership integration

Graph execution and governed service-save operations snapshot plans and nodes
into the invoking User arena. Pending mutation and authoritative root-read work
is cancelled and joined before reclaiming their state. Explicit Global graph
copies retain nested values after User destruction. A real SQLite transaction
test observes trigger-modified root values through its own executor before
rollback.

The execution checkpoint passes two selected ASan/UBSan checks with leak
detection, one MIPS/QEMU ownership/SQLite check, and all 520 original Rust
observations on both native and MIPS. The complete native suite above includes
these checks. This establishes caller-managed graph execution and its memory
contract; ledger execution, typed/erased transactional savers, TransactionScope
and audited typed save entry points remain migration gates.

### Ledger-execution ownership integration

Ledger execution snapshots locations, sparse commands and generated-ID results
into User storage while retaining the source's shared same-User ledger state.
Suspended classification reads and mutations are cancelled and joined on User
destruction. Resumed descriptor access rechecks its metadata owner; ID callbacks
also revalidate metadata before subsequent descriptor use. Explicit Global
generated-ID map copies survive actual User destruction. Default PMR allocation
is disabled in the native ownership tests.

The checkpoint passes native 149/149, two selected ASan/UBSan checks with leak
detection, and one MIPS/QEMU ownership/SQLite check plus 760 live original Rust
observations. Real SQLite verifies guarded updates, soft deletion, generated
insertion and caller-controlled rollback. The executor leaves ledger clearing
and transaction completion to the future saver layer. Mutable container
interiors still require their caller's invalidation contract.

### Transactional saver ownership

The typed saver marker is Context-owned and registered through UserContext.
It owns no executor or Context reference; each operation looks up its checked
typed executor when polled. Node/plan/governance state and returned graphs use
the invoking User arena. Explicit Global result copies survive User destruction.

Cancelled savers stop and join planning, begin, execution or terminal children
before abandoning the acquired transaction. Transaction cancellation also tracks
terminal work, so direct cancellation cannot leave a separate commit/rollback
future running. Native cleanup requires no new managed identity or asynchronous
operation. SQLite releases an active transaction immediately while User remains
open; failed begin-adapter allocation follows the same cleanup path. Failed
commit/rollback consumption does not prevent cleanup of a still-active provider.
Ledger state clears only after a successful commit, and governance activation
is restored after each poll/cancellation.

This ownership checkpoint passes native 151/151, ASan/UBSan 5/5 with leak
detection, MIPS/QEMU 4/4 native contracts, and 960/960 live original saver
observations on both native and MIPS. Tests cover cancellation in begin,
mutation, reread and terminal phases; direct cancellation of an independently
pending commit; commit/rollback failure cleanup; real SQLite connection reuse;
quota-exhausted begin handoff; and Global retention after User destruction.
Physical-device execution and the remaining transaction-scope/audited-entry
migration gates remain unproven.

### Audited entity and pending-ledger results

The audited-with-executor result owns an entity and an optional pending ledger
within the invoking User domain. Typed materialization can restore a runtime
state that aliases that ledger. Whole-result copies and allocator-aware moves
use one synchronous runtime-copy session so explicit Global retention preserves
the same alias in its destination, including cyclic graph state. Session maps
borrow source identities only while copying and are destroyed before returning;
they do not become a persistent Global cache of User pointers.

The entry cancels suspended ledger/graph/read descendants before User storage
is reclaimed. It leaves transaction completion and ledger clearing to its outer
owner. Native contracts cover retained result data, mutation through the retained
entity/ledger alias after actual User destruction, and SQLite outer rollback
that preserves retryable intent. Mutable native interiors retain their caller's
invalidation contract.

The audited-with-executor checkpoint passes native 153/153, ASan/UBSan 2/2
with leak detection, and MIPS/QEMU 1/1 ownership/SQLite contract plus 356/356
live original public Rust observations. Whole-result Global copies preserve
entity/ledger sharing, verified by mutating through the retained ledger and
reading through the entity runtime after actual User destruction. This does
not establish the remaining audited automatic-saver or TransactionScope gates.


### TransactionScope ownership

The complete scope owns the obligation to complete or abandon one native
transaction. Callback operations, scope requests, audited materialization,
flushed-ledger registrations and results remain in User. Registered quiescence
joins pending begin/read/write/terminal work before returning host storage.
Post-BEGIN failure to allocate the scope also abandons the acquired transaction
without requiring another allocation. Successful enclosing commit clears each
retained ledger once; every cancellation or typed failure preserves intent.

The Context-owned one-shot callback state permits mutable callables and is
consumed before invocation, including when a callback throws. Same-domain
adapter copies borrow that state; cross-domain copying is rejected. Captures
follow the existing allocator-aware/non-owning contract. Foreign callback result
futures are rejected without cancelling another Context's work. Native contracts
cover actual User destruction during pending work and explicit Global copies of
committed entity results. Runtime scopes cannot be retained into Global.


TransactionScope acceptance checkpoint on 2026-10-06: the rebuilt native Debug
suite passes 157/157. ASan/UBSan with leak detection passes both scope checks,
including the recorded original corpus. MIPS/QEMU passes the native scope
contract and matches all 1,288 live original public Rust observations. Native
contracts also confirm mutable FnOnce callbacks and cached-result reuse,
post-BEGIN quota failure cleanup, actual User destruction, foreign-future
isolation, real SQLite commit/rollback and failure diagnostics. Locked upstream,
58-module inventory and pinned clang-format 18.1.8 checks pass. This checkpoint
does not establish physical-router execution, final runtime example wiring or
completion of stages 2–6.


Runtime-example acceptance checkpoint on 2026-10-06: the rebuilt native Debug
suite passes 159/159. Four selected ASan/UBSan checks pass with leak detection:
both executable examples, the original business-result comparison, and the
Context ownership/provenance/graph-rollback contract. OpenWrt MIPS/QEMU passes
three native checks and both live original scenario comparisons. Eight CRUD
and 19 relationship-flow SQL operation/purpose/audit entries match the recorded
Rust oracle. Both original binaries run unchanged from the clean pinned
reference. clang-format 18.1.8, locked upstream and 58-module inventory checks
pass. Physical-device execution, full API/reexport/privacy/log acceptance,
missing runtime abstractions and complete source-test audits remain open.


### Lock ownership and teardown

The local lock directory belongs to Global, including copied keys and scalar
owner identities. It retains no User arena, even when the source-compatible
non-expiring lease outlives its User. Directory growth obeys the Global byte
budget; unlocking does not require another allocation. Distinct explicit Globals
use distinct application directories; no process singleton or default allocator
owns Context key data. Owner identities do not recycle after User destruction.

Remote interfaces and implementations use explicit resource registration. An
optional provider is selected at first poll, checked before virtual use, and
borrowed only within the invoking User. Request keys and owner tokens remain
alive through pending work. User quiescence cancels/joins descendants before
reclamation; a Global provider remains owned by Global. Closing User cancels work
without inventing a remote unlock or extending resource lifetime implicitly.


Context-locking acceptance checkpoint on 2026-10-06: the rebuilt native Debug
suite passes 161/161. ASan/UBSan with leak detection passes both locking checks.
OpenWrt MIPS/QEMU passes the native ownership/expiry/quota contract and matches
all 222 live original Rust scripts; the native probe matches the same corpus.
Pinned Rust v5.0.5, 58-module inventory and clang-format 18.1.8 checks pass.
These checks cover intra-application locking and Context resource lifetimes;
they do not establish process-wide sharing between independent Globals,
concurrent User access, a remote/cloud service or physical-router execution.
Complete API/privacy/source-test and remaining runtime-abstraction gates stay
open under the unchanged six-stage goal.

### In-memory query ownership

The in-memory query engine accepts an explicit destination domain independently
of its input query and rows. Group keys, index vectors, merge scratch, expressions,
output rows and execution metadata propagate that destination allocator. The
consuming overload transfers same-domain data and copies foreign-domain rows
before processing; the borrowed-input overload copies into the destination.
There is no default-PMR or process-owned query scratch. Group and projection
maps own nested output values instead of borrowing temporary keys or User layouts.

Native contracts run with the default PMR resource disabled, then actually
destroy User and resolve retained Global query rows and nested expression values
through a new User. Explicit Global execution and explicit Global result copying
are tested separately. Pure row execution needs no asynchronous registration or
native-resource cleanup; its temporary PMR containers must still end before their
owner is closed. This adds no concurrent-reader or escaped-native-pointer safety.

### Portable-reference ownership

Portable entity tokens remain separate from the eight-byte process-local object
IDs. UserContext delegates codec installation and encode/decode through checked
resource registration. Key tables, callback state, token strings, claims and
protocol buffers use explicit owners. Retained keys and expanded AES schedules
are wiped before destruction; plaintext scratch is wiped on normal and exceptional
exits. Authentication failure exposes no plaintext allocation or borrow.

Native contracts actually destroy User before resolving retained Global claims,
keys and allocator-aware clock/nonce callbacks through a later User. Expired
resource lookup rejects before virtual dispatch. Synchronous guards keep owner
and destination alive through callbacks and fail fast on reentrant close.
Captured state still follows the established explicit-copy contract. These
contracts do not implement the separate document/principal authorization layer
or promise cryptographic certification or physical-device execution.

### Context-bound reference ownership

The context-bound runtime keeps separate checked key and authorization providers,
scoped clock/nonce functions and explicit service/environment strings. Global
copying rebinds already Global-owned providers and copies allocator-aware callback
state; retaining User-owned providers into Global is rejected. Trusted principal
installation makes a User-owned copy rather than retaining caller strings.
Governed/raw wire values and resolved identity/key/time metadata use the requested
output domain. Native contracts destroy the original User before rebinding a
retained Global runtime and reading retained nested claims. Expired providers
reject lookup before their virtual methods; synchronous guards also cover the
function-policy adapter's owner/output reentrant-close paths.

HMAC/HKDF use fixed primitive state, with key-bearing HMAC state and derived/retained
key material wiped before destruction. Plaintext protocol scratch is wiped on
success and exception. These are scoped memory guarantees, not promises about all
compiler temporaries, foreign heaps or device memory. Provider, policy, wire and
deployment responsibilities stay separate, as documented in the
[round-trip contract](round-trip-reference.md).
