# Relation-analysis runtime conversions and embedded graph projections

This runtime-first checkpoint covers the original `Into<QuerySelection>`
responsibilities of facets, object grouping and relation count/detail aggregates.
It does not complete the generated API adapters or the separate matching,
roll-up and field-statistic conversion families.

## Input and ownership contract

`EntityQuery::facetBy`, `groupByObject` and `aggregateRelation` now accept checked
unpurposed requests from another executor, explicit `QuerySelection` values and
explicit `SelectQuery` values. Both fluent value categories are supported.
Wrapper inputs require the same checked Context binding. Direct values require
the parent's resource; foreign values need an explicit destination copy.
Raw value references remain borrows and must be valid during capture.

Facets retain the complete original selection. Grouping and relation aggregates
retain that selection in diagnostic origins and materialize its query for the
existing execution protocols. Every nested allocation uses the parent scope.
No child executor runs during capture and no ownership scope is added.

The original generated API independently compiles unrelated-entity request,
selection and core-value inputs. The C++ kernel has matching compile-time input
coverage. Generated application wrappers still retain their existing target
typing; thin public conversions and corresponding Assist remain the next step.

## Reproduced graph defect and correction

The initial complete comparison matches 1,920 of 1,944 outcomes. Twenty-four
typed list/page results lose nested reverse relationships in object-group
details. Request metadata and SQL agree; the missing information appears during
typed graph installation and presentation.

Grouping previously reduced fetched child rows to their ordinary field map.
The corrected attachment also retains their loaded-relation metadata. Before
typed decoding, the existing embedded-row installer incorporates those rows
into the runtime graph. A new overload allows installation into the builder
already used by planned relation hydration; the existing frozen-root entry
delegates to it.

Repeated identities need distinct projection views: two customers can reference
the same Platform while retaining different injected reverse views. Embedded
edges now use the existing view-aware compact-row decoder, with scope-local
view IDs and weak graph references. The ordinary identity table remains a
fallback. Reverse load flags and relation facets retain their view identity.
This uses the decoder's existing typed-row callback; it does not parse JSON,
install caller capabilities or perform additional database queries.

An unmatched forward object retains its original NULL/NotLoaded behavior. An
intermediate repair changed that boundary and was rejected by the same corpus;
the correction restores it. Intermediate failures and the single-case trace
are retained separately. Temporary tracing was removed before final checks.

## Source and execution evidence

All **1,944/1,944** original/native outcomes now agree without normalization.
The matrix covers eight original entry points, full/minimal parents and children,
six child configurations, request/selection/core conversion, compact/typed/page
execution and both facet options. Complete query/options, child immutability,
typed JSON/load state, facet rows, SQL, bindings, intent and trace are compared.

There are **1,584 successful executions**, **3,420 result rows**, **192 facet
rows** and **3,432 safe SQL records**. The remaining **360 outcomes** preserve
the original missing-child-comment error in facet execution; they are failures,
not successful facet loads. Named relationships and enhanced grouped graphs
retain their distinct source semantics.

The independent Rust observer keeps reference registry versions/checksums and
uses unchanged generated Rust and runtime sources. The C++ default PMR resource
is disabled. Each observation actually deletes its UserContext and checks zero
retained User bytes. The lifetime fixture checks snapshots, foreign request/
selection/core rejection, explicit copies, expiry and Global retention.

Final native source/lifetime and relation/facet regressions pass **9/9**.
Selected ASan/UBSan checks pass **4/4**, with leak scanning disabled; imported
module checks pass **3/3**. Canonical consumer verification covers **671**
bodies, module lookup covers **1,485** exports, and formatting checks **1,455**
C++ files. All compiler/linker/test processes have successful terminal exits.

The broader regression also found missing facet metadata on reconstructed
relation lists. The synchronous typed-row edge decoder now copies the existing
facet collection into that list, while the view-scoped facet index is retained.
The old Global facet fixture now rejects User capture and uses an explicit
Global copy before capture. These repairs and initial failures are recorded.

No producer or generated file is changed in this checkpoint. Generator delegates,
Assist, controlled reproduction and generated compiler/Java gates remain the
next work. No full-suite, MIPS or physical-device qualification follows.

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
  teaql_generated_relation_analysis_inputs_probe teaql_relation_analysis_input_tests
python3 scripts/verify_relation_analysis_inputs_5_0_6.py
ctest --preset native --output-on-failure \
  -R '^relation_analysis_input(s_rust_parity|_context_contracts)$'
```

The [source review](parity/relation_analysis_inputs_source_review_5_0_6.json)
retains eight actual original method bodies. The initial corpus, compiler-fixture
failure and intermediate repair/trace records remain immutable. Complete
G06/G08/G12/G13 and P4/P5/P7 requirements remain active.

[Final terminal evidence](parity/relation_analysis_inputs_terminal_5_0_6.json) records the focused gate scope; raw validation and failed intermediate records remain separate.

Before closing the graph responsibility, extend the composed metadata matrix
to NULL forward objects carrying nonempty loaded facet metadata and repeated
installation into an existing view-bearing builder. The current forward-NULL
branch retains the source alias behavior, but its early return precedes scoped
facet installation; that combination needs a focused source comparison and
correction before general acceptance. This remains required work, alongside
generated input adapters and the other original conversion families.

The [subsequent adapter and boundary checkpoint](relation_analysis_adapters_5_0_6.md)
qualifies NULL-forward facet preservation and repeated installation with a
builder-owned view namespace, then completes the scoped generated conversions.
The earlier terminal/source hashes above retain their historical snapshot;
current hashes and integration gates are recorded independently in that follow-up.
