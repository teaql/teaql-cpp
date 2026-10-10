# Owned Value input and allocation-domain acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Original From<String>, Value::List(Vec<Value>) and Value::object(BTreeMap)
consume owning input. The C++ constructors previously accepted only const
String/List/Object references, so forwarding an rvalue still copied its payload.
That imposed avoidable peak arena storage, even within one explicit owner.

## Implemented constructors

The three rvalue overloads transfer storage when the input allocator equals the
Value's explicit destination. Heap string characters, vector buffer/capacity,
nested Value payloads and map nodes/keys remain at the same addresses. C++ small
strings may use inline storage rather than Rust's string layout; no universal
short-string pointer identity is promised.

Foreign inputs are fully copied into the destination instead of retaining source
storage. They keep the established Value cross-domain relocation contract: the
source remains available, including after a successful explicit retention copy.
Failed construction also leaves the source untouched. This retained-source rule
is a C++ allocation-domain adaptation, not a claimed Rust post-move capability.
Borrowed const-reference constructors remain copying operations.

Nested strings, Values, JSON and container storage follow existing allocator
propagation. No shared ownership extends the source User lifetime. Resource
teardown and arena block reclamation are unchanged. The Expr IN-list range path
now calls the ordinary owning Value constructor instead of directly emplacing
its public variant storage to avoid a constructor copy. No query operation,
threshold, Value variant or snapshot/mutation role changes.

## Direct original and native evidence

The original-call corpus records 171 owning constructions: heap text including
Unicode/newlines/NUL, list and object sizes 0/1/2/20/21/64, all thirteen Value
variants, NaN, integer bounds, recursive JSON/list/object and exact Decimal scale.
The original oracle calls the actual constructors, observing complete Value
Debug/JSON, Clone equality and transferred character/vector/first-map-value
addresses. Native C++ matches all 171 unchanged observations.

The native contract runs with usable default PMR disabled. It verifies actual
same-owner string/vector/map storage identity, vector capacity and nested
character addresses. Recompiling against an isolated old declaration header and
old archive reproduces the unnecessary same-owner string copy; the repaired
production source passes the initial contract.

Foreign-copy failure injection precharges the Global object page, then disables
upstream allocation before attempting large text/list/object copies. Every source
keeps its content and addresses. Successful retry produces distinct destination
storage and complete nested Global allocator propagation. Actual source User
destruction leaves zero charged User bytes while all retained payloads remain
usable. Global destruction returns both successful and abandoned-copy blocks.
Failed object reservations retain the existing non-reuse policy.

Named value_owned_api_contracts/value_owned_api_rust_parity are registered.
Fresh OpenWrt MIPS/QEMU integration finished successfully: all ten selected
checks passed 10/10, including Value, query, collection, SmartList and Context
reference contracts. The freshly built MIPS probe matches all 171 live original
observations. The corresponding ASan/UBSan build and all ten selected checks
also completed successfully with leak detection enabled. The full native build and regression completed successfully: 219/219 checks
passed, and its named probe matches all 171 live original observations. The next
metadata/scalar API registration requires a separate fresh regression checkpoint.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin value_owned_api_oracle --locked --offline
python3 tests/parity/generate_value_owned_api.py \
  --rust /home/philip/shared-cargo-target/debug/value_owned_api_oracle
cmake --build build --target teaql_value_owned_api_tests teaql_value_owned_api_probe
ctest --test-dir build --output-on-failure \
  -R '^(value_owned_api_contracts|value_owned_api_rust_parity)$'
python3 tests/parity/compare_value_owned_api.py --cpp build/teaql_value_owned_api_probe \
  --rust /home/philip/shared-cargo-target/debug/value_owned_api_oracle
```

For MIPS add `--cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'`. This change removes
extra C++ copying within the selected lifetimes without changing core semantics
or adding a dependency/generator change. Complete remaining Value/Decimal/API/
privacy/source-test and all-stage acceptance remain open. Native views retain
caller invalidation obligations; physical-router execution remains unverified.
