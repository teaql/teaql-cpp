# Shared source prerequisites for imported consumers

The module test graph now carries the source prerequisites of its native
counterparts. This replaces 390 completely missing bindings and completes the
mutation-privacy entry's partially installed binding. All 403 original Python
module entries have explicit counterparts and complete argument checks.

This is test infrastructure. C++ implementations, generated SDKs, module exports
and the independent producer are unchanged. Rust 5.0.6 runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a` remain the reference.

## Implementation

[module_source_contracts.py](../scripts/module_source_contracts.py) generates
[source_contracts.json](../modules/source_contracts.json) from actual configured
native and module CTest registrations. Ten explicit aliases preserve renamed
module entries. Command matching retains corpus/model selections and admits
only declared frontend differences: executable placement, two canonical program
aliases, empty host-emulator arguments, the inherited-helper program prefix and
the three existing module control runners.

Pointer width and assertion mode are explicit in the affected module commands.
Existing module-only logging configuration and timeouts are retained where the
native declaration has no corresponding setting. The previously omitted
`teaql_module_sample_operand_tests` target is included in the committed module
build preset. The first complete execution additionally exposes eleven
omitted programs with no output. The subsequent configured-target check finds
four more omissions masked by cached outputs. All fifteen are now explicit
preset targets.

The canonical-consumer verifier reads the actual configured CMake codemodel.
It requires all 633 imported executables, including application targets with
`TEAQL_USE_MODULES=1`, to appear in the 634-target module preset. This covers
handwritten declarations as well as generated canonical consumers. Configuration
provides this metadata without requiring executable outputs for source-only
verification. The source manifest also binds the adapter and consumer-verifier
program identities.

[source_contracts.cmake](../modules/source_contracts.cmake) emits the shared
registrations during module configuration. It installs 62 source fixtures,
their complete parent dependencies, one executed-helper companion control and
the registration verifier. The resulting module preset contains 785 tests.
The manifest and build-directory CMake output are generated artifacts.

Thirty source commands retain their existing source-only checks while their
optional native `--ctest-build-dir` checks move to the module registration
verifier. The generator-child and executed-helper guards accept an explicit
`--frontend module`. Their default native behavior remains available, including
all retained source/process controls. Exact preceding checker sources are
archived under `tests/parity/historical` for evidence identity.

The registration verifier configures an independently owned temporary native
declaration tree with the committed toolchain. It collects real CTest metadata,
reconstructs the manifest and compares it with the installed module graph.
This needs no preexisting native build directory and performs no native
application build. It also verifies canonical imported consumer bodies and
rejects sixteen corrupted binding/fixture variants.

Raw `ctest --test-dir` output from a module build contains both header-native
and module registrations. The adapter applies the committed `^module_`
namespace and uses real CMake declaration locations to distinguish original
entries from generated fixture entries. The initial unfiltered manifest is
retained as an unaccepted diagnostic.

## Executed checks and current integration

- [Repaired ordinary and optimized verification](parity/module_source_adapter_preset_controls_5_0_6.json.gz)
  passes all 1,165 canonical consumer/fixture bodies, sixteen source-adapter
  corruptions and twenty preset/codemodel corruptions per mode. Fifteen
  missing-target variants reproduce acceptance by the preceding verifier and
  rejection by the repaired verifier in each mode. Its 7,828-byte owned
  controller directory is removed.
- The complete source-fixture selection passes 64/64 actual tests: all 62
  source fixtures, the companion control and the registration verifier.
- Both changed source guards pass their actual native default entries, 2/2.
  Their internal ordinary/optimized child and helper controls remain active.
- All 1,817 preceding C++ implementation/header/module/fixture inputs retain
  their bytes. Only `check_bindings` and `main` change in the two extended
  checkers; retained runners, execution helpers and source-validation constants
  remain identical.

The [first complete build and test epoch](parity/module_source_adapter_full_modules_5_0_6.json.gz)
builds successfully and ends with 773 passes, one failure and eleven skips.
The [exact JUnit capture](parity/module_source_adapter_initial_junit_5_0_6.json.gz)
and [closed source frame](parity/module_source_adapter_initial_epoch_closure_5_0_6.json.gz)
retain those results without acceptance. Eleven executables were omitted from
the preset; the failed example still received historical provenance data.
The actual codemodel check then identifies four more omissions masked by cached
outputs. The example now receives `runtime_examples_provenance_5_0_6.json`.

The [repair execution](parity/module_source_adapter_program_repairs_5_0_6.json.gz)
passes all sixteen affected tests, a final no-work build, unchanged 12,005
inputs and fixed references. Its 93,573-byte owned output directory is removed.

The [fresh repaired full module epoch](parity/module_source_adapter_repaired_full_modules_5_0_6.json.gz)
passes **785/785** actual tests with exact registry/JUnit membership and zero
failures, errors or skips. All **12,005** execution inputs, both locked references
and **633** imported ELF hashes remain unchanged. The actual committed preset
reports no work before and after testing. Its owned 1,168,195-byte output
directory and 6,014-byte outer controller directory are removed after the child
processes exit. The stable preset outputs remain available.

The [ordinary and optimized admission](parity/module_source_adapter_acceptance_repaired_5_0_6.json.gz)
independently checks current source, registry and executable identities and
rejects 21 corrupted receipts per mode. Its preceding failed admission is
retained: the validator expected another CTest success-summary format. Only
that validation and its added negative control change; the completed full
execution is explicitly reused without a new build or CTest run. The repaired
admission removes its 2,464,782-byte owned directory.

The [source transition review](parity/module_source_adapter_source_transition_5_0_6.json.gz)
records six added inputs and six changed existing inputs relative to parent
`af9cb40826e7efaea22fe99e943fba0eb0e6b487`. All 1,817 preceding C++ files and
853 independent-producer inputs retain their bytes. The two extended source
guards change only their binding checks and entrypoints. The consumer verifier
retains its canonical-body checks and adds configured target coverage; other
build/test presets remain unchanged. This qualifies the shared module source
prerequisites and complete current module execution for this source frame.

The full module execution receipt SHA-256 is
`fd4a44266dbfbf6205205a94d184650221ce40783c6ceb74080bd9e08ddae054`;
its repaired admission SHA-256 is
`a0ec4f09299bd5501e345fd0bfd143f4383da1d27d9b81c0b3d7e5cc0dcf57fa`.

The broader G13/G14/G17 and P0-P7 goal remains open at **14 qualified / 3 partial**.
Full AP02 configuration and identity requirements and the pending independent
compatibility choices retain their existing scope. The separately recorded
[constant-function differences](ap02_const_function_calls_5_0_6.md) are not fixed
or accepted by this test-infrastructure milestone. The preceding 1,101-test
native result retains its own source frame; no new full native, sanitizer, MIPS,
GPU or physical-router execution is claimed.

## Refresh and verification

Use the pinned environment and actual configured registries:

```sh
source scripts/toolchains/activate.sh
cmake --preset native
cmake --preset modules_native
python3 -B scripts/module_source_contracts.py refresh
cmake --preset modules_native
python3 -B scripts/module_source_contracts.py verify
python3 -B -O scripts/module_source_contracts.py verify
```

Refresh after changing native declarations or module callers. Regeneration
updates the source/program identities and derived fixture graph. Source
declaration reproduction checks the generated result against the actual
authorities during verification. Runtime, sanitizer and MIPS testing keep
their existing change-specific policies.
