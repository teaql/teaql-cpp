# Bare core query inspection: Rust 5.0.6

The original Request::query(&self) responsibility maps to the new non-consuming
C++ coreQuery() snapshot. The existing query() keeps its integrated execution
representation and existing callers. This is an explicit language/ownership
mapping, not a claim that the two inspection APIs expose identical metadata.

## Runtime-first implementation

A shared runtime helper clones into the request's allocation domain and removes
only independent option metadata: comment and comment trace nodes, raw SQL and
criteria, dynamic/raw projections, relation aggregates, object grouping, facets
and child enhancements. Ordinary predicates, projections, ordering, slices,
core relations and other query configuration remain. Snapshot extraction does
not mutate its request or invoke an executor.

The same clearing helper serves selection reconstruction. intoQuery() now uses
coreQuery() before invalidating the consumed request; it no longer constructs
and discards a complete option/filter/child snapshot. Generated models add only
one exact delegate. No dependency, cache, owner or execution layer is added.

coreQuery() is available before purpose on const and ordinary requests. The
returned value uses the bound Context allocator and must be destroyed before
that allocator closes. Retention requires explicit destination copies. It does
not extend Context lifetime or grant execution authority. This replaces Rust's
borrowed reference with an independent C++ value; existing query() borrows keep
their established contract.

## Actual source comparison

A new test-only source project executes the unchanged original dynamic_json
library at producer 45e1572c4956ca963410d8ea436ead3c8857087a and runtime
fe82a18982415f1746df9a2cf7c42f5e9abdc6d3. The previous source project and corpus
are unchanged. Registry versions and checksums must match the reference lock.

The 816-case matrix crosses Person/Order, full/minimal projection, ordinary,
empty and Unicode/NUL text, all subsets of six operations, reverse order and
repeated operations. Analysis operations include actual object grouping, facets
and reverse relation statistics. Eight full original outputs compare directly:
query(&self), options, three distinct request collections, direct consuming
core, metadata-applied core and QuerySelection. Construction-only comparisons
are not physical SQL evidence.

Runtime checks disable default PMR, compare non-consuming/consuming snapshots,
retain integrated execution options, verify independent later mutation, reject
stale and consumed wrappers and explicitly copy into Global before actual User
deletion. Compiler controls accept const inspection and reject purposed/extra
Context calls. SDK reproduction removes only exact new delegate bodies and
requires every old generated body to remain byte-identical.

## Completed gates

The complete original/native matrix passes **816/816**, comparing **6528** full
views and conversions. Native regression passes **13/13**, including existing
query state, criteria, membership, binding and facet/async contracts. Module
consumers pass **2/2** and targeted ASan/UBSan passes **3/3**; both execute the
complete 816-case matrix. Leak scanning is disabled.

Compiler checks pass **2 positive / 4 negative** after correcting the initial
positive stimulus to borrow the existing noncopyable SelectQuery. Related Java
checks pass **53/53**. All **23 workspaces / 276 files** and **44** contract SDKs
strictly reproduce. Only **67** exact new delegates and **30** Assist changes are
admitted; every prior generated body remains byte-identical. Formatting checks
**1420** handwritten C++ files, and canonical module verification checks **628**
consumer/fixture bodies. The catalog remains **1485** exports. No full-suite,
MIPS or physical-device result is inferred from these gates.

## Reproduction and remaining scope

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_core_query_view_probe teaql_generated_core_query_view_tests
python3 tests/parity/compare_generated_core_query_view_5_0_6.py \
  --cpp build_native_current/teaql_generated_core_query_view_probe
ctest --preset native -R '^generated_core_query_view_'
```

The specific bare inspection responsibility has a native API mapping and actual
source matrix. The whole G06/G07/P4/P7 goal still requires the remaining general,
model, execution, intent/policy/privacy and provider configurations. Terminal
gates and adaptations are recorded in
[the evidence record](parity/generated_core_query_view_5_0_6.json).
