# Core API capability acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
This checkpoint repairs two confirmed public API omissions. It does not complete
the remaining core, runtime or six-stage acceptance audits.

## Timestamp ordering and hashing

Timestamp now preserves the source's signed i64 millisecond ordering, with
constexpr equality and three-way comparison. Standard ordered and unordered
containers can use the value directly. `std::hash<Timestamp>` delegates to the
hash of its complete signed scalar; equal values created through signed and
wrapping unsigned constructors have equal hashes. No allocation or Context
storage is involved, and the eight-byte representation remains unchanged.

The original public Hash implementation is observed with a recording Hasher
that accepts exactly one signed i64 write. The corpus contains 67 Timestamp
inputs and 16,698 original pairwise comparisons, including both signed limits,
32-bit and 53-bit boundaries, negative values, duplicates, empty input and seed
505 random values. Default, sorted values and hash inputs match live Rust calls.
Standard bucket hashes are not advertised as Rust DefaultHasher outputs or a
persistent wire format. The existing timestamp JSON/date conversion contract
is unchanged.

## Dynamic audit-mask field collections

EntityDescriptor's mask builder accepts runtime input ranges whose elements
convert to string_view, including PMR/native string vectors, spans and range
views. The initializer-list API delegates to the same implementation.

The builder first copies all names and nested characters into its own allocator.
Only after that succeeds does it swap the complete list and set the independent
declared-policy flag. It preserves order, duplicates, empty names and explicit
empty declarations. It can safely read a span or reversed view of its current
fields while replacing them. Input mutation cannot affect the new list, and
cross-domain input does not retain its original allocator or characters.

The corpus contains 192 unchanged original EntityDescriptor public-call inputs,
covering unspecified versus declared initial policies, replacements, explicit
empty lists, repeated names, Unicode, embedded zero bytes, long strings,
self-copy results, default table naming and clone equality. The original Rust
owned Vec builder corresponds to C++'s explicit destination-domain snapshot;
copying temporary native inputs never adds a hidden ownership lifetime.

## Memory and failure contracts

Native tests disable the default PMR resource and allocate metadata through
UserContext, including Global delegation. Long fields survive actual source
User deletion and explicit Global rebinding. Reversed and ordinary self-aliasing
views preserve complete content. Character/list storage consumes bytes without
consuming another managed object identity.

At successive upstream allocation points, fault injection checks both initially
declared and legacy policies. Failure preserves the previous field list and its
flag; retry publishes the complete new policy. Previously published Global
objects remain valid, no source User content is retained, and destroying Global
returns every successful and abandoned block. Tests also check Timestamp map
ordering, unordered lookup, duplicate counts and constructor/hash consistency.

## Reproduction

On 2026-10-07 the complete rebuilt native Debug suite passed 191/191 checks.
The selected Core API contracts, original-call comparisons and core-model tests
passed 3/3 under ASan/UBSan with leak detection and 3/3 in the OpenWrt MIPS
cross-build under QEMU. All 259 observations also match live original Rust on
both native and MIPS runs. The unchanged original core library tests pass 65/65.
Pinned upstream, export-name, inventory and clang-format 18.1.8 checks passed.
These checks do not complete wider API audits or the remaining migration stages.

```sh
cmake --build build --target teaql_core_api_tests teaql_core_api_probe
ctest --test-dir build --output-on-failure \
  -R '^(core_api_contracts|core_api_rust_parity|core_model_contracts)$'
python3 tests/parity/compare_core_api.py --cpp build/teaql_core_api_probe \
  --rust /home/philip/shared-cargo-target/debug/core_api_oracle
```

The 259-case corpus is generated only from original public calls, through
`tests/parity/generate_core_api.py`. The source reference remains unchanged.
No production dependency, alternate backend or generator change is introduced.
Physical MIPS-device execution remains unverified.

## Original functional contracts

Seventeen additional source tests now have direct C++ fixture ports and
individual dispositions in source-test-dispositions.json. They invoke the
existing production APIs; the port contains no alternate merge, trim, expression
evaluation or SQL implementation.

