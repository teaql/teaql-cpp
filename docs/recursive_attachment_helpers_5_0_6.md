# Generated recursive runtime attachment: Rust 5.0.6

The complete unchanged original relations domain and the generated C++ domain
produce **192/192 identical observations**. This closes the forward-child
attachment review in the G01 helper matrix; it does not qualify database writes
or the complete runtime assembly.

## Source contract and delegation

The locked producer's entity/struct.rs.stg first adopts the object's mutations
into the target state, replaces its runtime state, then recursively attaches
stored forward objects. The derive macro's replacement retains the object's
loaded view through with_loaded_view_from. Reverse relation views are not
recursively visited by this template.

CppCoreGenerator emits the corresponding attachRuntimeStateRecursive adapter.
The shared EntityRuntimeState::attach_runtime_state owns adoption and loaded-view
preservation; the generated adapter visits its stored forward objects. No
production runtime or generator function changed for this checkpoint.

Attachment requires the same C++ allocation domain. A different Context requires
an explicit destination copy before attachment. This ownership adaptation retains
the existing C++ contract rather than allowing a root to retain another Context's
storage implicitly.

## Observations

The two actual models are School (required platform) and Device (optional
platform). Inputs combine embedded versus absent child objects, full versus
partial parent availability, plain/new/deleted children, empty targets and targets
with an existing parent change, child change or conflicting original version.
Text includes ordinary text and long Unicode/embedded-NUL inputs.

The observer calls the real generated recursive attachment method. Before and
after projections retain parent and child getters/evaluation, original loaded
values, selected fields, ordered changed fields, comments, original versions and
new/deleted flags. Subsequent target changes verify shared reads; the old root
is inspected separately. Embedded forward objects remain available through the
getter even when the parent relation field is unselected, preserving local-value
precedence.

Version-conflict cases compare the actual resulting public state. The private
composition-error diagnostic and later Save rejection are not observed here;
their separate runtime-ledger contracts remain applicable. Neither probe runs
SQL or replaces the original method body.

## Ownership and verification

Native, named-module and ASan/UBSan CTest each pass 1/1. Native observers disable
the default PMR resource, explicitly copy their output into Global, destroy
stack PMR objects, delete UserContext, then serialize the retained output.
Sanitizer leak scanning is disabled. MIPS and physical-device tests were not run.

The source project retains the reference registry versions and checksums. The
[source closure verifier](../scripts/verify_entity_helper_source_closure_5_0_6.py)
also recompiles the unchanged existing scalar/optional observer under an
independent manifest with the reference's chrono 0.4.45. All 1,624 scalar/relation
and 1,548 optional/Checker observations still match their retained corpora and
C++. Historical manifests and original generated files remain unchanged.

The [terminal record](parity/recursive_attachment_helpers_5_0_6.json) and
[source closure record](parity/entity_helper_source_closure_5_0_6.json) identify
the files, locks and actual command results. The remaining capability criteria
are tracked in the [G01 matrix](entity_helper_requirements_5_0_6.md).

## Reproduce

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
  teaql_generated_entity_helpers_probe teaql_recursive_attachment_helpers_probe
python3 scripts/verify_entity_helper_source_closure_5_0_6.py
ctest --preset native --output-on-failure -R '^recursive_attachment_helpers_rust_parity$'
```
