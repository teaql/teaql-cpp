# Value complete enum API mapping and scalar conversion audit

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-core/src/value.rs`. This mapping accounts for that enum's public items;
Decimal's separately reexported dependency API retains its own acceptance gate.

## Grouped source acceptance, 2026-10-08

The grouped value.rs/time.rs contract, including the enabled Decimal reexport,
is accepted after the complete production/trait/protocol/ownership reviews in
this document, timestamp_api_acceptance.md and decimal_api_contract.md. The
current final native refresh completes 452 tasks after the earlier 644-task
consumer build; the final build reports no work, all 375 dependency records are
nonempty and full CTest passes 303/303 in 74.81 seconds.

Fresh combined Value/time/Decimal consumers pass 21/21 under ASan/UBSan with
leak scanning disabled (78.13 seconds) and 21/21 on MIPS/QEMU (59.04 seconds).
The separate current scalar/DataType/memory selection passes 13/13 on sanitizer
(2.63 seconds) and 13/13 on MIPS (4.24 seconds). Exact original observations
include 4,280 Decimal foundation/default inputs and actual 238-input 32/64-bit
pointer profiles. No formatter/private-state fields are rewritten to make
comparisons pass. Source constant-function, noncopyable ownership, mutable
Timestamp, original source-test and nested-allocation contracts are included.

Explicit allocator/destination copies, Context borrow expiry, portable integer
carriers, exceptions/optional failure and named standard-clock adapters are the
documented C++ adaptations. Source variants/defaults/trait responsibilities,
numeric/format/wire boundaries and separate model responsibilities are retained.
This closes the last Stage 2 source group; stages 3–6 and physical-device execution
have independent requirements. The prepared Stage 3 opaque projection-state
fixture remains outside this suite and reproduces a separate missing capability.

## Complete body review and current integration gate, 2026-10-08

The complete pre-test value.rs body is traced through the native public enum,
constructors and source implementation: ten DataTypes, thirteen Value variants,
all sixteen explicit From implementations, object construction, eight optional
extractors and recursive JSON projection. Their derived traits remain distinct:
DataType is Debug/Clone/Copy/Eq; Value is Debug/Clone/PartialEq and has no source
Default/serde/Hash or total-order derive. TypedNull does not become untyped Null
in equality or diagnostics. Integer/Decimal coercions do not accept unrelated
Float/Text/Bool variants; date/timestamp conversions keep their separate limits.

Actual source conversion/equality, owning constructors, raw diagnostics, UTC
conversion and thirteen original source tests form complementary evidence.
Explicit cross-domain copies retain all nested values after actual User deletion;
same-domain owning inputs transfer storage. Physical variant order, owner pointers
and native hash/bucket implementation are not source wire discriminants.

The complete time.rs production body and both original tests are independently
reviewed in timestamp_api_acceptance.md, including its writable signed field,
wrapping unsigned construction, fallback datetime, current-clock floor and
transparent signed JSON integer. Decimal's enabled dependency/default review
now restores distinct inherited Ord operations; that contract also retains
actual pointer-width and constant-evaluation evidence. Fresh combined consumers
and the current complete native build remain pending. This body review does not
accept the grouped Value/time/Decimal source contract by declaration presence.

The combined current consumer selection subsequently passes 21/21 under
ASan/UBSan with leak scanning disabled (78.13 seconds) and 21/21 on MIPS/QEMU
(59.04 seconds), including original source Value assertions. Fresh scalar,
DataType and memory selections are tracked separately; the complete current
native final refresh remains live before accepting the grouped source contract.

The additional freshly built scalar/DataType/memory selection passes 13/13
under ASan/UBSan with leak scanning disabled (2.63 seconds) and 13/13 on
MIPS/QEMU (4.24 seconds). This includes complete Value extraction/format,
DataType diagnostics, default-PMR-disabled models, reference validity/budgets,
actual User teardown, scoped native resources and async stop/join. The current
452-task final native header/clock consumer refresh remains live; neither
group nor Stage 2 is accepted from these selected results alone.

| Source responsibility | C++ mapping and evidence |
| --- | --- |
| All thirteen Value variants | Null, Bool, I64, U64, F64, Decimal, Text, Json, Date, Timestamp, Object, List and TypedNull remain distinct variant alternatives. The physical C++ alternative order is not a wire discriminant. |
| All ten DataTypes | Ordinary value enum copying/equality and standalone derived diagnostics; the DataType audit covers every unit variant and formatter flags. TypedNull retains its type in equality and diagnostics while projecting null to JSON. |
| Six integer From conversions | Explicit-owner constructor templates exercise actual i16/i32/i64/u16/u32/u64 argument types at their limits, preserving signed versus unsigned Value variants. |
| Two floating From conversions | Actual float32 and float64 argument construction; finite optional results retain exact IEEE bit patterns, including subnormals and signed zero. Infinity and NaN remain present floating results rather than absent conversions. |
| Borrowed and owned text From | string_view snapshots and owning PMR String input; the separate owning audit checks same-domain transfer and foreign deep-copy/failure behavior. |
| Bool, Decimal, JSON, Date, Timestamp From | Native explicit-domain constructors exercise each original conversion. JSON remains a separate Value alternative rather than becoming Object/List. |
| DateTime<Utc> From | value_from_utc, documented separately with 64 actual original conversions and native negative-floor/overflow/ownership checks. |
| object and List/Object construction | Owning Object/List constructors retain all payloads; 171 original owning inputs and native storage/failure/lifetime contracts remain separate from these scalar observations. |
| All eight try methods | i64/u64/decimal/f64/text/bool/date/timestamp outputs are observed for every new input. Optional f64 observations expose exact finite bits and presence for nonfinite values, avoiding JSON-null conflation. |
| to_json_value | Complete projections, including recursive containers, all TypedNull types, nonfinite floating values and actual JSON number kinds. Value has no source serde derive; this projection is not an invented Value wire protocol. |
| Debug, Clone, PartialEq | Raw diagnostics, explicit-domain deep copies and structural comparison retain variant identity, nested values and NaN's non-reflexive comparison. C++ requires an explicit destination for copying, rather than an implicit default allocator. |

The new corpus contains 115 actual original-call inputs and 13,225 pairwise
comparisons. It includes all variants, all TypedNull types, integer widths and
limits, float32/float64 bit inputs, heap/empty text, Decimal scale/extremes,
recursive JSON/list/object payloads and source date/timestamp conversions.
Existing 722 extraction/projection cases, owning constructor cases and standalone
Debug audits remain complementary evidence, not replaced by the new corpus.

All 115 inputs and 13,225 equality results match both standalone native and
OpenWrt MIPS/QEMU probes after the documented NaN observation correction. value_api_rust_parity is registered in CMake after the owning Value consumer
rebuild passed 219/219. Fresh named integration completed: native full regression 224/224 and ten
selected sanitizer/MIPS checks 10/10 each, as recorded below. The Source oracle calls
actual original constructors and methods; it does not copy conversion/equality
algorithms. No production source, variant, dependency or lifetime changed.

Complete surrounding Decimal, arbitrary external serialization diagnostics,
query/runtime/log privacy and all-stage acceptance remain distinct requirements.
The initial MIPS comparison matched 113/115 inputs. Its only differences were
float32 NaNs widened by the legacy-MIPS soft-float ABI: both produced
7ff7ffffffffffff, while the host Rust oracle produced positive/negative preferred
NaNs. Complete Debug/JSON/extractor-presence/equality outputs already matched.
[Rust's floating-point contract](https://doc.rust-lang.org/std/primitive.f32.html)
permits nonportable NaN sign/payload across width-changing casts and documents
legacy MIPS's different signaling-bit interpretation. The fixture now compares
NaN classification only for NaNs passing through the actual float32-to-float64
conversion. It still compares exact bits for every finite result, infinity and
unchanged-width float64 NaN. No production conversion, variant, comparator
algorithm or other expected field was changed to accommodate this observation.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust_values/Cargo.toml \
  --bin value_api_oracle --locked --offline
python3 tests/parity/generate_value_api.py \
  --rust /home/philip/shared-cargo-target/debug/value_api_oracle
python3 tests/parity/compare_value_api.py --cpp <probe> \
  --rust /home/philip/shared-cargo-target/debug/value_api_oracle
```

