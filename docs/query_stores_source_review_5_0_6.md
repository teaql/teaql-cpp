# Aggregation cache and pagination source review

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This reviews the complete `data_service/cache.rs` (89 lines) and
`context/pagination.rs` (252 lines). Both files are unchanged from the previous
reference, but their historical 5.0.5 acceptance is not current integration
evidence. These are internal query-optimization protocols; no external cache
component or dependency is added.

Status: both complete source files are accepted under QS01-QS16 after durable
source reproduction, current native/imported gates and the actual Context
supplement below. This checkpoint advances source dispositions to **30 accepted /
92 pending / 2 excluded_external**. Wider caller/capability and P0-P7 acceptance
remain separate; the earlier frozen-epoch evidence is preserved.

## Complete responsibility mapping

| ID | Original responsibility | Current C++ implementation and acceptance boundary |
| --- | --- | --- |
| QS01 | `AggregationCacheBackend` namespace, get, put and invalidation protocol | Independent virtual SPI in `cache.hpp`; `namespace` becomes `name_space` because the original spelling is a C++ keyword. Reads and writes explicitly copy into their selected owning domains. |
| QS02 | Cache factories, default, inherent forwarding and clear | Explicit allocator-aware construction preserves empty or supplied namespace and an empty store. Concrete methods implement the same backend behavior. Clear removes logical entries without promising individual arena reclamation. |
| QS03 | Stored entry, monotonic age and replacement | `query_stores.cpp` uses the actual steady-clock timestamp. Zero maximum age is unlimited; the age boundary is inclusive; future timestamps remain fresh. Expired reads leave the entry present. Row ownership copying finishes before a replacement timestamp starts. |
| QS04 | Cache invalidation and clone/diagnostics | Invalidation matches the complete `namespace + "::"` prefix. Reads copy complete CompactRows. `query_store_debug.hpp` preserves complete entry fields and real timestamps; nondeterministic multi-entry HashMap ordering is not a wire protocol. |
| QS05 | `ContinuousPageCursor` model and derives | All eight fields, explicit-destination cloning, complete Debug modes and fieldwise equality remain distinct. Value NULL variants and NaN equality retain their existing source contracts. Construction defaults are C++ allocation conveniences, not an invented Rust Default implementation. |
| QS06 | Separate asynchronous cursor-store SPI | Get, put and invalidate retain their own result/error protocol. Operation state snapshots inputs into its Context and borrows the selected store. Unpolled cancellation performs no store mutation. |
| QS07 | Cursor keys, expiration and capacity | Keys concatenate query key, colon and unsigned decimal offset. Get removes only an expired requested entry when `expires_at <= now`. Default capacity is 4,096. At capacity, put evicts a minimum-expiry entry before insertion, including replacement. Equal-deadline tie order is unspecified by the original HashMap. |
| QS08 | Cursor invalidation | Match the entire `query_key + ":"` prefix, including the original behavior when query keys themselves contain colons. Do not reinterpret it as structured query-key equality. |
| QS09 | `RetainedIdSet` model and derives | Preserve query key, complete ordered u64 vector and expiration. Explicit-destination deep copies replace immutable Arc sharing without extending the source Context. Debug and Clone responsibilities do not imply a source PartialEq implementation. |
| QS10 | Separate asynchronous ID-set SPI and defaults | Independent get, put and exact-key invalidate. Defaults are 64 entries and 256 MiB of logical ID-vector bytes, not a budget for every allocator or native-library allocation. |
| QS11 | ID-set put ordering and eviction | Saturating incoming-vector bytes are checked before expiration pruning. Accepted puts prune expired entries, then evict earliest deadlines until count and combined logical bytes permit insertion. Existing-key replacement still participates in the original pre-insertion accounting. Zero capacity retains the source's empty-map escape and insertion behavior. |
| QS12 | ID-set get and invalidation | Get removes the requested expired value; invalidation removes exactly its key, unlike cursor-prefix and cache-namespace invalidation. Results use explicit destination storage. |
| QS13 | Context stores and observation methods | Independent cursor and ID-set registration, checked dispatch and copied observations in `context/pagination.cpp`. Initial observations remain `DISABLED`/None and `ID_SET_DISABLED`/None. Replacing a registered store does not redirect an already-created operation. The actual unchanged Rust Context now executes all 36 retained observation inputs and a store registration/replacement/default-capacity/lock-identity contract; the reduced shell is no longer the only source evidence. |
| QS14 | Private same-key build-lock directory | The process-static Weak directory becomes an explicit Global-owned directory accessed through UserContext. Same live key shares a logical mutex; distinct applications remain separate. Dead logical leases prune at the next lookup. Copied keys and leases do not own UserContext memory. |
| QS15 | Actual async mutex behavior and shutdown | Registry/waiter/guard code preserves pending polls, slot reuse, wake transfer after cancellation, overtaking and lack of FIFO fairness. Registered native work must join before guard release and User arena reclamation. |
| QS16 | Complete ownership and threading adaptation | Nested rows, strings, cursor boundaries and ID vectors propagate destination allocators. Store/result borrows reject invalid domains. Explicit copies survive actual User deletion; default PMR allocation is disabled by the complete lifetime contracts. Source Send/Sync/Mutex poisoning is adapted to the approved Context owner-thread profile, not claimed as unrestricted C++ concurrent access. |

The implementations remain in their independent runtime protocols. This review
does not substitute store correctness for complete entity-service miss/lock/
recheck/build behavior or for generated pagination execution. Those callers
retain their own acceptance obligations.

## Current source replay and dependency fidelity

