# Private save preparation and execution replay

This checkpoint qualifies historical inputs through actual locked Rust 5.0.6
private method bodies. It does not qualify a complete generator capability,
all model configurations, durable dynamic readbacks or P7.

## Source build and observer contract

The development-only builder extracts the exact locked git archive into the
ignored build_rust_private_save_5_0_6 directory. It verifies original bytes of
all **610** files. Five source files receive appended access bridges; original
method bodies remain byte-identical prefixes. Bridges delegate to actual
private methods. The graph-fix fixture installs a delegated DynGraphSaver hook
to observe original save/fix scope handling; it is not a persistence backend.
The frozen .reference/teaql-rs worktree remains clean.

Registry package versions and checksums must be a subset of the original
runtime Cargo.lock. Initial fresh lock generation chose newer transitive
versions; that build is not used for final semantic evidence. The committed
observer lock instead preserves the original versions. Subsequent builds use
--locked --offline and reject registry differences before compilation.
See [source/build provenance](parity/private_save_5_0_6_source_provenance.json).
These Rust dependencies remain development tools, not C++ runtime additions.

Six independently named drivers retain the original calls. Four change only
the shared fixture import. The graph/ledger execution observers inspect the
5.0.6 MutationRequest command envelope. A prepared UpdateCommand is inspected
directly, without inventing an intent or executing a request just to display it.
The initial compile error is preserved in
[interface evidence](parity/private_save_5_0_6_source_interface_errors.json).
The [driver review](parity/private_save_5_0_6_driver_review.json) records exact
historical and active source hashes and permitted adaptations.

The old ledger observer returns generated IDs. Its bridge extracts those IDs
from the actual LedgerExecutionOutcome and asserts that dynamic_readbacks is
absent. It cannot silently discard the new payload or qualify durable dynamic
execution. All historical stimuli in this checkpoint satisfy that assertion.

## Actual original/native comparisons

| Family | Historical inputs |
| --- | ---: |
| Fix scopes, required-create checks and save future publication | 1,597 |
| Save extraction, preflight, merge, hydration and locations | 1,719 |
| Ledger dependency ordering | 1,362 |
| Graph and ledger governance plan projections | 2,129 |
| Graph execution, save and current-row reads | 520 |
| Reference/remove/update/delete node commands | 1,620 |
| Entity/core graph save adapters | 541 |
| Recursive graph writes | 792 |
| Ledger execution | 760 |
| Total | 11,040 |

All **11,040/11,040** complete outputs match actual Rust and native C++.
Separate source reruns reproduce the versioned corpora. Historical input order,
drivers and corpora remain unchanged. Original clock predicates/markers are
retained; no additional output normalization is applied. The shared graph
execution driver is used for four families exactly as in the original fixtures.
The replay tool records the actual observer path rather than pretending there
is an independently compiled driver for every corpus.

## Request-intent validation order

The execution replay initially exposed **1/520** differences: a plan with no
root returned a structural graph error in C++, while original Rust first
rejected its absent mutation comment. The preserved
[initial execution differences](parity/private_save_execution_5_0_6_initial_gaps.json)
also record zero differences in the other four newly replayed execution families.

GraphExecutionOperation now derives and validates the plan's own mutation intent
before inspecting root structure, matching execute_graph_plan_internal. A valid
reusable parent comment cannot replace an absent plan-owned intent. The regression
checks both parent states with no metadata and asserts that no provider/query
work is performed. Existing runtime/service/executor/provider layers, Context
ownership and lifecycle mechanisms remain intact.

## Verification and remaining work

Relevant native checks pass **17/17**: nine active source comparisons plus fix,
preparation, dependency ordering, node/adapters, graph execution, recursive write
and ledger contracts. The canonical import-only graph execution consumer passes
**1/1** after terminal compilation/linking; **98** unchanged consumer/fixture
bodies verify. See the [terminal record](parity/private_save_replay_5_0_6.json). The pinned format/check covers
**993** handwritten C++ files. This validation-order correction introduces no
new lifetime mechanism; full sanitizer, MIPS and physical-device acceptance are
not inferred from this checkpoint.

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family graph_fix --family save_preparation --family ledger_plan \
  --family policy_plan \
  --driver-dir tests/parity/rust_private_save_5_0_6/src/bin \
  --rust-dir /tmp/teaql_cpp_private_save_rust_506/debug \
  --cpp-dir build_native_current --check
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family graph_execution --family graph_node_command \
  --family graph_save_adapter --family recursive_graph_write \
  --observer-family graph_execution \
  --driver-dir tests/parity/rust_private_save_5_0_6/src/bin \
  --rust-dir /tmp/teaql_cpp_private_save_rust_506/debug \
  --cpp-dir build_native_current --check
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family ledger_execution \
  --driver-dir tests/parity/rust_private_save_5_0_6/src/bin \
  --rust-dir /tmp/teaql_cpp_private_save_rust_506/debug \
  --cpp-dir build_native_current --check
```

Nine active CMake selections now use separate 5.0.6 corpora. Runtime mutation
layer adapters, transaction saver orchestration, complete source/configuration
matrices and final integration acceptance remain required. All full capability
statuses remain partial.
