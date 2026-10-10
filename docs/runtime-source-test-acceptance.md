# Runtime source-test acceptance

## Resumed graph-reference lifetime verification, 2026-10-08

Both original Arc test bodies and the C++ graph_reference_lifetime.cpp fixture
are reviewed again, including the production promotion/preservation/publication
paths. The adaptation preserves reference authority and identity and explicitly
changes last-Arc-drop reclamation to Context close. Both unchanged original Rust
tests pass individually. Native, ASan/UBSan with leak detection, and MIPS/QEMU
selections each pass 5/5 in 1.15, 2.41 and 1.65 seconds. These checks include
original_runtime_graph_reference_contracts, entity_runtime_contracts,
context_reference_contracts and the two core input-graph checks. Source-test
adaptation is accepted; complete production runtime/stage parity stays open.

## Graph-reference source adaptation checkpoint, 2026-10-07

The final two indexed entity-runtime source bodies have reviewed Context-
lifetime adaptation dispositions. See [the lifetime contract](context-lifetime.md#graph-reference-lifetime-adaptation-acceptance)
for the exact ownership difference. Their pinned Owner#1/children/GraphChild
shape is exercised in `graph_reference_lifetime.cpp`, registered as
`original_runtime_graph_reference_contracts`.

- Weak child publication is rejected before freeze and returns an intact builder.
- Actual allocator-aware GraphChild copying promotes a weak unfrozen anchor:
  the cloned child publishes entity 42, and original/weak views resolve exactly
  the same payload address. This observes authority and anchor preservation.
- Frozen detached copying preserves the same relation-list identity after the
  root view is destroyed. Same-domain checked access expires at User close.
- Internal graph back-references do not retain any User blocks after close or
  actual deletion; each graph payload is destroyed exactly once.
- Explicit Global deep copying preserves the cyclic relation identity and
  allocators of both list and child after the source User is actually deleted.
  Releasing the detached retained view leaves precisely the Global graph child;
  Global teardown destroys every constructed payload exactly once.

Rust's two unchanged Arc tests pass. The C++ tests deliberately do not claim
that dropping the last view immediately frees a graph: its explicit Context is
the owner and reclamation boundary. This is the agreed ownership adaptation,
not a reduction of identity graphs, weak roles or clone promotion. Default PMR
is disabled; no shared Context ownership or production dependency is added.
The direct native group passes. The complete native Debug regression passes
276/276 in 60.80 seconds with four CTest workers. Pinned-source, inventory,
strict registered source dispositions, root exports, compiled layers,
555-file clang-format 18.1.8 and whitespace checks pass separately.
Rebuilt source/entity-runtime/checked-reference
selections pass 3/3 under ASan/UBSan with leak detection (1.70 seconds) and
OpenWrt MIPS/QEMU (1.04 seconds).

The ledger now records 441 covered, zero pending, five backend and five external
exclusions. Its strict registration/disposition gate is only a source-test
omission gate. Complete public APIs, traits/defaults, serializer protocols,
privacy/provenance, lifetime composition, target execution and six-stage
acceptance remain separate requirements; their completion is unproven.

```sh
cmake --build build --target teaql_original_runtime_graph_reference_tests \
  teaql_entity_runtime_tests teaql_context_reference_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_graph_reference_contracts|entity_runtime_contracts|context_reference_contracts)$'
```


## Runtime-root memory and Context-tail checkpoint, 2026-10-07

All 75 indexed `teaql-runtime/src/lib.rs` test bodies now have individually
reviewed dispositions. Fourteen further bodies are exercised by three checks:
`original_runtime_memory_service_contracts`,
`original_runtime_context_tail_contracts`, and
`original_runtime_optimistic_conflict_output_contracts`.

The nine memory-service roots replay the pinned **cfg(test)-only** helper.
`memory_data_service.hpp` remains test support, not a production database or
complete helper API port. Count/Sum retain private U64 results; stddev retains
exact Decimal JSON `1.4142135623730951454746218583`, var_pop is Decimal ONE and
bit_or is I64 3. Private Soundex retains Robert/Rupert and excludes Ashcraft.
Ordinary comparisons/string predicates delegate to production ExprEvaluator;
non-aggregate ordering/projection/slicing delegates to InMemoryQueryEngine.
The published engine's I64 Count, F64 Sum and unsupported extended functions
remain unchanged. Metadata relation grouping retains parent IDs 1/2 and U64
counts 2/1. Typed filtering retains ID 3/version 1/gamma. Mutation replay retains
submitted/version 2, typed stale-version conflict, queryable soft-delete -3 and
recovery 4. These operations do not claim graph-save behavior. Wider helper
numeric ranges, textual Min/Max, grouping keys and poisoned-lock behavior are
not established by this selected fixture.

The four Context-tail bodies use actual UserContext schema, identity and lock
entry points. Missing schema returns the exact Schema error/message. USER then
USERNAME then main, PID/thread shape and all original explicit/optional setter
and builder values are checked. C++ native thread identity explicitly adapts
Rust's process-local ThreadId; expected C++ identity is independently constructed.
Two Users share one explicit Global local-lock directory: timeout zero, lease
50ms, actual wait 60ms, wrong-owner unlock and reacquisition retain every original
decision. The test remote provider has explicit Global ownership instead of Arc;
actual async delegation retains the original seven ownership decisions and
absent-provider success. No concrete cloud provider is introduced. Generated
isolated lock keys replace source wall-clock debug keys without changing leases.

The final diagnostic body launches a separate process that executes only the
mutation/optimistic-lock body. It verifies successful exit, `running 1 test` on
stdout, and absence of `OptimisticLockConflict in memory.rs` on both stdout and
stderr. The harness forwards the configured QEMU prefix on MIPS. It does not
substitute an in-process assertion for the source output-capture contract.

Rebuilt three-check selections pass 3/3 on native (0.15 seconds), ASan/UBSan
with leak detection (0.18 seconds), and OpenWrt MIPS/QEMU (0.37 seconds).
The complete native Debug regression passes 275/275 in 66.76 seconds with four
CTest workers, including all three new checks. Pinned-source, registered ledger,
inventory, public-export, compiled-layer, 554-file clang-format 18.1.8 and
whitespace checks pass separately. Default PMR is disabled inside the C++ fixture bodies. Unchanged original Rust
runtime passes 224 tests and ignores its subprocess-only file-log child in the
parent invocation. At that checkpoint the source ledger had 439 covered, two pending, five backend
and five external exclusions. The remaining source cases concern graph-owned
weak references and clone promotion; Context-close evidence alone cannot prove
those contracts. Complete API/default/privacy/composition/lifecycle and all
six-stage acceptance remain open. The generator and production dependencies
are unchanged. Physical-router execution remains unverified.

```sh
cmake --build build --target teaql_original_runtime_memory_service_tests \
  teaql_original_runtime_context_tail_tests
ctest --test-dir build --output-on-failure \
  -R '^original_runtime_(memory_service|context_tail|optimistic_conflict_output)_contracts$'
```

Use the sanitizer or configured MIPS build directories for the same selection.


## Runtime-root pagination checkpoint, 2026-10-07

Ten further `teaql-runtime/src/lib.rs` tests have direct ports in
`tests/parity/teaql-runtime/pagination.cpp`, registered as
`original_runtime_pagination_contracts`. Original principals, namespaces, ordered
rows, TTLs, query shapes, diagnostic strings and optional counts were reviewed.
The actual EntityRowsOperation/EnhancedEntitiesOperation and pagination stores,
registry, lock and recheck paths perform the behavior. Default PMR is disabled.

- Reusable capture rows IDs 100 down to 91, version 1 and `order-ID` names retain
  descending id pages (0,10)/(10,10), principal `tenant-1:user-1`, namespace
  `recent-orders` and TTL 60. Plans are exactly `OFFSET_FALLBACK:FIRST_PAGE` then
  `CURSOR_SEEK`; a cursor ID exists, second offset is zero, and its predicate
  retains `Lt` with U64 boundary 91.
- ID rows 1 through 100 and typed batches 21..30/51..60 retain the original
  `id-set-test:tenant-1:user-1`, `orders`, TTL 60/max IDs 1000, name ordering and
  pages (20,10)/(50,10). Both exact totals are 100, first row IDs are 21/51,
  and plans are BUILD/HIT. Exactly three queries run: ID-only projection, limit
  1001 and final id tie-breaker, then zero-offset page predicates containing the
  original U64 21/51 witnesses.
- Four candidate IDs with maximum three return no false exact total, the exact
  limit-exceeded fallback and diagnostic candidate count 4. An empty retained
  set separately returns an empty result with present exact total zero and BUILD.
- Two UserContexts sharing one explicit Global store and counter have the same
  `id-set-single-flight-user` and source query. Both real enhanced operations
  are polled pending while the first ID query is active; both return total 2 and
  exactly one SeqCst ID-only query runs. No test-local coalescer replaces the
  actual registry/lock/recheck sequence.
- The original 25ms native delay touches only the state-owned atomic and
  WakeSignal off the owner thread. Finish/cancel joins that work before its
  referenced host storage can disappear. Explicit Global store/counter ownership
  replaces Arc sharing; Context allocation and polling remain owner-thread work.
- With TTL one second, the original actual 1050ms wait expires the retained set:
  a second ID build runs and the final plan is BUILD. Two distinct original
  principals sharing the same store separately produce two ID builds.
- Unavailable store get/put/invalidate preserve the source `unavailable` error.
  Fallback returns Order 7 unchanged with no total and the exact store-unavailable
  diagnostic. Expression ordering by Column(name) likewise preserves Order 9
  and the exact unsupported-shape diagnostic.
- Retained IDs [1,2] with only entity 2 returned keep total 2 and one row/first
  ID 2. Missing entities do not shift or reduce the retained ID window.

The direct native group passes all ten bodies in 1.19 seconds. All ten source
names pass unchanged Rust; the original runtime reports 224 passed and its one
subprocess file-log child fixture ignored in the parent. At that checkpoint the ledger was 425
covered, 16 pending, five backend and five external exclusions. Source review
remaining is 14 runtime roots and two graph-reference lifetime adaptations.
Complete API/default/privacy/composition/lifecycle and six-stage acceptance
remain open; the generator and production dependencies are unchanged.

The completed native Debug regression passes 272/272 in 59.85 seconds after
rebuilding all targets. It includes the new source group and existing ID-page,
build-lock and query-store ownership contracts. Rebuilt selections pass 4/4 under
ASan/UBSan with leak detection and 4/4 under OpenWrt MIPS/QEMU, retaining the
actual delayed native work and expiry wait. Pinned-source, inventory, registered
ledger, root-export locators, compiled-layer, 551-file clang-format 18.1.8 and
whitespace checks pass separately. The runtime archive has no SQLite C API calls.
Physical-router and complete stage parity remain unverified.

Use `build-sanitize` with `ASAN_OPTIONS=detect_leaks=1` and
`UBSAN_OPTIONS=halt_on_error=1`, or `build-wndr4300-memory` with its configured
QEMU emulator, for the selected checks below.

```sh
cmake --build build --target teaql_original_runtime_pagination_tests \
  teaql_id_set_page_tests teaql_id_set_build_tests teaql_query_store_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_pagination_contracts|id_set_page_contracts|id_set_build_contracts|query_store_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib --locked --offline
```

## Runtime-root aggregate/cache checkpoint, 2026-10-07

Six more `teaql-runtime/src/lib.rs` tests have direct ports in
`tests/parity/teaql-runtime/aggregates_cache.cpp`, registered as
`original_runtime_aggregate_cache_contracts`. `queue_fixture.hpp` shares the
controlled source queue/capture modes with the preceding typed-query ports.
Original bodies, private dynamic mapping, Value variants, queries and TTLs were
reviewed. Default PMR is disabled; no generator or production dependency changed.

- OrderAggregate/orders maps U64 ID 1 and retains unknown dynamic properties
  I64 `lineCount = 3` and F64 `amount = 18.5`. Actual basic-list querying returns
  one model and its complete flattened JSON equals the original
  `{"id":1,"lineCount":3,"amount":18.5}`. Handwritten mapping does not implement
  the aggregate runtime or substitute a serialized observation for Value checks.
- Original parent IDs 1/2, version 1, names `first`/`second`, then aggregate
  order_id U64 1/lineCount I64 3 pass through actual EntityRowsOperation. The first
  result is exact I64 3; the unmatched parent's count is exact U64 0. Exactly two
  provider queries run and the second targets OrderLine, preserving the original
  queue's synthetic SQL-string assertion without inventing real SQL execution.
- Renaming logical order_id's physical column to order_ref retains the original
  U64 parent ID and I64 returned order_ref. Runtime maps the storage key to its
  relation property and attaches I64 lineCount 3. The aggregate still targets
  OrderLine; the signed/unsigned fixture is not normalized away in the provider.
- A registered concrete cache and 60,000ms `count` query return fully equal
  first/second rows, one provider query, exactly two `start:cache` events and
  one `start:provider` event through the actual recording telemetry.
- A `tenant-a` cache registered through AggregationCacheBackend retains the
  source interface boundary. First/cached I64 count 2 rows compare equal; the
  actual insert U64 ID 9/I64 version 1/name `new` invalidates that namespace.
  Refreshed I64 count 3 differs and only two provider queries run. Explicit User
  ownership replaces the source Arc resource without extending its lifetime.
- Parent projection `[id, version, name]` enables and propagates 60,000ms caching
  to the lines/lineCount/OrderLine/single=true relation aggregate. Two actual
  aggregate entries return fully equal row vectors and only two provider queries
  total. No local grouping, alias conversion or cache replay replaces the kernel.

The new native group and refactored typed-query group pass 2/2. All six source
names pass unchanged Rust; the complete original runtime reports 224 passed and
its one subprocess-only file-log child fixture ignored by the parent. The ledger
at this checkpoint was 415 covered, 26 pending, five backend and five external exclusions. The
pending source gate covers 24 runtime roots and two graph-reference lifetime
adaptations. Complete API/default/privacy/composition/lifecycle and six-stage
acceptance remain open.

The completed native Debug regression passes 271/271 in 87.78 seconds after
rebuilding all targets. It includes the new aggregate and refactored typed-query
groups plus existing query-store and relation-enhancement ownership contracts.
Rebuilt selections pass 4/4 under ASan/UBSan with leak detection and 4/4 under
OpenWrt MIPS/QEMU. Pinned-source, inventory, registered-ledger, root-export
locators, compiled-layer, 550-file clang-format 18.1.8 and whitespace checks pass
as separate scopes. The runtime archive has no SQLite C API calls. Physical-
router execution and full stage parity remain unverified.

Use `build-sanitize` with `ASAN_OPTIONS=detect_leaks=1` and
`UBSAN_OPTIONS=halt_on_error=1`, or `build-wndr4300-memory` with its configured
QEMU emulator, for the selected checks below.

```sh
cmake --build build --target teaql_original_runtime_aggregate_tests \
  teaql_original_runtime_typed_query_tests teaql_query_store_tests teaql_relation_enhancement_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_aggregate_cache_contracts|original_runtime_typed_query_contracts|query_store_contracts|relation_enhancement_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib --locked --offline
```

## Runtime-root typed-query checkpoint, 2026-10-07

Six more `teaql-runtime/src/lib.rs` tests have reviewed direct ports in
`tests/parity/teaql-runtime/typed_queries.cpp`, registered as
`original_runtime_typed_query_contracts`. `typed_models.hpp` contains private
handwritten field mappings and the original handwritten relation getters.
Rootless scalar models, skipped-root flat models and the strict handwritten
OrderEntity remain distinct. The generator is unchanged. Default PMR is disabled.

- Original inverse-relation queue batches contain line U64 ID 11/order 7/name
  `line`/product 101, then product 101/name `sku`. Actual enhanced querying and
  embedded relationship hydration produce a typed product whose inverse-many
  SmartList contains exactly one line, ID 11. Mappings only parse the kernel's
  List/Object payload; no local grouping algorithm replaces hydration.
- Actual RuntimeModule typed registration and `into_context` install FlatTrip
  and FlatVendor decoders. Trip 11/vendor 101 and Vendor 101/name `Acme` are fed
  through the actual enhanced query. The scalar optional vendor remains absent;
  the handwritten getter resolves `Acme` through the actual immutable identity
  graph. C++ member `vendor_value` avoids a field/method name collision while
  preserving the logical metadata/payload name `vendor`.
- FlatFleet 7 and trips 11/12, fleet 7, names `first`/`second` use the actual
  module, graph decoders and enhanced-query operation. The local relation field
  remains unloaded, while its getter resolves two adjacency items. An explicit
  copy in the same User domain adapts the borrowed C++ result to a mutable local
  carrier. The original mutable getter copies adjacency into that carrier before
  adding trip 13/fleet 7/name `third` with fresh runtime state. The local field
  becomes loaded with three items; the graph's `trip_list` remains two. C++ field
  `trip_list_value` preserves the original logical name and getter responsibilities.
- The original handwritten OrderEntity accepts U64 or nonnegative I64 ID,
  strictly I64 version and Text name. Its option-debug failure mapping remains
  separate from derive defaults. Actual basic-list querying of ID 7/version
  2/name `typed` returns exactly one entity with all three original fields.
- A reusable capture fixture supplies the same original row to basic and enhanced
  typed queries selecting only `name`. Both return one entity, exactly two queries
  are captured, and both complete projections equal `[name, id, version]` in
  that order. Projection completion is production runtime code.
- CatalogProduct/catalog_product's scalar derive mapping has no runtime-state
  field. Its original row U64 ID 9/name `derived` produces exactly one model with
  both expected fields through the actual Context/basic-list adapter.

The C++ test helpers construct the same private production operations used by
the public basic/enhanced adapters, preserving the unpurposed original private
entries. Public-purpose/policy boundaries have separate source acceptance.
Queue consumption and capture repeat behavior remain distinct modes with the
original metadata. These private mappings do not claim complete generator parity.

The native group passes, and all six source names pass unchanged Rust. The
original runtime suite reports 224 passed, with its one subprocess file-log
child fixture ignored by the parent run. At this checkpoint the ledger had 409 covered, 32 pending,
five backend and five external exclusions. Pending review is 30 runtime roots
and two graph-reference lifetime adaptations. Full API/default/privacy/composition/
lifecycle and six-stage acceptance remain open.

The completed native Debug regression passes 270/270 in 57.47 seconds after
rebuilding all targets. It includes the new source group and existing entity
service, registry and graph/ledger ownership checks. Rebuilt selections pass
4/4 under ASan/UBSan with leak detection and 4/4 under OpenWrt MIPS/QEMU.
All 548 handwritten C++ files pass clang-format 18.1.8 format/check; pinned-source,
inventory, registered-ledger, root-export locators, compiled-layer and whitespace
checks pass separately. The runtime archive contains no SQLite C API calls.
Physical-router and complete migration parity remain unverified.

Use `build-sanitize` with `ASAN_OPTIONS=detect_leaks=1` and
`UBSAN_OPTIONS=halt_on_error=1`, or `build-wndr4300-memory` with its configured
QEMU emulator, for the selected checks below.

```sh
cmake --build build --target teaql_original_runtime_typed_query_tests \
  teaql_entity_service_tests teaql_runtime_registry_tests teaql_entity_runtime_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_typed_query_contracts|entity_service_contracts|runtime_registry_contracts|entity_runtime_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib --locked --offline
```

## Runtime-root event/order/relation checkpoint, 2026-10-07

Eight further `teaql-runtime/src/lib.rs` tests have reviewed direct ports in
`tests/parity/teaql-runtime/events_relations.cpp`, registered as
`original_runtime_events_relations_contracts`. They reuse `root_fixture.hpp`;
production service, event, ledger and relation components perform the behavior.
The generator and runtime dependencies remain unchanged. Default PMR is disabled.

- With Order name masking, fixed ID 88 and the original controlled old row,
  actual internal insert/update/delete/recover calls deliver four raw and four
  application-safe events. Every original kind, ID/version Value, updated field,
  optional old/new snapshot and change assertion is retained: blind update has
  no dynamically fetched old name, new name is Text `updated`, deletion has no
  old/new snapshots, and recovery from -3 yields I64 version 4. The safe created
  event contains a masked name field whose value is not plaintext `created`.
- The actual ledger ordering helper maps original `[OrderLine10, Product88]`
  to `[Product88, OrderLine10]`; combined reverse-many/forward dependencies map
  `[OrderLine10, Product88, Order77]` to `[Order77, Product88, OrderLine10]`.
  Complete U64 payloads and source metadata are retained. Reciprocal required
  Alpha1/Beta2 foreign keys return Graph diagnostics containing both entity names
  before any provider query or mutation.
- OrderBehavior yields exactly one many `lines -> OrderLine` plan with local
  `id` and foreign `order_id`. Original parent IDs `[U64(11), U64(12), U64(11)]`
  produce the full deduplicated right-hand literal `[U64(11), U64(12)]`. Source
  commented-out SQL assertions are not claimed as executed coverage.
- Original child rows 101/11/l1, 102/11/l2 and 201/12/l3 are fed to the real
  hydration forest for parent IDs 11/12/13. Attached List cardinalities 2/1 and
  a `start:relation_load` telemetry event are required. Test plumbing connects
  the actual entity-fetch adapter to the forest; it implements no local grouping
  or relationship enhancement algorithm.
- TOPN retains exactly two original parents 11/12 and six child objects with
  U64 ranks 1 through 3. Actual query-plan construction and hydration issue one
  nonaggregate query with partition `order_id`, limit 3 and complete ordering
  `[name DESC, id ASC]`. Each parent has three Object children, and every internal
  partition-rank property is removed. Although the source test name mentions
  empty safety, its fixture contains no empty third parent; this port does not
  treat that name as evidence for an unexercised branch.

The native direct group passes. All eight reviewed source names pass in the
unchanged original runtime suite: 224 passed, with its subprocess-only file-log
child fixture ignored in the parent run. At this checkpoint the ledger had 403 covered, 38 pending,
five backend and five external exclusions. Pending source review is 36 runtime
roots and two graph-reference lifetime adaptations; full API/default/privacy/
composition/lifecycle and six-stage acceptance remain open.

The full native Debug regression passes 269/269 in 66.18 seconds after rebuilding
all targets. It includes the new source group and existing mutation, ledger-plan
and relation-hydration ownership/integration contracts. After rebuilding,
the same four-check selection passes 4/4 under ASan/UBSan with leak detection
and 4/4 under OpenWrt MIPS/QEMU. Pinned-source, inventory, registered-ledger,
root-export locators, compiled-layer separation, 546-file clang-format 18.1.8
and whitespace checks pass as separate scopes. No SQLite C API appears in the
runtime archive. Physical-router execution and full stage parity remain unproven.

Use `build-sanitize` with `ASAN_OPTIONS=detect_leaks=1` and
`UBSAN_OPTIONS=halt_on_error=1`, or `build-wndr4300-memory` with its configured
QEMU emulator, for the selected checks below.

```sh
cmake --build build --target teaql_original_runtime_events_relations_tests \
  teaql_runtime_mutation_tests teaql_ledger_plan_tests teaql_relation_hydration_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_events_relations_contracts|runtime_mutation_context_contracts|ledger_plan_context_contracts|relation_hydration_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib --locked --offline
```

## Runtime-root validation checkpoint, 2026-10-07

Fifteen further tests from `teaql-runtime/src/lib.rs` have direct ports in
`tests/parity/teaql-runtime/validation.cpp`, registered as
`original_runtime_validation_contracts`. Original bodies, descriptors, scalar
Value variants, checker implementations and private derive mappings were
reviewed. `root_fixture.hpp` now contains the shared handwritten Order,
StubExecutor, behavior, generator and service fixtures used by this group and
the preceding service-boundary ports. The generator and production kernel are
unchanged. Default PMR is disabled during both executable groups.

- OrderChecker with fixed generator 77 accepts `valid`, generates U64 ID 77 and
  I64 version 1, and removes its preparation status marker. `no` returns exactly
  one result at `name`. Checker-only nested Create ID 10 inserts version 1,
  reports one Required `lines[0].name`, then accepts `valid child`.
- Metadata-only School checks report one Required `contact_phone` on Create.
  An Update may omit that field; explicitly supplied Null fails. Create values
  U64 ID 1 and Text `TeaQL School` need no supplied runtime-managed version, and
  validation leaves version absent.
- The actual private required-create helper checks U64 ID 1/I64 version 1
  against required School address. Its root error is `address`; supplied
  `school_list[2]` yields model `school_list[2].address` and JSON
  `/schoolList/2/address`. Text `12 River Road` makes the same payload pass.
- Actual internal ledger execution checks new OrderLine U64 ID 9 with order_id
  U64 7. Missing name reports one Required `line_items[0].name` and JSON
  `/lineItems/0/name` before mutation. No test-local planner implements this.
- Updating School U64 ID 1 to `New School` without a loaded original version
  fails even though the original controlled provider has I64 ID 1, `Old School`
  and I64 version 3. Delete intent without a loaded version likewise fails.
  Both preserve the original action/entity and missing-version error fragments.
- Real attachment of School ID 1 root version 2/`Newer` and stale version
  1/`Stale` records a preflight failure. Ledger execution retains all original
  message fragments, including `School#1` and both version numbers. Added
  counters prove no query or mutation reaches the provider.
- TypedEntityChecker restores Text `2026-08-25` after TimestampedEntity
  materialization fails; it returns exactly one InvalidType result containing
  `happened_at` and the original input. The handwritten private mapper retains
  Timestamp/I64/U64 decoding and rejects Text, following the original derive.
- TypedOrderChecker fixes `fix` to `fixed`, with fixed generator 79 producing
  U64 ID 79/I64 version 1 and no status marker. An omitted name retains a Required
  result. Sparse Update ID 7/name `valid`, then name `fix`, never gains a default
  version field. Prepared-field assertions check presence before accessing data.
- The five original required/min/max/min-string/max-string results produce 75
  nonempty translations across 15 languages with no unresolved brace. English,
  Chinese and French witnesses and zh-CN/zh-TW aliases remain source fixture data.
- Chinese checker errors contain the required-field translation. An independent
  Context switches English to Spanish and preserves Spanish after invalid-code
  rejection. Parsing the original en/es catalog and installing it explicitly in
  User storage yields exact `ES Name`; catalog ownership is not extended by Arc.

The new native validation group and refactored service group pass 2/2. All
fifteen reviewed original root names pass in the unchanged original runtime
suite, which reports 224 passed and its one subprocess file-log child fixture
ignored in the parent run. At this checkpoint the ledger had 395 covered, 46 pending, five
backend and five external exclusions. The 46 source reviews are 44 runtime roots
and two entity-runtime graph-reference lifetime adaptations. Source-test coverage
does not close full API/default/privacy/composition/lifecycle or six-stage gates.

After rebuilding the final prepared-field assertions, the new validation group,
refactored source service group and existing checker/ledger ownership contracts
pass 4/4 in each configuration: native Debug, ASan/UBSan with leak detection,
and OpenWrt MIPS/QEMU. All 545 handwritten C++ files pass clang-format 18.1.8
format/check. Source lock, inventory, registered dispositions, root export
locators, compiled runtime layer boundaries and whitespace are separate gates.
The runtime archive has no SQLite C API calls. Physical-router execution remains
unverified.

The complete native Debug regression passes 268/268 in 62.96 seconds with four
workers after rebuilding all targets. This full native run and the selected
sanitizer/MIPS evidence above have different verification scopes. Source-test
completion and full stage acceptance remain open.

Use `build-sanitize` with `ASAN_OPTIONS=detect_leaks=1` and
`UBSAN_OPTIONS=halt_on_error=1`, or `build-wndr4300-memory` with its configured
QEMU emulator, for the selected checks below.

```sh
cmake --build build --target teaql_original_runtime_validation_tests \
  teaql_original_runtime_service_tests teaql_checker_tests teaql_ledger_execution_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_validation_contracts|original_runtime_service_boundary_contracts|checker_context_contracts|ledger_execution_context_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib --locked --offline
```

## Runtime-root service-boundary checkpoint, 2026-10-07

Eight more `teaql-runtime/src/lib.rs` tests have reviewed direct ports in
`tests/parity/teaql-runtime/service_boundaries.cpp`, registered as
`original_runtime_service_boundary_contracts`. Original bodies, complete
Order/OrderLine/Product descriptors, controlled StubExecutor metadata, private
Order mapping, behavior/policy implementations and telemetry scope were reviewed.
Default PMR is disabled while the C++ contracts run. Requests, rows, metadata,
events and operations use their explicit User domain; no new dependency or
generator change is introduced.

- RuntimeDataService retains its independent metadata/executor adapter. Updating
  Order U64 ID 1 with expected version 3 and Text name `next`, against affected
  count zero, returns the original typed Runtime OptimisticLockConflict.
- ContextDataService retains Context metadata and telemetry. Checked Context
  executor lookup plus managed adapter construction is the explicit C++ lifetime
  counterpart of the original crate-private convenience factory, not a local
  implementation of update. The same update with affected count one returns one
  and exactly `start:mutation, start:provider, success, success`.
- The public entity service binds entity/select to Order. Its actual internal
  insert of U64 ID 1, I64 version 1 and Text name `n` returns one. Separate
  behavior-hook execution supplies the default version and exactly one relation
  load, `lines`.
- Preparation invokes behavior before request policy. With named U64 tenant 9,
  TenantRequestPolicy overrides the behavior's I64 version 1 with exact I64 9.
  The source's commented-out SQL assertions are not treated as executed tests.
- All ten original public query calls reject with the exact typed Runtime Policy
  message `QUERY_POLICY_PROBE`: borrowed/owned fetch-all, stream, plain and
  relation-aggregate smart lists, typed Order fetch, enhanced fetch, borrowed/owned
  enhanced aggregate fetch and aggregate fetch-all. The two original purposes,
  count alias `orderCount`, and `lines/lineCount/OrderLine/single=true` aggregate
  are retained. Additional counters prove no query, stream or mutation reaches
  the provider. Private query entries cannot stand in for these public calls.
- Fixed generator 42 and OrderBehavior produce exact U64 ID 42, I64 version 1
  and Text name `n`; an explicitly supplied I64 zero version is separately
  normalized to I64 1 for the `zero-version` input.
- ContextAwareOrderBehavior reads actual named resources `tenant = acme` and
  I64 `initial_version = 7`, plus Text local `trace_id = req-9`. Preparing an
  empty insert with fixed generator 99 returns U64 ID 99, I64 version 7 and Text
  `acme:req-9`. Allocation-only Memory does not replace the invoking UserContext.

The original synthetic PostgresDialect is adapted to SQLite numbered syntax;
none of these generic service assertions depends on a PostgreSQL implementation.
StubExecutor returns controlled source results only. Production RuntimeDataService,
ContextDataService and EntityDataService perform adaptation, telemetry, policy,
checking and preparation. These layers are not merged.

The new native source group passes. The unchanged original runtime suite passes
224 tests with its one subprocess-only file-log child fixture ignored by the
parent run; every one of these eight reviewed source names runs and passes.
The ledger at this checkpoint had 380 covered, 61 pending, five backend and five external
exclusions. Pending source reviews are 59 runtime roots and two entity-runtime
graph-reference ownership adaptations. Full API/default/privacy/lifecycle/
composition and six-stage acceptance remain open.

The complete native Debug regression passes 267/267 in 71.69 seconds with four
workers. After rebuilding, the new group and existing Context query, entity
service and runtime mutation ownership/integration contracts pass 4/4 on native,
4/4 under ASan/UBSan with leak detection and 4/4 under OpenWrt MIPS/QEMU.
Public export locators, source inventory, registered ledger
and compiled runtime layer separation remain separate gates; all pass. These
checks do not establish execution on the physical router or complete API parity.

Use `build-sanitize` with `ASAN_OPTIONS=detect_leaks=1` and
`UBSAN_OPTIONS=halt_on_error=1`, or `build-wndr4300-memory` with its registered
QEMU emulator, for the selected checks below. Formatting/checking 543 handwritten
C++ files with clang-format 18.1.8 and `git diff --check` pass.

```sh
cmake --build build --target teaql_original_runtime_service_tests \
  teaql_context_query_tests teaql_entity_service_tests teaql_runtime_mutation_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_service_boundary_contracts|context_query_contracts|entity_service_contracts|runtime_mutation_context_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib --locked --offline
```

## Runtime-root registration checkpoint, 2026-10-07

Eight tests from `teaql-runtime/src/lib.rs` now have direct ports in
`tests/parity/teaql-runtime/registration.cpp`, registered as
`original_runtime_registration_contracts`. Original fixture declarations,
derive metadata rules and the actual `module!` expansion were reviewed along
with every body assertion. The generator remains unchanged.

- The physical `channel` column aliases to logical `channel_id`. Original I64
  row values 9 and 1002 map to scalar IDs; the typed optional PaymentChannel
  remains absent. The descriptor has exactly one matching physical property and
  one independent logical relation.
- DetachedProduct retains two scalar properties and the one detached many
  relation `line_list -> DetachedLine`, with `id / product_id` keys. Handwritten
  metadata preserves this responsibility even without a model relation member.
- Actual InMemoryMetadataStore and RuntimeModule install the complete Order
  descriptor, then expose metadata and service availability. Typed CatalogProduct
  registration additionally installs its behavior and real graph decoder.
- The source test named `module_macro_registers_multiple_entities` actually
  invokes the macro with one CatalogProductRow. Its typed `entity<T>()` expansion
  and the separate `entity_with_behavior<T>()` pair expansion are ported exactly;
  this is not an implementation of Rust macro syntax or the generator.
- UserContext independently indexes U64 resource 42, named String
  `tenant = acme`, and local Value::Text `trace_id = req-1`, alongside Order
  metadata. Explicit managed User allocation replaces source by-value ownership.

The original root's PostgresDialect is a synthetic compiler fixture. The port
retains conditional identifier quoting and selects SQLite numbered placeholders;
none of these assertions requires a PostgreSQL provider. These generic contracts
are covered, not backend exclusions. Pure mapping helpers do not implement local
service or registration behavior; production module, registry and Context paths
perform those responsibilities. This registration selection does not establish
complete derive, graph hydration, policy or generator parity.

After rebuilding the selected targets, native Debug, ASan/UBSan with leak
detection, and OpenWrt MIPS/QEMU each pass 3/3: the new source group and existing
registry/resource ownership contracts. The unchanged original runtime suite
passes 224 tests with one source-ignored test. All eight reviewed original root
names pass in that run. No production behavior or dependency changed.

The complete native Debug CTest regression passes 266/266 in 56.57 seconds
with four workers. The ignored original case is the `file_log_child` subprocess
fixture; its parent privacy test runs successfully. Pinned-source, inventory,
registered-disposition, public-export, compiled-layer, 542-file clang-format
18.1.8 and whitespace checks pass. These are separate verification scopes,
not a declaration of complete migration parity.

```sh
cmake --build build --target teaql_original_runtime_registration_tests \
  teaql_runtime_registry_tests teaql_runtime_context_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_registration_contracts|runtime_registry_contracts|runtime_context_resource_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib --locked --offline
```

Use `build-sanitize` with leak detection/UBSan halt-on-error or
`build-wndr4300-memory` with its registered QEMU emulator for the scoped checks.
The ledger at this checkpoint had 372 covered, 69 pending, five backend and five
external exclusions. Pending source reviews were 67 runtime roots and two entity-runtime
graph-reference ownership adaptations. Complete six-stage API/default/privacy/
lifecycle/composition acceptance and physical-router execution remain unproven.

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
These semantic ports retain original inputs and assertions. They complement,
rather than replace, the existing original-call differential corpora and native
ownership tests. The reference repository and generator remain unchanged.

## Entity-status model source contracts

All sixteen tests in `teaql-runtime/src/entity_status.rs` have reviewed direct
ports in `tests/parity/teaql-runtime/entity_status.cpp`. The port preserves
default New, all eleven valid transitions, every one of the seventeen invalid
state/action pairs, all seven persistence flags, the original helper predicates
and every intermediate state in the create/update/delete/recover lifecycle.
Additional assertions cover each predicate across the complete seven-state
domain and the Display names of all states and actions. Valid transitions and
names remain usable in constant expressions.

Review found two public-contract omissions: EntityAction had no Display-name
adaptation, and invalid transitions discarded the original state/action error
payload. `to_string(EntityAction)` now retains the four names, and the existing
`std::invalid_argument` adaptation carries the original complete message, for
example `invalid entity status transition: New + Delete`. This is ordinary C++
exception storage, not a managed Context object or an additional lifetime.

The error expectations use independent literal names. A native negative control
temporarily restores only the old generic error text in a temporary header and
then compiles the same port; it exits with failure at the complete-reason
assertion. The production header is not modified by this control. This confirms
that the original omission cannot pass by merely rejecting the transition.

Original Rust passes 16/16. The newly built named C++ contract passes on native
Debug, ASan/UBSan with leak detection and OpenWrt MIPS/QEMU. The existing native
core-model contract is rebuilt and passes alongside the new check. No runtime
dependency or generator changed. This accepts the standalone model contracts;
runtime/entity integration and full six-stage acceptance remain separate gates.

After rebuilding every affected native consumer, the complete native Debug
regression passes 250/250 CTest checks. Compiled-runtime layer validation,
pinned source/inventory/export checks, source-test registration and 524-file
clang-format 18.1.8 checks pass. The sanitizer/MIPS results above are scoped;
physical-router execution remains unverified.

```sh
cmake --build build --target teaql_original_entity_status_tests teaql_core_model_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_entity_status_contracts|core_model_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib entity_status::tests --locked --offline
```

## Aggregation keys and relation top-N planning

Four source tests have direct reviewed ports in
`tests/parity/teaql-runtime/cache_relation.cpp`: two aggregation-cache helpers
and two relation-plan/telemetry tests. The port calls production helpers without
supplying replacement normalization or planning logic. All runtime values use
an explicit User allocator, with default PMR disabled.

The original Trip queries retain version > I64(0), vendor_id U64(1), payment_type
I64(1), both predicate orders, both comments and TTL 1000/60000. Their tenant /
entity:Trip aggregation keys remain equal. Distinct vendor_id U64(1)/U64(2)
queries have distinct keys.

The Vendor.trips plan retains target Trip, local id, foreign vendor_id,
many=true, descending id and limit ten. All five original planner cases are
checked: default Window, provider-enabled probes at 10,000 parents, explicit
threshold-four boundary, the next count falling back to Window, and explicit
zero overriding the provider preference. Additional to-one, absent-query and
unsliced inputs preserve the helper's early exits.

The original threshold-32/six-parent telemetry checks retain exact String and
Integer variants, selected plan, counts, per-parent limit and absence of entity
ID. Additional checks retain all seven relation attributes and the two attributes
installed by RuntimeOperation::new, with no unexpected keys. Window telemetry
reports zero probes. Telemetry and cache keys are explicitly deep-copied into
Global; the original User is actually deleted before a later User resolves the
retained nested map/string data. The aggregate User byte charge is zero between
Users. This does not extend the original User lifetime.

Unchanged Rust passes both helper tests and both planner tests. The new named
C++ contract passes after building on native Debug, ASan/UBSan with leak
detection and OpenWrt MIPS/QEMU. Pinned-source, inventory/export locators,
source-test registration and 525-file clang-format checks pass. Production
implementation, dependencies and generator remain unchanged. The prior full
native 250-check result predates this additional registration; no complete
251-check or full-platform regression is claimed. Complete runtime composition
and overall six-stage acceptance remain open.

```sh
cmake --build build --target teaql_original_cache_relation_tests
ctest --test-dir build --output-on-failure \
  -R '^original_runtime_cache_relation_contracts$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib aggregation_cache_key_tests --locked --offline
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib data_service::relation::planner_tests --locked --offline
```

## In-memory query engine source contracts

All nineteen tests in `teaql-runtime/src/inmemory_engine.rs` have individually
reviewed direct ports in `tests/parity/teaql-runtime/inmemory_engine.cpp`.
The original User sample has U64 IDs 1/2/3, Text names Alice/Bob/Charlie and
I64 ages 30/25/35. The port invokes production InMemoryQueryEngine and
ExprEvaluator; no local filtering, sorting, pagination or aggregate algorithm
is supplied.

Unfiltered output preserves all three complete rows and the memory backend.
Equality, greater-than, ascending/descending age ordering, page(1,1), and
name-only projection retain the original inputs and assertions. Projection
additionally checks the row count so an empty result cannot pass its per-row
assertions. Both sort vectors retain their exact I64 values.

Aggregate assertions preserve variant and value: COUNT I64(3), SUM F64(90.0),
AVG F64(30.0), MAX I64(35), MIN I64(25). These use exact Value equality rather
than permissive numeric extraction. The AND/OR/NOT, Null predicates, BETWEEN
and IN-list inputs likewise retain their original types and outcomes.

All twelve private LIKE matcher inputs retain the original strings, patterns
and booleans. The Rust matcher is private; the C++ port reaches the same
production matcher through public ExprEvaluator::eval with a Text column and
LIKE expression. It does not expose a new public matcher API or substitute a
test implementation.

Default PMR is disabled throughout. All fixture rows, expressions, queries and
results use the explicit User resource. The original User is actually deleted
after local result carriers have ended, and the aggregate User byte charge is
zero. Existing in-memory ownership contracts separately check explicit Global
execution/copying, retained nested data, stable sorting and Unicode LIKE.

Unchanged Rust passes 19/19. The three named source/ownership/differential checks
pass after building on native Debug, ASan/UBSan with leak detection and OpenWrt
MIPS/QEMU. A rebuilt live Rust oracle also matches 6,532/6,532 observations on
native and MIPS. The MIPS comparison preserves executing-target 32-bit pagination
casts using the existing normalization of the 64-bit Rust inputs.

Verification exposed an emulator propagation gap: Python launched the MIPS
probe without CMake's configured CPU command. The comparator now accepts the
same semicolon-separated emulator argument as other parity runners, and CMake
passes it explicitly. Both pointer-width detection and observation calls use
that command. The initial direct-launch differential run failed with SIGILL;
the corrected named MIPS selection passes 3/3. No production engine behavior,
dependency or generator changed. This is selected evidence after the full
native 250-check checkpoint, not a complete 252-check or full-platform run.
Source-test acceptance does not replace remaining public trait, composition or
complete-stage requirements.

```sh
cmake --build build --target teaql_original_inmemory_tests \
  teaql_inmemory_tests teaql_inmemory_probe
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_inmemory_contracts|inmemory_context_contracts|inmemory_rust_parity)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib inmemory_engine::tests --locked --offline
```

## Checker paths, locale normalization and catalog source contracts

Nine source tests have individually reviewed direct ports in
`tests/parity/teaql-runtime/checker_locale.cpp`: five checker, two language and
two catalog tests. The locale and builtin catalog JSON fixtures are copied byte
for byte from the pinned source and checked with `cmp`. Non-English catalog
strings remain semantic test data, not project explanatory material.

ObjectLocation checks retain every root/hash/array/nested string, root flag and
level, plus model/native/CamelCase/SnakeCase/PascalCase path forms. JSON Pointer
escapes both slash and tilde. Required user_url wire projection preserves
customer_account, rendered /userUrl and submitted /user_url independently,
including all original serialized fields and the complete one-segment location.
CheckObjectStatus inference preserves the original no-ID/I64(1)/Create/Update/
clear-marker sequence.

Language normalization retains all four original canonical code assertions,
fourteen canonical/alias parses and both invalid inputs. The complete shared
JSON fixture checks every canonical roundtrip, alias and unsupported input.
The malformed builtin catalog replaces every original {location} occurrence
with {unsafe}, then verifies the production parser rejects it with the original
unknown-placeholder reason before installation. The exact minimal English
catalog queried in French preserves known-message, stable-key and builtin
checker-message fallback.

All dynamic fixture/runtime data uses the explicit User resource, with default
PMR disabled. Wire results and catalogs are explicitly copied into Global; the
source User is actually deleted before a later User resolves them. Nested wire
location strings retain the destination allocator and the catalog still returns
the complete English message. Aggregate User byte charge is zero between Users.
File streams are call-local fixture I/O wrappers, not Context data owners.

Unchanged Rust passes 5/5 checker, 2/2 language and 2/2 catalog tests. The source
and existing checker/i18n ownership selection passes 3/3 after building on
native Debug, ASan/UBSan with leak detection and OpenWrt MIPS/QEMU. The complete
locale fixture contains fifteen canonical codes, twenty-four aliases and four
unsupported codes; none is omitted. Pinned source, inventory/export locators,
registered source dispositions and 527-file clang-format 18.1.8 checks pass.
No production implementation, dependency or generator change was needed.
This is selected evidence after the full native 250-check checkpoint, not a
complete 253-check or full-platform regression. Complete default/trait/
composition and overall six-stage acceptance remain open.

```sh
cmake --build build --target teaql_original_checker_locale_tests \
  teaql_checker_tests teaql_i18n_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_checker_locale_contracts|checker_context_contracts|i18n_context_contracts)$'
cmp tests/parity/teaql-runtime/locale-cases-v1.json \
  .reference/teaql-rs/teaql-runtime/src/locale-cases-v1.json
cmp tests/parity/teaql-runtime/builtin-i18n-v1.json \
  .reference/teaql-rs/teaql-runtime/src/builtin-i18n-v1.json
```

## Formatter privacy and fail-open telemetry source contracts

Seven source tests have individually reviewed direct ports in
`tests/parity/teaql-runtime/log_telemetry.cpp`: four formatter and three telemetry
tests. The port invokes real formatter, truncation, operation and fail-open
runtime APIs; the controlled adapters only inject the original failures.

Both short and bounded diagnostic SQL retain their original inputs. The bounded
SQL is visibly marked non-executable, excludes the canary, and truncates before
the split four-byte UTF-8 lock scalar. Bounded audit output retains the complete
scalar at byte limit ten; an additional byte-nine assertion verifies retreat
before that scalar. Exact suffixes preserve the original byte limit and reason.

Human and Debug formatters receive the original School Created event with
U64(7) ID and private-school-name Text. Ordinary audit retains entity and field
names while withholding the value. Explicit sensitive formatting retains its
source payload contract; authorization remains the caller's separate boundary.
Summary and Full reject raw audit output; FullWithPayload permits it under the
exact original no-filter configuration with SQL/tool logging Silent.

Native error category checks retain the original three type strings and outcomes.
The School.list operation retains safe entity type and excludes the forbidden
I64(42) entity ID. A throwing start adapter cannot escape through success then
late failure calls. A separate throwing context adapter wraps a real
AsyncOperation through FailOpenRuntimeTelemetryScope::run; the business result
remains 42 with exactly one future poll. Context-owned boxes replace Rust Box/Arc
ownership without adding a lifetime or a local failure-isolation algorithm.

Default PMR is disabled. The source User is actually deleted; all its blocks
return, the telemetry delegate is destroyed exactly once, and a later User
resolves explicitly retained Global ordinary audit output without exposing the
canary. The adapter counter targets outlive their Context registrations.

Unchanged Rust passes 4/4 formatter and 3/3 telemetry tests. The source and
existing telemetry/formatter ownership selection passes 3/3 after building on
native Debug, ASan/UBSan with leak detection and OpenWrt MIPS/QEMU. Pinned source,
inventory/export locators, registered source dispositions and 528-file
clang-format 18.1.8 checks pass. Production implementation, dependencies and
generator are unchanged. This selected evidence follows the full native
250-check checkpoint; no complete 254-check or full-platform regression is
claimed. Source-test acceptance does not establish full logging/privacy/default
composition or complete-stage acceptance.

```sh
cmake --build build --target teaql_original_log_telemetry_tests \
  teaql_telemetry_tests teaql_log_formatter_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_log_telemetry_contracts|telemetry_contracts|log_formatter_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib log_formatter::diagnostic_log_tests --locked --offline
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib telemetry::tests --locked --offline
```

## Mutation-policy review and graph trace/batching source contracts

Seven source tests have individually reviewed direct ports in
`tests/parity/teaql-runtime/policy_graph.cpp`: five mutation-policy and two graph
tests. Every policy case retains the complete exec-1 / Order.saveGraph / Order /
submit order plan, with one Create operation for U64(7) and total_amount
I64(25,000). Controlled policies supply only the original allow/deny decisions;
actual Context review performs identity lookup, approval and warning handling.

Two generated-default reviews retain both complete warning vectors and deliver
two events with first-occurrence true then false. Warning sink Result failure
remains isolated from the allowed review; this is not blanket exception
suppression. The order-policy / 1.0.0 / sha256:test identity accepts the original
security-review epoch approval only when its fingerprint matches. The original
sha256:other approval produces Missing plus the original missing-approval code.
ORDER-DENIED returns a typed Policy error and keeps the source conditional
persistence counter at zero. This gate tests review ordering, not database
transaction or graph-save execution.

Both synchronous ScopedCommentNode and persistent TraceScopeToken recovery
retain complete User/Profile traces in order and omit only the AuditLog node's
empty comment. The User ID remains Some(1); child IDs remain absent, and token
indexes are 0/1/2. Context-owned token borrows replace Arc lifetime extension.
The six original graph pushes retain empty values, absent trace/snapshot and
name/email field vectors. Actual batching produces four ordered batches with
membership 2/2/1/1, and three grouped counts: User/Create 2, User/Update 3,
Profile/Create 1. No local grouping or trace-recovery algorithm is supplied.

Default PMR is disabled. Warning events and the plan are explicitly copied into
Global, including nested operation summaries, batches and field strings. Their
source Users are actually deleted before later Users resolve the copies; all
source User blocks return. These tests preserve distinct review, plan and save
responsibilities rather than claiming batching is graph-save support.

Unchanged Rust passes 5/5 policy and 2/2 graph tests. The source and existing
policy/graph ownership selection passes 3/3 after building on native Debug,
ASan/UBSan with leak detection and OpenWrt MIPS/QEMU. Pinned source,
inventory/export locators, registered dispositions and 529-file clang-format
18.1.8 checks pass. No production implementation, dependency or generator
change was needed. This selected evidence follows the full native 250-check
checkpoint; no complete 255-check or full-platform regression is claimed.
Complete API/default/composition and six-stage acceptance remain open.

```sh
cmake --build build --target teaql_original_policy_graph_tests \
  teaql_mutation_policy_tests teaql_runtime_graph_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_policy_graph_contracts|mutation_policy_context_contracts|runtime_graph_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib mutation_policy::tests --locked --offline
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib graph::tests --locked --offline
```

## Privacy acknowledgement, audit projection and isolated file sinks

All five indexed `log_privacy.rs` contracts have reviewed direct ports in
`tests/parity/teaql-runtime/log_privacy.cpp` and `file_log_privacy.py`. Rust's
ordinary test run passes four tests and reports one ignored child. The parent
executes that child in three fresh processes; the ignored entry is accepted on
that subprocess evidence, not counted as a standalone default Rust pass.

The exact acknowledgement test retains absent, empty, true, 1 and leading-space
rejections, plus the one accepted string. Rust's accepts helper is private;
C++ reaches the gate through the actual Global-owned manager. The test clears
its process-local opt-in before ordinary projection tests. Manager warning
ownership remains the documented explicit-Global adaptation.

The Customer audit fixture retains PRIVATE-CUSTOMER, access_token TOKEN-CANARY
and I64(1). Actual ordinary projection hides the business value; explicit
plaintext projection never exposes credentials; raw values and Debug output
remain unchanged. Created/Updated/Deleted/Recovered traces retain I64(1001),
the original version inputs and complete AuditReason nodes. Both safe-event and
ordinary projection scrub target text while preserving the source trace and ID.

The file child retains the original Customer/name/password event, masked
UPDATE binding, free-text metadata, credential clone and inherited-intent
readback. Readback preserves SELECT text, Plain U64(1) binding and the source's
unchanged Update operation. The real production manager writes all ordinary and
sensitive sinks; no local redaction or file-logger algorithm is supplied.

The Python parent retains all original unset/invalid/exact-ACK cases and output
assertions. Ordinary logs exclude all three canaries, retain authoritative-read
intent and exact expanded masked SQL, and carry non-replayable annotation.
Only exact opt-in produces annotated business plaintext diagnostics; credentials
never enter either diagnostic file. Stderr warning presence follows the same
gate. The expected mask is a fixed original fixture result, not generated by
C++ masking logic. Every process uses the configured emulator when cross-built.
Tiny synthetic logs remain inspectable in a unique temporary directory, as in
the source. No endpoint outside those test-owned files is used.

Default PMR is disabled. An explicit Global safe-event copy remains private
after actual source User deletion, which returns all User blocks. Original Rust
passes four tests with the child exercised by the parent. The source, isolated
file and existing manager ownership selection passes 3/3 after building on
native Debug, ASan/UBSan with leak detection and OpenWrt MIPS/QEMU. Pinned source,
inventory/export locators, registered dispositions and 530-file clang-format
18.1.8 checks pass. No production implementation, dependency or generator change
was needed. Complete logging/privacy/default composition and six-stage acceptance
remain open.

Full native integration at this checkpoint passes 257/257 CTest checks after
requesting the complete consumer build. Compiled-runtime layer validation
preserves SQLite independence. The three-platform privacy selection above
remains scoped; full-platform, physical-router and complete-stage acceptance
are not established by the native total.

```sh
cmake --build build --target teaql_original_log_privacy_tests teaql_log_manager_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_(log_privacy|file_log_privacy)_contracts|log_manager_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib log_privacy::tests --locked --offline
```

## UserContext roots, SQL logs and fresh mutation state

All six indexed Context tests have individually reviewed direct ports in
`tests/parity/teaql-runtime/context.cpp`. Typed Tenant root ID42, Organization
mismatch and absent root preserve the original fail-closed behavior. Additional
error checks retain the source's canonical tenant/organization names. SQL-log
switches retain All -> MutationOnly -> SelectOnly -> Disabled independently.
Disabled metadata retains the original sql backend, dollar-placeholder template,
I64(1) binding and supplied debug SQL; the gate rejects it before recording, so
this deliberately unsupported template does not implement another backend.

The account/secret fixture preserves private-value-0123, count one and its load
comment. Recorded SQL keeps the complete exact redacted parameter, masked expanded
form, identical pretty form and an equal unified-buffer payload. Every original
ordinary formatter assertion remains. The School query retains both structured
intent strings, all three relation levels and the seven ordered trace kinds,
including original root/provider/SQL names. Its subsequent Academy update
retains both redacted bindings, absent comment/purpose, exact audit reason,
four ordered kinds, affected count one and complete masked SQL without Academy.

Review found `UserContext::entity_runtime_state()` absent from C++. The public
factory now owns a lazy User graph anchor and delegates to the existing
fresh_with_shared_graph implementation. Each call obtains a distinct mutation
ledger. The original School/U64(1)/name First test retains one changed field in
the first state and none in the second. Additional checks publish one immutable
graph through the first state, resolve it from the second and third states, and
verify later ledgers remain empty. Existing original data-service callers that
construct independent roots remain unchanged; this repair restores the missing
public Context responsibility rather than altering those source paths.

The factory rejects closed Contexts before reading the anchor. Object-quota
exhaustion leaves an existing checked object usable and does not prevent User
reclamation. Explicit Global log/state copies retain projected SQL, graph payload
and first-state ledger after actual source User deletion. All User blocks return;
default PMR is disabled. The Context-owned state carrier remains a borrow and
does not extend its User lifetime.

Unchanged Rust passes one root, four SQL-option and one ledger tests. The newly
built native source contract passes. The five named source/entity/memory/logging
checks pass after rebuilding on ASan/UBSan with leak detection and OpenWrt
MIPS/QEMU. The SIZE_MAX allocation rejection input now enters through a volatile
runtime value, preserving the exact boundary while removing GCC's compile-time
oversized-allocation diagnostic; warning policy is unchanged. Both selections
pass again after that test-only adjustment.

The complete native consumer rebuild for the public Context layout change
finished and passed 258/258 checks. After refreshing the entity-runtime fixtures
and preserving the source's shared query/mutation timestamps, the paired named
selection passes 2/2 on native, ASan/UBSan and MIPS/QEMU. The latest full native
rerun then passes 258/258. Pinned source, inventory/export locators, registered
dispositions and 532-file clang-format 18.1.8 checks pass.
No production dependency or generator changed. Full API/default/composition and
complete-stage acceptance remain open.

```sh
cmake --build build --target teaql_original_context_tests \
  teaql_context_reference_tests teaql_context_memory_tests \
  teaql_context_logging_tests teaql_entity_runtime_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_context_contracts|context_reference_contracts|context_memory_contracts|context_logging_contracts|entity_runtime_contracts)$'
```

## Entity-runtime composition and lazy-state source contracts

Thirteen of fifteen indexed entity-runtime tests have reviewed direct ports in
`tests/parity/teaql-runtime/entity_runtime_contracts.hpp`, called by the existing
entity-runtime contract executable. The handwritten TestEntity retains the
source's optional state, empty values, descriptor and unused decoder error;
actual public LedgerEntity defaults implement composition.

The ports retain positive signed/unsigned key equality and ordering, distinct
entity types, negative-versus-maximum-unsigned separation, both missing-ledger
variants, copy/retry without source aliasing, signed snapshot versions for
unsigned keys, complete version-conflict payloads and atomic rejection, equal
versions across distinct types, and void attachment's retained preflight error.
All comparisons preserve exact Value variants and optional presence.

The deterministic source seed, wrapping constants, shift/rotation, field
schedule, all 512 trials and all 32 mutations per trial are unchanged. Each
compatible trial checks every source value and both resulting versions; each
conflict checks the complete target change set, unchanged target version and
absent other-type version. Every source change set remains retryable. Explicit
User lifetimes mirror the original trial-root teardown and return all blocks.

Lazy snapshot reads retain the original Example/U64(7)/I64(3) fixture and do not
allocate a ledger before mutation. Order/InferenceExecution share numeric ID1
while preserving their independent versions. Clone-before-write shares one
ledger; explicit Child/U64(17) attachment retains new marker, exact display name
and version. Loaded/empty/missing lists and present/null/absent to-one handles
retain all original states, full values and optional distinctions.

The two Arc strong/weak ownership tests remain pending C++ lifecycle-adaptation
review. They are not excluded, replaced by simple relation checks or counted as
accepted. Existing Context ownership/cyclic-copy tests remain separate evidence.

Unchanged Rust passes all fifteen source tests. The thirteen direct ports and
existing ownership tests pass the rebuilt named check under ASan/UBSan with leak
detection and OpenWrt MIPS/QEMU. The native entity target was refreshed after
the complete consumer rebuild so it includes every addition. The paired
entity/Context source selection passes 2/2 on native, ASan/UBSan and MIPS/QEMU,
and the latest complete native regression passes 258/258. Production runtime
behavior, dependencies and generator are unchanged by these fixture ports.
Complete API/lifecycle/composition and six-stage acceptance remain open.

```sh
cmake --build build --target teaql_entity_runtime_tests
ctest --test-dir build --output-on-failure -R '^entity_runtime_contracts$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib entity_runtime:: --locked --offline
```

## Masking contracts

The original `teaql-runtime/src/masking_contract_tests.rs` passes 35 unchanged
Rust tests. The C++ port covers the 34 contracts applicable to SQLite. The
PostgreSQL array shape/rendering test is excluded from the selected database
scope, not counted as a passing C++ comparison.

The ten-row `masking-v1.tsv` fixture is copied byte for byte from the reference.
Unicode, combining characters, ASCII versus non-ASCII digits, empty content,
boundary lengths and embedded quotes run through actual audit-field, audit-event
and SQL projection paths. The six macro fixtures preserve both expanded SQL and
audit algorithm checks. Source execution parameters and audit values remain
unchanged.

The other original contracts retain:

- Target-ID annotation scrubbing while preserving plain bindings and row counts.
- Cloned debug readback downgrading, safe-label monotonicity and idempotence.
- Tampered or reconstructed debug records failing closed on inherited intent.
- Sixteen independent clone/projection workers and mutation of one safe alternate
  without poisoning another. Each worker allocates in its own arena; all join
  before the immutable source borrow or its owner can expire.
- Malformed mask/policy counts, unknown bindings and unknown dialect rejection.
- Trace names, result text and untrusted omission reasons as privacy channels.
- Classified inherited intent, nested credentials, sibling protection,
  longest-first matching and typed numeric text, including NaN and lists.
- Explicit plaintext provenance, mixed policies, credential canaries and
  untrusted legacy/literal SQL rejection without raw-debug fallback.

Safe records are checked through both Debug serialization and Human formatting.
The port uses the existing privacy kernels; it contains no substitute redaction
implementation. Existing runtime-log tests separately cover retained Global
copies after actual User deletion and invalidation of private projection state.

## Flat identity graph

All four original `tests/flat_identity_graph.rs` contracts are ported. Separate
roots resolve the identical frozen Vendor address while keeping independent
mutation ledgers. The same numeric ID retains both Vendor and String payloads.
Repeated typed to-many lookup preserves list address, ordering and type checks.
Loaded-null to-one state remains distinct from an absent relation.

The handwritten Vendor has an allocator-aware String instead of an unscoped
heap string; the entity IDs and names match the original fixture. GraphBuilder,
frozen entity graph and EntityRuntimeState remain independent abstractions.

## Incremental ledger

All four original `tests/test_incremental_ledger.rs` contracts are ported. The
observability fixture retains its own sequential InternalIdGenerator provider,
five changed Task records, two deletions, overwritten name, version tampering
and separate original versions. Original printed field-signature groups and
deleted IDs are asserted; these are ledger observations, not persisted SQL
batch execution.

Commit cleanup removes changes, new/deleted markers, explicit original versions
and comment before an independent later save. The randomized test preserves all
eight original seeds, unsigned wrapping generators, three independent roots,
2,048 operations per seed and each per-step field/key/version comparison. Nested
change sets retain the original seed, 4,096 operations, depth limit eight,
exact popped changes and latest-value lookup after rollback. Standard containers
are independent test expectations; all runtime state and snapshots allocate in
their explicit UserContext. The iteration ranges are not shortened.

## Reproduction and remaining gates

On 2026-10-07 the complete rebuilt native Debug suite passed 187/187 checks.
The three new C++ source-test groups passed 3/3 under ASan/UBSan with leak
detection and 3/3 in the OpenWrt MIPS cross-build under QEMU. The original masking
TSV compares byte for byte; pinned upstream, inventory and clang-format 18.1.8
checks passed. No production implementation change was needed for these ports.

```sh
cmake --build build --target teaql_original_masking_tests \
  teaql_original_flat_graph_tests teaql_original_ledger_tests
ctest --test-dir build --output-on-failure \
  -R '^original_runtime_(masking|flat_identity_graph|incremental_ledger)_contracts$'
```

Original Rust evidence is 35 masking, four flat-graph and four incremental-ledger
tests. Full API/member/default/serialization and privacy/log acceptance remain
separate migration gates. The two original SQL execution test groups now have
[explicit source-test acceptance](sql-execution-source-test-acceptance.md).
Physical-router execution is unverified.

## Per-test assertion review, 2026-10-07

The masking review now records all 34 SQLite-applicable tests individually in
the disposition ledger. The PostgreSQL array test has a separate backend
exclusion; no mixed source module is discarded wholesale. Twelve of these
records are the two tests expanded in each of the six `mask_case!` modules.
The inventory previously counted two macro templates instead of twelve runnable
tests, undercounting the whole source index by ten. Its pinned-source adapter
now indexes 451 tests. All 35 masking names match the unchanged Rust compiler's
`cargo test -- --list` output exactly, including the backend-excluded test.

The expanded-SQL port checks the complete original Text binding and the SQL-
escaped partial mask, including `O'Reilly`. Each of the sixteen clone workers
checks the full single-element parameter count. The malformed-mask fixture
inserts its two false bits explicitly, avoiding an OpenWrt GCC 13.3 warning
from the optimized `vector<bool>` initializer-list assignment while preserving
the original input. No production implementation or compiler warning policy
changed.

After these assertion repairs, both the masking and SQL rendering targets were
rebuilt in native Debug, ASan/UBSan and OpenWrt MIPS configurations. Each scoped
selection passed 2/2, with leak detection enabled for ASan. Unchanged Rust passed
35/35 masking and 6/6 renderer tests, and the masking TSV still compares byte
for byte. This is scoped evidence after the earlier full 240-check checkpoint.

```sh
cmake --build build --target teaql_original_masking_tests \
  teaql_original_sql_rendering_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_masking_contracts|original_sql_log_rendering_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib masking_contract_tests --locked --offline -- --list
```

All eight original flat-graph and incremental-ledger tests now have individual
covered_in_scope records in source-test-dispositions.json, after reviewing
original inputs, iteration counts and assertions against their C++ ports.
The flat graph's first-root I64 value now uses exact Value equality instead of
a permissive numeric extractor. Randomized/popped ledger fields retain exact
U64 before independent model comparison. Nested lookup checks Some/None and
U64 separately, so a present nonnumeric value cannot collapse to missing.

A native negative control deliberately substitutes U64(100) for the original
flat-graph I64(100). The former converting assertion exits successfully; the
repaired exact assertion fails. Production runtime behavior is unchanged.

Both targets were rebuilt before the scoped native, ASan/UBSan with leak
detection and MIPS/QEMU runs; each configuration passed both checks. Unchanged
original Rust passes four flat-graph and four incremental-ledger tests. The
previous full native 240-check checkpoint predates these assertion repairs;
this acceptance is the subsequent scoped rerun, not another claimed full run.
All eight seeds/2048 steps/three roots and the nested 4096-step/depth-eight
contract remain intact. No production dependency or generator was changed.


## Entity-save source contracts

All eight indexed tests in `teaql-runtime/src/entity_save.rs` have individually
reviewed direct ports in `tests/parity/teaql-runtime/entity_save.cpp`. The source
has four registered test modules; filtering on `entity_save` runs exactly eight
unchanged Rust tests, and all pass. The direct C++ group passes native Debug.
It links only the runtime target, not the SQLite provider.

The five transactional cases retain the original Task/I64 identity/version/name
fixtures, sparse ledgers and controlled executor. The provider fixture only
records the source call sequence and returns the source storage behavior. The
actual GraphSaverFor performs planning, governance, begin, mutation, authoritative
root read, commit/rollback and ledger completion. The update/delete command
expectations, affected count, empty generated values/snapshot and unrecorded
metadata remain separate types as in the source. Same-thread Context storage
replaces the fixture's Arc/Mutex without creating another owner.

The source assertions retain these distinct outcomes:

- Returned update snapshot version2/name updated precedes commit's concurrent
  version3/name other writer visibility; last call is commit and no ambient
  reread occurs. The first raw event retains Task.saveGraph, GeneratedDefault
  governance and exactly one operation.
- Empty authoritative readback fails before reporting success, rolls back,
  never commits, restores provider version1 and retains ledger original version1.
- Soft deletion returns authoritative tombstone version-2 before commit and
  never performs an ambient reread.
- Create-then-delete Task7/version0/cancelled fails as a cancelled new root
  without a mutation or commit and cannot return a fictitious persisted entity.
- Customer task-policy/1/sha256:deny-task reviews exactly one Task.saveGraph
  operation and denies TASK-DENIED/Task.name before any provider call, including
  transaction begin.

The remaining three private contracts call actual kernel helpers. Scalar amount
change permits immutable relation reuse, while a child sku change or local
customer_id change forbids it. Order7/OrderLine9 locations retain the empty root,
line_items[0] model path and /lineItems/0 JSON path. Handwritten DirtyTrackedOrder
retains source scalar/ledger hooks and ORD-OLD to ORD-NEW intent; real graph
extraction/preflight and TypedEntityChecker must pass Update/dirty code to the
probe and remove the final _dirty_fields marker. The generator remains unchanged.

Default PMR is disabled, all rows/calls/events/ledgers/results use the invoking
User allocator, and registered runtime operations remain Context-owned borrows.
Existing saver/preparation ownership tests separately cover cancellation,
resource cleanup and explicit Global retention. These eight source assertions
do not establish every save/API/default/privacy/lifetime contract.

The ledger is 347 covered, 94 pending, five backend and five external exclusions.
Remaining source reviews are runtime roots, round-trip references and two
entity-runtime graph-reference ownership adaptations. Complete six-stage
acceptance remains open; no production behavior or dependency changed.


The complete native Debug regression then passes 264/264 in 88.48 seconds.
After rebuilding, the original entity-save group, existing graph-saver ownership
and save-preparation ownership checks pass 3/3 under ASan/UBSan with leak
detection and 3/3 under OpenWrt MIPS/QEMU. Pinned Rust, source inventory,
registered dispositions, public-export locators, compiled runtime layer separation,
540-file clang-format 18.1.8 and whitespace checks remain separate gates.

```sh
cmake --build build --target teaql_original_entity_save_tests \
  teaql_graph_saver_tests teaql_save_preparation_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_entity_save_contracts|graph_saver_context_contracts|save_preparation_context_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib entity_save --locked --offline
```

Use `build-sanitize` with leak detection/UBSan halt-on-error, or
`build-wndr4300-memory` with its registered QEMU emulator, for the selected
C++ checks. Physical-router and complete six-stage acceptance remain unproven.


## Complete entity-status API and structured error diagnostics, 2026-10-07

The complete production body of pinned `entity_status.rs` has been reviewed,
including derived traits and Display rather than only the sixteen source tests.
The standalone entity-status responsibility group is accepted. Runtime/save
composition remains a separate contract.

| Source requirement | Implementation and proof |
| --- | --- |
| Seven EntityStatus variants and four EntityAction variants | Native scoped enums retain every variant; the live original diagnostic corpus covers all 28 status/action pairs. |
| next | All eleven valid and seventeen rejected transitions match live source calls, including complete rejection text. C++ std::invalid_argument adapts the source Result<String> error. The original seven-step lifecycle retains every intermediate state. |
| need_persist, is_new, is_updated, is_deleted, is_recover | Every predicate is compared for all seven states with live Rust; no state or deleted-status distinction is omitted. |
| Default | EntityStatus value-initialization yields New, matching actual Rust Default. EntityAction has no source Default contract; callers select it explicitly. |
| Clone/Copy/PartialEq/Eq/Hash | Native enum copies preserve exact variant identity; actual unordered sets retain all seven/four distinct keys and deduplicate their copies. Trivial-copy static assertions and standard enum hashing map these source traits; identical Rust/C++ hash bytes are not promised. |
| Debug and Display | Status/action Display and four derived Debug modes match all 28 original pairs. Dedicated entity_status_debug.hpp is public through runtime.hpp; Debug does not substitute generic integer formatting. |
| Protocol and memory | Source types have no serde derive: no serialized enum protocol is invented. Scalar values allocate no Context-owned storage. Transition error exceptions are native C++ values; other runtime structured errors remain scoped. |

The broader error review found a missing public derived Debug capability.
`runtime/error_debug.hpp` now formats every RuntimeError and ContextError variant
and generic DataServiceError envelopes. It retains variant names, field order
and nested payloads rather than forwarding Display strings. Dedicated nested
formatters preserve SqlCompileError's twelve variants and CheckResult's rule,
ObjectLocation segments, input/system values and all three optional string
fields. Runtime and SQL facades expose the appropriate formatting headers.
Generic executor payloads use ADL or an explicit borrowed formatter; no string
conversion erases the concrete error. Automatic Debug is constrained by the
executor payload's own Debug capability. Compile-time positive/negative checks
accept String and reject a Display-only payload; the explicit borrowed formatter
still formats an otherwise unsupported concrete type. Invalid C++ discriminants are rejected
before indexing variant names.

The new source-call corpus has 1,174 actual pinned inputs. It expands the original
1,134 check/error inputs with full compact/pretty/lower-hex/pretty-upper-hex Debug,
twelve Context-error inputs and all 28 status/action pairs. Native and MIPS each
match all 1,174 live-original observations. Native tests additionally format a
concrete executor payload, inherit its numeric mode, and retain complete 4KiB
check messages, raw input and source paths in Global after actual User deletion.
Default PMR is disabled in those ownership tests. This raw diagnostic API is
separate from safe log projection; it intentionally preserves payloads.

### Target usize observations

The probe reports its actual sizeof(size_t) through the configured emulator.
The original fixture converts u64 location indexes with `as usize`; the C++
fixture uses the corresponding target-size cast. Eighty-six observations differ
between native 64-bit and MIPS O32. Golden `expected_32` entries are produced by
calling the actual unchanged Rust methods after normalizing **only** those input
indexes to target usize. Runtime check lists are normalized recursively at the
same LocationSegment boundary. Other integers, strings and outputs are not
rewritten. The comparison checks the golden target inputs against live original
calls when --rust is supplied. This is a target-width method oracle on the host,
not execution of Rust on MIPS. Both the existing 1,134-input and new 1,174-input
checks now use the actual target width; unchanged observations reuse the native
golden rather than duplicate storage.

The four-check native selection passes in 1.48 seconds. After the additional
automatic-Debug capability constraint, rebuilt four-check selections pass under
ASan/UBSan with leak detection (2.86 seconds) and OpenWrt MIPS/QEMU (1.89 seconds).
All sixteen original entity-status tests also pass unchanged.
The completed final native consumer build and full Debug regression pass
277/277 in 61.52 seconds, including every new diagnostic and capability check.
Pinned-source, strict source dispositions, inventory, public exports, compiled
layers, 559-file clang-format 18.1.8 and whitespace checks pass separately.

The error responsibility group remains partial: a compile-time reproduction
shows DataServiceError<noncopyable executor error> advertises an allocator-aware
copy despite the executor payload being noncopyable. This generic capability
bound needs repair before the complete error group can be accepted. Derived
Debug and passing String-error tests do not prove move-only executor support.
Full data-service propagation, resource lifetimes and six-stage acceptance also
remain separate. No production dependency or model-generator change was added.

```sh
cmake --build build --target teaql_runtime_check_tests teaql_runtime_check_probe \
  teaql_original_entity_status_tests
ctest --test-dir build --output-on-failure \
  -R '^(runtime_check_error_contracts|runtime_check_rust_parity|runtime_error_debug_rust_parity|original_runtime_entity_status_contracts)$'
python3 tests/parity/compare_runtime_error_debug.py --cpp build/teaql_runtime_check_probe \
  --rust build/rust-parity/debug/runtime_check_oracle
python3 tests/parity/compare_runtime_error_debug.py --cpp build-wndr4300-memory/teaql_runtime_check_probe \
  '--cpp-emulator=/usr/bin/qemu-mips;-cpu;74Kf' --rust build/rust-parity/debug/runtime_check_oracle
```


## Generic error capability repair checkpoint, 2026-10-07

This is an earlier checkpoint. The SQL completion integration below and the
later complete native 279/279 regression supersede its pending-build and SQL
adapter findings; the complete runtime group is still partial.

The two error envelopes now condition their allocator-aware copy constructors
on the concrete executor/transport payload's domain_copyable capability.
The source runtime and SQL error types require Display/Debug/Error rather than
Clone. C++ previously advertised an allocator-aware copy for every E, so moving
an envelope could instantiate an invalid copy of a move-only payload. Both
DataServiceError and SqlExecutorError now report the correct capability.
Nested Result and error envelopes propagate it without blanket Clone bounds.
Copyable String errors retain existing explicit cross-domain copies.

`tests/error_generic_tests.cpp` supplies a genuinely noncopyable, scoped native
error with a 4KiB message and exactly-once cleanup counter. It checks both wrappers,
nesting through SQL -> runtime -> Result, same-domain construction/assignment,
and rejection of foreign moves/assignment while preserving both sources and
previous destinations. Context close destroys the three transferred native
resources exactly once and returns all User bytes. Compile-time assertions reject
copy capability for the leaf, both envelopes, their nesting and enclosing Result;
positive String-error assertions preserve existing copy capability. The original
minimal failing compiler reproduction now compiles successfully.

RuntimeError, ContextError, DataServiceError and SqlExecutorError additionally
expose the source Error trait's default empty source chain. The wrappers do not
invent cause forwarding merely because they contain a nested error. Native
checks assert nullptr for all four classes. These are scoped typed C++ error
carriers; formatting into a caller-selected resource adapts Display, rather than
installing an owning std::exception cache or extending a Context lifetime.

The initial new native group and five-check sanitizer/MIPS selections pass.
After completing the empty-source API and correcting the SPI test selector,
final selected builds and the complete native consumer rebuild are in progress.
The latter has 546 affected tasks, so the earlier 277-check full checkpoint is
historical evidence until this rebuild and its following regression finish.
No new production dependency or model-generator change is introduced.

This repairs the carriers, not every asynchronous adapter. The current SQL query,
mutation and stream adapters contain clone_in calls on const completed transport
errors. Move-only transport-error propagation through those adapters still needs
explicit completion ownership/consumption work; carrier tests do not establish
that wider executor contract. Keep the SQL executor/transport responsibility
and complete stages open until those real generic paths are verified.

```sh
cmake --build build --target teaql_error_generic_tests teaql_runtime_check_tests \
  teaql_runtime_check_probe teaql_executor_tests teaql_sql_transport_tests
ctest --test-dir build --output-on-failure \
  -R '^(error_generic_context_contracts|runtime_check_error_contracts|runtime_error_debug_rust_parity|runtime_check_rust_parity|executor_spi_contracts|sql_transport_contracts)$'
```


## Noncopyable SQL completion integration checkpoint, 2026-10-07

The merged native consumer rebuild and subsequent header refresh now complete
with 279/279 passing in 67.99 seconds. Pending-build descriptions below are
historical. The selected sanitizer/MIPS evidence remains scoped to the named
checks; it is not full runtime acceptance.

The carrier copy-bound repair exposed further source-fidelity work: adapters
cloned errors from const borrowed child completions although the source future
and stream interfaces yield owned values. Explicit AsyncOperation::take_result
and AsyncStream::take_item now transfer completed payloads without Clone. The
[precise lifetime contract](context-lifetime.md#explicit-asynchronous-completion-consumption)
records borrow invalidation, Ready interpretation, last-item provenance, slot
retirement and callback ordering. The helpers copy existing copyable values and
take noncopyable values; they do not add shared ownership or cross-domain moves.

SQL query, diagnostic transport, mutation/readback/batch/journal, repeated fetch,
transaction begin/terminal, mapped/diagnostic/buffered streams now use the
appropriate completed error/result transfer. Basic runtime query/mutation and
stream mappings use the same boundary. Advanced runtime graph/cache/enhancement
composition requires its own generic review; the absence of remaining SQL clone
sites alone is not that acceptance.

`tests/sql_move_only_tests.cpp` instantiates the actual SqlDataServiceExecutor
with a genuinely noncopyable scoped transport error. It validates query, mutation,
failed begin, transaction query, failed commit/rollback with consumed access,
default repeated-query short circuit and native streaming through both mapping
and diagnostics. The diagnostic emits one Failure event and no duplicate output.
Every produced native error is destroyed once, and actual User deletion returns
all User bytes. Explicit future/stream consumption checks reject pending and
second takes, reject repoll after take, retire source item identity, preserve EOF
and avoid repeated finish/cancel hooks. The 4KiB leaf and cleanup instrumentation
are shared with the preceding carrier contracts; no production dependency is added.

The new SQL group passes on native after rebuilding, including actual transaction
terminal paths. The final five-check selections pass under ASan/UBSan with leak
detection (0.51 seconds) and OpenWrt MIPS/QEMU (1.07 seconds).

The initial added test failed under both configurations because its local PMR
Value vector and compiled-query buffers were destructed after user.reset(). MIPS
GDB identified Value-vector destruction in actual_sql_adapters; ASan reported
heap-use-after-free at the same owner boundary. The fixture now ends all local
PMR lifetimes before deleting its UserContext. Rebuilding and rerunning proves
the corrected lifecycle; no check was removed or weakened. The Context cannot
make externally scoped containers outlive their allocator. The earlier native
546-task build was deliberately interrupted at 112 completed tasks to fold these
foundational ownership fixes into a single final consumer rebuild; its exit was
an explicit SIGINT, not an observation timeout or a test failure. The merged
native consumer build and following full CTest run are now in progress. Earlier
277/277 evidence remains historical until that current run completes. The prior
six-check carrier selection passed ASan/UBSan in 2.16 seconds and MIPS/QEMU in
3.23 seconds before these additional async/SQL changes.

No full-stage, physical-router or complete generic runtime acceptance is claimed.

```sh
cmake --build build --target teaql_sql_move_only_tests teaql_error_generic_tests \
  teaql_async_tests teaql_sql_query_tests teaql_sql_transport_tests
ctest --test-dir build --output-on-failure \
  -R '^(sql_move_only_context_contracts|error_generic_context_contracts|async_context_contracts|sql_query_executor_contracts|sql_transport_contracts)$'
```


## Noncopyable enhanced-query integration, 2026-10-07

Pinned teaql-data-service/src/lib.rs:142-173 requires Error/Send/Sync, not Clone.
The owning futures in teaql-runtime/src/data_service/resolved.rs:1276-1450
propagate enhanced-query, ID-set and recursive hydration failures using `?`.
The C++ generic API must therefore retain a concrete noncopyable executor error.

A valid instantiation using the existing scoped MoveOnlyError failed to compile
because the adapters attempted an allocator-aware copy of DataServiceError.
Thirteen completed-error transfers in entity_query, enhanced_entities,
id_set_page, relation_execution, relation_hydration and relation_enhancement
now use copy_or_take_error. Detached child pointers are preserved locally before
transfer. Copyable completions retain their former stable borrows; noncopyable
errors transfer ownership from the completed child in the same Context.
Success rows, query snapshots, phase ordering and telemetry responsibilities
are unchanged. Taking a completed error does not invoke another terminal hook.

The new registered runtime_move_only_query_contracts uses the actual enhanced
entry and a native executor with a 4KiB noncopyable error. Twelve scenarios cover
direct fetch, ID enumeration/page fetch, compact flat and recursive relations,
embedded fallback, aggregate, child and object-group queries, explicit repeated
cancellation, wake forwarding and actual User deletion while nested work is
pending. It checks the concrete error tag and complete payload/domain, stable
outer completion borrow, immediate short circuit, exactly-once native hooks and
resource destruction, and zero retained User bytes. All local PMR owners end
before deleting User. The default PMR allocator is disabled throughout.

The initial ten-scenario native group passed after the repair. The rebuilt final
twelve-scenario MIPS/QEMU group and its seven related checks pass 8/8 in 1.56
seconds. The rebuilt eight-check ASan/UBSan selection with leak detection also
passes 8/8 in 0.73 seconds. The final expanded native consumer build and complete
regression subsequently pass 280/280 in 66.80 seconds, including the final
twelve-scenario fixture. The earlier 279/279 result predates these thirteen
adapter changes. The Rust source lock, 563-file clang-format write/check, layer
boundary and complete registered source-disposition checks pass. This is
integration work, not acceptance of every runtime completion path or the full
stages.

### Generic completion paths identified at that checkpoint

The following findings are addressed by the later whole-result integration
below; they describe the preceding checkpoint rather than current copied-error
sites. Complete module/stage acceptance remains separate.

The next source review must cover the still-copying public list/observe/Context
query adapters in context_query.hpp and EntityBasicListOperation in
entity_query.hpp, the mutation counterparts in context_mutation.hpp and
entity_mutation.hpp, and graph/ledger planning/execution, graph-fix scope and
provider-error formatting. Some graph/audited paths deliberately convert errors
to RuntimeError; that source adaptation must not require copying the native
payload merely to format it. RuntimeError-only copies are legal and must not be
removed based on a text search alone. Each generic entry needs actual noncopyable
instantiation and failure/cancellation coverage before its module can be accepted.


## Whole-result runtime completion integration, 2026-10-07

The complete source review found blanket copying beyond enhanced queries:
ObservedDataServiceOperation, ContextQueryOperation, EntityBasicListOperation,
ContextMutationOperation, EntityMutationOperation, graph planning/execution,
ledger execution, LedgerSaveExecution, GovernedOperation and GraphFixedOperation.
A valid native executor reproduction failed to compile for row vectors, both
SmartList result types, mutation counts and DataServiceError itself. These errors
were actual missing allocator-copy capabilities, not invalid fixture API calls.

Whole-result adapters now copy_or_take_completed; error-only adapters now
copy_or_take_error. Each detached child is retained locally before taking it.
The borrowed execution error in TransactionalGraphSave remains valid while
rollback is pending. The source provider-error-to-RuntimeError boundary now
accepts an owned payload and formats it without requiring Clone. RuntimeError
copies, governance snapshot copies and retained-ID/query/model snapshots remain
legal source/domain operations; they were reviewed individually rather than
removed by search.

Pinned entity_save.rs:71-207 preserves distinct graph and ledger savers, planning
and governance before begin, execution in a transaction, authoritative ledger
root reread before commit, rollback before reporting an execution error, and
terminal rollback-error precedence. This change does not unify those paths.
The source executor trait and both savers have no Error: Clone requirement.

The new registered runtime_generic_completion_contracts passes on native with
**78** scenarios: 54 row/list/typed-query and six-command cases across Base,
Context and Entity layers, each with failure and Pending/User-deletion paths;
two combined governance/graph-fix completion scopes; ten graph/ledger planning,
mutation and authoritative-read paths; and twelve transactional graph/ledger
cases covering begin/commit/rollback failure, rollback precedence, five suspended
transaction stages, ledger write/classification/authoritative reread failures.

The fixture disables default PMR, retains a 4KiB concrete noncopyable error,
checks stable outer borrows and full formatting, distinguishes native error
labels for terminal precedence, forwards wakes, verifies scope activation only
inside polling/cancellation, actually deletes User and checks all native errors
and terminal callbacks exactly once. Local PMR owners end before reclamation.
Native resources and Context blocks are not kept alive by shared ownership.

The rebuilt eleven-check MIPS/QEMU selection passes 11/11 in 2.18 seconds,
including all 78 scenarios. The rebuilt ASan/UBSan selection with leak detection
passes 11/11 in 1.02 seconds. The complete native consumer build and regression
now pass 281/281 in 67.65 seconds. This is the checkpoint before subsequent
Business ID API changes. The immediately preceding 280/280 in 66.80 seconds is a
completed checkpoint before these additional public-header changes. Pinned-source
and 565-file clang-format write/check passed. No production dependency or generator
change is introduced. Final integration uses those completed builds; the initial
native group alone does not cover every configuration or complete module.

### Complete runtime error source-surface review

The entire production body of pinned error.rs has been read against
runtime/error.hpp, context_error.hpp, error_debug.hpp and src/runtime/error.cpp.
SQL executor error envelopes are a separate module; their nested behavior is
integration evidence, not an expansion of this source group.

| Source requirement | Concrete C++ mapping and reviewed evidence |
| --- | --- |
| RuntimeError | All fourteen variants, String/SqlCompileError/Vec<CheckResult> payloads, and both two-field struct variants. Exact Display branches and original structured Debug are retained. Explicit owner factories replace Rust variant construction. |
| From<SqlCompileError> | RuntimeError::sql_compile takes the concrete compile payload and explicit destination; it does not stringify it or conflate it with Graph. |
| ContextError | All three variants and Display/Debug branches. Static MissingTypedResource names are copied into the explicit owner; allocation changes do not alter the name or classification. |
| DataServiceError<E> | Runtime/Entity/Executor remain distinct. Executor payload is concrete and may be noncopyable. Display delegates to its formatter; derived Debug requires the payload's Debug writer or an explicit borrowed formatter. A Display-only negative capability check remains. |
| Error defaults | All three source impl blocks use the default empty cause chain. Native source() returns nullptr; no nested-cause forwarding is invented. |
| Other traits/protocols | The source derives Debug only, without Clone/Eq/Default/serde. Conditional explicit domain copies support C++ borrows but do not impose Clone on execution. No source wire protocol is invented. |
| Ownership and integration | Explicit nested diagnostic relocation/Global retention after actual User deletion, concrete move-only envelopes/Result nesting, same-domain transfer, rejected foreign transfer preserving both operands, exact cleanup and the named runtime completion paths exercised by the two new native fixture groups. |

The 1174 actual-source diagnostic inputs remain the Display/Debug evidence,
including target-width location input adaptation. Those counts do not prove
arbitrary user formatter behavior or the surrounding runtime privacy/default/
policy composition. After final integration, fresh native and MIPS probes each
match all 1174 live-original diagnostic inputs. The source error responsibility
group is accepted on its complete requirement table and scoped ownership evidence.
The complete six-stage objective remains open.

## Complete masking reconciliation, 2026-10-09

All 634 source lines, macro/helper fixtures and assertions were reconciled with
the installed C++ port again. Fresh unchanged Rust passes all 35 indexed cases;
the ten-row TSV remains byte-identical. The 34 SQLite-applicable cases preserve
all privacy/provenance, immutable execution-value, retained clone, idempotence,
misclassification and unknown-dialect assertions. Sixteen workers each allocate
in a separate arena and all join before source reclamation. PostgreSQL ARRAY
rendering remains a single explicit backend exclusion, not a positive C++ pass.

Two native assertions now format the whole SqlLogContext, matching the original
Debug privacy boundary rather than inspecting only private fields. The existing
production Debug writer is used; no production masking logic changes. Current
native full and sanitizer/MIPS source consumers are in validation, so the source
group stays partial pending those results. Evidence is retained under
measurements/parity/2026-10-09-masking-source-review.

## Complete current source acceptance, 2026-10-09

Final native integration passes 371/371 in 210.67 seconds after its 656-task
consumer rebuild, with 418 nonempty compiler dependency records and no-work
before/after testing. All corresponding fresh sanitizer/MIPS consumer selections
pass; complete source/body/helper/input/assertion or API/default/trait/protocol/
ownership review is recorded above. The 34 SQLite-applicable masking source tests
are accepted within their explicit C++ ownership and SQLite scope. This does
not accept broader Context/services, graph/event/log privacy, runtime-root or
complete stages 4–6. Current full output is retained in
measurements/parity/2026-10-09-checker-direct-callback-review.
