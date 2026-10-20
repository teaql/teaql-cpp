# Generated pagination convenience qualification

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: the declared convenience-method execution matrix passes; complete G09/P4
and G13/P7 remain open.

## Original API and shared implementation

The [source review](parity/pagination_convenience_5_0_6_source_review.json) records
twelve original generated methods: `limit`, `skip`, `offset_only`, `offset`,
`page_offset`, `offset_size`, `top`, `page_number`, `page_number_default`, `page`,
`page_default` and `unlimited`. Their existing C++ lowerCamelCase equivalents
delegate to EntityQuery; this checkpoint changes neither production runtime
algorithms nor generator emission.

Offset-only methods preserve the limit, limit/top preserve the offset, and paired
methods replace both slice fields. Page numbers use saturating subtraction and
multiplication; default page size is ten. `unlimited` removes the slice. These
rules matter when operations are recomposed rather than used once in isolation.

The execution observer now accepts optional modifier sequences. Inputs without
them retain the previous behavior and output shape. All **1440** retained
generated-execution observations still match live original Rust, without changing
their corpus. Original generated domain files remain untouched.

## Matrix and purpose staging

The matrix contains **8448** inputs: 84 single-method/boundary sequences plus four
composition sequences, four generated models, two projection modes, six execution
forms and two purpose stages. Models are Person, Product, Order and Line;
projections are ordinary and minimal. Execution uses count, exists, first, one,
list and page through the actual SQLite/runtime stack.

Boundaries include zero, one, two, maximum u64, maximum page multiplication and
zero page size. Composition checks independent slice fields, clearing and
replacing pagination. Count removes the slice; exists/first preserve the offset
while selecting one; explicit page execution supplies its own offset and limit.
Actual SQL, bound Values, comments, purposes, typed IDs/versions, total counts
and exact errors are compared without normalization.

C++ applies each modifier before or after `purpose`, exercising both generated
request specializations. Original Rust supplies the plain methods through the
public `inner` request; the observer restores the captured purpose after modifying
that request. This is the previously agreed staged C++ facade adaptation, not a
claim that the Rust wrapper exposes the same methods directly.

All **8448/8448** outcomes agree: **7200** successful observations and **1248**
provider errors. Oversized SQLite integer bindings retain actual source failures;
they are not skipped or converted into empty successful pages. The stored corpus
reproduces against live original Rust.

## Verification and scope

Relevant native checks pass **3/3**, including old execution evidence and the
existing deferred scalar/cancellation/Context-destruction contract. Module-only
new/old execution checks pass **2/2**. All **138** canonical consumer/fixture
bodies verify, and pinned formatting/check passes **1016** handwritten C++ files.
The [terminal record](parity/pagination_convenience_5_0_6.json) retains source
hashes, actual test output and live reproduction results.

This is an observer-only extension using unchanged runtime ownership and generated
APIs. ASan/UBSan and MIPS were not rerun. No generator production change or new
artifact emission required Java/reproduction gates; the locked original source
and generated fixture integrity verifier passes.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build build_native_current --target teaql_generated_execution_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
cargo build --offline --locked \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin generated_execution_5_0_6
python3 tests/parity/generate_pagination_convenience_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_execution_5_0_6 \
  --cpp build_native_current/teaql_generated_execution_probe --check
python3 tests/parity/compare_generated_execution.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_execution_5_0_6 \
  --cpp build_native_current/teaql_generated_execution_probe
ctest --test-dir build_native_current --output-on-failure \
  -R '^(pagination_convenience_rust_parity|generated_execution_rust_parity|generated_execution_context_contracts)$'
```

The module observer target is `teaql_module_generated_execution_probe`.
Its new and retained checks are `module_pagination_convenience_rust_parity` and
`module_generated_execution_rust_parity` in `build_modules_native`.

## Remaining requirements

Deferred typed-page stages, resource replacement, cache interleavings and complete
cancellation branches remain separate G09 obligations. This matrix does not
qualify inherited/constant/candidate/dynamic-field, grouped or nested aggregate
page configurations. The independent quoted-Line aggregate-result discrepancy
remains pending the requested user choice. Complete capability/phase acceptance
and physical-device execution are not claimed.
