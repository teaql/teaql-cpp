# SmartList public API acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
This checkpoint audits `teaql-core/src/list.rs`, preserving the complete list
model and its entity adapters. It does not establish acceptance of all core
models or the six-stage migration. The separate
[model Debug contract](model-debug-acceptance.md) supplies structural diagnostics.

## Public responsibilities

| Source responsibility | C++ boundary and evidence |
| --- | --- |
| Six public fields: data, optional total_count, aggregations, summary, recursive facets, is_loaded | All six remain distinct. Every differential step compares the complete recursive state. |
| empty / Default versus new / From<Vec> / FromIterator | Explicit owner constructor is unloaded. Owned-vector, span, and from_range construction are loaded, including empty input. |
| push / Extend / extend without Clone | Owned inputs support move-only elements; borrowed input copies into the owner. Arbitrary single-pass input ranges are accepted. |
| set / get / get_mut / first / last / len / is_empty | Ordered lookup, optional replacement result and bounds are preserved. Native checks include move-only replacement. |
| iter / iter_mut / as_slice / as_mut_slice / borrowed IntoIterator / Deref | Explicit spans and const/mutable begin/end retain borrowing and mutation. Native and differential tests use both named iteration and mutable slices. |
| retain | Stable filtering releases rejected ownership before the next callback. A callback exception preserves the visited survivors and unvisited tail; it invokes no further filters. Both assignable and non-assignable elements are covered. |
| Index / IndexMut | operator[] checks bounds using at; an invalid index throws instead of Rust's panic. Valid mutable/const access is compared directly. |
| with_total_count / total_count_or_len | Explicit zero and maximum u64 totals override length; ordinary list edits do not change totals or loaded state. |
| with_aggregation / aggregation / aggregation_json | Ordered Value map and independent JSON projection retain all variants and replacement behavior. |
| with_summary / summary / summary_json | Public summary field remains present; summary_value is the named lookup because C++ cannot give a field and method the same name. |
| with_facet / add_facet / facets / facets_mut / facet / facet_mut / remove_facet / take_facets | Named access and the public facets field expose the same map. Replacements, missing names, recursively nested metadata and consuming extraction are compared. |
| map / into_vec / From<SmartList> / owned IntoIterator | Explicit rvalue map and into_vec transfer elements without Clone. Consuming traversal uses the extracted owned vector, preserving the element owner. Map preserves all five metadata fields. |
| to_list / to_set | Borrowed callbacks may return move-only owned output. to_set sorts and deduplicates; foreign callback references are explicitly copied into the list owner. |
| identity_map / group_by | Ordered keys require ordering without Clone; elements retain the source Clone responsibility. Last duplicate wins in identity_map; groups preserve element order. |
| merge_by | Arbitrary owned input needs neither element nor key Clone. Existing duplicate keys replace the last existing position; repeated incoming keys have one appended position. |
| into_value | Loaded or changed lists emit Value::List. Some(empty dirty fields) is change intent. Every entity converts even when an unloaded unchanged list emits None. Exact short-circuit and conversion call order is compared. |
| into_values | Distinct MutationValues results preserve every metadata field; conversion order is compared. |
| ids / map_by_id / versions | IdentifiableEntity and VersionedEntity remain separate constraints. All Value ID variants, typed keys, duplicates and full i64 versions are compared. |
| Clone / PartialEq | Explicit destination-owner copying and structural equality cover data and all metadata. NaN retains source non-reflexive equality. |
| Debug | The public model Debug boundary retains complete fields and recursive facets, compact/pretty and inherited scalar flags. Custom borrowed element formatters need neither Clone nor assignment. A separate 169-call corpus compares 3,042 exact row/list outputs. |

The matrix records each public source responsibility, including inherited trait
behavior; it is not satisfied merely by an exported SmartList name. Builders
mutate and return the C++ list reference rather than consume the Rust value.
Consumed C++ objects remain valid moved-from objects: owned-vector construction
and into_vec empty their sources. Invalid rvalue set leaves its argument intact;
Rust's by-value argument cannot be inspected after the call. These are explicit
C++ lifetime/calling-convention adaptations, not new query or mutation semantics.

## Ownership and generic consumption

