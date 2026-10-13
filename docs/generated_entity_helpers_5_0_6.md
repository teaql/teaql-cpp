# Same-model generated entity helper qualification

This checkpoint uses the active Rust 5.0.6 lock and its original generator
commit `45e1572c4956ca963410d8ea436ead3c8857087a`. It supplements P1/G01 and
G13 evidence; it does not qualify the complete model matrix or generator.

## Original source and reproduction

`scripts/generate_entity_references.py` renders the existing `types.xml` and
`relations.xml` models with the clean frozen producer. The original generated
files are retained under `tests/generator/parity/types_rust` and
`tests/generator/parity/relations_rust`. Each `PROVENANCE.json` records both
locked commits, the input model hash and every generated file hash. The
generator integrity verifier checks all three original fixture families,
including the previous Business-ID fixture. `--check` reproduces file sets and
bytes without editing the emitted Rust.

The complete original `cpp-relations-core` crate compiles against the pinned
runtime. The complete original `cpp-types-core` crate does not: generated
runtime.rs refers to the missing `module_with_behaviors_and_checkers`, and
PersonRequest invokes missing `whose_active_is` and `whose_active_is_not` methods
from its human boolean convenience methods. These are frozen producer errors,
not permission to alter source or claim complete original model compilation.

The handwritten `tests/parity/rust_types_entities` oracle compiles the six
original entity.rs files directly using `#[path]`. It adds no entity behavior
and excludes the broken request/runtime assembly. Its evidence covers entity
helpers only. The relation oracle imports the complete original crate.

## String mutation contract

The original `RustFieldDescriptorNaming` conversion and generated setters trim
Rust Unicode White_Space from the stored string, while storing the original
input `Value` in the mutation ledger. These are separate observations:

- Required string getters/evaluation read a convertible changed ledger value
  before falling back to the stored field. Their result can retain whitespace.
- Optional string getters/evaluation read the stored optional field. Non-NULL
  updates therefore expose trimmed text; an explicit NULL evaluates as Null.
- `changed` preserves the original operand, including explicit NULL. No pending
  change is distinguished from a pending NULL in the comparison projection.
- Ledger-only overlays do not mark a field loaded or replace an optional field's
  stored value. Cloned views retain independent availability/stored fields and
  share the existing Context ledger according to the runtime contract.

The runtime's `detail::EntityRuntimeAccess::update_text` implements the storage
and ledger distinction for required and optional text. It uses the existing
locale-independent Unicode trim helper and Context allocators. It copies the
operand before modifying storage and stages an optional string before replacing
it, so passing the field's own optional getter remains safe. Generated setters
delegate to this helper and retain their indexed mark-loaded operation.

## Direct observations

The paired probes use identical models and inputs. Scalar observations cover
required text, optional text, false/zero defaults, bool/integer updates, sparse
and loaded rows, NULL, Unicode whitespace, zero-width-space preservation, ledger
overlays, self updates and cloned views. Relation observations cover optional
foreign keys (absent, NULL, zero, matching and nonmatching IDs), selected versus
unselected forward views, cache fallback, absent/present/empty/wrong-type edges
and reverse to-one handles. Wrong-type source panics and C++ logic errors are
captured with their exact message; the corpus does not normalize outputs.

Initial comparison exposed eight optional string differences in 632 cases.
Additional self-update cases exercise the aliasing fix. The committed corpus
and its comparison script record only observed, matching outputs.

Remaining optional scalar/model configurations, inherited entities, candidates,
Checker configurations, aggregates and model assembly remain separate open
requirements. This checkpoint does not change their qualification status.

## Verification checkpoint

The extended live comparison passes **1,624/1,624** direct original/C++
observations: the previous 160 scalar and 512 relation cases plus 952 additional
field cases. The focused native parity test passes 1/1 after rebuilding its
probe. No production or generator code changes were needed for this extension;
the previous sanitizer/module/generator results below belong to the string
setter implementation checkpoint.

| Field family | Additional direct observations |
| --- | --- |
| Decimal | Zero, negative values, retained decimal scale, seven fractional digits and the source 96-bit maximum. |
| Double / float | Positive/negative zero, precision conversion, subnormal values and finite maximum values; float setters receive a typed f32 on both sides before constructing their ledger operand. |
| Date / timestamp | Epoch, pre-epoch, leap day, year one and negative/positive millisecond timestamps. |
| JSON | Absent field, explicit NULL, Some(JSON null), false, nested Unicode objects and arrays; stored optional evaluation remains distinct from ledger-only overlays. |
| ID / version | Sparse defaults, zero, negative versions, 32-bit ID boundary, u64/i64 limits and a changed ID's resulting ledger key; version getter overlays remain separate from stored VersionedEntity metadata. |
| Snapshot / clone | Original row values stay unchanged by setters and ledger-only overlays; pending NULL is distinguished from no pending mutation; clone availability and stored optional values remain independent. |

Null, typed-null, incompatible text and bool operands also exercise ledger
conversion fallback for every added field. The source probe obtains the
entity's own state through `LedgerEntity::entity_runtime_state`; it does not
replace that state with an unrelated root. This matches the C++ probe and
retains each row's original loaded view. The complete types.xml Person field
matrix is covered by these two scalar groups, while arbitrary optional scalar
combinations and inherited/candidate model configurations remain open.

### Initial string-setter checkpoint

The live original/C++ comparison passes 672/672 observations (160 scalar cases
and 512 relation cases). Focused native CTest passes 7/7: the new parity test,
generated API, Checker, relation, Business-ID and dynamic-field contracts, and
the generated query/expression example. Java generator/layout tests pass 12/12.
The five C++ fixture sets reproduce all 55 generated files; the two original
Rust fixtures reproduce all 44 and 32 files respectively, including provenance.
The frozen runtime and producer worktrees remain clean.

Targeted ASan/UBSan CTest passes 3/3 (the new parity probe, generated API
contracts and generated relation contracts), with final leak scanning disabled.
Native named-module consumption passes 3/3 (import contracts, generated
query/expression example and generated dynamic-field contracts). The existing
1,406 export declarations and seven isolated module lookups verify.

Reproduce the direct comparison with:

```sh
source scripts/toolchains/activate.sh
python3 scripts/generate_entity_references.py --check
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin generated_entity_helpers_5_0_6
cmake --build build_native_current --target teaql_generated_entity_helpers_probe
python3 tests/parity/compare_generated_entity_helpers.py \
  --cpp build_native_current/teaql_generated_entity_helpers_probe \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_entity_helpers_5_0_6
```
