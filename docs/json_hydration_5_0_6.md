# Context typed JSON hydration against Rust 5.0.6

The shared runtime now implements UserContext::decode_json_entity<T> and
UserContext::decode_json_entities<T>. The implementation follows the locked
context/json.rs and context/json/graph.rs responsibilities. Decoded input is
presentation data and does not authorize persistence or replace authoritative
loading. No generator-specific parsing algorithm was added.

## Descriptor and scalar boundary

The entry first resolves installed entity metadata, compares every property and
relation against the typed descriptor, then requires generated field indexes.
Missing installation, incompatible metadata, absent layouts and layout errors
retain their separate source errors and ordering. Arrays are validated afterward.

Scalar conversion retains JSON number kinds and the declared property types,
nullable fields, mapped columns and readonly dynamic JSON values. Incoming
runtime state and persistent dynamic fields are rejected. Relation input is
handled only by graph-aware hydration. Every field is converted before sorted
canonical names are checked for duplicate aliases, matching original failure
precedence. Unknown or invalid values are not echoed in errors.

The operation-local actual-column cache shares immutable layouts across compatible
projections in an array. Payloads stay private. Query-only relationship availability
is added through the existing RelationShapeCache; no relation JSON payload is
installed as a scalar value. All keys, payloads, layouts and nested JSON are
allocated in the supplied Context.

## Recursive graph plan and installation

Graph input retains the source depth limit of 64 and node limit of 100,000. Each
node receives an independent JSON view. Even empty relations require an installed
target and its registered typed decoder capability. Object/array/null cardinality,
explicit owner identity and local/foreign property mappings are checked in the
original order. Missing forward join keys can be inferred from nested projections;
conflicting supplied keys, child joins and native values for repeated identities
are rejected. Repeated projections do not widen each other's availability.

Edge installation recurses through children before invoking the shared registered
JSON decoder. Child models carry their child coordinates and relation payloads
are stored at the parent's coordinate. The graph is frozen before the root is
decoded. Scalar-only input uses the ordinary from_compact_row entry without
attaching runtime graph state, as in the original source.

## Actual-source observations

The append-only observer builds the complete locked runtime and invokes its
public JSON entry points. The [source provenance](parity/json_hydration_5_0_6_source_provenance.json)
records the terminal build of 34 observers and unchanged original archive bytes.
The committed corpus compares **7,072** complete outcomes: **596** successful
projections and **6,476** errors. It covers four layout/dynamic fixtures, installed
and missing/mismatched metadata, callback registration, scalar kinds and limits,
aliases, prohibited state, readonly values, list/option graphs, join conflicts,
repeated identities, shape-sharing matrices and depth boundaries. Model fixture
contracts are explicit; this is not every generated model configuration.

The initial eight differences were duplicate-alias error precedence. They are
retained in parity/json_hydration_5_0_6_initial_gaps.json. Conversion now finishes
before alias validation and all original/native outcomes reproduce exactly.
No error normalization or projection replacement is used. A later lifecycle
fixture compile typo (try_str instead of the actual try_text API) was corrected
before executing that fixture.

## Reproduction

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
cmake --build build_native_current --target teaql_json_hydration_probe
python3 tests/parity/replay_json_hydration_5_0_6.py \
  --rust /tmp/teaql_cpp_private_save_rust_506/debug/json_hydration_5_0_6 \
  --cpp build_native_current/teaql_json_hydration_probe --check
ctest --test-dir build_native_current --output-on-failure \
  -R '^json_hydration_(rust_parity|context_contracts)$'
```

## Remaining acceptance

Generated model presentation readback (see [the follow-up](generated_json_hydration_5_0_6.md)), every supported inheritance/dynamic model shape,
readonly-field authority and database hydration composition remain separate
qualification. This runtime entry does not turn client input into a save
capability. Complete capabilities G01-G17 and phases P0-P7 remain partial.

## Terminal verification

Relevant native checks pass **6/6**, targeted ASan/UBSan checks pass **3/3**
with leak scanning disabled, and import-only module checks pass **2/2** after
successful terminal compiler/linker processes. The lifecycle fixture destroys
UserContext, rejects its expired model reference and reads explicitly copied
Global strings and nested view-specific relationships through another Context.
Default PMR allocation is disabled for every native/module probe.

All **121** canonical consumer/fixture bodies, **1,477** exports, module paths
and the acyclic layer dependency graph verify. Pinned format/check covers
**1,010** handwritten C++ files. The [terminal record](parity/json_hydration_5_0_6.json)
retains source hashes and exact selected tests. The
[static binding inventory](parity/active_corpus_baseline_after_json_hydration_5_0_6.json)
counts 147 active, 63 historical and 21 custom selections; these counts establish
binding only, not acceptance. No MIPS or physical-device test was run here.