Allocator-aware local moves and assignment work with non-copyable elements.
Set, merge replacement and stable retain also work with non-assignable elements:
the fallback destroys and reconstructs the destination with a noexcept move.
This is a C++ object-lifetime implementation of Rust value replacement, not an
additional entity assignment requirement.
Foreign moves prepare a complete destination-domain copy; foreign move-only
lists or vectors are rejected before their source is consumed. Foreign retained
data never points into a shorter User arena. Cloneability is checked through the
allocator-aware domain-copy boundary, rather than ordinary deleted C++ copies.

Borrowed extension/merge ranges are snapshotted before growth or replacement,
allowing self/subrange/reverse views. Consuming the list's own backing vector
as incoming data is rejected: the same container cannot simultaneously be the
source owner and destination. Other explicitly moving aliased views must obey
ordinary C++ invalidation rules. These APIs do not provide raw-pointer lifetime
checking or a universal transactional exception guarantee for arbitrary custom
element moves and callbacks.

Mapper-produced values and keys move into their owner; borrowed callback
results copy into it. This retains Rust's lack of Clone requirements on new
outputs and on ordered/hash keys. Custom owning elements still must propagate
allocators correctly; an arbitrary pointer graph is not automatically deep-copied.

Native tests disable default PMR. They cover move-only elements and ordered/hash
keys, single-pass stream ranges, failed foreign copying without source loss,
same-owner vector transfer, safe aliased ranges, and exact all-block reclamation.
Nested Global list data, strings, summary, aggregation, facets and immutable row
layouts remain complete after actual source User deletion and explicit rebinding.

## Original-call corpus

`smart_list_api_cases.json.gz` records 526 original public-call inputs:

- 296 default/owned construction and operation sequences, including 256 seeded
  sequences of twenty operations each.
- 230 entity-adapter cases covering loaded/unloaded empty results, every Value
  ID variant, duplicate IDs, all new/deleted/None/Some-empty/Some-dirty flag
  combinations selected in the generator, complete i64 versions, recursive
  metadata and the complete observable conversion-call trace.

Fixed sequences cover every generic method in the matrix. Full recursive
metadata, typed values, JSON projections, optional results, counts and cloned
equality are compared after every operation. The generator calls the unchanged
original Rust SmartList methods. Its handwritten Entity fixture supplies only
the required SPI and records callbacks; it does not implement list decisions.
The two unchanged original list module tests also pass independently.

The complete corpus is losslessly compressed: 41,855,019 JSON bytes become
730,947 gzip bytes with fixed mtime. No operation, metadata or expected result
is removed. Python gzip is a standard-library test facility, not a C++ runtime
dependency. The comparator checks the pinned commit and can verify live Rust.
The source does not derive serde for SmartList; fixture JSON is an observation
format, not an invented SmartList wire protocol.

## Consuming iterator ordering repair, 2026-10-07

A new source-call review exposed an observable difference in `merge_by`: the
C++ implementation snapshotted all incoming items before applying any merge.
The original implementation requests an incoming item, computes its key and
applies it before requesting the next. A lazy iterator can observe callback
order or fail after part of the merge, so final successful data alone is too
weak to establish this contract.

The native reproduction failed before the repair on callback ordering. The
refreshed corpus retains all 526 previous inputs and adds two direct original
calls with a traced lazy owned iterator, including a caught iterator panic.
With original data `[1,2]`, incoming `[12,3,14]` and `item % 10` keys, original
callbacks are `key(1), key(2), yield(12), key(12), yield(3), key(3), yield(14),
key(14)`. Failure while yielding 3 preserves the already applied `[1,12]`
prefix. The oracle calls the actual pinned SmartList method and catches the
source panic; it does not reconstruct the merge algorithm.

The consuming C++ path now applies one item at a time. Ownership is recognized
from the yielded reference category as well as an rvalue owning range, including
a named transform view yielding values. Borrowed lvalue ranges still snapshot
before replacement/growth because C++ permits them to alias the destination;
Rust's exclusive mutable borrow prevents that input. Explicitly moving aliased
views remain subject to the ordinary invalidation rules described above.

