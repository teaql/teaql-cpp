# Public SmartList aggregate diagnostic scope (Rust 5.0.6)

Status: three original/native differences reproduced; repaired native comparison
passes 8/8. The module comparison also passes 8/8; repaired joined native and
module gates each pass 31/31.

## Reproduced wrapper gap

The original `fetch_smart_list_with_relation_aggregates` unconditionally calls
`query_scoped_service_with_aggregates` using the diagnostic source when present,
otherwise the executable query. Separate aggregate arguments contribute to that
scope before preparation. The supplied query and aggregate arguments remain
separate from executable relation selections.

C++ previously delegated through `fetch_all_with_relation_aggregates` and then
converted the rows into a SmartList. That raw-row wrapper creates a diagnostic
scope only when a diagnostic source exists. Its later row operation skips scope
creation when intent is already bound. Consequently:

- Bound intent without a diagnostic source skipped classification altogether.
- A diagnostic source classified the parent but omitted the separate aggregate.
- Capture-disabled cases and the ordinary unbound case already matched.

The [pre-repair receipt](parity/resolved_public_aggregate_scope_native_before_5_0_6.json.gz)
retains all three differences (5/8 matches). The actual original observer varies
bound intent, explicit diagnostic source and metadata capture. The diagnostic
source is produced by the real `for_exact_count` transformation.

All eight cases intentionally lack entity metadata and ultimately return the
same missing-descriptor error. This isolates the classification that must happen
before preparation rejects. It does not establish successful provider execution,
physical SQL logs or complete aggregate persistence. A recording classifier
observes the real wrapper call sequence and the reusable parent's unchanged
privacy/intent; no fabricated SQL is used as evidence.

## Shared correction

The SmartList aggregate wrapper now uses the existing
`execute_scoped_purposed_query` operation, as the typed enhanced aggregate entry
already does. Its execution callback delegates to the existing aggregate row
operation and row-to-SmartList conversion. The raw-row extension keeps its
separate behavior. No generator output, public signature, allocation domain,
provider layer or underlying cleanup algorithm changes.

The [repaired native receipt](parity/resolved_public_aggregate_scope_native_repaired_5_0_6.json.gz)
records 8/8 matches. The [original receipt](parity/resolved_public_aggregate_scope_original_5_0_6.json.gz)
binds unchanged original bodies, observer hash and one passing test. Its
161,482,839-byte disposable build/source view was removed after completion.
Two initial C++ fixture compile attempts incorrectly treated the in-place
`for_exact_count` method as returning an assignable value; correcting fixture use
produced the pre-repair observations. Those compile failures are not behavior
results.

## Verification

```sh
source scripts/toolchains/activate.sh
python3 scripts/observe_resolved_public_aggregate_scope_5_0_6.py --output <fresh-original.json.gz>
cmake --build --preset native --target teaql_resolved_public_aggregate_scope_probe -j 2
python3 tests/parity/compare_resolved_public_aggregate_scope_5_0_6.py --cpp build_native_current/teaql_resolved_public_aggregate_scope_probe
python3 scripts/verify_resolved_native_closure_5_0_6.py --output <fresh-native-gate.json.gz>
```

The joined native gate builds 16 targets and requires an exact 31-test selection,
terminal successful CTest/JUnit results and unchanged source inputs. It covers
preparation, rows, schemas, typed/dynamic reads, streaming, cursor/membership,
mutation, scoped privacy and complete enhancement. The module gate runs the same
commands with canonical imported executables. The joined gates consolidate
existing witnesses; they do not by themselves close all 75 source definitions
or P0-P7. Source disposition remains pending_review.

The [repaired module receipt](parity/resolved_public_aggregate_scope_modules_repaired_5_0_6.json.gz)
records 8/8 matching observations. Pinned formatting/check passes for 1,795
handwritten C++ files. No fresh sanitizer or MIPS verdict is claimed.

The first joined native run passed 29/31 and exposed an independent ordinary-row
inline-aggregate shortcut regression. The [joined regression record](resolved_joined_regression_5_0_6.md)
retains that failure and G09 reopening. After restoring the source shortcut,
the current native union passes 31/31; this includes the public SmartList scope
observer against the final shared header. The corresponding module union also passes 31/31.
