# Layered mutation and transactional saver replay

This checkpoint continues the locked private-source observer from
[private_save_replay_5_0_6.md](private_save_replay_5_0_6.md). Original bytes of
610 archive files remain intact, access bridges are appended, and registry
versions/checksums retain the original runtime lock. It does not qualify all
generated/model configurations, durable dynamic paths or P7.

## Actual source observations

| Family | Historical inputs |
| --- | ---: |
| Base/Context/entity/prepared mutation entry points | 1,502 |
| Registered transaction-owned graph/ledger saver | 960 |
| Automatic audited save | 944 |
| TransactionScope callbacks and terminal behavior | 1,288 |
| Move-only transaction capture/result destruction | 16 |
| Total | 4,710 |

The new versioned corpora preserve all historical inputs in their original
order. All **4,710/4,710** complete actual Rust/native outputs match; source-only
reruns reproduce the actual Rust outputs. The
[source provenance](parity/mutation_saver_5_0_6_source_provenance.json) records the
completed build and original registry lock; the
[driver review](parity/mutation_saver_5_0_6_driver_review.json) records original
and adapted observer hashes. Original clock projections are retained, with no
additional output normalization.

The registered saver bridge delegates to the actual erased DynGraphSaver.
Transaction fixtures implement only executor/provider/sink SPI behavior and
observe begin, write, event, read, commit, rollback and active-transaction Drop.
The move-only family uses the unchanged non-Clone path of the same Rust
transaction observer and its separately named C++ probe. No C++ production
dependency or generated artifact is added or changed.

## Preserving missing request intent in old fixtures

Three old transaction observer expressions no longer compile: QueryRequest has
an intent instead of a comment field, and MutationRequest is a validated command
envelope. The [initial interface errors](parity/mutation_saver_5_0_6_source_interface_errors.json)
are preserved. Versioned observers now call actual fallible constructors and
propagate their typed errors through the original transaction callback.

These historical raw query/mutation stimuli provide no intent. The observer
does not invent a valid comment or purpose to make them execute. C++ previously
added an implicit "transaction scope command" in the mutation observer and used
a throwing query constructor. It now preserves absence with the corresponding
fallible constructors. Existing successful save/transaction and non-Clone
stimuli remain; this corpus alone does not qualify successful explicitly
purposed raw TransactionScope queries or mutations. Those positive contracts
require separate evidence.

## Internal preparation responsibility

The initial comparison found **350/1,502** layered mutation differences. The
remaining four complete families already matched. See the retained
[initial differences](parity/mutation_saver_5_0_6_initial_gaps.json).

Rust internal insert/update helpers run preparation before calling the physical
mutation layer. Delete/recover run their behavior/policy callbacks before that
layer. C++ had an earlier intent rejection that erased ID allocation, fixes and
callback effects, and sometimes returned a different error before missing
metadata or a failing behavior could be observed.

The redundant early rejection is removed. Shared physical request validation
still rejects absent intent before provider work, cache invalidation or event
delivery. The internal helper now preserves source preparation order and the
distinction between entity, Context and base runtime responsibilities. Public
entry-point intent boundaries are unchanged.

The added native regression observes behavior, ID generation and Checker
ordering, advances the ID counter exactly once, and asserts no physical request
or event after rejection. The caller's command snapshot is unchanged. Existing
reentrant cleanup, explicit Global retention, transaction ownership and
pending/cancel paths remain in the relevant canonical contracts.

## Reproduction and remaining gates

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family runtime_mutation --family graph_saver --family audited_auto_save \
  --family transaction_scope \
  --driver-dir tests/parity/rust_private_save_5_0_6/src/bin \
  --rust-dir /tmp/teaql_cpp_private_save_rust_506/debug \
  --cpp-dir build_native_current --check
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family transaction_move_only --observer-family transaction_scope \
  --cpp-observer-family transaction_move_only \
  --driver-dir tests/parity/rust_private_save_5_0_6/src/bin \
  --rust-dir /tmp/teaql_cpp_private_save_rust_506/debug \
  --cpp-dir build_native_current --check
```

Relevant native checks pass **12/12**, including preparation/event reentrancy.
Targeted ASan/UBSan passes **4/4** (leak scan disabled), and the canonical
import-only mutation consumer passes **1/1**. All **98** consumer/fixture bodies
verify; **993** handwritten files pass pinned formatting/check. Compiler/linker
and test processes terminate before success is recorded. The first native
selection retained **11/12** passes in
[initial verification](parity/mutation_saver_5_0_6_initial_verification.json):
a non-ledger mock readback supplied only id, so its declared version/name are
now included and the original non-ledger entity assertion remains reachable.
See the [terminal record](parity/mutation_saver_replay_5_0_6.json). Five active CMake
selections use separately versioned 5.0.6 corpora; historical evidence remains
intact. Complete source/configuration matrices and final integration acceptance
remain required. All full capability statuses stay partial.
