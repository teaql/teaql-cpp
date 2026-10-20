# Relation-selection input conversions against Rust 5.0.6

The original forward and reverse `select_*_with` methods accept
`impl Into<QuerySelection>`. Their bodies materialize the selection with
`into_query()` before attaching it to the named relation. The C++ selector
surface previously accepted only the relation's generated target request and
implicitly required its executor type. Those restrictions were stronger than
the source contract.

## Runtime and generated adapters

`EntityQuery::selectRelation` now accepts an explicit `QuerySelection`, an
explicit `SelectQuery`, or an unpurposed `EntityQuery` with another executor
type. Selection input is copied into the parent resource before materializing
its core query. The existing relation-load container then captures that query.
No child executor runs during construction.

Wrapper input validates the checked Context binding. Direct values must use
the destination resource; foreign values require an explicit destination copy.
These values remain kernel borrows, so callers must keep them alive during
capture. The checks do not intercept arbitrary dangling raw C++ references.
Lvalue and rvalue overloads preserve fluent value categories.

The producer adds six thin delegates per generated forward/reverse selector:
two public request conversions, two selection conversions and two core-query
conversions. Target-specific overloads now delegate to the same runtime capture, using
public `runtimeQuery()` access. This also enforces explicit destination copying
when a Global parent captures a User child. Generated Assist
explains the conversion, binding, snapshot and lifetime boundaries.

The old `wrong_relation` and `expression_diagnostic_wrong_child` compiler
expectations rejected other entities solely because of C++ target typing. Their
replacement acceptance tests follow the original general conversion bound.
An independently compiled original program verifies unrelated-entity forward
and reverse inputs and equivalent request/selection/core conversion. This is
construction evidence, not successful execution of an unrelated entity graph.
Purposed requests, invalid scalar inputs and extra Context arguments remain
compile errors in the generated C++ API.

## Source execution and ownership evidence

All **1,152/1,152** original/generated executions agree: three models, full/minimal
selection, compact/typed results, eight operation sequences, three conversions
and four child modes. Every outcome succeeds through in-memory SQLite. The
matrix records **2,784 result rows** and **1,800 safe SQL records**, comparing
complete query/options/selection diagnostics, typed JSON and loaded states,
errors, SQL, bindings, intent and trace without normalization.

The matrix exercises forward relations, repeated capture, removal and
reselections, computed parent/child properties, deleted children and sliced
child requests. Reverse generic inputs have separate source/compiler evidence;
this matrix does not establish reverse conversion SQL or every model/privacy
configuration. Existing named-selection version behavior remains unchanged.

The default PMR resource is disabled in C++ observers and the ownership test.
Tests check independent snapshots, foreign wrapper/selection/core rejection,
explicit destination copies, actual UserContext deletion with zero retained
User bytes, expired wrapper access and a retained Global copy read through a
new UserContext. Local PMR diagnostics are destroyed before their allocator.

Initial sanitizer failure retained a local diagnostic string past UserContext
deletion; its scope was corrected in the fixture. The first imported-module
build lacked `TEAQL_SOURCE_DIR` on the new target; the target definition was
added. Both initial records are preserved. Neither correction changes runtime
semantics.

## Qualification gates

Runtime-first native checks pass 3/3, imported-module checks 2/2 and selected
ASan/UBSan checks 2/2 with leak scanning disabled. The original observer uses
registry versions/checksums from the locked Rust workspace; generated Rust
sources and reference checkouts remain unchanged.

Final generated-adapter integration passes native **4/4**, imported modules
**2/2**, selected ASan/UBSan **2/2** and compiler **3 positive / 3 negative**
contracts. Java passes **59/59**. All **24 workspaces / 288 generated files**
and **44 contract SDKs** reproduce strictly. Controlled SDK updates prove
**456 exact new delegates** and **76 typed selector delegation replacements**,
with every unrelated body preserved. Module checks cover **669 canonical
consumer/fixture bodies** and **1,485 exports**; formatting checks **1,453**
C++ files. These counts are scoped gates, not complete semantic acceptance.

This checkpoint does not qualify all G06/G08/G12/G13 responsibilities or
complete P4/P5/P7. Matching, grouping, facets and relation
aggregate conversion families retain their separate original input bounds and
review obligations.

Reproduce after building the two C++ targets:

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
  teaql_generated_relation_selection_inputs_probe teaql_relation_selection_input_tests
python3 scripts/verify_relation_selection_inputs_5_0_6.py
ctest --preset native --output-on-failure \
  -R '^relation_selection_input(s_rust_parity|_context_contracts)$'
```

The [source review](parity/relation_selection_inputs_source_review_5_0_6.json)
retains actual selector bodies and fixture hashes. The original corpus and
initial failures remain separate from
[final terminal evidence](parity/relation_selection_inputs_terminal_5_0_6.json).
The two controlled SDK proofs are
[new input delegates](parity/relation_selection_inputs_sdk_change_5_0_6.json) and
[shared typed delegation](parity/relation_selection_typed_delegates_sdk_change_5_0_6.json).
