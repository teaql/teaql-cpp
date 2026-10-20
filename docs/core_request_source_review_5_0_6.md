# Core request builder source review: Rust 5.0.6

Status: complete-body pre-acceptance review; `teaql-core/src/request.rs` remains
`pending_review`. Temporary imported consumers do not replace permanent module
registration or the current full native integration gate. Complete G08/G13/G17
and P0-P7 acceptance remain independent.

The reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The complete
624-line source has been read alongside request.hpp, request_debug.hpp and
request.cpp. The historical [API review](request_api_acceptance.md) retains its
5.0.5 evidence and ownership repairs; it is not relabeled as current acceptance.

## Active source delta

Compared with the retained exact 5.0.5 reference, this file adds two statements
in `apply_runtime_metadata`: copy `options.facets` into the core query and
replace core relation aggregates using `runtime_relation_aggregates(options)`.
The current C++ helper already performs both operations, including clearing old
metadata when the new options are empty. No production repair is made here.

This is the builder request module. Its QuerySelection, RelationAggregate and
ObjectGroupBy remain distinct from core query structures; it is not the
data-service request-intent envelope. Conversion, intent validation, provider
execution and masked logging retain their separate responsibilities.

## Complete responsibility mapping

| ID | Source responsibility | C++ mapping and required observation |
| --- | --- | --- |
| RQ01 | Three constants and eighteen FieldOperator variants | Preserve exact values/names, copy/equality and raw diagnostics; no application naming transformation of canonical identifiers. |
| RQ02 | Generic DateRange | Consuming construction needs no Clone bound. Copy and equality capabilities follow the payload. Explicit allocator-aware movement/retention does not add a hidden owner; noncopyable endpoints remain supported. |
| RQ03 | EntityReference | Preserve consuming Value/u64 and custom implementations without a Clone requirement. C++ receiver constraints follow the actual value category; the result is relocated into the explicit destination. |
| RQ04 | QuerySelection | Preserve query, relation selections, relation filters, child enhancements and options. Construction from a core query starts with empty builder metadata; conversion applies selections before runtime metadata. |
| RQ05 | RelationSelection and RelationFilter | Retain all six fields and construction from a selection. RelationSelection has consuming conversion; RelationFilter remains a separate stored model. Stored relation filters are not silently applied during conversion. |
| RQ06 | QueryOptions | Preserve eight independent fields, empty Default and None/Some(empty) distinctions. Clone/equality/Debug retain every collection, duplicate and nested selection. Options have no invented purpose field. |
| RQ07 | Trusted raw SQL and two projection models | Preserve the private trusted string and distinct RawDynamicProperty/RawProjection types. Consuming extraction retains the full string. Trust marking does not sanitize SQL or establish authorization. |
| RQ08 | Builder relation aggregates | Preserve relation name, alias, QuerySelection and single-result flag. Convert recursively to the independent core RelationAggregate type. |
| RQ09 | FacetRequest and ObjectGroupBy | Preserve all names, flags, storage fields and nested selections. Core facet storage retains the builder facet model; grouping converts its child selection into a core query. |
| RQ10 | apply_relation_selections | Convert each nested selection and delegate in original order to the core relation-query builder. Do not turn this into relation loading or execute SQL. |
| RQ11 | apply_runtime_metadata | An absent option comment leaves the base comment unchanged. Raw SQL, facets, aggregates, criteria, dynamic/raw projections, object groups and enhancements replace their respective core fields. Empty options must also erase prior values. |
| RQ12 | runtime_relation_aggregates | Preserve collection order, alias and flags; recursively convert each complete child selection rather than copying only its bare query. |
| RQ13 | Facet merge and attachment | Merge the outer filter only into direct builder aggregates with the same entity and only when the outer filter exists. Attach ordered facet-map entries through SmartList, preserving rows/count and replacement behavior. |
| RQ14 | Value operators and required operands | Cover all eighteen branches. Between evaluates lower before upper; null operators ignore operands; IN accepts its whole collection. Missing/wrong-type failures retain operator/index/full raw Value diagnostics. |
| RQ15 | Property-to-property operators | Preserve six comparisons and six LIKE/NOT LIKE spellings. Reject the other variants with the original diagnostic; do not invent text wildcard operands for column comparison. |
| RQ16 | Live-filter removal | Remove the exact version > I64(0) expression and recurse through AND only. Preserve other Value variants, OR/NOT and recursive zero/one/many conjunction collapse. |
| RQ17 | Dynamic JSON value conversion | Preserve NULL/Boolean/numeric variants, I64-before-U64 preference, trimmed strings, nested lists and recursive object.id extraction. A missing id becomes NULL; input does not become trusted runtime metadata. |
| RQ18 | Dynamic JSON operator/value/filter helpers | Recognize case-insensitive null markers before trimming. First-string/object arrays select IN before the two-element BETWEEN rule. Empty arrays retain the missing-operand failure; values are not flattened recursively or coerced to match a modeled field. |
| RQ19 | Unsigned JSON field access | Accept the original nonnegative integer forms; reject strings, Booleans, negative and floating representations. Missing fields remain absent. |
| RQ20 | Derived capabilities and ownership | Account for all eleven structs and 43 fields, generic Debug/Clone/PartialEq/Eq bounds and raw field order. These types have no source serde protocol. C++ destination copies propagate nested allocators and do not extend Context lifetime. |

