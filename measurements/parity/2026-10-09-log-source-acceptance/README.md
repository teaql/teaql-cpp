# Logging source acceptance, 2026-10-09

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

The complete production bodies of Context logging (391 lines), privacy (410)
and formatter/manager (566) were reconciled with their C++ responsibilities.
See [the API review](../../../docs/runtime-log-api-contract.md) for public
surfaces, defaults, traits, diagnostics, provenance, ordering and ownership.
[results.json](results.json) records counts and the independent full-regression
checkpoint. No production changes followed that successful 375/375 run.

Fresh consumer builds finish successfully before all executions. Native,
ASan/UBSan and MIPS/QEMU each pass 16/16 logging checks; post-test builds report
no work. Sanitizer leak scanning is disabled for this selection. Live Rust
comparisons validate stored observations and actual C++ output for every model,
shared-buffer Clone, projection, Context, formatter and isolated manager input.
MIPS rejects six out-of-range native-width counters before formatting; the
comparator validates this against actual original Rust u32 parsing.

The source-library privacy run passes 39 tests with one ignored child fixture.
The passing `file_log_gate_runs_in_isolated_processes` parent explicitly launches
that child with `--ignored` for unset, invalid and exact acknowledgements. The
child is intentionally not a separately run ordinary test. All four original
formatter tests pass.

Manager comparisons use fresh processes and temporary directories, and compare
stdout/stderr/files. Only volatile Human UTC prefixes and the documented
informational header correction are normalized. The corrected header explains
that `json` selects Debug text; it does not invent structured JSON output.

The formatter comparator takes build/debug directories because it launches
three different probes. An earlier invocation incorrectly passed a single
executable as the directory; it failed before comparison. Correct directory
arguments then completed successfully on both profiles. Accepted logs below
contain only the completed successful runs.

To reproduce the formatter and manager live comparisons after successful builds:

```sh
python3 tests/parity/compare_log_formatter.py --cpp build-root-integration --rust build/rust-original-roundtrip/debug
python3 tests/parity/compare_log_manager.py --cpp build-root-integration/teaql_log_manager_probe --rust build/rust-original-roundtrip/debug/log_manager_oracle
python3 tests/parity/compare_log_formatter.py --cpp build-wndr4300-memory --rust build/rust-original-roundtrip/debug '--cpp-emulator=/usr/bin/qemu-mips;-cpu;74Kf'
python3 tests/parity/compare_log_manager.py --cpp build-wndr4300-memory/teaql_log_manager_probe --rust build/rust-original-roundtrip/debug/log_manager_oracle '--cpp-emulator=/usr/bin/qemu-mips;-cpu;74Kf'
```

Physical WNDR4300 testing is explicitly deferred by the user. These results
accept this source group, not the remaining Context/service/graph/runtime-root
or all migration stages. Generator and dependency scope are unchanged.
