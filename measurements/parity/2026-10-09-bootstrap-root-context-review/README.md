# Bootstrap root and Context review, 2026-10-09

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

A generated schema bootstrap must install its typed active root while
`UserContext::ensure_schema` is running. The original public helper takes
`&self` and writes its OnceLock. The C++ helper incorrectly used ensure_mutable,
which rejects every Context with pending schema initialization. This blocks the
same operation that the handwritten migration must expose for future generated
bootstrap code. The locked school/conformance/order runtime assemblies call
this helper during generated bootstrap. The generator itself is unchanged.

The standalone native reproduction compiles against the installed production
archives and exits2 with `Context is borrowed by schema initialization`.
The independent Rust binary calls the unchanged pinned Context, schema provider
and RuntimeModule; it installs Tenant42, repeats the same assignment, rejects a
different ID and invalid input, and reads the installed root after completion.
The source's canonical error uses `tenant`, not `Tenant`. Its execution exits0.
See `before.json`, the output logs and the [original probe](../../../tests/parity/rust-values/src/bin/bootstrap_root_scope_oracle.rs).

The fresh pre-repair Context selections pass64/64 on native, ASan/UBSan and
MIPS/QEMU, demonstrating that those previous tests omit this composition.
The new registered schema runtime fixture exercises the real callback, pending
ordinary mutation rejection, idempotent repeat, canonical errors, post-completion
root visibility and closed access rejection. Acceptance requires its negative
control, repair, stable consumer rebuilds and final validation.

Physical-router testing remains explicitly deferred. The initial negative-control checkpoint did not accept Context or close a stage;
the final source-group acceptance is recorded below.


## Repair and target validation

The helper now checks a live, nonclosing Context without blocking an existing
schema borrow. The ordinary with_active_root and resource/provider mutation
gates are unchanged. The new registered schema fixture rejects ordinary root
assignment while pending, installs the generated root in the callback, verifies
same-value idempotence and exact canonical conflict/invalid errors, repeats the
pipeline, observes the completed root and rejects access after close.

Selected repaired consumer builds each finish43 tasks, then no-work refreshes
precede the64-check selections. ASan/UBSan passes64/64 in33.16 seconds with leak
scanning disabled; MIPS/QEMU passes64/64 in24.00 seconds. Post-test builds report
no work. The complete native 262-task consumer relink finishes before focused schema
validation and full 375/375 regression (76.30 seconds, four CTest workers).
All 418 compiled object dependency records are nonempty. Pre/post builds report
no work. The standalone native reproduction now exits0. Both native and MIPS
match all 2,384 live original defaults/models/store/resources/schema/ID/business-
service observations after the correction. See results.json and the retained
build/test/comparison logs.

The complete Context source group is accepted after the full production review
in docs/context-api-contract.md and this final validation. This does not accept
runtime service, graph/save, runtime-root or complete stages 4–6.
