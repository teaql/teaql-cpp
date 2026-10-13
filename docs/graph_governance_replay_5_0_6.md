# Graph planning, audited save and governance replay

This checkpoint replays complete historical input families against locked Rust
5.0.6 and fixes shared authoritative readback behavior. It does not qualify every
generated model, graph/ledger configuration, private save adapter or P7.

## Actual source observations

| Family | Historical inputs |
| --- | ---: |
| Checker and typed Checker entry points | 1,362 |
| Mutation policy review, approval and delivery | 276 |
| Public entity graph planning | 704 |
| Move-only ledger audited save | 356 |
| Repository extraction | 141 |
| Graph models, planning batches and module graph helpers | 464 |
| Public audited save with an explicit executor | 356 |
| Total | 3,659 |

All **3,659/3,659** complete original/native observations match. The historical
input order is unchanged and a second actual Rust run reproduces the new
versioned corpora. Historical drivers and corpora remain intact. Original driver
clock brackets and timestamp markers are retained; no additional normalization
of source/native outputs is applied. These fixture-defined clock observations
are not equality of absolute timestamps between processes.

The [driver review](parity/graph_governance_5_0_6_driver_review.json) verifies
byte-identical logic or explicitly reviewed fixture imports/comments. Two old
audited observers initially failed with eight compile errors because 5.0.6
MutationRequest is a request envelope. They now inspect its actual MutationCommand
payload while observing the same operations, fields and trace values; no runtime
or expected behavior is replaced. The
[initial interface errors](parity/graph_governance_5_0_6_source_interface_errors.json)
are preserved. The existing versioned graph-planning observer is reused rather
than overwritten.

## Authoritative readback correction

The initial original/native comparison found **44/704** graph-planning and
**76/356** cases in each audited-save family that disagreed. The remaining four
families matched immediately. See the preserved
[initial differences](parity/graph_governance_5_0_6_initial_gaps.json).

Original fetch_graph_current_row_internal validates every descriptor property
when a row is returned. A sparse ordinary Q result remains legal, but a full
save readback may not infer a loaded NULL from an absent field. C++ previously
accepted the sparse row, allowing some operations to continue into mutation and
event delivery after Rust had already rejected the readback.

The shared GraphCurrentRowOperation now checks a matching ordered shape in one
allocation-free pass, with a name-based fallback for reordered rows or additional
projections. Missing mapped fields produce the authentic graph error in descriptor
order. Explicit NULL and no matching row retain their existing meanings. Public
graph planning uses the same readback operation as graph execution, ledger save
and recursive writes, instead of a second unchecked existence query.

The helper also captures the target ID on its private query scope for free-text
log provenance, including IDs allocated during a write. Existing parent provenance
is copied by the scoped service; the reusable parent is not changed. SQL parameter
values and their ordinary classification remain intact. Safe log projection
consumes provenance and removes the target from comment text before delivery.
The helper's existing module export name is preserved; only its declaration
locator moves to graph_current_row.hpp. Runtime/service/executor/provider
responsibilities and explicit Context ownership remain separate.

## Integration and lifecycle checks

The new contract checks missing-field error order, explicit NULL, reordered and
extra fields, absent rows, real SQLite logging, plain SQL bindings and reusable
parent isolation. Existing pending/resume/cancel, actual Context destruction,
Global plan retention, audited transactions and version-overflow tests remain.

The first scoped checks are retained in
[initial verification](parity/graph_governance_5_0_6_initial_verification.json).
Three older mock fixtures supplied incomplete readback rows; their rows now
contain the declared fields so their original save/commit/overflow assertions
remain reachable. The log check initially inspected provenance after safe
projection had consumed it; it now checks the delivered scrubbed comment and
unchanged binding/policy. The initial native/module test compilation also caught
attempted copies of borrowed move-only Result values; the contract now keeps
the returned references. Failed attempts are not success evidence.

Final native **16/16**, targeted ASan/UBSan **5/5** (leak scan disabled) and
canonical import-only module **1/1** checks pass after terminal builds. All **97**
canonical consumer/fixture bodies verify. The plain-binding SQLite fixture
explicitly declares its audit field policy; undeclared policies remain Unknown,
so they cannot demonstrate a plain binding. See the
[terminal evidence](parity/graph_governance_replay_5_0_6.json). No MIPS, physical
device, external integration or full-suite qualification is inferred.

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family checker --family mutation_policy --family entity_graph_plan \
  --family ledger_move_only --family graph_repository --family runtime_graph \
  --family audited_save \
  --rust-dir /tmp/teaql_cpp_field_layout_rust_506/debug \
  --cpp-dir build_native_current --check
```

Seven active CMake selections now use separate 5.0.6 corpora. Remaining private
graph/save/governance helpers, complete source/configuration matrices and final
integration acceptance remain required; all full capability statuses stay partial.
