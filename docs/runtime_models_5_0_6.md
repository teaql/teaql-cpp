# Runtime model and lazy mutation binding replay

This checkpoint replays complete historical input families against the locked
Rust 5.0.6 runtime. It does not qualify a complete generator capability, every
model configuration, JSON view coordinate adaptation, or P7.

## Actual source observations

| Family | Observations |
| --- | ---: |
| Entity runtime keys, change sets, state and graph relations | 252 |
| Context defaults and module assembly | 75 |
| Reference identity, claims, principal, keys and errors | 229 |
| Mutation policy models and default providers | 177 |
| SQL/unified log models and log options | 172 |
| Pagination cursors, retained IDs and aggregation cache models | 74 |
| Context roots and fix evidence | 234 |
| Total | 1,213 |

All **1,213/1,213** complete outputs match actual Rust and native C++, without
normalizing diagnostic strings or values. A second source-only run reproduces
the separately versioned corpora. Original input order, historical corpora and
historical drivers remain intact. Three drivers are byte-identical; four only
change shared fixture imports. The log observer includes the unchanged original
event, log privacy and formatter modules through existing test-only fixtures.
See [driver review](parity/runtime_models_5_0_6_driver_review.json).

Context clocks are observed using the original driver predicates and independent
clock brackets. This is not equality of absolute clocks between processes.
Reference model coverage is structural; it does not qualify all token codecs or
authorization flows. Log entries here do not exercise every audit/privacy path.
The unified buffer's separate shared-clone corpus is not part of this family.

## Runtime corrections

The initial comparison found **52/252** entity-runtime differences; the other
six whole families already agreed. The initial
[gap record](parity/runtime_models_5_0_6_initial_gaps.json) remains available.

EntityRuntimeState now delays its mutation-slot allocation until the first
clone or mutation. Cloning an untouched state publishes one empty binding shared
by same-Context siblings; further clones reuse it. The ledger itself still waits
for mutation. Empty reads, diagnostics and clear operations do not allocate the
binding. Independent untouched states remain distinct, while self comparison
is reflexive. Explicit cross-Context copies still use the destination's existing
copy cache and arena; there is no ownership extension or new runtime dependency.

Shared diagnostics now traverse the actual dynamic changes, schema/original
provenance, entity comments, per-entity original snapshots, loaded snapshot
carrier and loaded dynamic fields. A missing metadata field is not replaced with
a constant empty value. Inline/shared snapshot roles reflect the real C++
carrier. Mutex/OnceLock names describe source protocol states; C++ retains its
existing Context ownership and does not acquire Rust synchronization or poisoning.
The zero JSON coordinate describes the ordinary shared graph observed here;
operation-specific JSON graph-domain adaptation remains a separate qualification.

The C++ observer previously cloned before capturing original-state diagnostics.
It now follows the original calls' observation order, preserving the visible
inline-to-shared promotion. This is an observer correction, not normalization.

## Verification

Relevant native checks pass **13/13**, including all seven active source
comparators and entity-runtime model/state, immutable loaded snapshot, dynamic
graph save, entity graph plan and move-only ledger contracts. The added contract
uses a null default PMR resource, checks managed allocation counts, shared first
mutation, and nonempty production dynamic provenance. Existing explicit Global
copies and actual UserContext destruction checks remain in the same canonical
model test. Targeted ASan/UBSan checks pass **3/3** (leak scanning disabled).
The canonical import-only module model consumer passes **1/1**, and **97**
canonical consumer/fixture bodies verify. Builds terminate successfully before
the executables are used. A first native test compile rejected direct
construction of DynamicFieldValues; the fixture now uses its public validating
from_values factory and the corrected build passes.

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family entity_runtime_model --family context_defaults \
  --family reference_model --family policy_model --family log_model \
  --family query_store_model --family context_model \
  --rust-dir /tmp/teaql_cpp_field_layout_rust_506/debug \
  --cpp-dir build_native_current --check
```

These seven active CMake selections now use distinct 5.0.6 comparators. Remaining
historical source families, initialized layout/cache representations, complete
generator configurations and the final whole integration run remain required.
