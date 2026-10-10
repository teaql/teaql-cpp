# Runtime DataStore contract

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-runtime/src/context.rs`. The asynchronous `DataStore` SPI and
`InMemoryDataStore` are separate from Context locals, aggregation caches,
continuous-page cursor stores, and ID-set stores. No external cache or service
dependency is introduced.

## Source behavior

`get` returns an optional copy. A stored `Value::Null` is `Some(Null)`, distinct
from a missing key. `put` replaces the value and its optional seconds-based
expiry. Expiry uses a monotonic clock, with the source's strict `now > deadline`
comparison. Expired entries remain allocated until replacement, removal, or
owner teardown. `remove` succeeds for both present and absent keys.

The source computes the expiry while holding its write lock. An overflowing
`Instant + Duration` panics and poisons that lock; subsequent get, put, and
remove calls also panic. The C++ counterpart throws `std::out_of_range` for
deadline overflow and `std::logic_error` for subsequent poisoned-store access.
It does not silently wrap, turn overflow into a permanent value, or recover
the original poisoned store. A C++ allocation failure is a separate ownership
adaptation and is not treated as an original Rust timeout panic.

UserContext preserves three separate entry points:

| Entry | Missing registered DataStore |
| --- | --- |
| `get_in_store(key)` | A ready empty optional. |
| `put_in_store(key, value, timeout_seconds)` | A successful no-op. |
| `clear_in_store(key)` | A successful no-op. |

Resource selection occurs at the first poll, matching execution of the original
async method. Pending operations snapshot their keys and values into their
User's domain. Cancelling a put before polling does not write.

## C++ ownership and registration

Allocate the concrete store in Global or User explicitly and register its
`DataStore` interface, as the source registers `Box<dyn DataStore>`:

```cpp
auto& store = global.make_resource<teaql::InMemoryDataStore>();
user.insert_resource(global, static_cast<teaql::DataStore&>(store));
user.put_in_store("key", teaql::Value(user.memory_resource(), "value"),
                  std::nullopt).wait();
const auto& value = user.get_in_store("key").wait();
```

Bootstrap owns Global; application functions only receive UserContext and use
its store methods. Registration is a checked borrow and adds no shared owner.
Business code can allocate and install a Global store entirely through User:

```cpp
auto store = user.make_global_resource<teaql::InMemoryDataStore>();
user.set_data_store(store);
user.put_in_store("key", "value", std::nullopt).wait();
```

`set_data_store` registers the base SPI from the checked concrete reference,
without exposing Global or treating a derived reference as an unrelated ID.
Keys, stored values, nested containers, and their characters belong to the
store's arena. `get` copies into the requested operation domain. Thus a Global
store can survive actual User deletion without retaining the original User
arena. Its returned results remain User borrows. Allocator-aware store copying
explicitly clones its entries and deadlines into the destination domain.

The source's RwLock and Send/Sync permit shared concurrent calls. The C++ runtime
currently requires owner-thread allocation, polling, mutation and close; this
store does not independently make Context access thread-safe. The fixed
seconds/nanoseconds deadline carrier is sampled from `steady_clock`, never the
business clock or wall clock. Foreign-domain child operations are rejected
without cancelling work owned by another Context. Registered child operations
are cancelled before Context memory is returned.

## Acceptance

The public oracle invokes the unchanged original implementation and Context
methods through a forwarding fixture. Its 251 committed scripts cover every
Value variant, nested payloads, empty/Unicode/embedded-zero keys, missing
registration, shared User access, User reconstruction, replacement, repeated
remove, finite and zero TTL, deadline overflow and lock poisoning. Both Rust
Debug and JSON projections distinguish returned values. These observations
establish this store contract, not complete runtime parity.

The native lifetime tests disable the default PMR allocator, mutate input keys
and nested values after creating a future, actually delete the source User,
verify retained Global data through a new User, reject early-destroyed stores
before virtual dispatch, and verify lazy resource selection and cancellation.
Stack PMR containers are destroyed before their allocating Context closes.

On 2026-10-07, the final native selection passed 4/4 tests: the store contracts,
251-script differential corpus, generated-support contracts and an existing
locking comparison regression for the shared comparison harness. ASan/UBSan
with leak detection passed 2/2 lifetime selections. The OpenWrt MIPS build
passed 3/3 selections under QEMU, including the 251-script corpus; a separate
live Rust/MIPS comparison also passed 251/251. The Python comparison harness
accepts an explicit CMake emulator command, so its CTest entry does not attempt
to execute a foreign binary directly. Physical-device execution remains
unverified. Pinned source, inventory, root reexport names and formatting checks
passed. This store checkpoint does not establish the wider migration. Later
facet acceptance is recorded in [the runtime-support contract](generated-runtime-support.md).

```sh
cmake --build build --target teaql_data_store_tests teaql_data_store_probe
ctest --test-dir build --output-on-failure -R '^data_store_(contracts|rust_parity)$'
python3 tests/parity/compare_data_store.py --cpp build/teaql_data_store_probe \
  --rust /home/philip/shared-cargo-target/debug/data_store_oracle
```

The three umbrella headers also compile together in both include orders in the
store and generated-support tests. Core's annotated-input `EntityGraphBuilder`
and runtime's identity `EntityGraphBuilder` remain separate types in
`teaql::core` and `teaql::runtime`. The legacy flat core spelling is
`CoreEntityGraphBuilder`; the flat runtime spelling remains `EntityGraphBuilder`.
Runtime's schema provider also remains distinct from the data-service metadata
SPI. Combined-header compilation alone does not prove every public export's
behavior or complete the API audit.

`python3 scripts/verify-public-exports.py` checks all root `pub use` statements
in the entire locked source file, including the checker block after the test
module and the generated-support wildcard. It verifies 69 core, 22 SQL and
188 runtime reexport names. Only the explicitly excluded OpenTelemetry adapter
is omitted. This is a declaration omission check; it does not establish macros,
members, generic instantiation, defaults, serialization, privacy or behavior.

## Foreign child cleanup

A Context adapter rejects a provider future in another allocation domain. It
cancels that returned work before discarding the pointer, while the borrowed
request key remains live. The controlled foreign-work contract checks exactly-once
cancellation and no deferred access after actual User deletion. This rejection
does not change the original ordinary store/TTL/no-op behavior.