| Source | Reviewed original assertions |
| --- | --- |
| safe_expression.rs, four tests | Stored i32 root 2 produces 6; supplied root 4 produces 12. Root 5 reaches the optional mapper once and never reaches the later mapper. Present 7 skips fallback/error factories; missing evaluation invokes the original fallback/error branches. Text teaql and missing Text retain exact null/value callback counts and optional presence. |
| list.rs, two tests | Complete original Item vectors retain existing positions, append new IDs in order, and collapse repeated incoming IDs to a single appended position with the final value. Consuming into_vec uses full length/order/value equality. |
| serde_utils.rs, four tests | Original spaces/newline/tab inputs retain complete required/optional JSON and decoded values. None is a present JSON null; whitespace-only optional text is Some(empty), not None. The handwritten two-field fixture calls the real trimmed helpers. |
| web.rs, one test | Success/failure/empty-list constructors keep all original result/status/message/version/count/data assertions. Success JSON is an object with the original keys, empty data/facets and an omitted absent message. This is a core data model, not a web server integration. |
| lib.rs, six tests | Complete column-comparison/Soundex/string-match/explicit-large-IN AST equality; all four ordinary IN operators at the original U64 20/21-value boundary; original SubQuery kind/left/op/entity/single projection using handwritten OrderLine metadata. |

The Item's string_view fields refer only to static source literals, matching
Rust's static string fields. All dynamic strings, lists, callback state and JSON
use the invoking UserContext, with default PMR disabled. Counter borrows are
invoked synchronously before their scope ends. The handwritten descriptor
retains the original table, scalar fields and detached Product relation; the
generator and reference source remain unchanged.

On 2026-10-07 the rebuilt original_core_functional_contracts check passes on
native Debug, ASan/UBSan with leak detection and OpenWrt MIPS/QEMU. Unchanged
Rust passes all 65 core library tests, including these seventeen source tests.
The prior complete native checkpoint is still 242/242; the newly registered
test is subsequent scoped evidence, not a claimed full 243-check rerun.
Original-call corpora and ownership acceptance remain complementary checks.
The current disposition ledger has 120 covered, 324 pending, two backend
exclusions and five external exclusions. Complete migration acceptance remains
open, including the other handwritten entity mappings and core defaults.

```sh
cmake --build build --target teaql_original_core_functional_tests
ctest --test-dir build --output-on-failure \
  -R '^original_core_functional_contracts$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-core --lib --locked --offline
```

## Root composition and query source contracts

The remaining five root lib.rs tests and all six query.rs tests now have direct
ports in composition_contracts.cpp. All 26 root-library source tests therefore
have individually reviewed dispositions. This is fixture acceptance; it does
not prove every core public member, trait, reexport or composition capability.

The query-builder fixture preserves every original builder call and typed
binding, then compares complete ordered projections, named expressions, sort,
groups, aggregates and relations. Optional cache settings, exact Having AST,
slice limit/offset and the outer Or variant remain distinct assertions.
The two entity-list fixtures preserve loaded versus unloaded inputs, typed
U64 IDs/I64 versions, consuming records, all metadata/JSON helpers, ordered
sets, typed identity keys, grouping, merge, retain and borrowed iteration.
They reuse the handwritten Order model rather than implement list behavior.

The expression fixtures preserve the source's Clone BaseEntity root, name and
loaded single line, including Some IDs/versions, missing get(4), size, lazy
fallback and callbacks. Some(empty String) remains distinct from None. The
private test entity's unused record materializer remains explicitly unused,
matching the original helper; no application capability is claimed for it.

Query validation uses the original independent inputs: default list limit
Some(10000), rejected 10001, a fresh 10001 query with hard limit 20000, explicit
recent-orders cursor/ID-set options, the single-space namespace, zero max_ids,
and fresh Trip inputs for None/Some(32)/Some(0). C++ invalid_argument adapts
the source Result/panic boundary; both should_panic fixtures retain their exact
source rejection text. Explicit zero is not collapsed into missing.

