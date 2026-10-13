# Audit event identity, bootstrap and invocation provenance

Status: shared runtime/model/projection and real mutation dispatch corrected;
scoped source, native, lifetime and module gates pass. Full G14/G17/P1/P7 remain
open. Runtime: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Preserved event responsibilities

SafeAuditEvent now carries entity_id and bootstrap_audit. Target identity is a
positive U64 or I64 from the raw event's current values. It is separate from
lineage identity and does not fall back to old snapshots, text, floating-point
or boolean coercion. Bootstrap attribution is copied, preserving category,
version and timestamp; actor/reason use the same scrubbed operands as lineage.

RawAuditEvent retains invocation-local SqlIntentRedactions in its explicit
allocation scope. Trusted binding snapshots the source and handles self-binding
without destroying referenced data. Allocator-aware event copies duplicate the
complete nested provenance into the destination. No shared ownership extends a
Context lifetime. The existing public method signatures and event/service/SQL
responsibilities remain separate.

EntityMutationOperation's common event dispatch attaches its service's mutation
provenance for insert/update/delete/recover/batch events before UserContext
delivery. Safe event projection and log projection consume those protected
operands even when they are absent from changed fields. Log projection clears
the private source in the projected copy; the original event remains intact.
Credentials remain protected in plaintext mode. Durable `#fields` follow the
source default masking rule; nonmasked id/version values retain their original
safe field output while their tokens can still be scrubbed from intent text.

Both canonical structural Debug and the log formatter reflect the actual new
fields. They do not fabricate metadata to satisfy expected text.

## Actual source evidence

The separately replayed historical event-model corpus now matches actual frozen
Rust plus native C++ **112/112**. Active CMake selection uses the new comparator;
the old corpus/driver and initial failure record remain intact.

A second **104/104** corpus compares nonempty provenance, plaintext/masked modes,
bootstrap presence and thirteen identifier variants. The Rust observer compiles
the unchanged original event.rs and log_privacy.rs modules in its test-only
crate, allowing their actual crate-private provenance path to execute. Source
file hashes and the exact commit are recorded. This is original implementation
execution, not a rewritten privacy evaluator. Complete safe event/log projection
strings and clone/provenance observations match without normalization and
reproduce in a second live run.

These cases include absent/NULL/typed-NULL/zero/negative/max IDs, misleading old
snapshot and parent lineage IDs, operands absent from changed fields, forced
credential redaction and dynamic fields. The source comparison does not qualify
every generated model, governance configuration or asynchronous event sink.

## Native and ownership gates

Native event/audit/Context/service/privacy/reentrancy selection passes **12/12**.
Subsequent real mutation-dispatch selection passes **5/5**, including the two
source comparators. Its first attempt assumed nonempty lineage without supplying
one; the fixture now explicitly binds lineage before checking projection. No
extra lineage is manufactured in runtime to satisfy that test.

The lifetime regression disables default PMR, snapshots mutable source
provenance, tests self-binding and retains raw/safe events through explicit
Global copies. It actually deletes the source UserContext, rebinds into a new
one and verifies independent bootstrap/provenance storage plus effective later
projection. Targeted ASan/UBSan passes **4/4** for event/audit/Context/service and
**1/1** for the real mutation dispatch, with leak scanning disabled. Host SQLite
is not instrumented.

The same event ownership contract passes through the canonical named-module
consumer **1/1**. All 95 canonical consumer/fixture bodies verify. Pinned
format/check passes **990** handwritten C++ files; all 1,477 module catalog
exports and active paths verify. No generator output is hand-edited and no new
runtime dependency is added. MIPS/physical-device execution is not part of this
checkpoint.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_event_model_probe \
  teaql_audit_event_projection_probe teaql_event_model_tests teaql_runtime_mutation_tests
python3 tests/parity/compare_event_model_5_0_6.py \
  --cpp build_native_current/teaql_event_model_probe \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/event_model_5_0_6
python3 tests/parity/generate_audit_event_projection_5_0_6.py \
  --cpp build_native_current/teaql_audit_event_projection_probe \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/audit_event_projection_5_0_6 --check
```

Build the named Rust observer with the locked rust_field_layout manifest and
existing target directory before consuming it. Complete new-baseline integration,
row/request diagnostics and further protected-I/O/generator configurations remain
required. Prior whole-suite records are diagnostic history, not acceptance of
these subsequent changes.