The first replay attempt rejected the existing observer workspaces before
compilation: their registry identities differed from the frozen reference lock.
The optimization workspace contained 45 differing name/version/source/checksum
tuples and the broad field-layout workspace contained 46. This is a test
dependency-fidelity gap, not an observed runtime difference. The
[initial rejection](parity/query_stores_source_initial_dependency_rejection_5_0_6.json.gz)
preserves the differences and terminal validation failure. Existing workspaces,
their historical results and the running native epoch remain unchanged.

The continuation uses a temporary minimal observer workspace seeded from the
actual frozen Cargo.lock. It validates registry identities before compilation
and builds offline with `--locked`. All three original observer bodies are used
unchanged. The existing extraction build script changes only its source-root
locator; the compiled cache file, pagination stores, Context observation methods
and build-lock function remain exact original source. Store observations use
private-field bridges for deadlines, capacities and key inspection. The Context
observation fixture is an explicit reduced shell initialized from real Context
defaults; it is not claimed to instantiate the complete Context implementation.
The model observer uses actual exported runtime types directly.

The same historical inputs are compared against fresh original and imported C++
outputs without normalization. Canonical module preparation preserves the
entire bodies of the three existing store, model and build-lock tests and their
three probes. It uses a temporary configuration in memory, not an edit to the
committed module registry. The
[replay receipt](parity/query_stores_source_replay_5_0_6.json.gz)
records terminal commands, full results, source hashes, temporary configuration,
prepared consumers and the reproducer. Its `complete` field distinguishes an
ongoing replay from success.

The replay is now terminal and successful: **259 store + 144 build-lock + 74
model observations = 477/477** complete outcomes match both the current actual
original and imported C++ consumers. All three complete imported test bodies
pass. Runtime sources, original observers, historical corpora and committed
module configuration are unchanged. This adds current reference-locked evidence
without converting the earlier dependency-drifted executions into such evidence.

The model contracts check populated-cache timestamps against real before/after
clock brackets. They do not replace live Instants with constants. Exact empty-map
Debug comparisons, sorted fixture key observations and real clock brackets
serve different obligations and must not be conflated.

## Remaining integration work

At the temporary checkpoint source dispositions stayed pending while the complete
native epoch froze execution inputs. After its terminal result, preserve that epoch, install a
durable reference-locked observer/reproducer, register the complete canonical
module contracts and review current native outcomes before promoting either
source disposition. The temporary imported tests establish their executed scope
only. They do not establish full G09/G13/P0-P7, sanitizer, MIPS or physical-device
acceptance. Historical full-suite and cross-profile results retain their dates
and original scope.

## Installed source qualification

The three full existing C++ store/model/build-lock bodies and their three probes
are now permanently registered as canonical imported consumers and included in
the module preset. All implementation and assertion bodies remain unchanged.
The [installed gates](parity/query_stores_installed_gates_5_0_6.json.gz) pass
**native 6/6** and **imported 3/3**; every compiler/linker exits successfully
before its executable is used. The existing complete lifetime contracts retain
explicit domain copies, actual User deletion and native cancellation/join tests.
There are no production C++ or generated SDK changes.

The [dedicated reference project](../tests/parity/rust_query_stores_source_5_0_6/Cargo.toml)
uses repository-relative paths to the same original observers and the original
extraction build script. Its location has the same relative source root, so the
build script itself needs no patch. Exact original registry versions/checksums,
observer/helper/corpus hashes and package versions are validated before its
locked offline build. The previous dependency-drifted workspaces are unchanged.
All **477/477 outcomes per frontend** match fresh original execution and the
existing expectations in the
[durable replay](parity/query_stores_installed_replay_5_0_6.json.gz).

The same replay adds two tests to a disposable byte-checked copy of the actual
complete runtime. Every original source, test and test-vector body is retained;
only the [observer module](../tests/parity/query_stores_context_source_5_0_6.rs)
is appended to context/pagination.rs. The
[source-view helper](../scripts/reference_runtime_view_5_0_6.py) preserves normal
dependency features, omits only disabled external-exporter test dependencies,
checks the entire copy and validates original registry/package identities before
compilation. Neither Context, store, mutex nor observation logic is replaced.

The first test executes the **36 existing Context observation inputs** through
real UserContext methods and reproduces their full expected output. Those inputs
are a subset of 259 and are not counted again in 477. The second test checks
actual default capacities, initial observations, cursor/ID-set registration and
replacement, data retained by the earlier stores and Context build-lock identity.
Both pass. The earlier shell remains accurately labeled as a reduced observer;
it is not relabeled as actual complete Context execution.

This accepts the complete cache and pagination source contracts, including the
explicit QS14-QS16 C++ ownership/threading adaptations. It does not accept the
whole entity-service miss/lock/recheck/build path, all generated pagination or
G08/G13/P0-P7. No new full suite, sanitizer, MIPS, external-cache service or
physical-device result follows from this source qualification.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_query_store_tests \
    teaql_query_store_model_tests teaql_id_set_build_tests \
    teaql_query_store_probe teaql_query_store_model_probe teaql_id_set_build_probe
cmake --build --preset modules_native --target teaql_module_query_store_tests \
    teaql_module_query_store_model_tests teaql_module_id_set_build_tests \
    teaql_module_query_store_probe teaql_module_query_store_model_probe \
    teaql_module_id_set_build_probe
ctest --preset native -R '^(query_store|query_store_model|id_set_build)_(contracts|rust_parity)$'
ctest --preset modules_native -R '^module_(query_store|query_store_model|id_set_build)_contracts$'
python3 scripts/verify_query_stores_source_5_0_6.py \
    --module-build-dir build_modules_native --record /tmp/query_stores_source_fresh.json.gz
```

The [terminal acceptance receipt](parity/query_stores_source_acceptance_5_0_6.json.gz)
joins the complete source mapping, current installed gates, actual Context
supplement, immutable observers/corpora and preserved before-state. Broader
capability statuses remain unchanged.