The corpus now contains 528 observations, losslessly compressed from 41,855,641
JSON bytes to 731,035 gzip bytes. The two unchanged original list module tests
pass again using the repository-local Cargo target. The four selected native
checks pass 4/4 in 49.57 seconds, including the actual failed-iterator prefix and
the refreshed differential corpus; all 528 observations also match live original
Rust on native. The rebuilt MIPS/QEMU selection passes 4/4 in 38.24 seconds;
all 528 MIPS observations also match the repository-local live original oracle.
The rebuilt ASan/UBSan selection with leak detection passes 4/4 in 70.92 seconds.
The subsequent native consumer rebuild was explicitly interrupted to batch the
additional generic repairs below. No complete regression or full-module
acceptance is claimed from this earlier checkpoint.

## Iterator, callback and Boolean storage review, 2026-10-07

The next direct review reproduced two more differences. Owned `extend` discarded
the already yielded prefix when its iterator failed; `retain` left `[1,3,3,4,5]`
after a filter rejected 2 and failed on 4, where the original retained
`[1,3,4,5]`. Owned extension now applies items individually. Retention compacts
the unvisited tail on callback failure without calling the filter again, and
releases each rejected item's ownership before the next callback. Scoped moves
still support non-assignable elements; arbitrary throwing element moves do not
gain a universal transaction guarantee.

The 595-input intermediate corpus preserved all 528 earlier observations and
added four extension/construction iterator cases, five merge-key failures, 36
callback cases across retain/map/to_list/to_set/identity_map/group_by, 16
independent equality cases and six observed ownership-cleanup traces. The
equality cases isolate every public field, data order, nested facet metadata
and NaN non-reflexivity in data, aggregation, summary and facet values. The
cleanup fixture is move-only and non-assignable, with scoped 4 KiB payloads;
it observes exactly-once release during filtering and after unwinding. All six
selected checks passed native (18.49 seconds), ASan/UBSan with leak detection
(52.48 seconds) and MIPS/QEMU (13.73 seconds). All 595 native observations matched
the live original oracle. These are completed results before the Boolean repair.

A compile reproduction exposed a generic Boolean gap: `std::vector<bool>` has
proxy references, so `get` and `as_mut_slice` could not instantiate. Boolean list
storage now uses an explicitly scoped typed Boolean array instead. It grows
through the existing memory resource, propagates its destination allocator and
has no default resource, singleton, shared owner or added dependency. Ordinary
list element types continue to use the standard PMR vector. Boolean snapshots,
extraction and same-domain construction use real Boolean storage; packed
standard C++ vector inputs are explicitly decoded before publication.

The current corpus has 723 inputs, adding 128 original Boolean sequences with
all construction states, pointer/slice mutation, replacement, extension,
retention, keyed merging, map, list/set/identity/group projections, extraction,
reconstruction and raw structural Debug. Native contracts check pointer identity,
safe self-extension, failed growth without loss, same-domain storage transfer,
independent clones, conversion from packed inputs, Global copying followed by
actual User deletion and all-block reclamation with default PMR disabled.
The compressed artifact contains all 42,372,175 JSON bytes in 739,041 gzip bytes.
The six refreshed checks pass on native (32.97 seconds), ASan/UBSan with leak
detection (71.09 seconds) and MIPS/QEMU (17.47 seconds). All 723 observations on
both native and MIPS also match the live original oracle. Formatting checks cover 567 handwritten
C++ files; layer verification retains independent dependencies and no SQLite
C API calls in the runtime archive. The complete native consumer rebuild was
restarted after batching the repairs; its terminal regression and complete
module acceptance remain open. QEMU results do not establish physical-router
execution or all-stage completion.

## Consumed ownership and f32 diagnostics, 2026-10-08

Two additional reproductions refine the generic contract. An owned mapper
failed on the third item with only `1,-1,2,-2,3,-3` cleanup events before C++
returned to the caller; original Rust additionally released unvisited items
4 and 5 before unwinding. A float element containing binary32 0.1 formatted as
the widened double `0.10000000149011612` instead of source `0.1`.

Consuming map and entity-list projection now extract an owned input container
before conversion. Local element ownership supplies immediate cleanup even
when a C++ callback accepts T&&; unvisited input is destroyed on failure rather
than retained in the moved-from Context object. Neither the input nor output
requires Clone. Into-values and into-value preserve their separate return types,
loaded/change intent, metadata and conversion order. Twenty-four direct original
entity projections cover both return paths, loaded/unloaded state, success and
failure at every item, using an entity with observable Drop and no Clone.