The rebuilt original_core_composition_contracts check passes on native Debug,
ASan/UBSan with leak detection and OpenWrt MIPS/QEMU. Unchanged Rust again passes
all 65 core library tests. Default PMR is disabled, and every dynamic value uses
the invoking UserContext. No generator or production dependency changed.
The ledger now has 146 covered, 298 pending, two backend exclusions and five
external exclusions. The full native 242-check checkpoint predates these later
test additions; this selection does not claim a full 245-check rerun.

```sh
cmake --build build --target teaql_original_core_composition_tests
ctest --test-dir build --output-on-failure \
  -R '^original_core_composition_contracts$'
```

## Handwritten entity-mapping source fixtures

Ten root-library contracts now have direct handwritten fixture counterparts in
teaql-core/entity_mapping.cpp and entity_mapping_fixture.hpp. These replace the
private Rust test models' derive output within the selected tests; they do not
introduce or claim a C++ generator or universal automatic entity mapping.

The fixtures retain Order's explicit orders/display_name/max-length metadata;
TypedNumber's Decimal precision/scale and checked i32/u32 narrowing; missing and
explicit Null scalar defaults versus invalid present types; Product relation
keys and attach/delete-missing flags; actual default and ordered registration;
boxed optional relations serialized as Value::Object; default table naming;
distinct Json/Date/Timestamp conversions; dynamic Object/I64/F64 values and
BaseEntityData's record roundtrip. Error outcomes and optional presence remain
explicit. The fixture decoder uses narrow resource parameters because it only
maps fields and allocates output; it needs no runtime policy or logging capability.

Order's allocator-aware Clone copies complete characters into Global through
UserContext. Its long retained name remains independent after source mutation
and actual source User deletion. Product and boxed fixtures retain the source's
lack of Clone; Context relocation cannot silently copy or share those payloads.
ScopedBox owns the nested Product destructor within the same lifetime. Default
PMR is disabled during all contracts. This is scoped fixture evidence; complete
generic/generated model acceptance remains a separate gate.

The eight mapping tests and two dynamic/record tests pass after rebuilding in
native Debug, ASan/UBSan with leak detection and OpenWrt MIPS/QEMU. The paired
mapping/presentation selection passes 2/2 in each configuration; unchanged Rust
passes all 65 core library tests. Pinned source, inventory and formatting checks
pass. The latest full native checkpoint remains 242/242 and predates these test
additions; no full 244-check rerun is claimed here.

```sh
cmake --build build --target teaql_original_core_mapping_tests \
  teaql_presentation_api_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_core_entity_mapping_contracts|presentation_api_contracts)$'
```

## Published evaluation module correction

The pinned core `lib.rs` publicly declares `eval` after its test module. The
previous module map and three reference-only test dispositions were incorrect;
this is an implementation omission, not an unpublished source oracle. The map
now restores this Stage 2 responsibility and the core facade exposes
`teaql::core::eval`. Export verification checks all six public core module
namespaces as well as root reexport names. The disposition verifier rejects
reference-only exclusions for directly published root modules; an isolated
negative control confirms that the old erroneous eval exclusion now fails.

`LoadState` retains all five variants: NotLoaded, Partial, PartialCompact,
SharedColumns and FullyLoaded. Partial has set equality and removes duplicates;
PartialCompact and SharedColumns preserve sequence order and duplicates. The
default is NotLoaded, and FullyLoaded accepts every field name. Compact field
storage uses standard PMR vectors rather than adding smallvec as a dependency.
Its inline-storage optimization is not promised by this C++ implementation.

SharedColumns borrows the existing immutable Context-owned ColumnLayout within
the same scope or from Global into User. Same-domain clones share that layout.
Explicit cross-domain retention copies the layout and every character into the
destination. User storage cannot be retained by a sibling User or Global borrow.
These raw layout borrows, like CompactRow's, require the layout to remain alive;
they do not intercept arbitrary pointer access or individual manual destruction.
Context close invalidates the enclosing checked handles without Arc/shared_ptr
extending memory lifetime.

