# Generated relationship analysis: Rust 5.0.6 checkpoint

Historical checkpoint: this records the original relationship construction and
execution evidence before the quoted-FK decision. The user subsequently approved
[the bounded C++ compatibility exception](quoted_fk_compatibility_exception_5_0_6.md).
The [current source binding](relation_analysis_cli_source_binding_5_0_6.md)
checks all 7,122 observations with strict source comparisons and approved C++
expectations reported separately; the source comparisons retain the declared
Line descriptor Debug spelling adapter. G08 has later qualified evidence; this
historical checkpoint alone does not complete P5 or P0–P7.

## Fixed references and runtime responsibilities

Runtime: v5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
The actual unchanged `dynamic_json_rust` crate supplies request and model methods.
The request template's `objectStorageFilterMethods`, `reverseRelationMethods`,
`relationAggregateMethods` and `relationFieldAggregateMethod` define the surface.

Context-bound EntityQuery builders delegate existing core/query and data-service
protocols. Generated adapters do not implement SQL, database transport or result
attachment. List, one and page pass the runtime relation-aggregate descriptions
to enhanced-entity execution. Rows and streams preserve the source boundaries:
they do not enter generated list aggregation/facet execution.

## Restored interfaces

Forward relations expose groupBy...With, groupBy...WithDetails,
groupBy...WithDetailsFrom, rollUpTo... and rollUpTo...With. Roll-up combines
membership and object grouping. Reverse collections expose count...,
count...As, count...With, statsFrom..., statsFrom...As and groupBy...WithDetails.
Numeric and comparable field aggregates use the fixed producer's centralized
RustRelationAggregateMethod metadata, then change application method casing only.
Default aliases such as count_orders, sum_amount_of_orders and refinements retain
their source spelling. Presence/absence and with...Matching/without...Matching
methods use actual local/foreign keys, including reverse to-one relations.

Default aggregate/detail children are full live-row queries with unlimited
slicing. Reverse presence checks use minimal live-row children. Explicit child
requests are copied snapshots. All builders check the Context binding and copy
nested strings/containers into the parent's explicit allocation domain. Explicit
Global copies and rebinding preserve that domain; ordinary wrappers expire when
UserContext is destroyed.

Membership uses the bare child SelectQuery, before request options. Grouping and
relation aggregate descriptions use converted request metadata. Field aggregate
conveniences convert through a core SelectQuery and an empty QuerySelection:
request-option metadata is cleared, while the applied comment/core state remains.
These differences are source behavior, not interchangeable shortcuts.

## Model decoding prerequisite

Shared GeneratedRelationAccess helpers implement raw-column and forward-reference
lookup. A reference object must contain id; scalar aliases are decoded as foreign
keys. Lookup follows row iteration order. Scalar aliases mark the native FK loaded
and the object relation unloaded unless explicit relation metadata proves a view.
To-one decoding ignores non-object values after FK validation, as the source macro
does. Negative i64 to u64 errors retain the source-specific diagnostic.

The fixed source's reverse relations are runtime-installed views, not struct
fields reconstructed from arbitrary raw columns. Generated local reverse
fallbacks stay empty during row construction; actual query views remain installed
by the independent graph/hydration layers. Bare constructor and relation-handle
observations supplement actual query execution.

## Evidence and pending source failure

The matrix contains 7,122 original/C++ observations: 3,040 request constructions,
4,044 SQLite executions and 38 direct model-constructor cases. It observes complete
SelectQuery Debug, attached typed metrics, object details, raw rows, page totals
and actual SQL/parameters/comment/purpose. It covers normal/minimal roots, default
and explicit children, filters, deleted rows, slicing/grouping, child options,
empty parents, repeated/recombined requests, generic statistics and source backend
errors. Database fixtures are existing foreign-key-enabled tables; no schema
bootstrap claim is made.

At this checkpoint, 6,576 source comparisons passed, including 3,498 SQLite
executions. Another 546 real executions awaited the quoted-FK decision. Their
later approved C++ expectations are kept separate from strict source matches.

Only the previously documented Line descriptor spelling adaptation is allowed:
`column_name: "\\\"order\\\""` becomes the C++ raw identifier spelling in Debug.
Original observations remain verbatim. Result values are never normalized.

[The quoted-FK failure record](parity/relation_aggregate_quoted_fk_source_failure_5_0_6.json)
shows identical SQL logs but different attachment results. Rust queries the correct
Line counts, then cannot find the raw result key using its quoted column metadata.
It attaches default zero/empty results. Existing C++ raw-identifier metadata
attaches actual counts. The later user decision retains the correct C++ results
under an explicit compatibility exception. Review snapshots protect both sets of
values without claiming strict semantic equality for those 546 cases.

The historical corpus verifier reports matched and formerly pending records
separately. `--require-complete` rejects those labelled source-failure cases;
the later exception-aware binding validates their approved C++ expectations.
At this checkpoint, validation passed native CTest 14/14, targeted ASan/UBSan 4/4 with leak scanning
disabled, module actual grouping/counting/relationship attachment 1/1, compiler
2 positive and 2 negative contracts, selected Java generator/layout tests 27/27,
and exact reproduction of all 15 C++ generated sets (165 files). Formatting checks
pass for 922 handwritten C++ files. Export/path/reference integrity checks pass;
all 17 capabilities remained unqualified. The partial relationship test includes
review-snapshot stability checks; those are not semantic acceptance. The pinned
SQLite archive is not sanitizer-instrumented. No MIPS or physical-device execution
is claimed.

## Work remaining at this checkpoint

The quoted-FK disposition was resolved later as documented above. At this
checkpoint, remaining work included generated facets, outer-filter merging,
restriction semantics, nested facet/aggregate execution and result attachment;
aggregation-cache expiry/propagation, trace and privacy; additional
numeric/temporal/inherited/guarded models; and async suspension and cancellation.
The complete G08 and P5 exit criteria were open at this checkpoint. Consult the
current capability ledger for their later disposition.
