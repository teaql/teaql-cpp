# Generated model JSON hydration and presentation readback

The C++ generated domain now uses the existing UserContext JSON decoder and
implements the runtime Entity borrowed_json protocol. Locked Rust generation
uses Context decode_json_entity/entities and derive-generated borrowed_json; it
has no separate model JSON factory. C++ preserves that responsibility split.

## Source gap and shared runtime correction

The first actual generated-model comparison found 22 differences in 165 outcomes.
Loaded relationship state was correct, but C++ did not override borrowed_json.
Its consuming fallback therefore omitted graph-owned forward and reverse edges.
The [initial differences](parity/generated_json_hydration_5_0_6_initial_gaps.json)
retain the original and C++ outputs without normalization.

GeneratedEntityJson now owns the shared presentation traversal. Generated models
supply raw storage fields, load flags, identity/version flags and typed relation
mappings. Serialization includes only loaded scalar fields; omitted fields stay
omitted. Embedded forward objects take precedence, then selected runtime edges
and identity lookup. Loaded NULL is emitted only at the source-defined boundary;
missing filtered targets are not synthesized as NULL. Reverse views require
loaded availability and an installed runtime edge, including NULL/empty edges.

The existing traversal guard limits recursive expansion by path/address and depth;
a stopped expansion retains loaded identity/version fields. Readonly dynamic values
and trusted persistent values retain their separate serialization paths. Runtime
state markers are removed. Scalar encoding is deferred until a field qualifies,
so omitted fields do not allocate presentation payloads. Child serialization
failure preserves the optional protocol instead of inventing a partial result.
The implementation adds no production dependency and does not change into_values,
mutation ledgers or save authorization.

## Generated guidance and source observations

Generated Assist documents module installation, the exact snake_case Context and
Entity runtime entry names, Result handling and presentation-only authority.
Application Q/E/model methods retain camelCase; runtime protocol overrides retain
their authentic names. Java regression checks prevent guidance from rewriting
apply_to, decode_json_entity/entities or into_json to nonexistent camelCase APIs.

The observer executes the unchanged original generated dynamic_json domain module
and Context decoder, then reads back JSON, complete top-level load flags, dirty
fields and new/deleted state for Platform, Person, Profile, Company, Order, Product
and Line. The declared matrix covers scalar and array projections, readonly values,
unknown/prohibited fields, nested forward and reverse views, empty/NULL relations,
join conflicts and repeated projections. All **165/165** original/native outcomes
match and reproduce. This is the declared seven-model matrix, not every model or
column alias configuration. See the [driver review](parity/generated_json_hydration_5_0_6_driver_review.json)
and [original generated/source provenance](parity/generated_json_hydration_5_0_6_source_provenance.json).

The first corrected C++ fixture build exposed a missing generated-support include
for the shared helper. The generator template was fixed and artifacts regenerated
before execution. Original generated Rust files remain unchanged.

## Reproduction

```sh
source scripts/toolchains/activate.sh
python3 scripts/generate_examples.py
python3 scripts/generate_examples.py --check
cmake --build build_native_current --target teaql_generated_json_hydration_probe
python3 tests/parity/replay_generated_json_hydration_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_json_hydration_5_0_6 \
  --cpp build_native_current/teaql_generated_json_hydration_probe --check
ctest --test-dir build_native_current --output-on-failure \
  -R '^generated_json_hydration_(rust_parity|context_contracts)$'
```

## Remaining acceptance

Complete inherited/dynamic model configurations, authentic storage aliases,
explicit cycle/filtered-target matrices, readonly/persistent authority and database
hydration/save composition retain separate source qualification. The tested
presentation entry grants no authority to persist client data. No complete
capability or phase is closed; G01-G17 and P0-P7 remain partial.

## Additional alias audit and verification state

A separate [nine-case alias audit](parity/generated_json_column_alias_5_0_6_initial_audit.json)
finds **four** input/error differences for Line's order column. Rust's generated
property stores the column spelling `"order"`; the C++ SQL descriptor uses `order`
and lets the dialect quote it. Canonical order_id input matches, but primitive
bare/quoted alias acceptance differs. The 165-case corpus remains its original
matrix; this new audit is not normalized or claimed as accepted. JSON alias
alignment remains required before full qualification.

The runtime/example native selections pass **8/8** across the new JSON contracts,
generated API, runnable example, inheritance, readonly, relation and optional-type
fixtures. Targeted ASan/UBSan passes **2/2** with leak scanning disabled; import-only
module checks pass **2/2**. The explicit lifetime fixture deletes UserContext,
rejects its old model ID and reads back the Global deep copy's long strings,
readonly JSON and nested typed relationships through another Context. Default PMR
allocation is disabled. Java checks pass **34/34**, all **18** generated workspaces
/**216** files reproduce, **122** canonical consumer/fixture bodies and **1,479**
exports verify, and pinned format/check covers **1,012** handwritten files.

The original full 57-case generated positive/negative compiler command terminated
at the 1,500-second CTest timeout; its result is not accepted. See the
[alias/compiler follow-up](generated_json_column_alias_5_0_6.md). Native runtime selections do
not substitute for that compiler gate. No MIPS or physical-device tests were run.