The Debug writer now distinguishes binary32 from binary64. Shortest conversion
uses the original scalar width, including the existing exact midpoint correction;
fixed precision uses the standard width-specific conversion. The existing
binary64 Value, JSON and Decimal behavior remains separately checked. The
538-input binary32 corpus includes both signs of zero/subnormal/normal/extreme/
infinite/NaN boundaries and 512 seeded bit patterns, each observed in nine raw
source-derived SmartList Debug modes. Six further owned-mapper traces observe
success and failure at every item. Existing source/model generators are unchanged;
only differential observation fixtures are expanded.

The current artifact has 1,291 original inputs, losslessly compressed from
43,368,834 JSON bytes to 773,585 gzip bytes. The eight selected native checks
pass 8/8 in 18.73 seconds, including the earlier Value/format comparisons and
move-only ownership tests with both by-value and T&& callbacks. All 1,291 native
observations on native and MIPS match live original Rust. The rebuilt MIPS/QEMU selection passes
8/8 in 37.32 seconds. The refreshed ASan/UBSan selection with leak detection
passes 8/8 in 74.90 seconds. The complete native consumer build/regression is
running. The preceding Boolean full rebuild was explicitly interrupted to
batch these reproduced repairs. Full SmartList/module and stage acceptance
are still open; partial green selections cannot close the all-stage goal.

## Whole-source review and remaining trait gate, 2026-10-08

The review now accounts for all 44 public methods in the complete pinned
production body, the six fields, three derived traits and twelve explicit trait
implementations. The public responsibility table maps every method; the count
is only an omission check and does not replace the review.

| Source trait | C++ responsibility |
| --- | --- |
| From<Vec<T>> | Explicit scoped owned-vector construction, loaded even for empty input. Boolean packed inputs decode into typed storage. |
| Default | Explicit owner construction, empty and unloaded, without a T: Default requirement. |
| IntoIterator (owned) | Consuming extraction transfers the owned container, which supplies iteration without Clone. |
| IntoIterator (shared/mutable borrow) | Const and mutable begin/end and spans borrow the original elements. Growth, replacement, erase and owner closure follow the documented invalidation rules. |
| FromIterator<T> | from_range consumes a single-pass input and returns a loaded list. Failure destroys the partially constructed owned result. |
| Extend<T> | Owned yields apply incrementally; borrowed C++ aliasing ranges take an explicit snapshot. |
| Index / IndexMut | Checked const/mutable indexing; source panic maps to out_of_range. Optional get/set remain separate. |
| Deref / DerefMut | Explicit const/mutable spans expose the same ordered element storage through standard C++ range/slice facilities. |
| From<SmartList<T>> for Vec<T> | into_vec transfers the container without cloning elements; ordinary C++ moved-from objects remain valid. |
| Clone | Destination-allocator copying is conditional on the element's domain-copy capability. Nested metadata and facets copy into that destination. |
| PartialEq | Every field and recursive facet participates. NaN remains non-reflexive. The conditional declaration issue below is not yet repaired. |
| Debug | Default supported scalar/model writing and caller-provided element formatters supply the source element Debug responsibility. The writer borrows each element and does not impose Clone or assignment. |

No source Eq, Hash or Serialize/Deserialize implementation is invented. The
private id_key helper retains every Value variant's distinct key spelling,
including shared Null/TypedNull keys and the constant Object/List keys; public
map_by_id observations exercise the helper rather than reimplementing it in
the fixture.

A negative compiler probe adds an explicit remaining gate. For an element type
with no equality operator, `std::equality_comparable<SmartList<NoEquality>>`
currently evaluates true because the unconstrained operator declaration is
visible before its body is instantiated. The pinned derived PartialEq requires
element PartialEq. Positive equality and NaN corpora do not establish this
negative capability boundary. The complete native consumer build and regression
subsequently passed 281/281 in 87.32 seconds, with no pending Ninja work. Only
after that terminal result were the element-equality declaration constraint and
positive/negative compile assertions applied.

An isolated header copy with an element-equality declaration constraint passes
the negative NoEquality assertion and positive assertions for an equality-only
custom element, bool, float and Value. This is a concrete compiler-checked
prototype; it supplied the subsequent production constraint after the full
build became terminal. Production assertions preserve equality-only/bool/float/
Value capabilities while rejecting NoEquality.

