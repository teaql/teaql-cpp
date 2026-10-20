# Generated identity and load-state entries: Rust 5.0.6

The previously missing G01 application entries now exist in the shared generated
Model adapter: `withId(user_context, id)`, `isLoaded(field)` and
`setLoadState(state)`. Existing framework names and public member access remain
compatible. Complete G01/P1 and the wider generator goal remain open.

## Runtime-first adapters

The locked Rust entity template provides with_id, is_loaded and set_load_state.
C++ uses its established lowerCamelCase application naming. withId constructs
the existing U64 Value using UserContext's memory resource, so its allocation
domain is explicit; Rust's inline Value helper has no Context argument.

setLoadState copies the input through the existing scoped LoadState constructor
into the model's domain, then uses the existing SnapshotPool/indexed conversion
with that model's FieldLayout. Only successful conversion replaces the stored
state. It stages no mutation and changes no scalar or original value. No new
loaded-state algorithm, cache, provider or ownership scope is emitted.

An incompatible indexed layout rejects and preserves the previous state.
The original Rust entry panics through expect; C++ throws through Result::value.
The observer records the actual rejection and full before/after state rather
than treating either failure as success or claiming identical language wrappers.

Generated Assist documents each concrete Model's helper and these boundaries.
For example:

```cpp
auto id_value = cpp_relations::School::withId(user_context, 42);
std::array<std::string_view, 2> fields{"id", "name"};
school.setLoadState(teaql::eval::LoadState::partial(
    user_context.memory_resource(), fields));
bool name_available = school.isLoaded("name");
```

## Source observations

The complete unchanged original relations domain supplies Platform, School and
Device. A separate observer workspace uses reference registry versions and
checksums, including rusqlite 0.40.1, and actual original helper calls.

All **1,008/1,008 original/native outputs match** over three models, seven input
states, two prior states, four field selections and six identifiers. States are
NotLoaded, FullyLoaded, Partial, PartialCompact, SharedColumns, compatible Indexed
and another entity's Indexed layout. Selections include duplicate names,
relationship availability, a trusted `#extra` alias and unknown names.
IDs cover zero, one, 2^32, both sides of the signed-64 boundary and U64 maximum.

Outputs retain the U64 value, ten named before/after flags, rejection, unchanged
name, no newly staged name mutation and the immutable original name. The native
observer also checks the selected Value resource. The corpus has no normalization;
failure wrapper text is outside its declared projection.

## Ownership and integration

Twelve separate native controls cover Partial, PartialCompact, SharedColumns
and Indexed transfer across two Users in one application Global, for all three
models. A long selected name belongs to the destination after actual source User
deletion. Scalar values and pending mutation stay unchanged. An explicit Global
model copy then survives destination User deletion and is read through an
explicitly rebound reference in a new User. Default PMR is disabled.

The initial native test used a nonexistent EntitySnapshot.get method; it now
reads the public fields map. A first ownership run attempted to use a Global
reference still bound to the deleted User and correctly rejected it. The test
now uses the existing rebind_global contract. These are test corrections, not
changes to snapshot or reference semantics.

Java regression passes **43/43**. Native helper checks pass **2/2**, module checks
pass **2/2**, ASan/UBSan passes **2/2** with leak scanning disabled, and existing
generated API/optional/candidate/example regression passes **4/4**.
Twenty workspaces/240 files regenerate and reproduce. Forty-four contract SDKs
reproduce; thirty update only their generated support.hpp and Assist. The
explicit recording mode requires identical file sets and every other byte,
then a strict check verifies all new bytes/provenance. No generated header is
patched manually. Historical C++ and original Rust artifacts stay unchanged.

Canonical module verification covers 544 bodies and 1,479 exports; pinned C++
format/check covers 1,371 files. The
[terminal record](parity/entity_helper_entries_5_0_6.json) preserves results and
hashes. No production runtime function, dependency, MIPS or physical-device
claim is introduced. The [G01 matrix](entity_helper_requirements_5_0_6.md)
retains the remaining source/body/configuration closure obligations.

## Reproduce

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/generate_examples.py --check
python3 scripts/reproduce_cpp_contract_fixtures.py --check
cmake --build --preset native --target \
  teaql_entity_helper_entries_probe teaql_entity_helper_entries_lifetime_tests
CARGO_TARGET_DIR=/tmp/teaql_cpp_inherited_business_binding_bridge_506 \
  CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  cargo build --offline --locked \
  --manifest-path tests/parity/rust_entity_helper_entries_5_0_6/Cargo.toml
python3 tests/parity/compare_entity_helper_entries_5_0_6.py \
  --cpp build_native_current/teaql_entity_helper_entries_probe \
  --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/entity_helper_entries_506
ctest --preset native --output-on-failure \
  -R '^entity_helper_entries_(rust_parity|context_contracts)$'
```
