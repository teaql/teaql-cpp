# Row diagnostics and invalid request observations

Status: shared diagnostic implementation and three source selections corrected;
native/source, targeted sanitizer and canonical module gates pass. Full G13/G15/G16/G17/P7 remain open. Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Runtime representation

CompactRow formatting now reports its actual CompactRowLayout, values and
optional LoadedRelationResults. Layout formatting includes names, relation
availability, readonly property definitions, primary loaded snapshot and other
type memoized snapshots. Sidecar formatting reads actual relation lists and
durable DynamicFieldValues independently from physical native projections.
FieldLayout, LoadedSnapshot and DynamicPropertyDefinitions also have diagnostic
writers. The shared implementation lives in core; service/result/list/facet
formatters delegate to it rather than reimplementing rows.

Diagnostics borrow existing data and allocate output in the explicit destination.
They do not issue SQL, create sidecars, initialize loaded state or change schema.
Ordinary C++ rows retain their lazy empty-layout implementation. No artificial
mutex, OnceLock object, reference-count owner or new runtime dependency is added.

## C++ ownership and cache adaptations

The ordinary layout diagnostic uses authentic source container vocabulary:
OnceLock describes its primary memoized state and Mutex describes its secondary
state map. C++ stores scoped borrows, not Rust Arc/Weak objects. The poison marker
is always false because this Context-local map has no poisonable synchronization
object; the label does not claim an additional C++ mutex or thread-safety contract.
Initialized secondary entries show actual C++ snapshot data and scoped allocation
identities instead of Rust Weak pointer diagnostics.

FieldLayout owns immutable C++ geometry, while state-interning hints belong to
the result Context's separate SnapshotPool. Its formatter is therefore explicitly
nonexhaustive and does not invent Rust's type-local states mutex. Readonly schema
diagnostics report the current C++ fingerprint; its std-hash numeric value is not
qualified as Rust's private SipHash value. Complete nonempty schema/hash/cache
diagnostic source qualification remains a named G15/G16 requirement. Exact source
equality below is limited to the historical uninitialized layouts, not these
implementation-specific initialized cache diagnostics.

## Data-service observer failure handling

The maintained C++ observer now checks QueryIntent construction before accessing
its value. It returns the authentic RequestIntentError code, field and text for
the same invalid historical inputs. Runtime validation is unchanged; no default
comment or fallback authorizes a blank request. The existing adapted Rust driver
uses the authentic 5.0.6 request/envelope interfaces and the recorded caller labels.
Its **27/40** invalid inputs reject early; **13** produce full request/result
diagnostics, now using the corrected shared row formatter.

## Source and native evidence

All unchanged historical inputs match actual frozen Rust plus finalized native
C++: model/row/list diagnostics **169/169**, request helpers **872/872** and
data-service models **40/40**, totaling **1,081**. Existing versioned corpora are
checked without replacement or normalization. All three active CMake selections
now use the separate 5.0.6 comparators. Initial discrepancy records and original
5.0.5 artifacts remain intact.

Relevant native source/model/row/presentation/dynamic-carrier/list gates pass
**8/8**. A new regression exercises nonempty readonly schema, two actual cached
model layouts, relation and durable sidecars, unchanged snapshot identity and
no new managed-object allocations during formatting. It disables default PMR,
retains a full row through an explicit Global copy, actually destroys the source
Context and verifies later diagnostic access to copied metadata in a new Context.
Those initialized-state checks are C++ ownership evidence, not extra Rust cases.

Pinned format/check passes **990** handwritten C++ files. The module export
catalog remains 1,477 declarations; the new overloads belong to the existing
canonical write_debug export. A canonical model-debug consumer is added to the
module contract manifest and presets. Targeted ASan/UBSan passes 2/2 with leak scanning disabled; the canonical
model-debug module consumer passes 1/1 after both builds finish. All 96 canonical
consumer/fixture bodies verify. Host SQLite is not sanitizer-instrumented. No MIPS or
physical-device acceptance is inferred.

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family model_debug --family request_helpers --family data_service_model \
  --rust-dir /tmp/teaql_cpp_field_layout_rust_506/debug \
  --cpp-dir build_native_current --check
ctest --preset native --output-on-failure \
  -R '^(model_debug_contracts|model_debug_rust_parity|request_helpers_rust_parity|data_service_model_contracts|data_service_model_rust_parity|core_shape_presentation_contracts|dynamic_carrier_contracts|smart_list_api_contracts)$'
```

The complete source/generator/configuration matrix, other historical replay and
final new-baseline integration are still required. Prior complete native runs
are retained as diagnostic history, not success of subsequent edits.
