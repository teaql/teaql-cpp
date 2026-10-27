# Comparator lifecycle and result completeness

Status: repaired comparison contracts. Thirteen saved-observation mutation
controls pass, and both actual imported consumers pass their stricter existing
checks. Full current native compilation/testing remains in progress. This is
SQ02 evidence, not complete G13 or P0-P7 acceptance.

## Reproduced false passes

Two actual comparators accepted incomplete observation payloads:

- `compare_resource_bindings_5_0_6.py` used `zip` across semantic rounds and
  ownership rounds without requiring equal cardinality. A saved valid semantic
  result with an empty ownership list passed all 384 inputs; an extra ownership
  round also passed. Missing records bypassed destruction/liveness assertions.
- `verify_deferred_resource_replacement_5_0_6.py` compared `result` and `retry`
  only when present. Removing both passed all 143 scenarios with zero business
  comparisons. Adding a successful result to denied/expired scenarios also
  passed, increasing the comparison count from 163 to 234.

These controls run the actual comparators against explicitly mutated saved
observations. They do not execute the current C++ runtime, diagnose a runtime
memory error or replace original Rust behavior. Valid saved observations pass;
corrupted counts/flags fail as independent controls. The original failing
validation behavior is retained in the
[resource cardinality receipt](parity/resource_binding_comparator_cardinality_controls_initial_5_0_6.json.gz)
and [deferred payload receipt](parity/deferred_resource_comparator_payload_controls_initial_5_0_6.json.gz).

## Strict contracts

Resource binding now requires all four round arrays (source/native semantic
rounds and source/native ownership rounds) to be lists of exactly the requested
round count. Existing per-action destructor counts, final destruction totals,
expired get/copy rejection and captured-borrow checks still run for every round.

Deferred-resource output must contain exactly the required business payloads:

| Scenario | Required payloads |
| --- | --- |
| Context close | Neither `result` nor `retry` |
| Resume with a valid executor/registry/store replacement | Both `result` and `retry` |
| Explicit cancel | `retry` only |
| Expired dependency or denied Policy | `retry` only |

All 143 cases therefore require exactly **163** business comparisons. Extra
success payloads fail, as do omissions. Pending/closed/cancelled/denial/expiry
flags retain actual JSON boolean types. The comparison corpus must match the
active `upstream.lock.json` baseline before its results are used.

The stricter checks change no input, source result, C++ implementation, ownership
adaptation or error normalization. Resource replacement still follows the
explicit arena lifetime adaptation instead of inventing mutable Rust actions
through a live borrowed future.

## Verification

`verify_comparator_result_contracts_5_0_6.py` creates a temporary replay executable
and invokes the real comparators. It accepts both valid saved streams and
rejects empty/extra/partial ownership, corrupt destruction/liveness, missing or
forbidden business results, post-close retries and nonboolean/false flags.
The [fixed receipt](parity/comparator_result_contracts_fixed_5_0_6.json.gz)
retains all thirteen process results and the replay source.

The actual `module_resource_bindings_rust_parity` and
`module_deferred_resource_replacement_contracts` checks then pass **2/2** in
9.39 seconds. They execute the real previously completed imported C++ binaries;
the saved-data controls are not counted as native runtime evidence.

```bash
source scripts/toolchains/activate.sh
python3 tests/parity/verify_comparator_result_contracts_5_0_6.py
ctest --preset modules_native --output-on-failure \
    -R '^(module_resource_bindings_rust_parity|module_deferred_resource_replacement_contracts)$'
```

The helper is registered as `parity_comparator_result_contracts` for host CTest
configurations. The running native build predates that registration; after
reconfiguration the expected native registry is **720**, not its original 719.
The initial registry and build-input snapshot are preserved.

## Current optimized-interpreter continuation

The current [62-control replay](parity/comparator_optimized_partial_state_controls_5_0_6.json.gz)
adds nonempty partial-output checks to nine typed/delegated comparator families.
It also runs complete and one-record-prefix outputs under both ordinary Python
and `python -O` for explicit Save, indexed LoadState, generated lookup Context
cleanup, runtime state bridge, generated state bridge and Entity source. Before
the fix, the explicit Save and indexed LoadState comparators returned success
for the one-record prefix under `python -O`, because `assert` was removed and
`zip` compared only the first pair. Explicit checks now preserve cardinality,
source identity and result comparison in optimized mode. The three state/entity
comparators and the lookup check also use unconditional result checks. The
selected five explicit/eval/source and four state/entity/source CTests pass.
No frozen input, expected business result, C++ runtime or generator source is
changed by this continuation. The 160 custom comparator locators still require
their separate complete SQ02/G13 review; this replay qualifies only its named
families and mutation modes.

## Integration boundary

The [exact-delta receipt](parity/comparator_result_contracts_exact_test_deltas_5_0_6.json.gz)
reconstructs the previous three existing files and verifies each reconstruction
against its original pre-build hash. It isolates this patch from the many
pre-existing migration edits. A new Python control helper is the only added
source file. No C++ or generator input body changes.

The original native controller must stop at its unchanged-input check because
these intentional test-only changes are present. This is expected; compilation
continues. After it is terminal, review only these exact deltas, configure the
720-test epoch, require a stable build and run the complete current preset.
Do not overwrite the original input snapshot or call the earlier weak checks
current whole-suite acceptance. The
[input transition record](parity/p7_native_comparator_test_epoch_change_5_0_6.json.gz)
preserves that boundary.

The later [explicit TransactionScope check](transaction_scope_intent_5_0_6.md)
adds one native registration and increases the queued registry to **721**.
Its new consumer may also require work during the controller's second build.
The [current native gate](native_integration_5_0_6.md) preserves the original
epoch and explains both protective stops before full-suite execution.

The [custom-dispatcher inventory](parity/custom_dispatcher_source_inventory_5_0_6.json.gz)
locates 97 distinct comparators behind 128 registrations, including literal
corpus candidates and exact CLI selections. It is a locator, not proof that all
dispatchers, source dependencies or adaptations have been qualified. SQ02
and the remaining requirements stay open. Python-only validation changes need
no new sanitizer or MIPS run.
