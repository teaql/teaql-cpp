# Generated-model governed SQLite save comparison

This P1 checkpoint extends the tenant/note original-source comparison from
preflight denial to actual SQLite writes. It uses the same complete original
generated crate and the active Rust 5.0.6 locks. It does not replace the separate
1,620-case direct/preflight corpus or claim full generator qualification.

## Actual operations

The probes explicitly initialize the SQLite provider and schema, seed trusted
Tenant rows and call the public audited-ledger save API. Custom mutation policy
captures the reviewed plan and permits the write. The typed business Checker
still reads a UserContext resource, can reject and can fix text after predefined
Context/time checks. Original generated descriptors, entity methods, Checkers
and ServiceRuntimeExecutor execute unchanged on the Rust side.

Create starts from a fresh entity and explicit field updates, rather than a
loaded row containing the proposed values as its original snapshot. The trusted
tenant comes from UserContext through the generated Context-root fix. Update
starts with the database's old name and version, then uses the public name setter
to establish mutation intent. Sparse availability and mismatched/missing active
roots remain part of the negative cases.

Successful create returns version 1; successful update advances stored version 3
to 4. The observation includes the returned typed entity, physical database rows,
ordered fix evidence, custom-hook views, policy plans and pending field/new-key
state after completion. Trigger failures and a stale optimistic-lock version
compare rollback state against the physical row before the operation.

## Stable observation contract

Clock epoch milliseconds and volatile execution IDs are excluded from this
explicit projection. The SQL observations retain equality of the persisted
create/update clock fields and whether each clock still equals its seeded value.
Typed returned values retain clock-change and availability flags. Ordinary
names, tenant IDs, versions, diagnostics, plans and fix evidence compare exactly.

Backend error observations include the runtime error kind. The dedicated trigger
and optimistic-conflict fixtures additionally classify the known failure marker;
this is a declared probe projection, not a rewrite of the runtime error object.
All Check diagnostics retain their complete wire representation in the three
naming profiles. Other backend errors retain their actual display string.

The private Context-root setter in original Rust is not invoked by application
probe code. Controlled malformed/raw-field cases remain in the previous
preflight corpus. At this checkpoint C++ exposed the generated setter publicly.
The subsequent [visibility checkpoint](context_root_visibility_5_0_6.md) closes
that model-API gap while preserving the runtime root checks and these observations.

## Scope and evidence

The 540-case matrix covers create/update, matching/missing/mismatched
active roots, four availability patterns, ordinary/empty/Unicode-whitespace text,
custom fix/reject hooks, trigger failure and stale-version conflict. The live
original/C++ comparison passes 540/540: 36 successful writes, 36 trigger failures,
four optimistic conflicts and 464 Checker rejections. Every trigger/conflict
case retains its physical database state, and every success clears pending
fields and new keys. Success is checked from actual SQL rows and typed readback,
not inferred from the absence of an error.

This fixture registers generated descriptors and protocols explicitly. It does
not qualify generated bootstrap/sample-data assembly, inherited entities,
candidates, all business extensions or the later P2-P7 capabilities.

## Verification checkpoint

Native CTest passes 3/3: the earlier preflight corpus, this actual-save corpus
and generated Checker integration. Targeted ASan/UBSan passes both corpora, 2/2,
with leak scanning disabled. The runtime and producer reference worktrees remain
clean, and generator integrity verification retains all 17 complete-capability
qualifications as pending. No production runtime or generator changes were
needed for this additional commit/rollback evidence.

Reproduce the direct comparison with:

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin generated_checker_flow_5_0_6
cmake --build build_native_current --target teaql_generated_checker_flow_probe
python3 tests/parity/compare_generated_checker_commit.py \
  --cpp build_native_current/teaql_generated_checker_flow_probe \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_checker_flow_5_0_6
```
