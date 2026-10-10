# In-memory engine source contract

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-runtime/src/inmemory_engine.rs`, production lines 1–511.
The separate ExprEvaluator and InMemoryQueryEngine public abstractions are both
retained. They operate on rows and allocation resources; contextual locking,
logging and metadata services are not required by their source methods.

## Public methods and production behavior

| Responsibility | C++ mapping and semantic contract |
| --- | --- |
| InMemoryQueryEngine.execute | Consuming row overload plus explicit-destination borrowed extension. Filter first; aggregates short-circuit; otherwise stable sort, paginate, then project. |
| ExprEvaluator.eval | All expression variants remain distinct. AND/OR short-circuit with source empty identities; bare values require Bool(true); functions and subqueries are false. |
| ExprEvaluator.resolve | Explicit destination copies of columns/literals; missing columns, functions and subqueries resolve to strict Null. Other expressions resolve to Bool. |
| Binary comparison | Eq/Ne use original Value equality; signed/unsigned ordering preserves full-width integers; other cross-type numerics use try_f64. Incomparable values and NaN compare equal for ordering. |
| Null behavior | Strict Null remains distinct from TypedNull in predicates, count and extrema, even where numeric conversion or ordering is unavailable. |
| LIKE / NOT LIKE | Case-sensitive scalar matching: percent accepts zero or more scalars; underscore accepts one scalar. Non-text operands return false/true respectively. The iterative matcher replaces recursive allocation without changing matches. |
| IN / NOT IN / large counterparts | List membership uses Value equality; a non-list right operand falls back to scalar equality. |
| Sorting | Field lookup uses Null for absence and ignores the OrderBy expression field, as the source does. Asc/Desc and multi-column order preserve equal-key encounter order. PMR scratch replaces an unscoped temporary allocation. |
| Pagination | Offset/limit convert to the executing target's size_t, preserving source usize casts. Absence of limit means the remainder. |
| Projection | Empty projection keeps the original layout; nonempty projection uses an ordered map and retains only present requested fields. Duplicate row keys and projection names follow the source map behavior. |
| Aggregation | Encounter-order groups use raw Value-vector equality. Group fields precede aggregate insertion; aliases can replace group fields or earlier aliases. Without group fields an empty input produces one aggregate row; grouped empty input produces none. |
| Aggregate values | Count returns I64; Sum/Avg use convertible numeric values and return Null when none exist. Min/Max skip strict Null and retain the original typed extreme. All seven unsupported aggregate variants return Null. |
| ExecutionMetadata | Query operation, backend memory, actual result count and bracketed system times. Statements, SQL logs, debug query, affected rows, trace, comment, request ID, parameterized query and params keep source empty defaults. Input query provenance is not copied into these fields. |

The source ignores HAVING, hard limit, standalone group fields without aggregates,
expression ordering and query comment in this engine. The C++ implementation
preserves these decisions rather than extending the engine's interpretation.
No new error variants, serialization derives, database dispatch or generators
are introduced.

## Ownership and architecture

Results, resolved values, row layouts, projected maps, grouping keys and sorting
scratch use the explicitly selected resource. Same-domain consuming execution
moves rows; destination-aware cross-domain execution deep-copies nested strings,
lists and shared layouts. Native contracts disable default PMR, retain complete
results and resolved values in Global through UserContext, delete the source
User and verify those results from a new User. Unretained results remain borrows
with the usual Context lifetime. Returning blocks does not imply temporary bytes
are reclaimed before Context close.

The source comparator can be non-total for mixed types or NaN. Its sort_by
contract does not guarantee a portable permutation for such large invalid-order
inputs. Evidence includes the original small-input ordering and large total-key
stable sorting; it does not claim identical private Rust sorting algorithms or
cross-platform ordering outside that contract. C++ uses an explicit stable merge
to avoid a standard-sort strict-weak-order precondition on source values.

## Evidence and open gates

The unchanged public-call corpus contains 6,532 observations covering expression
variants, all BinaryOps, cross-type values, strict/typed Null, nonfinite floats,
Unicode LIKE, aggregate variants/aliases/groups, pipeline short-circuiting,
projection, stable sorting and boundary pagination. Original clock timestamps
are nondeterministic; observations compare their ordering and complete remaining
metadata instead of captured absolute times.

All nineteen original source tests have direct reviewed C++ ports in
`tests/parity/teaql_runtime/inmemory_engine.cpp`. Private LIKE assertions enter
through ExprEvaluator so they exercise the same production matcher. These ports
are separate from the broad generated public-call corpus and allocator tests.

On a 64-bit Rust host, MIPS expectations narrow only pagination inputs at the
source's explicit usize cast boundary, then invoke the unchanged public engine.
This is a documented architecture adaptation, not a Rust binary executed on MIPS.
The C++ probe reports its actual pointer width before selecting expectations.
Fresh three-target builds finish before selected checks. ASan/UBSan passes
3/3 in 4.63 seconds with leak scanning disabled; MIPS/QEMU passes 3/3 in
2.75 seconds. The MIPS probe matches all 6,532 live original observations with
the documented target-width adaptation. Final current native integration
passes 318/318 in 133.86 seconds after a terminal no-work build. This accepts the complete source group within the
documented ownership, target-width and sort contracts. Complete runtime and
Stage 4/6 acceptance remain independent.
