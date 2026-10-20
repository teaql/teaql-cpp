# Generated wire mappings and alias metadata: Rust 5.0.6

Status: all twelve original/native mapping comparisons, artifact reproduction
and focused lifetime/compiler/module gates pass. G12/G14/P1/P7 remain open.

The locked producer runtime template defines two distinct APIs:
generated_wire_field_mappings and generated_wire_field_aliases. They return typed
ordered maps, independent of provider/transport execution. C++ previously emitted
neither API; decoder support or ordinary generated JSON was not equivalent.

## Preserved source contracts

| Responsibility | C++ implementation |
| --- | --- |
| Entity key | Preserve the original RustStructName metadata identity. |
| Canonical field key | Use FieldDescriptorNaming.canonicalPropertyName, including the model relation name; never substitute its FK storage key. |
| Selected JSON value | Use the shared FieldDescriptor.getJsonFieldName and Domain naming policy for camelCase, snake_case and PascalCase. |
| Own-field selection | The original template iterates entity.fields, not predefinedFields. C++ likewise emits that entity's own fields without flattening inheritance. |
| Alias table | Emit a separate empty typed map, as the frozen template does. Physical column aliases do not populate this table. |
| Validation | Preserve original model-profile failures instead of inventing a fallback spelling. Source artifact behavior is observed separately from C++ rejection. |

Application-facing functions are generatedWireFieldMappings(user_context) and
generatedWireFieldAliases(user_context). Both allocate through the supplied
UserContext and return Context-owned mutable map borrows. They do not configure
the runtime, authorize input, install aliases or provide HTTP/TFP endpoints.

## Shared runtime and ownership

GeneratedWireFieldMap and GeneratedWireFieldMappings are shared runtime types in
generated_wire_fields.hpp. They are standard PMR ordered maps with String keys
and values; the generator emits only model-specific entries. No third-party
dependency or duplicate naming/transport algorithm is added.

The nested standard uses-allocator construction propagates the owner's resource.
Transparent comparison supports lookup with borrowed string_view keys without
creating default-resource strings, including long names.
The native lifetime contract disables default PMR and checks all map/key/value
allocators using long strings. It explicitly copies the complete mapping into
Global through UserContext, ends stack PMR scopes, deletes the source UserContext,
requires zero remaining User bytes, rejects its old checked ID, rebinds the Global
copy through a new UserContext and reads the preserved values and allocators.
Ordinary map borrows remain invalid after their Context closes.

The two runtime aliases are exported by the existing teaql_runtime module and
covered by its isolated lookup catalog. New interfaces preserve the seven-package
import graph; no function implementation is copied into a module facade.

## Actual original source comparison

The observer renders complete original runtime artifacts and extracts each of
the two complete function bodies byte-for-byte. It compiles those bodies without
changing their statements and invokes them, then calls the generated C++ APIs
through the complete native facade. The source functions use std::BTreeMap; the
observation encoder uses reference-locked serde_json. No entity/metadata algorithm
reconstructs the original expected maps locally.

The twelve configurations combine three naming profiles with ordinary,
inherited, constant and 140-field models. They include a relationship FK and
nondefault scalar column so wire/model/storage names cannot be silently conflated.
Both the mapping and alias tables compare without output normalization.
Invalid-profile generation behavior is retained as a separate boundary.

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_wire_mapping_lifetime_tests
ctest --preset native --output-on-failure -R '^generated_wire_mapping_context_contracts$'
python3 scripts/audit_generated_wire_mappings_5_0_6.py \
  --output /tmp/generated_wire_mapping_replay.json.gz
```

Use a fresh output path for the source auditor. The development generator and
Rust oracle are separate from normal application/runtime builds.

The initial generated C++ compile caught a missing shared-header include; it was
fixed in the generator support template rather than patched in emitted headers.
A module command also named a nonexistent lookup target. Those
[initial failures](parity/generated_wire_mappings_initial_validation_5_0_6.json.gz)
are retained. A no-test module selection is not presented as passed coverage;
the new lifecycle test receives its own explicit module CTest registration.

Complete wire/JSON/privacy consumer composition, model/configuration breadth,
runtime assembly and final P1/P7 integration retain their existing obligations.
This metadata API is not an external integration or cross-language transport
guarantee. Physical-device testing remains deferred.

The invalid-profile inspection shows a frozen source-template defect: generation
returns success, emits empty wire strings and provides no diagnostic. C++ rejects
the invalid model profile explicitly. The [actual source/Cpp inspection](parity/generated_wire_mappings_profile_inspection_5_0_6.json.gz)
retains the original output and the auditor's initial incorrect rejection/error
assumptions. Neither a default spelling nor a fabricated source diagnostic is
used to claim equivalence for that broken configuration.

The [source comparison](parity/generated_wire_mappings_source_5_0_6.json.gz)
retains twelve complete original/native results: 30 entity maps and 531 canonical/
wire pairs. In the inherited fixture, Child exposes only id and child_name, as
its original own-field template specifies; parent fields are not synthesized.
The invalid source artifact contains empty wire strings with success exit, while
C++ reports a generation error. Their different boundaries stay explicit.

All 23 generated workspaces/276 files reproduce. The 44 contract SDKs were
regenerated with a dedicated review mode: remove exactly the two new metadata
functions and the new header include, then require pinned-format byte equality
with every old runtime/support body. Only regenerated metadata functions/include
and genuine Assist changes can pass that mode; all other files stay identical.
[SDK regeneration](parity/wire_mapping_sdk_regeneration_5_0_6.json) and its strict
follow-up pass. An initial separator-normalization error in that review mode is
retained in [the validation record](parity/generated_wire_mappings_sdk_initial_validation_5_0_6.json.gz).

## Terminal verification

Relevant native tests pass 3/3, targeted ASan/UBSan 1/1 with leak scanning disabled,
and the canonical module lifetime contract 1/1. The new metadata API compiler
subset passes one positive and two negatives: GlobalContext and an extra execution
argument cannot replace the required UserContext. Java generation/metadata tests
pass 52/52. The module catalog verifies 1481 exports, including both new shared
types; 586 canonical consumer/fixture bodies verify. Pinned format/check covers
1407 handwritten C++ files. No full-suite, MIPS, QEMU or physical-device execution
is claimed. See the [terminal record](parity/generated_wire_mappings_5_0_6.json).

The original functions' complete bodies and original generated trees remain in
the source archive. The transparent-comparator refinement changes borrowed lookup
availability without changing generated mapping entries; current native/module/
sanitizer/compiler controls verify that final runtime representation. All 23
workspaces and 44 SDKs reproduce after regeneration. Broader JSON/privacy/transport
consumer behavior and full model/assembly qualification retain their other owners.
