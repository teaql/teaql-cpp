# Relation facet ownership and grouped top-N planning

This closes the two adjacent source gaps identified by
[relation planning replay](relation_planning_replay_5_0_6.md) within a new explicit
planning matrix. Full G08/G09/generated/model/configuration acceptance remains
open.

## Actual source observations

The original complete `query_for_plan`, `query_for_compact_plan` and
`base_relation_query` methods execute through appended test access bridges.
Original method bodies remain intact. The
[source provenance](parity/relation_planning_extensions_5_0_6_source_provenance.json)
records a completed 22-observer build, 610 original archive file prefixes and
original registry versions/checksums. The original unused executor panics on
any capability lookup or provider I/O.

All **1,152/1,152** actual original/native observations match and reproduce,
including **3,456** planned snapshots and **1,152** reusable caller snapshots.
Dimensions vary independently:

- Four group layouts: empty, one field, two fields and duplicate fields.
- Scalar/aggregate queries and absent/present offset/limit.
- Six existing orders: empty, direct group-field, expression group-field,
  entity-ID, second group-field and both direct group-fields.
- Facet ownership, metadata with/without an ID property and empty/NULL-only/
  mixed valid-and-NULL parent keys.

Each snapshot records entity, projection, group fields, aggregate descriptors,
facet/relation counts, partition, filter presence, slice and ordered fields with
expression flags. It does not qualify complete filter serialization, every
ordering expression or SQL execution parity. The earlier 112 planning inputs
retain complete filter snapshots. See the
[driver review](parity/relation_planning_extensions_5_0_6_driver_review.json).

The first source observer needed an explicit request-module FacetRequest import;
the C++ observer needed complete request types and an unsigned JSON accessor.
These fixture-only [source](parity/relation_planning_extensions_5_0_6_initial_source_error.json)
and [C++](parity/relation_planning_extensions_5_0_6_initial_cpp_error.json) build
failures remain recorded.

## Shared runtime corrections

The initial comparison found **777/1,152** differences: copied facet ownership
affects 576 observations and stable ordering affects 402, with overlap. The
[initial outputs](parity/relation_planning_extensions_5_0_6_initial_gaps.json)
remain intact.

The shared relation query constructor now clears copied facets alongside nested
relations. The planner owns their recursive execution; its prepared child query
must not execute them again. The caller retains its reusable selection and
facet tree. The existing base helper delegates to this same constructor, so
normal, compact and base routes share the correction.

For sliced grouped/aggregate queries, stable ordering now appends only missing
direct group fields. An expression order on a group field does not substitute
for its direct field order. Duplicate group names do not add duplicate direct
orders, and existing direction/order is preserved. Aggregate-only queries do
not gain a source-row identity order. Ordinary sliced rows retain their metadata
ID tie-breaker; unsliced queries gain no new order.

No public API, generated artifact or dependency changes. Context-bound ownership
and explicit destination copying are preserved.

## SQLite and lifetime integration

The real SQLite relation regression adds grouped counts with per-parent offset
and limit. Both capture modes verify three expected grouped rows, actual counts,
missing group-field ordering, absence of the unrelated entity ID tie-breaker,
and existing batch versus individual-probe behavior. This is C++ integration
evidence, separately from the original planning snapshots.

The new contract also runs with a failing default PMR resource, checks caller
facet reuse and explicitly copies nested diagnostic results into Global memory
before destroying UserContext. The same contract body runs through canonical
import-only modules; no production implementation is copied into fixtures.

## Reproduction and terminal gates

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
cmake --build build_native_current --target teaql_relation_planning_extensions_probe
python3 tests/parity/replay_relation_planning_extensions_5_0_6.py \
  --rust /tmp/teaql_cpp_private_save_rust_506/debug/relation_planning_extensions_5_0_6 \
  --cpp build_native_current/teaql_relation_planning_extensions_probe --check
```

Relevant native tests pass **13/13**, including existing generated facet async,
source and relationship-analysis contracts. Targeted ASan/UBSan passes **2/2**
(leak scan disabled), canonical module contracts **2/2**, and **109** canonical
consumer/fixture bodies verify. Pinned format/check covers **994** handwritten
files; module paths verify. Compiler/linker and test processes terminate before
success is recorded. See the [terminal record](parity/relation_planning_extensions_5_0_6.json).

One new direct comparator selects the versioned matrix; the
[binding inventory](parity/active_corpus_baseline_after_planning_extensions_5_0_6.json)
counts 133 active, 66 historical and 21 custom selections. Static bindings do
not prove behavioral acceptance. Additional relations, expressions, metadata,
SQL/privacy/generated/model configurations and P7 remain open.
