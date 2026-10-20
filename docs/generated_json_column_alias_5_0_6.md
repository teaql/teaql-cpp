# Authentic JSON column aliases and independent SQL identifiers

C++ now preserves the original Rust JSON input column spelling separately from
its existing SQL identifier spelling. PropertyDescriptor has an optional
Context-owned json_column_name adaptation. The JSON scalar reader uses it when
present; SQL execution and relation join metadata keep column_name unchanged.
Full descriptor comparison includes the adaptation, so incompatible installed
JSON metadata is rejected rather than silently interpreted through another model.

## Original source and generated mapping

The locked Rust field template emits the producer's rustColumnName in a Rust
string literal attribute. The derive receives its decoded semantic characters.
SQLite's generated keyword spelling therefore includes quote characters in the
actual JSON alias. C++ already stores the SQL identifier without those characters
and delegates SQL quoting to the dialect. Reusing the physical identifier changed
which primitive JSON keys were accepted and changed the source error outcome.

CppJsonInputMetadata invokes the original centralized naming routine, decodes the
Rust literal once during generation and emits an explicit UTF-8-length string view.
Quoted characters, backslashes, scalar/byte escapes and embedded zeros are retained.
No runtime language parser or dependency was added. Only a different effective
input spelling adds an override; ordinary columns retain their original metadata.
The original naming routine's empty/redundant attribute omission is preserved.

The source lexer rejects leading underscore and non-ASCII hex characters in
Unicode escapes; direct rustc probes confirmed both errors and accepted trailing
underscores and CRLF continuation. Java fixture coverage retains those distinctions.
The initial helper test import used a JUnit API absent from the existing reactor;
it was corrected to the project's existing JUnit API, without adding a dependency.

## Actual generated-source observations

The original **165**-case corpus remains byte-for-byte intact. A separate **189**-case
corpus retains that exact input/output prefix and adds primitive bare/quoted foreign
key aliases, canonical keys, duplicate alias rejection and nested graph joins.
All **189/189** actual unchanged generated Rust/native outputs reproduce with no
normalization. The [initial quoted-column audit](parity/generated_json_column_alias_5_0_6_initial_audit.json)
retains four original differences. The additional foreign-key audit records
10/10 already-matching cases; they were not treated as gaps or changed behavior.

The ownership fixture additionally publishes a descriptor with a long input alias
to Global, checks distinct destination allocation, destroys the original User and
reads the descriptor again through explicit Global rebinding. Model/readback
lifecycle assertions retain their original nested graph and readonly JSON checks.
The metadata layout change requires rebuilding dependent consumers.

## Compiler gate reliability

The previous sequential generated compiler gate terminated at CTest's 1,500-second
timeout. Its [terminal timeout record](parity/generated_api_compiler_timeout_after_json_readback_5_0_6.json)
is not accepted as a pass or a semantic rejection. The verifier retains every one
of the **57** original case bodies, positive/negative outcomes and diagnostic checks.

For Clang it now precompiles each exact shared header/namespace prologue once in
a temporary operation-local directory, waits for successful production, and runs
each original body through the same compiler flags and syntax check. Independent
cases use distinct source files and four bounded workers; the worker group joins
before temporary artifacts are reclaimed. Clang's stale
PCH validation remains enabled. Other compilers use the ordinary path; --no-pch
provides a control replay. There is no persistent cache or runtime dependency.
The direct no-PCH control passes one positive and one negative case. The full
57-case CTest replay passes 17 positive and 40 negative cases in 693.20 seconds;
see the [terminal compiler record](parity/generated_api_compiler_after_json_alias_5_0_6.json). A sequential PCH
partial run was deliberately terminated for bounded parallel scheduling; its
partial successes are not accepted or added to the final total.

## Reproduction

```sh
source scripts/toolchains/activate.sh
python3 scripts/generate_examples.py --check
cmake --build build_native_current --target teaql_generated_json_hydration_probe
python3 tests/parity/replay_generated_json_hydration_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_json_hydration_5_0_6 \
  --cpp build_native_current/teaql_generated_json_hydration_probe --check
python3 scripts/verify_generated_type_errors.py --build-dir build_native_current
python3 scripts/verify_generated_type_errors.py --build-dir build_native_current \
  --no-pch --cases positive unknown_field
```

Complete inherited/dynamic/model/explicit-column/authority and serialization cycle
matrices remain required. This alias adapter does not authorize writes. Complete
capabilities G01-G17 and phases P0-P7 remain partial; terminal profile/compiler
results are recorded independently.

## Profile checkpoint

Actual source/native reproduction passes **189/189**. Relevant native checks pass
**7/7**, targeted ASan/UBSan passes **4/4** with leak scanning disabled, and
import-only module checks pass **3/3** after terminal builds. Java passes **37/37**;
all **18/216** generated artifacts reproduce. The **122** canonical consumer/fixture
bodies, **1,479** exports, module paths/dependencies and **1,012**-file format/check
verify. Static corpus bindings are 149 active, 63 historical and 21 custom; these
counts establish binding only. No MIPS or physical-device test was run here.