`EvalResult<T>` preserves Value, Null and NotLoaded as distinct variants. Its
consuming map/and_then support move-only payloads and one-shot callbacks; absent
and missing values never invoke the callback. Missing results preserve their
failed node, and composition retains the source's empty/equal/nested path rules.
Conditional allocator-aware copies retain the original Clone capability bound.
Diagnostic strings always use the chosen Context. Debug output is raw source
diagnostics, not a safe log projection.

The JSON codec keeps the original externally tagged enum representation and
acceptance behavior. Generic payloads provide their own encoder/decoder callback.
In particular, serde_json rejects an array as the NotLoaded struct-variant
payload; the C++ codec does too. The original three tracking-chain fixtures run
through the actual implementation, and 607 public-call inputs match live Rust
for membership, clone equality, serialization/deserialization, callback counts,
path composition and four Debug modes. Partial HashSet JSON is normalized by
set ordering; multi-element HashSet Debug has no fixed Rust ordering and is not
compared as a deterministic string.

The two named evaluation checks passed on native, ASan/UBSan with leak detection
and OpenWrt MIPS/QEMU after rebuilding. Native and MIPS each match 607/607 live
original inputs; unchanged Rust passes its three eval tests. Ownership contracts
disable default PMR, share same-domain layouts, reject sibling borrows, consume
non-copyable payloads and retain complete long diagnostic strings/layouts in
Global after actual source User deletion. These are scoped checks; complete
all-stage acceptance remains open.

After exposing the module through core.hpp, the complete native consumer rebuild
finished and all 242/242 CTest checks passed. Layer validation confirms the
compiled runtime still has no SQLite C API calls. Rust lock, source inventory,
public export/module, disposition and 515-file formatting checks pass. No
production dependency, generator or reference source was changed.

```sh
cmake --build build --target teaql_eval_tests teaql_eval_probe
ctest --test-dir build --output-on-failure \
  -R '^eval_(context_contracts|rust_parity)$'
python3 tests/parity/compare_eval.py --cpp build/teaql_eval_probe \
  --rust build/rust-original-roundtrip/debug/eval_oracle
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-core --lib eval::tests --locked --offline
```


## Complete eval module contract review, 2026-10-07

The complete pinned `teaql-core/src/eval.rs` production body (lines 1–95) has
been reviewed independently of its three source tests. This responsibility group
is accepted under the agreed explicit Context ownership and native JSON codec
mapping. This accepts this module's defined surface, not every module, stage,
third-party serializer or arbitrary C++ borrow. There are no source constants,
trait defaults or public methods beyond the items below.

