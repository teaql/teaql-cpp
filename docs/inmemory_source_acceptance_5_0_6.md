# Complete memory query and evaluator source review: Rust 5.0.6

Status: IM01-IM14 are qualified; the complete source responsibility is accepted
with the standing ownership, sorting, clock and target-width dispositions. The reference is
`teaql-runtime/src/inmemory_engine.rs` at runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; the generator reference remains
`45e1572c4956ca963410d8ea436ead3c8857087a`. The earlier
[5.0.5 contract](inmemory_engine_contract.md) retains its historical evidence.

## Complete source responsibilities

Production lines 1-511 retain two public abstractions, eleven public/private
methods and six free helpers. Lines 513-723 contain the two fixture helpers and
nineteen original tests. The review follows actual branches and calls, rather
than treating that inventory as proof.

| ID | Responsibility | Body disposition and evidence |
| --- | --- | --- |
| IM01 | Public engine/evaluator boundaries | Separate `InMemoryQueryEngine` and `ExprEvaluator` remain in the runtime package. Their pure row operations use explicit allocation resources rather than contextual services. They add no providers, logging calls or authorization capability. |
| IM02 | Execute pipeline and aggregate short-circuit | Filter precedes aggregation. Aggregation returns before ordinary sorting, slicing and projection. Otherwise stable sort, pagination and projection run in that order. Source metadata records actual final count and backend `memory`. The 6,532 retained observations cover ordinary and grouped/alias/short-circuit paths. |
| IM03 | ExprEvaluator.eval | Every Expr variant keeps its source branch. AND/OR use source empty identities and short-circuiting. Bare column/value/LikePattern predicates are true only for Bool(true); functions and subqueries are false. The new typed-pattern matrix closes the 5.0.6 variant omitted by the historical observer. |
| IM04 | ExprEvaluator.resolve | Columns clone their actual value or strict Null; literals clone their value. LikePattern resolves the pattern string, never its separate original text. Predicate expressions become Bool; functions/subqueries become Null. Destination-aware C++ copies preserve nested allocators. |
| IM05 | Binary operators and value comparison | All twelve operators remain distinct. Eq/Ne and list membership use raw Value equality; non-list IN falls back to scalar equality. Comparisons keep signed/unsigned full width, same-variant primitive/Decimal/text/date/time rules and source numeric fallback. Incomparables and NaN compare Equal for ordering, without becoming equal Values. |
| IM06 | LIKE helper and Unicode matching | Percent accepts any number of Unicode scalars, underscore exactly one. Backslash has no extra escape meaning. Non-text operands return false for LIKE and true for NOT LIKE. The existing iterative C++ matcher replaces Rust's temporary scalar vectors/backtracking while retaining observed matches. Original private-helper assertions are also reached through the native public evaluator. |
| IM07 | Stable sort and source partial-order boundary | Multi-column field order is stable; missing fields compare as Null and expression ordering is ignored. PMR index/merge scratch avoids a hidden heap owner. The standing non-total-comparator disposition remains: small original partial comparisons and larger total-key sorts are verified; no portable private-sort permutation is promised for large invalid order relations. |
| IM08 | Target-width pagination | Preserve explicit usize/size_t casts of offset/limit. Missing limit takes the remainder; zero and maximum values retain their semantics. This checkpoint executes native64. Historical 32-bit expectations and MIPS evidence are not relabeled as current execution. |
| IM09 | Projection and row metadata | Empty projection keeps complete input rows, layouts and read sidecars. A nonempty projection passes through sorted Record conversion, keeps only present requested keys and drops query-only sidecars/schema/marks. Duplicate keys use the last map value. The new 400 pipeline cases inspect complete rows in nine Debug modes and verify borrowed input isolation. |
| IM10 | Group construction and aliases | Groups use raw Value-vector equality and encounter order; absent group values are strict Null. Group values are inserted before aggregates, so aggregate aliases may replace them or earlier aliases. Empty ungrouped input yields one aggregate row; empty grouped input yields none. |
| IM11 | All aggregate variants | Count returns I64 and skips only strict Null for a named field. Sum/Avg use actual numeric conversions and return Null if none exist; extrema skip strict Null and retain the original winning type/value. All seven unsupported functions return Null. Retained direct observations and all nineteen source tests preserve these source choices. |
| IM12 | Complete execution metadata | Operation Query, backend memory, final count and system times remain distinct from input query provenance. Statements, SQL log, debug query, affected rows, trace, comment, backend request ID, parameterized query and bindings retain source empty defaults. The new observer checks actual timestamp ordering and both call brackets; absolute clock values are not a deterministic wire contract. |
| IM13 | Ignored query fields and trusted layering | Rich requests populate ignored SQL, expression-projection, grouping-without-aggregate, relation/facet, cache, partition, hard-limit, dynamic-selection/schema, stream and pagination-hint state. These fields neither execute SQL nor manufacture loaded values. Runtime intent/Policy/provider duties remain with their separate layers. |
| IM14 | Ownership, copies and original tests | Same-domain consuming execution and explicit-destination borrowed execution retain their C++ bindings. All result storage, grouping keys, sorting scratch and resolved values use the destination. Actual source User destruction follows explicit Global copies; ordinary results remain Context borrows. The original nineteen-test port now also compiles and executes as an unchanged canonical module consumer. |

