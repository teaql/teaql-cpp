# Complete hydration allocation source review: Rust 5.0.6

Status: accepted after complete original and installed native/module verification.
This review concerns the complete 196-line
`teaql-runtime/tests/hydration_allocations.rs`, not the entire hydration runtime
or generator capability.

## Responsibility correspondence

| Original responsibility | Installed C++ counterpart |
| --- | --- |
| Indexed HydrationProbe with ID, version, name, optional note and load state | Retained generated `cpp_hydration_allocation::HydrationProbe`, produced from the dedicated model. No handwritten model substitutes for its decoder. |
| Stable four-field indexed layout and logical/physical mappings | Generated field layout and fixed model metadata. The test checks canonical layout and shared entity-name storage within the owning Context; no cross-language revision-string or address identity is claimed. |
| Plain helper requires U64 ID, I64 version and Text name | `decode_plain` checks the exact Value alternatives before extracting them. Signed ID and unsigned version are rejected rather than coerced. |
| Plain note: absent/NULL means None, Text means Some, other values reject | `decode_plain` preserves all branches. Supplemental controls cover absent, NULL, empty Text, long Text, and rejected ID/version/name/note variants, including destination string allocation. |
| Prepared input rows and layouts are outside measurement | `rows` constructs all inputs before `measured`; layouts, warm-up, registry and roots are prepared outside the counted action. Database I/O and logging are absent from these measured operations. |
| Sparse/full shapes at sizes 1, 100 and 10,000 | `sample` retains both projections and all three sizes. Sparse omits note; full includes a real NULL note. IDs start at one, version is one, name is `sample name`. |
| Warm snapshot retains canonical layout and shared entity-name storage | The warmed generated decoder supplies a LoadedSnapshot. Identity checks compare its layout and repeated generated entity-name storage before the measurement grid. |
| Typed hydration collects actual generated entities | The typed path decodes and moves each prepared row into `Vector<Probe>`. Every entity is checked for scalar values, note availability, shared snapshot identity and empty dirty fields. |
| Context hydration shares snapshot shape but keeps row comments private | The contextual path uses one bound EntityRuntimeState, checks every entity, then sets the first row's comment and verifies the second remains unannotated for multi-row cases. |
| Identity batch decodes directly into the graph builder | The actual generated registry decodes the compact batch into EntityGraphBuilder. Freezing occurs after measurement; first and last identities resolve with the warmed snapshot. No temporary typed vector is inserted into this path. |
| Original allocation ceilings | Typed `3*n+1`, Context `2*n+1`, graph `3*n+32`; both arena requests and replaced-new heap calls must satisfy them independently. No ceiling is introduced for the original unbounded plain baseline. |
| Plain baseline retains and checks every decoded result | `Vector<Plain>` is retained; ID, version, name and absent note are checked for the full 24-row measurement grid. |
| Measurement returns the action result and reports calls, bytes and elapsed time | The test-only counter resets before each action and disables before output, including exceptional exits. Result values are consumed by checks. Both arena/heap metrics are retained; time and byte totals are not claimed to be cross-language equal. |

The source's four paths, two projections and three sizes produce exactly 24
measurement rows. The supplemental helper controls are additional cases and
must not be reported as part of those 24. The source helper accepts text notes
although its measured input generator only creates missing/NULL notes; testing
only the measured grid would leave that helper branch unqualified.

## C++ ownership and measurement adaptations

Context arenas retain their blocks until explicit Context destruction. Reserve
uses the known input cardinality to obtain result-vector storage once, matching
the original sized collection intent. Standard string short-string storage can
reduce observed allocations; the test retains real decoded strings and adds a
long-string move/deep-copy control. The existing Context allocation limits are
raised only for this large test fixture. No query-private ownership domain,
hidden singleton or lifetime extension is added.

Rust's global allocator count and C++'s arena-request count are different
instrumentation boundaries. The C++ test therefore checks the original ceiling
against both arena and heap counts separately, with real positive controls for
both Context scopes and ordinary/aligned heap allocation. Adding these two counts
would double-count different layers and is not an accepted object-count metric.
The [counter review](allocation_counter_source_review_5_0_6.md) covers that helper.

The move control additionally verifies original-row buffer transfer, no allocation
for a same-Context move, and explicit deep copying into Global ownership across
actual UserContext destruction. These checks support the C++ optimization; they
do not replace any original hydration assertion.

## Evidence and outstanding acceptance

The [allocation audit](remaining_allocation_source_audit_5_0_6.md) retains the
unchanged original 24-row run and earlier native/sanitizer measurements. The
[Plain helper original experiment](parity/plain_hydration_helper_original_5_0_6.json.gz)
uses the unchanged original helper body for its extra input cases; panic text is
not claimed identical to C++ exceptions. The complete helper is now installed.

The current installed verification is
`parity/source_completion_candidate_integration_5_0_6.json.gz`. It requires both
native and canonical imported full grids, exact helper/ceiling success markers,
canonical body preservation, no-work rebuilds, stable binary hashes and final
source/reference checks. A running or failed receipt is not acceptance. The
source disposition remains pending until that evidence is terminal and reconciled
with this review. No claim is made about every provider, GPU support, physical
MIPS execution, all generator capabilities or P0-P7 completion.

## Final source acceptance

The complete source responsibility reviewed here is accepted under the
[current closure evidence](hydration_trace_core_source_closure_5_0_6.md).
Original and installed native/imported gates are terminal and successful; the
relevant focused concurrency sanitizer evidence is retained with its own scope.
Earlier pending/isolated-candidate statements describe historical checkpoints.
This accepts only the named source, not the enclosing runtime, provider, generator
capability or P0-P7 goal.
