# Generated facets: Rust 5.0.6 checkpoint

Status: fixed-model facet construction/execution checkpoint verified. Full G08/P5 acceptance
remains open, including the independent quoted-Line result decision and cache,
additional model, asynchronous and trace/privacy matrices.

## Fixed source and responsibility

Runtime: v5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
The unchanged complete generated dynamic_json_rust crate supplies the actual
facet methods, execute_facets, restriction and runtime assembly. Its request
source remains frozen; probes are separate executables.

Generated facetBy...As(name, child) defaults includeAll to true.
facetBy...AsWithOptions(name, child, includeAll) preserves source argument order.
Both delegate Context binding, allocation and snapshot mechanics to EntityQuery.
Model-aware Assist records their names and execution boundaries. No new library
or database integration is introduced.

Shared runtime execution fetches the outer typed list first, then sequentially
fetches compact facet lists through independent entity data services. Each facet
gets the source Calculate facet purpose and a separate trace context. Duplicate
names replace earlier results after both requests execute. List and page attach
facet maps; one/first execute facets before returning the first entity. Raw rows,
count, exists and stream retain their source behavior and do not run facets.
Nested child facet metadata is retained; the frozen runtime does not recursively
run generated execute_facets on a compact facet fetch.

## QuerySelection state and filter rules

QueryDescription now retains allocator-aware analysis origins, relation filters
and child selections alongside its established core view. Snapshot conversion
preserves QuerySelection, QueryOptions, raw SQL/projection metadata, object groups
and relation aggregate selections as distinct responsibilities. The public core
query API remains intact. Analysis metadata is materialized in both the core view
and its original builder representation; reducing that duplication is separate
from this semantic migration.

When a facet's relation aggregate targets the outer entity, the outer predicate
is merged into that aggregate's bare query. includeAll false also restricts facet
entities using the actual outer relation keys and an IN subquery with slicing,
ordering, projections, grouping and aggregates cleared as in source. A facet
query's own comment is required at execution; an outer comment does not satisfy
that independent request boundary. Empty outer lists still run facets.

Field aggregate origins retain the intermediate core query before empty-options
conversion clears request metadata. This is necessary for complete nested Debug
and source option provenance. Comments in generated request state are applied
once; error mapping occurs once per application boundary, including pagination.

## Evidence scope

The matrix includes 1,088 exact original/C++ observations, including 720 actual SQLite executions, with complete SelectQuery Debug and SQL/parameters/comment/purpose. It covers normal/minimal roots, default and
explicit includeAll, aliases, empty roots, child comments/filters/deleted rows,
raw properties, slicing/ordering, count/stat/sum aggregate children, outer filter
merging, relation membership, object group origins, repeated names, second facets,
nested facet metadata and six execution modes. Successful attachment cases and
source missing-comment failures are both included. Outputs are not normalized.
The existing FK-enabled fixture is used; generated schema bootstrap is separate.

Context tests use a null default PMR resource, transient caller strings, independent
child snapshots, actual SQLite results, explicit Global query/result copies,
UserContext destruction, expired wrapper rejection, foreign binding rejection and
Global rebinding. The module fixture executes a real facet with relation counts.

Artifacts:

- tests/parity/rust_field_layout/src/bin/generated_facets_5_0_6.rs
- tests/parity/generated_facets_probe.cpp
- tests/parity/generate_generated_facets.py
- tests/parity/compare_generated_facets.py
- tests/parity/generated_facets_5_0_6_cases.json.gz
- teaql_runtime/tests/generated_facets_tests.cpp
- teaql_runtime/include/teaql/runtime/detail/generated_query_facets.hpp

Validation passes native CTest 6/6 (including existing execution, relationship and
dynamic JSON corpora), targeted ASan/UBSan 3/3 with leak scanning disabled, actual
module facet/count attachment 1/1, compiler 2 positive and 2 negative contracts,
selected Java generator tests 26/26, and exact reproduction of all 15 generated
C++ sets (165 files). Formatting checks pass for 925 handwritten C++ files.
Reference/export/path integrity checks pass; all 17 capabilities remain
unqualified. The corpus records 180 source missing-comment execution errors.
The pinned SQLite archive is not sanitizer-instrumented. This checkpoint does
not qualify all facet configurations or complete G08. No MIPS, physical-router
or GPU execution is claimed.


The subsequent [asynchronous/error checkpoint](generated_facets_async_5_0_6.md)
extends the corpus to 1,184 exact observations (816 SQLite executions), preserves
all earlier source inputs and adds real public metadata/registry/table failures.
C++ deferred executor tests cover wake forwarding and actual Context deletion at
both execution stages; these are ownership adaptation gates, not live scheduling
parity. Full G08 remains partial.