## Exact-source continuation

The independent `rust_inmemory_source_5_0_6` project builds the unchanged retained
oracle alongside a new observer. Registry versions and checksums must be a subset
of the frozen original lock. No earlier observer, manifest, lock or corpus is
rewritten. Actual original runtime unit tests execute through the original crate.

The 6,532 retained cases reproduce with current reference dependencies. Their
complete Value variants, outputs and metadata properties stay unchanged. A further
2,200 inputs comprise:

- 1,800 typed-pattern observations: six patterns, three independently retained
  original-text values, missing/present field values, all twelve operators and
  standalone/null/not wrappers. Long, NUL and Unicode text distinguish pattern
  execution from diagnostic provenance. Eval and complete resolved-value Debug
  in nine modes are observed through real public methods.
- 400 execution observations: ten absent/empty/populated relation/dynamic/readonly
  metadata profiles, five pipelines, minimal/rich requests, empty/nonempty input,
  and native borrowed/consuming entries. Original public execution receives
  owned rows; borrowed native input isolation is compared with the original
  clone-before-call control.

The new native observer disables default PMR allocation, explicitly copies each
resolved Value or QueryResult into Global through UserContext, destroys the source
UserContext and checks all retained row states through new bindings. Execution
metadata keeps its Global allocator, backend and result count after destruction.
Nested child text, dynamic fields and readonly schema survive only through those
explicit destination copies.

## Preserved failures and implementation boundary

The first original observer used C++'s `all()` factory spelling instead of Rust's
`All` enum variant. Its [compiler failure](parity/inmemory_source_initial_adapter_failure_5_0_6.json.gz)
is preserved. The native observer initially omitted the complete FacetRequest
header and used copying/implicit-boolean syntax for move-only Expr/explicit Ref.
That [failed build](parity/inmemory_source_initial_native_failure_5_0_6.json.gz)
is also preserved. Both repairs change observer adapters only.

An explicit parity registration duplicated an existing generic module entry.
The [failed configure](parity/inmemory_source_initial_module_configure_failure_5_0_6.json.gz)
stopped before any module compilation. Removing only the duplicate preserves
the existing comparator; the new source comparator and original test consumer
remain independently registered. No failed attempt is passing evidence.

The production engine/evaluator header and implementation, original native
contract/test bodies, retained probes and all generated SDK artifacts remain
byte-identical to the pre-review snapshot. No runtime algorithm, Context lifetime,
asynchronous cleanup, generator or public API implementation changes are needed.

## Final verification

The [qualification record](parity/inmemory_source_qualification_5_0_6.json.gz)
joins the [fresh original/native/imported replay](parity/inmemory_source_final_5_0_6.json.gz),
[native](parity/inmemory_source_native_installed_5_0_6.json.gz) and
[imported](parity/inmemory_source_modules_native_installed_5_0_6.json.gz) selections,
and [exact preservation proof](parity/inmemory_source_preservation_5_0_6.json.gz).

| Gate | Terminal result |
| --- | --- |
| Original tests | 19/19 from the unchanged runtime crate |
| Retained source corpus | 6,532 exact original/native/imported outputs; corpus bytes unchanged |
| New source corpus | 2,200 exact original/native/imported outputs; fresh source reproduction preserves corpus bytes |
| Native | 4/4 selected checks, then a no-work build |
| Modules | 4/4 selected checks, including the complete original test body as an imported consumer, then a no-work build |
| Context controls | Actual source User destruction before reading all explicitly retained values/results; default PMR disabled |
| Formatting and module provenance | 1,694 handwritten C++ files; 959 canonical consumer/fixture bodies |
| Preserved artifacts | 78 SDKs/3,457 generated files and 2,201 protected artifacts; no new generation/Java claim |

Every compiler, linker, observer and test process exited successfully. Current
source status advances to **52 accepted / 70 pending / 2 excluded_external**;
all seventeen capability statuses remain unchanged. Native discovery is **763**
and imported discovery **519**, not whole-suite pass claims. Neither failed
historical full epoch is relabeled. No sanitizer, MIPS, external provider or
physical-device run is added for this observer/qualification-only change.

This accepts the complete engine/evaluator source and its original tests.
Complete G08/G13/G17, query-service/provider orchestration and P0-P7 keep their
independent exits.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --parallel 2 --target teaql_inmemory_source_probe \
    teaql_inmemory_probe teaql_inmemory_tests teaql_original_inmemory_tests
replay_directory=$(mktemp -d)
python3 scripts/verify_inmemory_source_5_0_6.py \
    --record "$replay_directory/engine.json.gz"
ctest --preset native --no-tests=error --output-on-failure \
    -R '^(inmemory_(source_rust_parity|rust_parity|context_contracts)|original_runtime_inmemory_contracts)$'
```
