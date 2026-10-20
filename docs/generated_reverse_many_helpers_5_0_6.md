# Generated reverse-many helper checkpoint: Rust 5.0.6

This adds G01/G13 evidence without completing either capability. The complete
original relations domain is unchanged and compiled against the locked runtime
with registry versions/checksums from its reference Cargo.lock.

## Actual generated methods

Platform's original school_list/device_list handles and eval methods execute
against a frozen EntityGraphBuilder. C++ calls the corresponding generated
schoolListHandle/deviceListHandle and eval methods over the shared runtime.
These cover required and optional foreign-key child model shapes. Neither side
installs a data service for reading these views; helpers cannot issue implicit SQL.

The 864 inputs vary relation, zero/one/three members, stored list is_loaded,
parent selection, absent/list/wrong-type edge, matching/wrong owner/wrong identity,
sparse/loaded child names and short/long Unicode names with embedded NUL.
Complete outputs preserve handle state/loaded/empty, stored list flag, actual
member ID/name/eval and relation Value/Null/NotLoaded, plus exact type-mismatch
panic/exception text. There is no normalization.

All **864/864 original/native outcomes match**. An installed empty list evaluates
as Value even when its stored is_loaded is false. Installed nonempty views are
also available independently of that flag and parent projection. Absent or
wrong-owner/identity keys stay NotLoaded. Matching wrong-type edges fail with
the original message. Sparse member names stay NotLoaded; reading the relation
does not manufacture their scalar load state.

The Rust observer initially needed an explicit child type in its projection
closure. Only that handwritten observer annotation changed; original generated
methods and runtime code were not edited.

## Ownership and verification

Native output is explicitly copied into Global, stack PMR values leave scope,
and UserContext is actually deleted before serialization. Default PMR allocation
is disabled. Native, module and targeted ASan/UBSan each pass **1/1** over all
864 cases. Sanitizer leak scanning is disabled. Pinned format/check covers
1,369 C++ files; canonical module verification covers 542 bodies.
The [terminal record](parity/generated_reverse_many_helpers_5_0_6.json) retains
the exact profiles and provenance. No runtime/generator change, dependency or
MIPS/physical-device acceptance is included.

The [G01 requirements](entity_helper_requirements_5_0_6.md) retain concrete
missing generated entry points and the remaining closure review.

## Reproduce

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_reverse_many_helpers_probe
CARGO_TARGET_DIR=/tmp/teaql_cpp_inherited_business_binding_bridge_506 \
  CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  cargo build --offline --locked \
  --manifest-path tests/parity/rust_reverse_many_helpers_5_0_6/Cargo.toml
python3 tests/parity/compare_generated_reverse_many_helpers_5_0_6.py \
  --cpp build_native_current/teaql_generated_reverse_many_helpers_probe \
  --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/generated_reverse_many_helpers_506
ctest --preset native --output-on-failure \
  -R '^generated_reverse_many_helpers_rust_parity$'
```