| Source contract | Concrete C++ implementation and authoritative evidence |
| --- | --- |
| LoadState: five variants and is_loaded | `eval.hpp` Kind/factories/is_loaded preserve NotLoaded=false, FullyLoaded=true, exact membership and empty-name/Unicode behavior. The actual pinned-method corpus exercises all five variants. |
| Default | The native probe constructs a default LoadState with an explicit allocator and compares its wire value with actual Rust LoadState::default(): NotLoaded. EvalResult has no source Default and no default C++ constructor. |
| PartialEq/Eq | A 23-by-23 live-original matrix covers all variants, empty values, field permutations, duplicates and Unicode. Partial compares set membership; compact/shared states compare ordered sequences; different variants remain unequal even with equal fields. |
| LoadState Clone | Same-domain layout identity is shared; explicit cross-domain copies own the layout and characters. Clone equality is checked for every accepted corpus state. Native tests reject sibling User borrowing and retain Global copies after source User deletion. |
| LoadState Debug | Four exact Debug modes cover variants, Unicode, escapes and ordered payloads. Multi-element HashSet order is unspecified by Rust: JSON is set-normalized and deterministic string comparison is excluded only for that unordered set output. C++ emits the same set members with valid Rust Debug structure. |
| LoadState Serialize/Deserialize | Externally tagged JSON unit/newtype forms, five variant names, set deduplication and sequence preservation, malformed tags/cardinality/payload types are compared against actual serde_json::from_value/to_value calls. SharedColumns is decoded into an owner-local immutable layout. |
| EvalResult<T>: three variants and fields | Value(T), Null and NotLoaded{failed_node,attempted_path} remain distinct. Generic payloads retain their native type; the missing strings are explicitly scoped. No Eq/Default contract is invented for this source type. |
| EvalResult Clone/Debug | Conditional allocator-aware copying maps the source T:Clone capability to managed copyable payloads. Four Debug modes preserve Value/Null/NotLoaded and both named diagnostic fields. User-to-Global nested payload and missing-path copies are checked. |
| Consuming map<U,F:FnOnce(T)> | Actual Value callbacks run once; Null/NotLoaded never invoke them and preserve missing fields. Native tests consume a move-only payload and a noncopyable callback that moves its unique resource on invocation; no Clone requirement is added to map. |
| Consuming and_then<U,F:FnOnce(T)> | Callback Value/Null/NotLoaded, short circuits and empty/equal/nested attempted-path rules match the live source matrix. A foreign callback result is explicitly relocated into the input result domain, including nested container/string payloads and missing diagnostics, before composition. |
| EvalResult Serialize/Deserialize | Externally tagged variants and required string fields preserve source JSON acceptance, including rejection of array NotLoaded struct payloads and acceptance of unknown struct fields. Generic codec callbacks execute only for Value; native integer payloads, codec errors and destination-owned JSON supplement the live String oracle. |

The expanded corpus contains **1,136 actual pinned Rust inputs**, including the
529 state-pair observations. Native and MIPS each match all 1,136 live outputs;
ASan/UBSan matches the same committed live-original observations. The original
three tracking tests pass unchanged. Native ownership tests disable default PMR,
verify generic callback domain relocation, and retain full 4KiB nested strings
in Global after actual deletion of both source Users. The two CTest checks pass
2/2 on native (0.80s), ASan/UBSan with leak detection (1.81s), and OpenWrt MIPS/QEMU
(1.18s). The published `teaql::core::eval` alias and source lock remain unchanged.

The implementation deliberately uses PMR vectors for PartialCompact instead of
adding smallvec. Its inline-eight optimization is not an accepted promise.
SharedColumns uses a Context-owned layout instead of Arc, so no clone extends
Context lifetime. Pointer-valued generic payloads remain native borrows; explicit
Context allocation cannot prove their pointee lifetimes. Generic payload codec
semantics belong to that codec, not to EvalResult; this audit covers the wrapper
protocol and callback boundaries. The native structured decode-error adaptation
does not promise identical serde_json error text or non-JSON serializer formats.

Reproduce live observations after building the pinned source-call oracle:

```sh
CARGO_TARGET_DIR="$PWD/build/rust-parity" cargo build \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin eval_oracle --locked --offline
python3 tests/parity/compare_eval.py --cpp build/teaql_eval_probe \
  --rust build/rust-parity/debug/eval_oracle
python3 tests/parity/compare_eval.py --cpp build-wndr4300-memory/teaql_eval_probe \
  '--cpp-emulator=/usr/bin/qemu-mips;-cpu;74Kf' --rust build/rust-parity/debug/eval_oracle
```

The completed full native Debug regression passes 276/276 in 56.82 seconds
with four CTest workers. The strict source-test disposition gate, pinned-source,
inventory, public-export, compiled-layer and 555-file formatting checks pass.
No production C++ implementation, model generator or pinned source was changed;
the source-call oracle, test corpus recorder and native contract tests were
expanded.

The module-map accepted status records this manual requirement-level review.
Inventory validates its baseline and evidence locators; it cannot itself prove
the semantics in this table. Stage 2–6 acceptance remains open.
