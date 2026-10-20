# Generated query return types: Rust 5.0.6

The later [result-marker qualification](generated_query_result_marker_5_0_6.md)
supersedes this checkpoint's early non-Entity construction refusal. Ordinary
requests now admit arbitrary markers, matching the original; public execution
and creation retain the enclosing Entity guard. The original compiler fixture
and previous behavior remain archived separately.

Current status: G04/P2 is qualified with documented language/ownership adaptations;
normal inheritance generation is enabled. See [the final acceptance](inheritance_acceptance_5_0_6.md).
The checkpoint status and guard statements below are retained historical evidence.

Status: partial G04/G06/G09 evidence. Normal inheritance generation remains
guarded. This extends the earlier inherited field, query-construction and SQLite
enhancement checkpoints; it does not qualify complete graph/save behavior.

## Source and result responsibilities

Runtime reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Original generator: `45e1572c4956ca963410d8ea436ead3c8857087a`.
The original request/index.stg `return_type<T>` transfers the existing request
description into a request with a different decoded result type. Query entity,
filters, projection, child selections and options remain unchanged. Original
`new_entity` still constructs the source model even after result adaptation.

The runtime counterpart is `EntityQuery<Source, Executor, Purposed, Result>`;
Result defaults to Source, preserving existing three-argument uses. Generated
requests add a defaulted result parameter after the existing purpose parameter.
`returnType<Model>()` delegates to runtime. It changes list/one/stream decoding,
while repository lookup, query defaults and creation continue to use Source.
Relation and enhancement adapters accept adapted child requests without changing
their source-entity constraint or bypassing Context identity checks.

Lvalues produce independent Context-owned descriptions; rvalues transfer the
checked reference. Purpose transitions, further adaptations, explicit destination
copies and Global retention/rebinding preserve both source and result types.
Adaptation does not add projections or fetch missing child fields. Sparse fields
remain NotLoaded. Returned models and E expressions remain Context borrows.

## Application error boundary

The original generated runtime maps service failures to RuntimeError::Graph.
`detail/query_results.hpp` implements that boundary once for the C++ facade,
including stream item failures. The existing C++ DataServiceError carrier remains
the public carrier; its runtime branch stores the mapped Graph error. Direct
data-service callers retain the underlying error protocol. Result/stream adapters
use the execution Context, forward wakes and cancellation, and consume their
private child results without extending Context lifetime.

The paired probe preserves complete error text, including the source's nested
Graph prefix. It does not normalize failure messages or implement unsupported
storage. This SQLite model has no `internal_type` or `type_group` columns: type
filter/group requests preserve their API but fail SQL compilation, as source does.
Source streaming rejects relation/child enhancement; the C++ facade preserves
that rejection. Under the existing C++ async protocol, unsupported stream shape
is reported by the start operation; the original lazy stream reports it when
polled. The corpus observes the execution outcome and exact error, not identical
error delivery timing.

## Evidence

The complete, unchanged original inherited crate supplies the runtime, requests
and model types. The SQLite oracle now varies source and result models
independently across School, Academy and ResearchAcademy. It compares list, one
and chunk-size-one streams; normal/minimal/with-children entries; automatic and
explicit enhancement; slicing; model identity; loaded fields; native serialization;
and complete original snapshots.

All **702 observations** match: 492 successful outcomes, 156 unsupported stream
enhancement errors, 27 missing type-column errors and 27 missing type-group errors.
The retained corpus is `tests/parity/generated_return_type_5_0_6_cases.json.gz`.
Original output, baseline hashes and producer provenance are recorded separately
from C++ artifacts. The Rust harness extension leaves the earlier 144-case
inheritance output unchanged for its original inputs.

Native application tests additionally verify helper/lvalue independence, sparse
decoding, purpose preservation, source-model creation, explicit User/Global copies,
and query/E invalidation after actual source UserContext deletion. They disable
default PMR allocation. Compiler contracts reject non-entity result types,
execution without purpose and extra Context arguments, and retain wrong-relation
rejection. Modules consume the same generated domain through runtime imports.

Final selected native checks pass 12/12. Targeted sanitizer checks pass 3/3 for
the adapted lifetime contract, existing query lifecycle contract and source
outcome corpus; the corpus was replayed again after adding all 54 type-column
failures, with leak scanning disabled. Module import/integration passes 1/1.
Selected Java tests pass 14/14. All seven artifact sets reproduce byte-for-byte
(77 files). Formatting checks 829 C++ files; active path/source integrity checks
pass for seven packages, 116 source units and all 124 Rust source links.

## Remaining work

Qualify inherited graph/save, mutation/readback/conflict behavior and complete
generated model configurations. Streaming cleanup failure matrices and exact
error timing remain distinct from successful/error outcome comparisons. General
Q execution/pagination, readonly hooks, assembly/bootstrap and the remaining
P0-P7 capability matrix remain open. This checkpoint cannot remove the normal
inheritance guard or establish full generator parity.
