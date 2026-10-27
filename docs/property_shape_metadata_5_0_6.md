# Generated property shape metadata

The C++ generator now delegates string-length and numeric precision/scale
normalization to the existing `RustFieldDescriptorNaming.rustFieldAttr` helper.
Its relevant methods are byte-identical to the locked generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. Runtime PropertyDescriptor APIs,
allocation domains and SQLite execution are unchanged.

The previous C++ emitter interpreted raw model values independently. Compiling
two complete models with the locked Rust generator/runtime and the previous C++
generator exposed 16 different property rows / 22 different metadata values,
including inherited fields:

| Model input | Locked Rust result | Previous C++ result |
| --- | --- | --- |
| `string(max=0017)`, `string(max=+17)`, Arabic decimal digits for 17 | Length 17 | No length |
| `string(max=2147483648)` | No length | Length 2147483648 |
| `number(sqlType=NUMERIC(010,005))` | Precision 10, scale 5 | Precision 8, scale 5 |
| `NUMERIC(0,0)`, `DECIMAL(5,6)`, overflowing precision | No numeric shape | Invalid shape retained |

The adapter consumes only the helper's normalized decimal attributes before its
final column attribute. Raw column text cannot impersonate additional shape
attributes. Positive lengths use the producer's signed 32-bit bound; precision
must be positive and scale must be between zero and precision. Invalid values
omit metadata as the reference does. This is metadata normalization, not SQL
validation or a new compatibility exception.

## Verification

The retained caller corpus is
[`property_shape_models_5_0_6.json.gz`](../tests/parity/property_shape_models_5_0_6.json.gz).
The compiler gate generates both languages from each complete model, compiles
the original Rust and current C++, executes both, and compares every property
name, column, data type, nullability, ID/version flag, length, precision and
scale in Platform, Item and Child. It includes actual inherited descriptors.
The stable final comparison passes **2 models / 6 descriptors / 44 properties**.
Current metadata regression passes **63,918** observations and **137** Java
tests. The four relevant native tests pass. All **80 SDKs / 3,560 generated
files** reproduce unchanged bodies, with refreshed producer provenance.

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_property_shape_metadata_5_0_6.py \
  --record /tmp/property_shape_fresh.json.gz \
  --corpus tests/parity/property_shape_models_5_0_6.json.gz \
  --cases property_shape_lengths property_shape_numeric
```

Use a fresh receipt path. Generation and compilation run in owned temporary
directories, which are removed after their source, commands and results are
retained. This command needs the supported native libraries built beforehand.
The producer also retains source-derived JUnit expectations and tests integer
bounds, whitespace, Unicode digits, malformed numeric shapes and multiline
column text. These are finite cases, not complete G14 grammar acceptance.

The shared compiler observer also needed a lifetime correction: its stack JSON
containers must be destroyed before closing their UserContext. Moving a JSON
array into the current constructor still copies that array, so the old outer
container remained live at Context close. An inner scope fixes the test caller;
the runtime's ownership contract is unchanged. The retained old generated SDKs
are used for a separate scoped ASan/UBSan check of this observer correction.

## Evidence and remaining work

Initial differences are retained in
[`property_shape_initial_scoped_5_0_6.json.gz`](parity/property_shape_initial_scoped_5_0_6.json.gz).
Earlier observer compilation and lifetime failures remain separate receipts.
The first fixed comparison matched both models, but its input guard detected
a concurrent edit to the separate metadata regression verifier; that run is
retained as incomplete. The final acceptance uses a fresh stable input epoch.
The first sanitizer run likewise executed cleanly but failed its outer input
guard while generator sources changed; its clean replay is recorded separately.
The first native gate passed its checks and tests but detected five independent
SDK provenance updates in its input guard. The final native gate repeats the
build, tests and source guards after that refresh. Its unchanged C++ input hashes
also retain the preceding successful pinned format/check evidence.

Final receipts:

- [Compiled original/C++ comparison](parity/property_shape_acceptance_5_0_6.json.gz)
- [Metadata and Java regression](parity/property_shape_source_regression_5_0_6.json.gz)
- [Example/contract reproduction](parity/property_shape_sdk_reproduction_5_0_6.json.gz)
- [Five independent SDKs](parity/property_shape_independent_sdks_5_0_6.json.gz)
- [Native checks and tests](parity/property_shape_native_acceptance_5_0_6.json.gz)
- [Scoped observer ASan/UBSan replay](parity/property_shape_sanitizer_acceptance_5_0_6.json.gz)

This correction does not close general delegated grammar, other metadata
consumers, source provenance or protected execution. G13/G14/G17 remain partial;
P0–P7 remains active. Earlier native893 evidence retains its original source and
producer identities and is not relabeled as a new full-suite run.

All recorded task-owned compiler, SDK and source-regression temporary directories
have been removed. Measured removals total **421,013,400 bytes**; two earlier
failed probes and the example/contract script contexts were removed without size
measurements. Only compact sources, commands, hashes and outcomes remain as
evidence. Stable native/sanitizer preset outputs and shared caches are retained.
