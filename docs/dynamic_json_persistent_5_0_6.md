# Dynamic JSON with trusted persistent fields and SQLite execution

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Generator reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This qualifies the recorded positive persistent-loading composition. Full G10,
G16/G17 and phase acceptance retain their remaining requirements.

## Actual original and C++ execution

The unchanged original generated domain and generated C++ domain both advertise
indexed dynamic-field carrier support. The plain Entity default is not the
contract of these generated models. Both observers install the actual
DatabaseDynamicFieldsProvider, create its SQLite extension storage and seed it
through the original transaction/store protocol. Existing fixture tables avoid
the separately recorded original generated-bootstrap failure. No bootstrap
success or governed business mutation is inferred from fixture setup.

Four owner types (Person, Order, Product and Line) have trusted Text `title`
definitions in namespace `json_positive`. Existing IDs have alternating Text and
explicit NULL values. Definitions, provider and query controls are installed
before the execution policy and SELECT authorizer; setup I/O is excluded from
execution observations. Neither observer implements query filtering or dynamic
hydration locally.

All **9,216/9,216** original/native outcomes agree, including **6,912** successes
and **2,304** explicit policy denials. The matrix crosses four models, full/minimal
projections, six trusted configurations, four policies, six execution forms,
four JSON inputs and two logging settings. Trusted configurations are ordinary,
readonly literal, readonly ID expression, empty persistent selection, all fields
and a typed `title` selection. Readonly and persistent selections are separate
configurations here; their simultaneous composition is not inferred.

List, one, stream and page rows retain ID, version, `#title` load state, persistent
Value Debug and readonly `_computed` Value Debug. Across repeated observations,
**1,128** row snapshots have loaded title state: **712** Text values and **416**
NULL values. Another **1,128** snapshots contain readonly values. Unselected
persistent fields stay unavailable rather than becoming loaded NULL.
Count and exists preserve scalar outcomes without inventing entity hydration.

Untrusted JSON attempts to replace `_computed`, `#title`, field definitions and
selection cannot install trusted metadata or SQL expressions. The recorded
**11,904** policy callbacks preserve before/after query snapshots, ordering,
aggregates, intent and traces. All **4,608** safe SQL log records match. No denied
request reaches a SELECT authorizer callback. Logging switches do not disable
policy. Stream observers drop/close their cursor and attempt executor reuse;
all **1,152** non-denied stream cases reuse successfully, while **384** denied
cases retain policy rejection on the reuse attempt.

## Ownership, source review and evidence boundaries

C++ disables the default PMR resource. Definitions, provider, requests, result
containers and SQLite RAII resources remain owned by the UserContext. Ordinary
scope exit destroys the Context; existing integration lifetime tests separately
check explicit deletion and destination copies. This matrix does not compare
process-specific storage identity numbers or establish every replacement,
held-stream, async cancellation or cross-domain retention scenario.

The [source review](parity/dynamic_json_persistent_source_review_5_0_6.json)
retains inspected original provider, indexed carrier and stream excerpts with
line numbers, full-file/excerpt hashes and the shared C++ implementations.
Definition validation, trusted executor resolution, validated intent, storage
binding and stream preparation stay runtime/provider responsibilities.
The existing exact quoted-Line descriptor Debug adaptation remains; execution
SQL, values and errors are not rewritten.

Original generated files and historical corpora remain unchanged. The observer
Cargo.lock uses reference registry versions/checksums. Initial observer trait
errors and the wrong-linker libc failure are retained as separate raw archives.
The reproducible runner explicitly uses the established original-Rust host
linker setting; C++ builds retain the pinned project compiler.

## Reproduce and remaining scope

After building the native observer, reproduce the live source comparison:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_dynamic_json_persistent_probe
python3 scripts/verify_dynamic_json_persistent_5_0_6.py
ctest --preset native --output-on-failure -R 'dynamic_json'
```

The module target is `teaql_module_generated_dynamic_json_persistent_probe`;
its test is `module_generated_dynamic_json_persistent_rust_parity`.
The sanitizer target/test use the native names in the separate `sanitize`
profile, with leak scanning disabled by that preset.

Native JSON integration passes **10/10**, the new module check **1/1** and the
new ASan/UBSan check **1/1**, with leak scanning disabled. Format/check covers
**1,444** C++ files; **660** canonical consumer bodies and **1,485** exports verify.

See the [terminal record](parity/dynamic_json_persistent_5_0_6_terminal.json)
for completed gates and raw validation. Positive loading is additional evidence
to the retained [provider-missing matrix](dynamic_json_composition_5_0_6.md).
Wider readonly/facet/privacy/model configurations and final full integration
remain required by the [G10 matrix](dynamic_json_qualification_matrix_5_0_6.md)
and [full parity plan](cpp_generator_parity_plan.md). No production or generator
function changed for this observer addition. No MIPS or physical-device run,
Java refresh, generated-artifact refresh or full-suite acceptance is inferred.