The added conditional Clone assertions also reproduced a second declaration
gap: a move-only list appeared domain-copyable through conversion to the
unconstrained borrowed-span constructor. The initial native trait build failed
that assertion; the affected sanitizer/MIPS builds were explicitly interrupted
to batch its fix. Borrowed-span construction now requires element domain-copy
capability, while owned vector/range consumption retains its no-Clone boundary.
The corrected six-check selections pass native (36.42 seconds) and MIPS/QEMU
(45.68 seconds). The later combined 14-check ASan/UBSan selection passes in
175.21 seconds with leak scanning disabled: the initial leak-enabled run failed
because LeakSanitizer cannot operate under the restricted environment's tracing
rules. Its completed build and all 872 direct helper observations preceded that
scanner shutdown error; the failed leak evidence is retained. Explicit all-block
reclamation assertions remain active. The current full native consumer build/
regression is running. These negative capability checks are required for module
acceptance; positive runtime corpora alone could not detect either declaration
mismatch. The leak-disabled run is not evidence of a successful leak scan.

## Complete list responsibility-group acceptance, 2026-10-08

The complete source table above is now accepted for pinned `teaql-core/src/list.rs`:
all six fields, 44 public methods, three derives, twelve explicit trait
implementations and the private public-observable ID-key helper are mapped.
The conditional declaration probes now preserve the actual element equality and
Clone boundaries. No Eq, Hash, serde, element Default, output/key Clone or entity
assignment capability is added to the source contract. Standard C++ scalar/model
formatters and explicit element formatter callbacks supply element Debug through
the preserved generic diagnostic boundary.

The final complete native consumer build and explicit stale search-fixture refresh
pass all 281 CTests in 69.60 seconds, with no pending Ninja work. An earlier full
run passed 280/281 solely because the search probe predated its latest test-only
field observations; its two-task refresh closes that integration mismatch.
The 1,291 original list observations remain matched on native and MIPS, and the
3,042 earlier row/list diagnostic observations are retained. Current selected
native/MIPS trait/ownership checks and the combined ASan/UBSan run pass; the
current leak scanner limitation and successful earlier leak-enabled ownership
checkpoints remain distinguished rather than rewritten.

Explicit Global copies, nested allocator propagation, actual source User deletion,
same-domain transfers, growth failure preservation, exactly-once observed cleanup
and all-block reclamation are demonstrated in the native contracts with default
PMR disabled. Results and views remain Context borrows; this module adds no
shared lifetime or dependency. The typed Boolean array is a C++ storage adapter
for the source Boolean pointer/slice responsibilities, not a new ownership scope.

This accepts only the list responsibility group. The surrounding row/query/entity,
Decimal/reexport, service/runtime/save/log/privacy and six-stage acceptance remain
separate. Physical-router execution remains unverified.


## Reproduction and remaining gates

Checkpoint on 2026-10-07: all 526 observations matched live original Rust with
the native probe. The two unchanged source module tests passed 2/2. The latest
four selected contracts passed 4/4 under ASan/UBSan with leak detection and 4/4
under OpenWrt MIPS/QEMU. All 526 MIPS probe observations also matched live Rust.
The complete native consumer rebuild and final incremental build both completed
successfully. The full native CTest regression passed 197/197 checks after
these builds. This is the SmartList checkpoint before the later compatibility
library boundary changes; wider stage completion remains unproven.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin smart_list_api_oracle --locked --offline
python3 tests/parity/generate_smart_list_api.py \
  --rust /home/philip/shared-cargo-target/debug/smart_list_api_oracle
cmake --build build --target teaql_smart_list_api_tests teaql_smart_list_api_probe \
  teaql_core_collection_tests teaql_entity_expression_tests
ctest --test-dir build --output-on-failure \
  -R '^(smart_list_api_contracts|smart_list_api_rust_parity|core_collection_contracts|entity_expression_contracts)$'
python3 tests/parity/compare_smart_list_api.py --cpp build/teaql_smart_list_api_probe \
  --rust /home/philip/shared-cargo-target/debug/smart_list_api_oracle
```

Use the same four selected checks for ASan/UBSan with leak detection and for the
OpenWrt MIPS build under QEMU. Original source and the model generator remain
unchanged. Wider model diagnostics, model/API/serialization audits,
query execution, save, log privacy and stage acceptance remain separate gates.
Physical-router execution is not established by QEMU.