For MIPS add --cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'.

## Original Value source-test review

All thirteen tests in the pinned value.rs now have direct ports in
teaql-core/value_contracts.cpp and individual covered dispositions. Every
original accepted/rejected input remains present: signed/unsigned integer
limits, unrelated Float/Text/Bool/Null variants, Decimal and text extraction,
136.25 and human 129.95 conversion, leap-day text/epoch conversion, timestamp
offsets, naive/date text, and malformed calendar/time input. Assertions preserve
optional presence and exact scalar values. Expected UTC milliseconds use
standard calendar arithmetic independently of the production text parser.

An additional native contract checks all ten TypedNull types: type identity
remains present, all eight scalar extractors return None, and JSON projection
is null. This complements the existing live original-call corpus; it is not
counted as another original source test. The new executable links only core
and memory, adds no runtime/provider dependency, and disables default PMR.

After rebuilding, original_core_value_contracts passes on native Debug,
ASan/UBSan with leak detection and OpenWrt MIPS/QEMU. Unchanged Rust passes
13/13 value tests. The complete source disposition review now covers all 73
core tests: 65 library tests, whose indexed names match the original compiler
list, and eight dynamic-search integration tests, which also pass in Rust.
This closes the core source-test review subset, not the full public API,
Decimal dependency or Stage 2/6 acceptance gates.

The subsequent full native consumer build completed and all 246/246 CTest
checks passed, integrating the new core source fixtures. Compiled layer checks
confirm the runtime still has no SQLite C API calls. The scoped Value sanitizer
and MIPS results are not claimed as full-platform regression runs. Pinned source,
inventory, public-module/export, disposition and 520-file formatting checks pass.

```sh
cmake --build build --target teaql_original_core_value_tests
ctest --test-dir build --output-on-failure -R '^original_core_value_contracts$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-core --lib value::tests --locked --offline
```

## Named integration checkpoint

The fresh ASan/UBSan build with leak detection and OpenWrt MIPS/QEMU build each
completed successfully and passed all ten selected checks: the five new
metadata/scalar checks plus core model, core API (native contract and Rust
comparison), Value model and Context reference contracts. Fresh MIPS probes
match all 160 metadata, 99 Timestamp and 115 Value inputs against live original
Rust. The metadata contract includes the latest qualified public-facade
assertion; the Timestamp contract includes both exact original source tests.
The full native build and regression completed with 224/224 checks passed.
Fresh named native probes also match all 160 metadata, 99 Timestamp and 115 Value
inputs against live original Rust. These are completed integration results;
remaining stage-wide API/privacy/source-test requirements are separate.
