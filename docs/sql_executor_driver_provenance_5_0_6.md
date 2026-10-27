# SQL executor driver source provenance

Status: all nine matrices in the existing SQL executor source cohort now have
complete fresh source/native output bindings, covering **8,243 observations**.
The cumulative bounded scope is **58 direct corpora / 75,504 observations**.
G13/G14/G17 and P0-P7 remain open; verdicts stay **14 qualified / 3 partial**.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Complete executor matrices and original tests

The [registry](../tests/parity/sql_executor_driver_contracts_5_0_6.json) names the
actual source targets and native comparisons from the existing complete executor
review. It preserves runtime, executor, transport and provider responsibilities.

| Matrix | Complete observations |
| --- | ---: |
| Dynamic stream | 720 |
| Diagnostic guards | 352 |
| Query protocol | 767 |
| Stream protocol | 1,765 |
| Execution protocol | 2,029 |
| Transaction protocol | 2,029 |
| Executor kind | 93 |
| Transport | 344 |
| Executor errors | 144 |
| Total | 8,243 |

The [final execution](parity/sql_executor_driver_execution_final_5_0_6.json.gz)
matches every original result to the existing corpus and current native output.
It also executes **50/50** original SQL tests: executor16, query diagnostics6,
mutation diagnostics21 and diagnostic stream7. No input, expected value or
embedded provenance label changes.

The [replay driver](../scripts/replay_sql_executor_drivers_5_0_6.py) uses the
existing [byte-preserved SQL view](../scripts/reference_sql_view_5_0_6.py).
Original source/test/vector bodies remain unchanged; only the existing diagnostic
observer is appended. The observer adds `serde_json=1.0.150` from the reference
lock, with no original dependency omitted. The source project changes only path
locations and the SQL dependency path to this checked view.

Three copied helper files preserve the existing `CARGO_MANIFEST_DIR` include
layout byte for byte. Compiled source targets, original/prepared manifests,
original/prepared locks, metadata, raw stdout, payload hashes and binary hashes
are retained. All registry identities agree with the reference lock. One owned
target is used, incremental compilation disabled and debug information reduced;
debug assertions remain active. The clean reference and all 466 tracked
Rust/manifests/lock files are checked before and after replay.

## Corrected test-result collection

The [first controller failure](parity/sql_executor_driver_execution_5_0_6.json.gz)
already matches all nine source/native matrices and successfully runs the first
16-test group, but an over-escaped regular expression rejects its names. The
[exact prior source](parity/sql_executor_driver_before_test_parser_fix_5_0_6.json.gz)
is preserved. The [second attempt](parity/sql_executor_driver_execution_repaired_5_0_6.json.gz)
exposes a second issue: one successful test writes diagnostics between the test
name and its trailing `ok`, so an end-of-line matcher captures only 15 names.

The [interleaving witness](parity/sql_executor_driver_before_interleaving_fix_5_0_6.json.gz)
retains the actual 15/16 false rejection and exact controller bytes. Collection
now checks the actual test-name prefixes together with process success and the
complete group success summary. The final fresh run passes all four groups.
The permanent validator consumes this same actual interleaved stdout, retaining
the regression. Earlier attempts remain failed; no failed record is relabeled.

## Permanent source controls and native acceptance

The [validator](../scripts/verify_sql_executor_driver_contracts_5_0_6.py) checks
the full SQL-view source footprint, exact appended observer, metadata-derived
manifest, all copied includes, prepared project/dependencies, all50 test names
and summaries, full matrix cardinality, strict JSON types and every raw original/
native observation. It does not build or execute Rust. Existing comparisons
execute current C++ binaries rather than freezing archived binary identities.

[Twenty-four controls](parity/sql_executor_driver_controls_5_0_6.json.gz) reject
22 effective corruptions and both actual early dependency locks. SQL-specific
controls alter original source prefixes, self-consistently rehash the observer,
omit a copied include or remove one original test responsibility. Common controls
cover baselines, manifests/locks, actual targets/dependencies, payloads, missing/
duplicate rows, omitted members and boolean/integer substitution. Untouched
bindings pass first.

Permanent host checks `parity_sql_executor_driver_contracts` and
`parity_sql_executor_driver_controls` preserve this scope. All nine comparisons
require their source fixture. The [native gate](parity/sql_executor_driver_gates_5_0_6.json.gz)
passes **21/21**: nine comparisons, ten existing SQL/lifetime/transport contract
programs and two source checks. JUnit names are exact, with no failures, errors or
skips. Focused query/stream selection automatically runs the source fixture first
(**3/3**). The [dispatcher replay](parity/sql_executor_driver_dispatcher_5_0_6.json.gz)
executes the actual provenance invariant without fabricated probe substitution.

## Remaining scope and cleanup

The [locator](parity/sql_executor_driver_inventory_5_0_6.json.gz) records 893 native
tests, 305 direct active-baseline entries and 177 custom registrations requiring
full review. Cumulative source binding leaves **247 direct entries** and **77 of
107 declared-driver entries** to reconcile. Eight of this cohort's nine corpora
belong to that declared-driver worklist; transport uses a different metadata form.
These overlapping worklists do not narrow complete dependency/observer/
normalization, metadata/configuration or whole current integration obligations.

The [preceding snapshot](parity/sql_executor_driver_before_registration_5_0_6.json.gz)
preserves the exact native configuration, registration, dispatcher and documents.
Production C++, generator/SDKs, dependencies and module exports retain their
bodies. The final replay removes **139,309,461 bytes** of owned outputs after
compiler/linker and probe completion. Both failed replay directories were also
removed; their sizes were not captured before the exception cleanup. The
[cleanup record](parity/sql_executor_driver_cleanup_5_0_6.json.gz) lists these
and temporary gate/inventory directories explicitly. Shared targets are not
used or evicted; stable presets and full-output evidence remain.

The [milestone](parity/sql_executor_driver_milestone_5_0_6.json.gz) binds only this
complete finite cohort. Routine source checks invoke neither Rust compilation
nor Java, sanitizer, MIPS or physical-device profiles.
