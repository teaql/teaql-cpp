# Generated Context/time Checker and governed preflight flow

This P1 checkpoint uses the active Rust 5.0.6 runtime and original producer
locks. `tests/generator/checkers.xml` is rendered unchanged into
`tests/generator/parity/checkers_rust`: 19 original files plus their provenance
record. The complete original `cpp-checker-fixture-core` crate compiles against
the frozen runtime; no entity-only substitute or source repair is used here.
The integrity verifier checks this fifth original fixture family.

## Compared responsibilities

The paired probes call the original generated NoteChecker and the generated C++
NoteChecker directly for Create, Update and Unknown status. A second group calls
the public audited-ledger save API with a mutation policy that denies writes
after observing preflight. The source side registers the original generated
descriptors, typed Checkers and ServiceRuntimeExecutor explicitly; it does not
qualify the separate generated bootstrap/sample-data assembly.

Inputs cover absent/matching/mismatched active roots, zero/matching/mismatched
tenant IDs, full and sparse availability, ordinary/empty/Unicode-whitespace
names, and custom rejection or text fixing through a UserContext resource.
Observations retain complete diagnostics in all three wire naming profiles,
custom hook order/location, original entity view after preflight, mutation-plan
native operands/versions, clock-field presence and ordered FixEvidence.

Clock epoch milliseconds and volatile execution IDs are outside this explicit
projection. Changed-versus-input clock sentinels and equality of the operation's
two assigned clock values are observed directly. No timestamps are rewritten
and no output normalization is used to make the comparison pass.

The original producer emits both a createFunction and an updateFunction for
updated_at. Create therefore records two Clock evidence entries for that field.
Context/time fixes run before the custom typed hook; sparse-update and required
checks retain source ordering and multiplicity. Business fixing does not retract
diagnostics already produced in that operation.

## Selected native serialization

The initial direct comparison found 385 differences among 1,620 observations,
all in governed flow. C++ serialized hidden stored fields into graph/preflight
inputs even when their indexed availability said NotLoaded. Rust's TeaqlEntity
derive serializes an indexed native field only if that field is loaded. As a
result, a C++ business Checker could see a stored name that the source Checker
received as absent/default; required diagnostics and fix evidence also differed.

The shared runtime helper
`detail::EntityRuntimeAccess::retain_loaded_native_values` filters native
mutation values by the carrier's load state. Generated `into_values` calls it
after encoding native fields and before injecting relationship views and runtime
metadata. It does not change immutable original snapshots, dynamic values,
Context ownership or relation mutation responsibilities. Generated code contains
one thin call; fields are not filtered by duplicated per-model algorithms.

The direct and governed probes exercise this through typed reconstruction and
the real audited-save preflight path. Passing a source inventory or emitting
the helper call is not sufficient evidence; matching observations and integration
checks are recorded below after their commands complete.

Filtering selected native fields makes hook views, fix evidence, caller views
and policy plans match in the first follow-up comparison. It also reveals a
second metadata defect: C++ used the KSML explicit `required` flag to emit SQL
nullability, while Rust's derive marks every non-Option storage field NOT NULL.
The C++ generator now follows the same storage optionality. It also describes
scalar storage before generated foreign keys, matching derive property order
independently of the indexed field-layout order. This preserves required
diagnostic ordering and source schema constraints; optional storage remains
nullable and version remains runtime-managed during creation.

Successful generated SQLite save remains separately covered by
`generated_checker_tests.cpp`, including Context root assignment, operation-clock
fixes, checker registration and persisted readback. Policy-denied observations
do not claim that a mutation was committed.

Full inherited/candidate configurations, successful governed-save source matrix,
business workspace scaffolds, bootstrap and later phases remain open. This
checkpoint does not qualify complete G02/G03/G12 or generator parity.

## Verification checkpoint

The live original/C++ corpus passes **1,620/1,620** observations: 972 direct
typed Checker calls and 648 public governed-preflight calls. Final native CTest
passes 8/8, covering this corpus and existing generated API, Checker, relation,
Business-ID, optional scalar, dynamic-field and query/expression integration.
Targeted ASan/UBSan passes 3/3 with leak scanning disabled. Named-module generated
query/expression and optional scalar consumers pass 2/2. Java generator/layout
tests pass 12/12. All six C++ fixture sets reproduce 66 files; the complete
original Checker fixture reproduces its 20 files including provenance.

An earlier C++-only readonly adapter assertion expected omitted name to produce
no diagnostic. It now checks the source-required diagnostic while still proving
that the omitted default is not inserted and readonly reconstruction does not
initialize a mutation ledger. Typed fixing continues to merge only actual
changes and regenerate dirty metadata.

The frozen runtime and original producer remain clean. The 1,406 catalog exports,
seven active packages and 116 source units verify without manufacturing semantic
acceptance from structure or inventory counts.

Reproduce the direct comparison with:

```sh
source scripts/toolchains/activate.sh
python3 scripts/generate_entity_references.py --check --fixtures checkers
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin generated_checker_flow_5_0_6
cmake --build build_native_current --target teaql_generated_checker_flow_probe
python3 tests/parity/compare_generated_checker_flow.py \
  --cpp build_native_current/teaql_generated_checker_flow_probe \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_checker_flow_5_0_6
```
