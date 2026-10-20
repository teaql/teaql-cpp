# SQLite baseline qualification against Rust 5.0.6

This checkpoint advances P7 integration and the SQLite decoding prerequisite of
G13/G16. It does not complete these capabilities or any P0-P7 phase.

## Locked source and preserved inputs

The runtime reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, verified by
`scripts/verify_upstream.py`. The independent development observer manifest is
`tests/parity/rust_sqlite_baselines_5_0_6/Cargo.toml`. It compiles the seven
historical driver bodies unchanged against that reference. The direct provider
dependencies match the original lock: rusqlite 0.40.1 and libsqlite3-sys 0.38.1.
Rust and these observer dependencies are not C++ production dependencies.

Every historical input remains in its original order. Historical corpora and
driver files remain byte-identical to HEAD. New profile-specific observations,
manifest/lock fingerprints and wrapper/helper fingerprints are separate files.
Ordinary successful and error results are compared exactly, with no text, field,
number-kind or negative-zero normalization.

| Corpus family | Historical inputs | Changed checked-source outcomes |
| --- | ---: | ---: |
| Physical schema | 735 | 0 |
| Row decoding | 6,029 | 10 |
| Binding/storage | 1,084 | 0 |
| Provider error model | 563 | 0 |
| Native diagnostics | 5,810 | 0 |
| Scalar/text/unit errors | 62 | 0 |
| Converted errors | 36 | 0 |
| Raw converted Debug | 36 | 0 |
| Total | 14,355 | 10 |

Raw converted Debug remains a native 64-bit observation because its sentinel
formats depend on pointer width. It is not a 32-bit corpus.

## Checked timestamp failure and evaluation order

The new original fast timestamp parser uses `then_some` in its digit and offset
helpers. Its argument is evaluated before the digit predicate selects `None`.
With checked arithmetic, subtracting ASCII `0` from a byte below it panics.
The unchanged original decoding driver therefore exits 101 at historical input
5429 instead of producing the old ordinary binding error. The input represents
`2024-02-29 01:-2:03.123+08:00`.

The [unchanged-driver failure](parity/sqlite_timestamp_checked_source_failure_5_0_6.json)
retains the exact input, exit code and raw original stderr. The original source
and historical driver are not edited to suppress the failure.

Thin development wrappers expose only this exact panic/exception signature as
`checked_failure: arithmetic_overflow` with its original message. Unexpected
Rust panics resume unwinding; unrelated C++ exceptions escape. The structured
failure tag is an explicit observer protocol adaptation, not a claim that raw
Rust panic text and C++ exception rendering are identical. Raw original panic
diagnostics are [retained separately](parity/sqlite_baselines_5_0_6_runtime_diagnostics.json.gz).

C++ fast decoding now preserves the original short-circuit order: calendar
validation precedes clock digits; clock validation precedes offset pairs; an
invalid hour pair prevents minute-pair evaluation. Debug builds throw
`std::out_of_range` for the checked subtraction boundary. Builds with `NDEBUG`
retain ordinary fallback parsing for malformed digits instead. No raw unsigned
underflow is introduced into C++.

The observer additionally compares 64 actual public-fetch controls covering all
fixed digit positions, malformed fractions, invalid calendars/clocks and offset
pair evaluation. Nineteen checked controls expose the subtraction failure. The
focused provider test also checks valid timestamps and early return before a
later malformed digit. Its existing row-lifetime, default-PMR-disabled and
physical-type contracts remain active.

## Execution profiles and evidence

`sqlite_decoding_checked_probe --profile` reports the compiled C++ arithmetic
profile. Its comparator selects checked or unchecked source observations and
rejects a mismatched corpus profile. This keeps Debug-only panic expectations
out of the Release path. A [negative control](parity/sqlite_decoding_profile_negative_control_5_0_6.json)
rejects the actual unchecked Rust executable against the checked C++ corpus.

The [checked replay record](parity/sqlite_baselines_5_0_6.json) records matching
14,355 outcomes and the 64 additional controls. Native tests pass 9/9, canonical module
tests pass 9/9 and targeted ASan/UBSan tests pass 2/2. The same full matrix and
controls reproduce through the actual original executable and module consumers;
the 64 controls also pass under ASan/UBSan. Sanitizer leak scanning is disabled.
There are 452 verified canonical consumer/fixture bodies and 1,479 catalog exports.
Pinned format/check covers 1,351 handwritten C++ files.

The [unchecked replay record](parity/sqlite_baselines_unchecked_5_0_6.json) separately
qualifies Release Rust and Release C++. All 14,355 outcomes and 64 controls match;
none of these unchecked source outcomes changed from the historical corpus.
Release CTest passes 9/9. The [terminal record](parity/sqlite_baselines_5_0_6_terminal.json)
retains each profile's scope and evidence. This checkpoint makes no MIPS/QEMU or
physical-device execution claim.

The fresh native registry has 583 tests. Its
[static corpus inventory](parity/active_corpus_baseline_after_sqlite_baselines_5_0_6.json)
finds 182 direct active bindings, 41 historical direct bindings and 60 custom
dispatchers requiring independent review. The decoding dispatcher is custom
because it selects a corpus by arithmetic profile. These are static counts, not
runtime acceptance totals.

## Reproduction

Build the seven observer targets and `teaql_provider_sqlite_decoding_tests` before
running the replay. Use the pinned tools and wait for each build to finish.

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_sqlite_baselines_5_0_6.py
ctest --preset native --output-on-failure \
  -R '^(provider_sqlite_decoding_contracts|provider_sqlite_schema_rust_parity|sqlite_(decoding|bindings|error_model|native_diagnostics|simple_error|from_sql_error|from_sql_raw_debug)_rust_parity)$'
python3 scripts/verify_sqlite_baselines_5_0_6.py \
  --cpp-prefix teaql_module_ --build-dir build_modules_native/modules
ctest --preset modules_native --output-on-failure \
  -R '^module_(provider_sqlite_decoding_contracts|sqlite_(schema|decoding|bindings|error_model|native_diagnostics|simple_error|from_sql_error|from_sql_raw_debug)_rust_parity)$'
ctest --preset sanitize --output-on-failure \
  -R '^(provider_sqlite_decoding_contracts|sqlite_decoding_rust_parity)$'
```

An unchecked replay uses `--profile unchecked --build-dir PATH` with the completed
Release C++ targets. `--record` writes new corpora only when the complete declared
comparison passes; ordinary replay verifies committed source observations.

Five test source paths previously depended on putting the build directory inside
the repository. They now resolve against `CMAKE_CURRENT_SOURCE_DIR`, allowing the
independent external Release build without moving production sources or changing
function behavior.

## Remaining integration

Eight failures from the original 549-test native integration have successful
selected follow-ups. Together with earlier follow-ups, 51 recorded baseline
failures remain unresolved. The original 472/549 result remains historical and
failing; the fresh complete suite has not been rerun or accepted. The selected
SQLite families do not prove all generated configurations, dynamic storage,
governance, privacy, lifecycle, provider composition or P0-P7 exit criteria.
