# Complete JSON streams for compiler projection documents

This repairs a remaining versioned-protocol input gap under the locked Rust
5.0.6 baseline. The explicit projection-only reader checked duplicate members
and the first object's bindings but did not require the byte stream to end.
Full model documents already used `FAIL_ON_TRAILING_TOKENS`.

The [fresh diagnosis](parity/projection_trailing_diagnosis_5_0_6.json.gz)
compiles current main sources and the existing synthetic projection fixture.
An otherwise valid bound document is accepted with no suffix, JSON whitespace,
a second object, an array, NULL, Boolean, number and ordinary trailing garbage.
All eight inputs return facts. The diagnosis is complete but retains
`semantic_acceptance: false`.

`CppCompilerProjectionResolver.read` now enables the same Jackson complete-stream
validation as the model reader. The change preserves source/model/compiler
bindings, projection membership, cardinality and consumption state. It does not
add a dependency or reinterpret Rust configuration. Explicit older document
formats retain their declared observation authority.

The [fresh complete Java gate](parity/projection_trailing_java_5_0_6.json.gz)
passes 98 shared and 305 C++ tests, totaling 403. Replacing only the production
reader with its exact previous source causes one failure in the twelve projection
tests. The new rejection method covers all six tails. A separate whitespace
control confirms identical generated code/Assist while retaining distinct
byte-level resolution provenance.

The [unchanged direct probe](parity/projection_trailing_replay_5_0_6.json.gz)
now accepts only the complete document and whitespace control; all six tails
raise actual parser errors. Ordinary and optimized imported replay checks each
reject eight corrupted records, including missing cases, Boolean exit codes,
changed compiler inputs and invented acceptance.

The [retained SDK generation/check](parity/projection_trailing_sdks_5_0_6.json.gz)
completes all six recipes and preserves all 3,560 code/Assist files across eighty
SDKs. Only the projection-reader fingerprint changes. The
[source continuation](parity/projection_trailing_sources_5_0_6.json.gz) binds two
producer files, two admission readers and eighty manifest transitions. Existing
execution/comparison/control bodies remain unchanged while exact historical
identities forward to this source epoch. Its helper rejects sixteen corruptions
in each Python mode.

Imported helper qualification preserves those sixteen controls together with
nineteen source-configuration, 27 String-alias and twelve column-source controls,
totaling 74 rejected corruptions per mode. Exact current source frames stay
stable; earlier successful executions keep their declared scopes.

The [affected native source/receipt checks](parity/projection_trailing_native_5_0_6.json.gz)
pass 40/40 in each Python mode. Both JUnit files have exact expected membership
and no failures, errors or skips. Complete source frames remain unchanged, and
the owned test-output directory is removed after both runs finish.

This is a scoped protocol repair. Complete G13/G14/G17 and all E01-E06/P0-P7
requirements retain their independent acceptance boundaries.

The [checkpoint](parity/projection_trailing_checkpoint_5_0_6.json.gz) binds
thirteen preceding reports totaling 9,058,019 bytes and the current source,
native and reference identities. All five explicitly owned temporary roots are
absent; their available measurements record 4,967,215 bytes of disposed outputs.
Nested recipe outputs use their own cleanup. Stable build/download caches and
source fixtures remain available. Formatting write/check passes for all 1,820
selected C++ files, with no C++ source changes.