The model observer exercises actual original constructors, conversions, derived
Debug/equality and a non-Clone custom EntityReference. The helper observer calls
the original functions, catches their actual panics and retains precise failure
text. Neither supplies an alternate conversion, equality or filter algorithm.

The strict UI dynamic-search module is a different source file. Its validation
and warning protocol cannot substitute for these permissive JSON helpers, nor
does this review qualify it. Raw request diagnostics likewise do not replace
runtime privacy projections.

## Frozen-epoch verification approach

The native integration epoch is already building all consumers. Its 10,653
execution inputs must remain unchanged. Additional Rust manifests, canonical
test mirrors, compiled consumers and logs therefore live under a task-owned
`/tmp` directory. Only this narrative and compressed evidence are added here.

The temporary Rust workspace calls the unchanged active drivers, checks its
registry versions/checksums against the exact reference lock and replays the
immutable corpora containing 174 model observations and 872 helper observations.
Its source bodies and expected
outcomes remain unchanged. The current C++ module archives and BMIs are used
only after their earlier successful build; no output from the running native
compiler/linker is executed.

An exact copy of the canonical module preparer adapts the two probes and two
existing ownership contracts. The mirrored handwritten bodies and quoted
fixtures are byte-identical before adaptation; after adaptation, only include
prologues/paths differ. Each compiler/linker must exit successfully before its
consumer is used. Archive/BMI hashes and frozen execution inputs are checked
again after execution.

Permanent registration of these four consumers, repository-owned reproduction
and full native/source acceptance remain required after the frozen epoch ends.
No source disposition, capability verdict, baseline, producer or generated SDK
is changed by this review. No additional sanitizer, MIPS or physical-device
acceptance is claimed.

## Terminal imported checkpoint

The [fresh source/import receipt](parity/request_source_import_repaired_5_0_6.json.gz)
is complete. All **1,046 original observations** match the immutable active
corpora; all **1,046 imported observations** match the actual original results.
The **two existing unit contracts pass**, and all four probe/unit programs
compile and link successfully before execution. Six canonical entry/fixture
bodies preserve the handwritten sources. All **87 registry identities** match
the exact reference lock; there is no expected-value or diagnostic normalization.

The two unit programs retain generic noncopyable endpoint construction, honest
copy/receiver constraints, rejection before consuming foreign noncopyable data,
nested allocator propagation, explicit Global retention and actual User
destruction. Default PMR is disabled. They execute existing implementations;
this checkpoint adds no production ownership mechanism.

The [earlier controller receipt](parity/request_source_import_review_5_0_6.json.gz)
remains unsuccessful: after matching the original observations, it tries to hash
a nonexistent native teaql_async archive. That package is header-only outside
its module frontend. The corrected controller derives archive paths from the
actual CMake link command. This is an observer setup failure, not a runtime
semantic difference or a successful imported gate.

The completed gate verifies that all module archives/BMIs it uses and all
10,653 frozen execution inputs are unchanged afterward. The running native
epoch continues independently. Whole-file status remains pending until the
permanent module/reproduction and full native obligations above are satisfied.
