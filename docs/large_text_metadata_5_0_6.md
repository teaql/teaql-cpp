# Large-text metadata and sample branch precedence

The C++ generator now applies the locked Rust derive's `large_text` override to
all non-JSON scalar descriptors. It also honors StringTemplate's attribute
precedence when selecting string and numeric sample-fill branches. Runtime
functions, ownership domains, public signatures and dependencies are unchanged.

The baseline remains Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, with generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Problems reproduced

The locked `RustFieldDescriptorNaming.rustFieldAttr` emits `large_text` for
`sqlType=LARGE_TEXT` or a true `isText` attribute. The runtime derive applies
that override after mapping the Rust storage type. Previously C++ applied it
only to string descriptors. Two complete models, with required and optional
scalar fields plus inheritance, exposed **24 descriptor type differences**.
The observations retain all six descriptors and all 52 property rows, including
unaffected names, columns, flags, lengths and numeric shapes.

The public C++ SDK also failed to compile: `FieldDescriptor.isString()` treats
`isText` as string-like, but the Rust template resolves a nonempty `isString`
attribute before that Java method. A Boolean with `isText=true` consequently
selected a string sample in C++ and a Boolean sample in Rust. Decimal, date,
timestamp and float fields had the same branch-selection problem.

The metadata repair moves the non-JSON override before ordinary type dispatch.
The sample repair resolves `isString` with source attribute precedence and
reuses the existing corresponding `isNumber` resolver. Typed generated setters
and shared runtime sample algorithms remain the implementation boundary.

The [approved JSON exception](json_payload_compatibility_exception_5_0_6.md)
still takes priority: JSON-valued C++ fields keep `DataType::Json`. No new
compatibility exception is introduced.

## Executed evidence

- [Complete descriptor comparison](parity/large_text_fixed_5_0_6.json.gz):
  fresh original generated Rust entities and the public C++ SDK compile and
  execute; **2 models / 6 descriptors / 52 properties** compare exactly.
- [Scalar sample comparison](parity/large_text_samples_final_5_0_6.json.gz):
  **144 field observations**, with exact operand kind/value, typed getter and
  load state. Six deterministic fields cover ordinary/inherited entities,
  required/optional storage, three seeds and two indexes. Rust executes complete
  original generated scalar statements/blocks with retained line spans; C++
  executes its actual generated recipe. Generated sources are copied unchanged.
  Temporal statements, relationship selection and persistence are outside this
  focused comparison.
- [Java regression](parity/large_text_java_final_5_0_6.json.gz): **139 tests**,
  including source-derived metadata/branch expectations and the JSON exception.
  These tests also replay the **63,918** retained metadata contracts against
  current Java. The original Rust parser results reuse the unchanged locked
  parser and corpus identities; there is no fresh 63,918-case Rust run here.
- [Thirteen strict sample controls](parity/large_text_sample_controls_final_5_0_6.json.gz)
  reject missing/reordered/duplicated rows, missing/reordered fields, wrong
  identity, lost getters/load state and changed operand/number kinds. The final
  sample controller also enforces exact output cardinality and shape.
- [Relevant native gate](parity/large_text_native_5_0_6.json.gz): **10/10**,
  including sample execution/failures, configured types, original operands,
  temporal fields, async save and Context state. Builds finish before execution.
- [SDK reproduction](parity/large_text_sdk_reproduction_5_0_6.json.gz), including
  [five independent SDKs](parity/large_text_independent_sdks_5_0_6.json.gz): all
  **80 SDKs / 3,560 files** retain their generated bodies. Producer fingerprints
  refresh; original Rust and historical artifacts remain unchanged.
- [Pinned format/check](parity/large_text_format_5_0_6.json.gz) and
  [identity guards](parity/large_text_identity_guards_5_0_6.json.gz): all
  **1,813 frozen C++ inputs** retain their previous identities. No fresh full
  native, module, sanitizer, MIPS or physical-device gate is claimed.

The diagnostic
[model-header-only replay](parity/large_text_descriptor_diagnostic_5_0_6.json.gz)
isolates the 24 old descriptor differences from the separately retained
[public SDK compilation failure](parity/large_text_reproduction_5_0_6.json.gz).
It is not acceptance of the old SDK. The
[producer change record](parity/large_text_producer_change_5_0_6.json.gz) retains
both exact source versions and verifies the preceding source hash.

## Reproduction

Build the supported native libraries first, then use fresh receipt paths:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_property_shape_metadata_5_0_6.py \
  --record /tmp/large_text_descriptors.json.gz \
  --corpus tests/parity/large_text_models_5_0_6.json.gz \
  --cases large_text_scalars large_text_optional_scalars
python3 scripts/verify_large_text_samples_5_0_6.py \
  --source-receipt /tmp/large_text_descriptors.json.gz \
  --record /tmp/large_text_samples.json.gz
```

Both commands retain sources, exact commands, dependency identities, outputs and
temporary-directory cleanup outcomes. The second command needs an accepted
descriptor receipt whose input identities still match the worktree.

## Qualification boundary

This closes the two reproduced emitter/consumer defects. It does not establish
full SQL or governed sample execution for every unusual large-text/type
combination, complete delegated grammar, complete source provenance or protected
publication. The existing core corpus totals remain 142 / 155,380 observations;
these model callers are a separate finite gate. G13/G14/G17 and P0–P7 remain
open; capability status stays **14 qualified / 3 partial**.

Early probe failures remain separate: an initial caller fixture omitted the
shared compiler's required `input` key, and the first JSON exception unit model
omitted SQLite configuration. Both test fixtures were corrected without
changing runtime behavior or the locked references.

All seven recorded task-owned probe/SDK directories have been removed. Measured
removals total **536,870,677 bytes**; two failed probes and the regular
example/contract script contexts were removed without size measurements.
Stable preset outputs and shared download/Maven caches are retained.
