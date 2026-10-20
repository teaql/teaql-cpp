# Shared SQLite statement caching for dynamic storage

Dynamic field writes and loads now borrow statements from the same connection-owned
cache as ordinary native SQL. The locked Rust provider uses prepare_cached for its
UPSERT/DELETE pair and owner-ID read statements; the previous C++ codec prepared
and finalized each statement independently. No generator or runtime/data-service
algorithm is duplicated or merged into the provider.

## Borrowing and source ordering

A provider-private statement borrowing bridge returns a move-only native RAII
handle. Its destructor resets the statement, clears bindings and returns it to the
connection cache before the operation releases its lease. Statement keys and cache
slots belong to the connection's explicit Context; User-owned parameter buffers
are not retained. Schema initialization keeps the original execute-batch path.

Batch validation still finishes before SQL preparation. Both write statements are
prepared before the first mutation, preserving source failure precedence. Loading
retains the existing 400-ID chunk size, selection/type/revision checks, storage
encoding and masked DynamicFieldError conversion. The connection lease, transaction
binding and async finish/cancel behavior remain responsible for exclusivity.

Shared caching exposed a previously latent slot bug: a checked-out statement has
a null idle pointer, but that does not make its slot available. Slots now separately
track borrowing. Preparing DELETE while UPSERT is checked out cannot overwrite the
UPSERT key. Returning duplicate-key checkouts retains one idle map entry. When all
fixed slots are borrowed, a transient statement is reclaimed without touching an
active slot. The existing fixed capacity and owner-thread model remain unchanged.

## Evidence and actual Context cleanup

The [original source review](parity/sqlite_dynamic_statement_cache_5_0_6_source_review.json)
records cached preparation responsibilities. All **123/123** unchanged original
Rust/native storage observations match live replay, including exact errors,
full-width values, negative zero, NULL/Delete and transactional rollback.

Native tests verify two distinct retained write statements, repeat-read reuse,
cleared binding behavior, ambient/native lease isolation, ordinary SQL cache and
cursor regressions, and generated model dynamic save/load. A Global-owned connection
and provider cache survive actual destruction of a temporary User after commit;
all retained native statements are idle and a new User reads their persisted data.
The storage contract executable disables the default PMR resource explicitly.
A native authorizer rejects DELETE during preparation: the test checks safe error
projection, absence of earlier UPSERT mutations, return of the already-prepared
statement and successful retry after removing the denial. This additional control
verifies original source ordering; it is not counted among live Rust observations.

The first module build referred to a test target that had not been registered;
canonical storage/transport consumers were then explicitly added to the module
manifest. The first registered build also found the test-local `sql` helper
ambiguous with exported `teaql::sql`; renaming it to `fixture_sql` preserves every
assertion. No failed build is counted as execution evidence.

## Terminal verification

The [verification record](parity/sqlite_dynamic_statement_cache_5_0_6.json) retains
actual CTest output and source hashes. Relevant native checks pass **4/4**; the
same four ASan/UBSan checks pass with leak scanning disabled. Import-only module
checks pass **3/3**, and all **134** canonical consumer bodies verify. Pinned
formatting and its check pass for **1014** handwritten C++ files. No target or
physical-device run was performed.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build build_native_current --target \
  teaql_sqlite_dynamic_storage_tests teaql_sqlite_dynamic_storage_probe \
  teaql_generated_dynamic_field_tests teaql_provider_sqlite_transport_tests
python3 tests/parity/compare_sqlite_dynamic_storage.py \
  --cpp build_native_current/teaql_sqlite_dynamic_storage_probe \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/sqlite_dynamic_storage_5_0_6
ctest --test-dir build_native_current --output-on-failure \
  -R '^(sqlite_dynamic_storage_(contracts|rust_parity)|generated_dynamic_field_contracts|provider_sqlite_transport_contracts)$'
```

Protected dynamic audit/log, whole-model/provider/async configuration and complete
capabilities G01-G17/P0-P7 remain separate acceptance work. Shared caching does not
authorize dynamic writes or extend Context lifetime. No new dependency was added.
