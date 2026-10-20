# Generated request-state views and consuming core conversion: Rust 5.0.6

The shared EntityQuery runtime and thin generated adapters restore five source
request-state operations. The generator remains pinned to
`45e1572c4956ca963410d8ea436ead3c8857087a`, with runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. This closes the five named missing
entry points, not all G06/G07/P4/P7 responsibilities.

## Source responsibilities and C++ mapping

The complete original request template exposes query, relation selections,
relation filters, child enhancements, query options and a consuming into_query.
Its From<Request> for SelectQuery is a different conversion: it first builds a
QuerySelection, then applies its options and enhancements.

| Original operation | Generated C++ API and ownership |
| --- | --- |
| relation_selections() | relationSelections(), a read-only span into the checked Context-owned request. The current template never appends this collection; the distinct runtime collection is retained. |
| relation_filters() | relationFilters(), a read-only span retaining full nested selections and options. |
| child_enhancements() | childEnhancements(), a read-only span retaining independent child requests. |
| query_options() | queryOptions(), an independent allocator-aware snapshot in the request's domain. It does not use a hidden cache or extend Context lifetime. |
| Request::into_query(self) | std::move(request).intoQuery(), rvalue-only and before purpose. It consumes the checked request and extracts the core query without independent options or child enhancements. |
| From<Request> for SelectQuery | std::move(request.selection()).into_query(), the existing core conversion that applies options and enhancements. |

The shared snapshot builder supplies options; model adapters contain only exact
runtime delegates. QueryDescription retains the separate relation selection
collection, including explicit destination copies. Consuming conversion clears
the wrapper's root binding only after snapshot construction succeeds; later
access rejects as moved-from. The arena retains registered roots until close.

The existing C++ query() accessor remains its integrated runtime inspection
representation, which can contain already-applied option metadata. This new
checkpoint does not claim that accessor is the original Rust bare query(&self)
view. A copied request's consuming intoQuery() supplies the bare core snapshot;
remaining source-view and complete representation review stay G06/P4 obligations.
No existing query() caller or execution implementation is changed here.

Spans and returned kernel values remain Context borrows. Destroy a QueryOptions
or SelectQuery value before its allocator closes, or explicitly copy it into a
destination scope. Global copies do not bypass access Context checks. These
inspection/conversion operations invoke no executor and grant no execution
permission. Purposed requests do not acquire the new inspection/conversion APIs.
Accessor entry checks the request; a returned raw span does not check each later
access. Do not retain it across builder mutations that invalidate vector storage
or across Context close, following the existing native borrow contract.

## Actual original observations

A separate test-only Rust crate executes the complete unchanged generated
cpp_dynamic_json library. Its dependency lock derives from the current
reference-dependency sample logger project, adding only the original domain and
the new observer root. Original generated files, producer model and provenance
hashes are verified before every comparison.

The immutable 432-case matrix crosses Person/Order, full/minimal requests,
ordinary/empty/long Unicode-NUL text, all 32 subsets of five operations, reverse
operation order and repeated option/relation/filter/child operations. The seven
complete original outputs are query options, relation selections, relation
filters, child enhancements, raw consuming core, metadata-applied core and full
QuerySelection. All 3024 views/conversions compare directly without output
normalization. Construction-only observations do not claim physical SQL results.

Runtime tests disable default PMR and exercise nested allocator propagation,
independent snapshots/request copies, post-consumption rejection, exact option
removal/application, explicit Global retention and actual UserContext deletion.
All new accessor calls on stale requests reject before storage access. Returned
raw spans retain the existing caller-managed borrow boundary.

Compiler controls check correct calls and reject lvalue consumption, purposed
consumption/options and extra Context arguments. Generator regressions verify
actual adapter spelling, constraints and guidance. The SDK update verifier
removes only five exact new function signatures and delegate bodies, formats
what remains and requires every old query body to be byte-identical. Across 44
SDKs, 335 delegates are added; other generated code remains unchanged.

## Completed gates

Native regression passes **8/8**, including the new 432-case matrix and existing
field criteria/membership/general/binding checks. Module consumers pass **2/2**
and targeted ASan/UBSan passes **3/3**; each profile executes the complete new
432-case corpus. Leak scanning is disabled. Compiler controls pass **1 positive /
4 negative**, and related Java checks pass **53/53**.

All **23 workspaces / 276 files** and **44** complete contract SDKs strictly
reproduce. The controlled SDK update adds **335** exact delegates and changes
**30** Assist files; every prior query method and all other code remain identical.
Canonical module verification covers **626** consumer/fixture bodies. Pinned
format/check covers **1418** handwritten C++ files. No full-suite, MIPS or
physical-device run is inferred from these selected gates.

## Reproduction and verification scope

```sh
set -e
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_inherited_business_binding_bridge_506 \
cargo build --offline --locked --manifest-path tests/parity/rust_request_state_5_0_6/Cargo.toml
cmake --build --preset native --target teaql_generated_request_state_probe teaql_generated_request_state_tests
python3 tests/parity/compare_generated_request_state_5_0_6.py \
  --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/teaql_request_state_506 \
  --cpp build_native_current/teaql_generated_request_state_probe
ctest --preset native -R '^generated_request_state_'
```

Initial capture remains immutable. Terminal native/compiler/Java/module/lifetime,
strict generation and SDK reproduction outcomes are retained in
[the evidence record](parity/generated_request_state_5_0_6.json).
Complete general/predicate/model/raw/grouped/stream/page/policy/privacy and source
inspection obligations remain open; neither G06/G07 nor the full goal is marked
complete by this entry-point checkpoint. No new runtime dependency is added.
