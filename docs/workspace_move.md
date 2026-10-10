# Rust-shaped C++ workspace move

Scope: M1a of [the module plan](cpp_modules_plan.md), using Rust v5.0.5 at
the exact commit in `upstream.lock.json`. This change organizes existing code;
production named-module interfaces remain M1b work.

The five in-scope Rust crates map to `teaql_core`, `teaql_data_service`,
`teaql_sql`, `teaql_runtime` and `teaql_provider_sqlite`. The C++ arena/lifetime
and asynchronous protocol adaptations reside in `teaql_memory` and `teaql_async`.
The retained SQLite compatibility adapter is isolated in
`compatibility/teaql_legacy_sqlite` and does not enter the production dependency
graph. Each package has its own CMake definition and include directory.

Runtime context methods reside in `teaql_runtime/src/context`; data-service
implementation helpers reside in `teaql_runtime/src/data_service`. Corresponding
private headers are grouped under runtime `detail/context` and
`detail/data_service`. Public header spellings, namespaces, signatures, type
layouts and ownership semantics are preserved. The existing service include
cycle remains within runtime; no import cycle is introduced by directory moves.

Standalone contract tests reside with their responsibility package. Core API
tests with an unambiguous accepted Rust core mapping reside in `teaql_core/tests`,
even when their fixtures need an explicit runtime dependency. Integration tests
that exercise providers reside in `teaql_provider_sqlite/tests`. Shared fixtures
remain single definitions in `tests/support`; the Rust oracles, source-test ports
and comparison harness remain in `tests/parity`. Relative fixture includes have
been adjusted to their actual destinations rather than copied across packages.

Package-local include directories expose previously implicit dependencies.
`teaql_data_service` explicitly publishes `teaql::async`, whose interface target
publishes `teaql::memory`. Contract fixtures explicitly link runtime or provider packages
whose headers they instantiate, rather than relying on the old workspace-wide
include directory. The asynchronous tests also declare their runtime dependency.
The runtime logging test explicitly links the compatibility adapter because it
checks that current and legacy log-entry types remain distinct. These test
dependencies preserve the independent production kernel and provider layers.

Owned scripts, documentation and fixture paths use underscores. Pinned Rust
source IDs, upstream crates, tool download names and historical measurement
directories retain their original spelling. Current documentation links and
the C++ paths in acceptance/test ledgers follow moved files. Raw historical
commands and results remain unchanged.

The [path ledger](module_path_migration.json) records 776 moved files, including
315 production files. It includes pre-move byte hashes, production non-include
content fingerprints and existing Rust responsibility locators. In addition,
all 315 production files match `0.0.1` byte for byte after removing include
directive lines, including function bodies, declarations and comments. Verify the move:

```sh
python3 scripts/verify_module_paths.py
python3 scripts/verify_upstream.py
python3 scripts/verify_public_exports.py
python3 scripts/inventory_upstream.py --check
python3 scripts/verify_source_test_dispositions.py --require-complete
python3 scripts/format_cpp.py
python3 scripts/format_cpp.py --check
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native
ctest --preset native
```

The pre-move fingerprint permits include relocation and whitespace formatting;
the stricter checkpoint fingerprint also requires exact non-include bytes. It is an M1a
checkpoint check, not a permanent ban on later reviewed implementation changes.
The formatter now discovers package directories and `.cppm` files, preserving
the pinned version and generated-table exclusions.
The dependency checker ignores retired objects left by path changes and verifies
compiled dependency records for the current Ninja graph.

M1a validation is complete: the native workspace builds successfully and all
388 CTest contracts pass (71.30 seconds), including the two production examples
and their Rust comparisons. All 427 active compiled objects have nonempty
dependency records; source exports, the 451 test dispositions/registrations,
content fingerprints and pinned formatting checks pass. See
[the evidence](../measurements/modules/2026_10_10_workspace_move/README.md).
ASan/UBSan and MIPS/QEMU were not repeated for this directory-only move; their
profiles remain available under the selective validation policy.
