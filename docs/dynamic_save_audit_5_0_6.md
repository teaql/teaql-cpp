# Durable dynamic save and audit qualification

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Generated source: producer `45e1572c4956ca963410d8ea436ead3c8857087a`,
the unchanged `dynamic_json_rust` Product model and runtime assembly.
Status: the declared same-model matrix passes; complete G01/G14/G16/G17/P7 remain open.

## Actual generated execution

Both observers initialize the same existing SQLite schema and dataset, install
the generated Checker/behavior assembly and executor, and set trusted Platform
root 1. The fixture creates extension storage through the actual database dynamic
provider, then seeds one trusted Text definition, `note`, for Product 7.

Generated Q loads the complete Product and its durable extension. The generated
model stages Set, NULL or Delete and performs an audited save with an intent
containing both the old and new extension operands. Runtime owns validation,
governance, optimistic writes, native/dynamic transaction coordination, readback,
audit enrichment and delivery. Observers do not implement these responsibilities.
The Rust generated files and runtime production methods remain unchanged.

The matrix has **24** cases: three mutation kinds, optional storage-trigger
failure, optional optimistic version conflict and optional audit-sink failure.
The trigger rejects INSERT, so it rejects Set/NULL but permits Delete; this also
checks that the fixture does not manufacture a failure for every mutation kind.
Version conflict is introduced after loading through an explicit fixture update.

Each outcome compares:

- Exact save error text and pending ledger state.
- Persisted native version and extension row/null/payload, including row absence.
- Complete safe audit event Debug, including dynamic change details, trace,
  field flags/lengths and the generated-default governance snapshot.
- Ordered SQL statements, projected bindings, comments and purposes.

Time and duration fields are omitted from the SQL observation in both observers
because execution clocks are nondeterministic. Retained values and strings are
compared without normalization. All **24/24** actual source/native outcomes agree;
the stored corpus also reproduces against live original Rust.

## Transaction and privacy results

Successful Set/NULL/Delete saves increment the native version and clear the
shared pending ledger. NULL retains its extension row; Delete removes it.
Storage-trigger failure rolls back the version and extension change, retains
pending mutations and delivers no application audit. Version conflict rejects
before dynamic persistence and likewise retains the staged changes.

Application audit failure occurs after commit: the persisted change remains,
pending mutations clear, and the exact committed-operation error is returned.
It is not a rollback or a reason to blindly retry the mutation.

Old/new dynamic operands participate in privacy collection before SQL executes.
The exact retained SQL intents and complete safe audit trace replace those
operands with redaction markers. Dynamic audit fields retain source masking and
SET/DELETE details; raw SQLite storage retains the intended business value.
The built-in log endpoint is off while Context SQL diagnostics and intent
validation remain active. File/sensitive endpoint delivery is outside this matrix.

## Ownership and terminal gates

The C++ observer disables the default PMR allocator and actually destroys its
UserContext and GlobalContext after every input. Its application audit sink is a
scoped native resource allocated with `make_resource`; retained diagnostic strings
and event values use the explicit User allocator. No shared owner extends it.

Relevant native checks pass **3/3**, including the generated dynamic carrier and
the earlier complete projection matrix. The new save matrix passes under
ASan/UBSan with leak scanning disabled and through a module-only consumer.
All **136** canonical consumer bodies verify. Pinned formatting/check passes
**1016** handwritten C++ files. The
[verification record](parity/dynamic_save_audit_5_0_6.json) retains actual test output,
source hashes and the initial fixture-only compiler corrections.
No production algorithm change was needed for these outcomes.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build build_native_current --target teaql_dynamic_save_audit_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
cargo build --offline --locked \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin dynamic_save_audit_5_0_6
python3 tests/parity/generate_dynamic_save_audit_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/dynamic_save_audit_5_0_6 \
  --cpp build_native_current/teaql_dynamic_save_audit_probe --check
ctest --test-dir build_native_current --output-on-failure \
  -R '^(dynamic_save_audit_rust_parity|generated_dynamic_field_contracts|dynamic_audit_projection_rust_parity)$'
```

The sanitizer target/test has the same spelling in `build_sanitize_current` and
runs with `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1`.
The module target is `teaql_module_dynamic_save_audit_probe`; its test is
`module_dynamic_save_audit_rust_parity` in `build_modules_native`.

## Remaining qualification

This fixture uses explicit existing-schema SQL. It does not repair or qualify the
generated schema/bootstrap failure already recorded for the complete model.
One trusted Text field and default generated governance do not qualify every
dynamic type or application policy. Inherited/constant/candidate models, storage
identity replacement, held-stream/save interleavings, asynchronous delivery,
custom policies and actual log endpoints remain required. This evidence does not
close complete capabilities or phases. No MIPS or physical-device run was made.
