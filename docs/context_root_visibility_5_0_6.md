# Generated Context-root assignment visibility

This P1 API checkpoint follows the frozen producer's
`rust-lib-core/entity/struct.rs.stg` object-field setter: a Context-root foreign-ID
setter is `pub(crate)` rather than public application API. Ordinary foreign-ID
setters remain public. The C++ adaptation uses a private member accessible to
the generated model's friend Checker; it does not emulate a separate ownership
domain or introduce a new runtime service.

Generated C++ Context-root ID and object setters are internal. The object setter
is also restricted because allowing it publicly would provide a second path to
the same Context-root ID assignment. Their implementations remain present and
unchanged. The generated Checker retains access for UserContext assignment and
validation. Getters, changed/eval helpers, relation evaluation, typed predicates
and relation selection remain public.

Model-aware Assist describes assignment through UserContext and omits both
setters. It continues to list reading, filtering and projection APIs. All six
fixture families are regenerated, including the Checker and Business-ID models;
generated files are not hand-edited.

This is an application API boundary, not a claim of protection against deliberate
use of low-level reconstruction or mutation-ledger protocols. Those APIs remain
available according to their runtime responsibilities and still undergo Context
root/preflight/policy checks. The controlled mismatch integration fixture now
uses the runtime ledger to supply invalid input instead of calling an application
setter that the original producer keeps internal.

## Evidence

Java regression assertions verify internal setters, public evaluation and
accurate Assist. Real compiler contracts accept the public read/query surface
and reject direct ID assignment, object assignment and Business-ID model root
assignment. A named-module consumer uses compile-time access checks and performs
an actual generated SQLite save whose Checker supplies the trusted root and
operation timestamps internally. The existing 1,620 preflight and 540 successful/
rollback observations preserve their original source outputs.

Verification passes Java generation/layout tests 13/13, targeted compiler
contracts (one positive, three negative), native integration 4/4 and the new
named-module access/save consumer 1/1. The original 1,620 and 540 source corpora
replay unchanged. All six C++ fixture sets reproduce 66 files. Pinned formatting
checks 815 current C++ files, and the frozen reference worktrees remain clean.
No runtime implementation, allocator or lifetime behavior changed in this
checkpoint. This closes the documented setter-visibility gap;
inherited/candidate configurations and other P1-P7 requirements remain separate
qualification work.

Run the focused application compiler checks with:

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_type_errors.py --build-dir build_native_current \
  --cases context_root_getters context_root_id_setter \
  context_root_object_setter business_context_root_setter
```

The verifier's default remains the full compiler contract set. Selecting these
cases reports only their actual focused coverage.
