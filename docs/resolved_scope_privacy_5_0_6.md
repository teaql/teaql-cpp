# Scoped repository privacy snapshots (Rust 5.0.6)

Status: eight direct original-source observations match native and module
consumers. This is focused RQ25/RQ26 evidence; complete `resolved.rs` acceptance
remains pending.

## Original-source boundary

The append-only observer executes `scoped_data_service_internal` and
`query_scoped_service` in the complete locked `data_service/resolved.rs`.
All original bodies are preserved. It varies existing privacy presence, a bound
request intent, and valid versus missing query intent (2 x 2 x 2).

Each case derives two repositories before modifying the parent and first child.
It then creates a query-scoped repository and modifies that repository's privacy.
Observations retain entity names, optional intent and optional privacy snapshots
for parent, child, sibling and query scope. A recording executor observes actual
diagnostic classification order; its SQL and mutation methods reject any call.

The inputs prove these specific behaviors:

- Absent privacy remains absent in an untouched sibling. An initial privacy
  snapshot is inherited, while later additions stay local to each repository.
- Bound request intent is inherited and takes precedence over query text.
  Without bound or valid query intent, the operation rejects before classification.
- Query scope retains the repository entity rather than adopting the diagnostic
  entity, and adds diagnostic privacy to an independent parent snapshot.
- Two diagnostic child enhancements are visited in LIFO order after the root:
  `RootDiagnostic`, `LastChild`, `FirstChild`. No SQL is executed.

These cases do not claim complete facet/relationship/aggregate traversal,
metadata-capture-disabled behavior, trace reset/restore, mutation privacy or
concurrent repository safety. Those responsibilities retain separate evidence.
The synthetic secret strings are test data, not application credentials.

## Evidence

- [Original successful receipt](parity/resolved_scope_privacy_original_v2_5_0_6.json.gz):
  one original test, eight observations, unchanged-source provenance and cleanup.
- [Native receipt](parity/resolved_scope_privacy_native_5_0_6.json.gz): 8/8 match.
- [Module receipt](parity/resolved_scope_privacy_modules_5_0_6.json.gz): 8/8 match.
- Registered `resolved_scope_privacy_rust_parity` CTest: 1/1 pass.
- Canonical module consumer verification: 1,128 bodies pass.

The first Rust fixture failed to import `teaql_core::QueryIntent`; its
[incomplete receipt](parity/resolved_scope_privacy_original_5_0_6.json.gz) is
retained without counting it as a behavior result. The first C++ fixture called
a private member directly; it now uses the existing `EntityDataServiceAccess`
forwarder. Neither correction changes production code or original Rust bodies.

The failed original view used 134,715,277 bytes and the successful view used
160,945,080 bytes; both disposable directories were confirmed absent after the
controllers exited. Stable native/module build directories are retained. No new
sanitizer, MIPS or physical-device result is claimed for these test-only changes.

## Reproduction

```sh
source scripts/toolchains/activate.sh
python3 scripts/observe_resolved_scope_privacy_5_0_6.py --output <fresh-original.json.gz>
cmake --build --preset native --target teaql_resolved_scope_privacy_probe -j 2
ctest --test-dir build_native_current -R '^resolved_scope_privacy_rust_parity$' --output-on-failure
cmake --build --preset modules_native --target teaql_module_resolved_scope_privacy_probe -j 2
python3 tests/parity/compare_resolved_scope_privacy_5_0_6.py --cpp build_modules_native/modules/teaql_module_resolved_scope_privacy_probe
```

The comparator checks original source/observer hashes, exact case identities,
complete observation count and executable identity before reporting equality.
Source disposition remains 112 accepted / 10 pending / 2 excluded_external;
no generator phase or capability is promoted by this focused replay.
