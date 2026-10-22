# Private memory entry points: Rust 5.0.6

Status: focused native/module/sanitizer entry-point qualification.
Complete private-source acceptance remains pending.

## Restored abstractions

The test-only `MemoryDataService` now exposes standalone
`enhance_relation_aggregates`, `fetch_smart_list`,
`fetch_smart_list_with_relation_aggregates` and
`fetch_entities_with_relation_aggregates<T>`. The combined fetch delegates to
standalone enhancement; SmartList and entity wrappers share their conversion
paths. `with_rows` delegates to the explicit `seed` operation. These adapters
preserve the original responsibilities without duplicating query execution.

Metadata lookup in standalone enhancement occurs inside the aggregate loop.
An empty aggregate list therefore succeeds even for an unknown parent entity.
The method changes caller-owned rows in place. If a later aggregate fails,
previous successful changes remain visible, while stored source rows remain
unchanged. Fetching and then enhancing retains its existing error propagation.

## Source observations

The [original receipt](parity/memory_entries_original_5_0_6.json.gz) records
43 observations from appended tests calling byte-preserved Rust 5.0.6 bodies:

- Eight datasets across four list/entity entry points: complete and empty input,
  omitted or invalid typed fields, unknown entity, invalid Boolean filter,
  missing relation and a partial projection. The comparison checks complete
  rows, error category/message, loaded state, total count and empty aggregation,
  summary and facet metadata.
- Ten standalone enhancement cases: empty aggregates, unknown parent,
  missing relation, absent child metadata with no parent keys, later filter or
  metadata errors, and an alias that overwrites the next aggregate's key.
  Both caller rows after success/failure and unchanged stored rows are compared.
- Shared service seeding: replacement rows written through an alias are visible
  through both access paths.

The [native comparison](parity/memory_entries_native_5_0_6.json.gz) matches
43/43, as do the [canonical module](parity/memory_entries_modules_5_0_6.json.gz)
and [ASan/UBSan](parity/memory_entries_sanitize_5_0_6.json.gz) comparisons.
The [execution checkpoint](parity/memory_entries_checkpoint_5_0_6.json.gz) retains
native 4/4 and sanitizer 2/2 CTests plus the module lifetime check. Native
regression includes the original memory callers and the 160 relation cases.
Sanitizer leak scanning is disabled; undefined behavior stops execution.
Canonical verification checks 1,116 bodies; pinned format/check covers 1,784
handwritten files. Module paths and locked-source checks pass.
The original macro-derived Order is compared with the existing handwritten
Order fixture. This checks the wrapper's typed conversion responsibility; it is
not a new full generator-reproduction claim or per-field load-state proof.

## Explicit lifetime adaptation

Rust's service clone shares state through Arc. C++ sharing uses a checked
`ResourceBorrow<MemoryDataService>` bound to the same UserContext. It aliases
state without copying the service or extending its lifetime. A separate
`--lifetime-test` verifies successful live resolution and `ReferenceError::Closed`
after Context close. This C++-only ownership check is distinct from the exact
43-observation semantic comparison. The probe disables the default PMR resource.

## Reproduction and remaining work

```bash
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_memory_entries_probe
ctest --test-dir build_native_current -R '^private_memory_entries_' --output-on-failure
```

The owned original runtime view and Cargo target were removed after execution:
161,055,997 bytes, with cleanup confirmed in the original receipt. Stable native,
module and sanitizer output directories are reused. No MIPS or physical-device
execution is claimed. Production runtime bodies and generated artifacts are
unchanged by this checkpoint.

Recursive evaluator coverage, stable sorting complexity and consolidated source
review remain in the [source checklist](memory_source_upgrade_audit_5_0_6.md#remaining-boundary).
These entry points alone do not close the memory source or any full P0-P7 phase.
