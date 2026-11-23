# Compiled metadata and the source column boundary

The approved [column-attribute refusal](field_attribute_compatibility_exception_5_0_6.md)
also applies when generation receives trusted compiled metadata. A resolved
column does not authorize an unescaped quote in the source column literal.

`CppCompilerModelConfiguration.bind` now invokes the existing
`CppJsonInputMetadata.column` validator for every modeled scalar field before
returning its bound resolution. All other metadata still comes from the
compiled configuration. In particular, this does not interpret audit-limit
expressions or replace compiled property shapes with Java estimates.

The guard covers the model, scalar and semantic-storage document formats.
The two attribute-injection forms fail with
`Unescaped quote in original Rust column literal`. Plain columns, escaped quotes
and escaped ASCII bytes remain accepted. The tests verify the parsed literal
itself: KSML consumes one backslash layer before Rust literal decoding.

The [current Java execution](parity/compiled_column_java_5_0_6.json.gz)
passes all **98 shared evaluation tests and 297 C++ generator tests**. It includes
six rejected configurations and nine valid configurations across the three
formats. An actual overlay of the preceding production class fails exactly
the missing-refusal regression; the valid-column test still passes. Twelve
receipt corruptions are rejected under ordinary and optimized Python.

## Reference evidence has a narrower historical scope

The earlier approved exception compiled and executed the original entity
modules. It did not compile the complete generated Rust SDK. A fresh
[complete-SDK compiler diagnosis](parity/compiled_column_original_boundary_5_0_6.json.gz)
for the first unchanged injection fixture fails in the original sample-data
module before C++ emission. No SDK is published. Its nested compiler result is
an actual refusal; the outer attempted bypass reproduction is incomplete and
is not acceptance evidence.

The successful current reader tests use explicitly synthetic trusted metadata.
They prove the source check cannot be bypassed by any of the three document
readers, without claiming a successful original complete-SDK compilation for
this fixture. Early failed test-development probes remain diagnostic records;
the final preceding-class overlay supplies the qualified regression.

## Retained SDK reproduction

The retained SDKs are development fixtures. Their reproduction uses an explicit
qualification entry, while normal application generation continues to require
trusted locked compiler configuration. `generate_examples.py` now names that
entry explicitly instead of calling the unconfigured formal entry.

The [complete SDK reproduction](parity/compiled_column_sdks_complete_5_0_6.json.gz)
regenerates and checks all **80 SDKs**, preserving all **3,560 code and guidance
files**. Only the eighty generator fingerprints change. The first interrupted
reproduction remains recorded; its expected manifest updates are explicitly
validated before resuming. Twelve imported SDK-receipt corruptions reject in
each Python mode.

The active [source continuation](parity/compiled_column_sources_repaired_5_0_6.json.gz)
binds ninety exact source/manifest transitions to the successful Java and SDK
executions. The first native selection passed 38 of 40 tests; two historical
readers did not forward an already-qualified source change. The repair extends
only that source-identity forwarding, preserves the original source proof and
failed JUnit epoch, and does not rerun successful business/compiler execution
to repair a result reader. Ordinary and optimized source controls reject twelve
corruptions per mode.

The [repaired native selection](parity/compiled_column_native_repaired_5_0_6.json.gz)
passes **40/40 tests in each Python mode**, including the required fixtures.
Both JUnit sets have zero failures, errors and skips; the complete execution
input frame stays unchanged. All 1,824 C/C++ source inputs retain their prior
bytes, and pinned formatting passes for 1,820 handwritten C++ files. This is a
source/receipt continuation, not a new full native or module qualification.

Complete configuration, protected-consumer composition and P0-P7 integration
remain independent exits. G13, G14 and G17 remain partial.
