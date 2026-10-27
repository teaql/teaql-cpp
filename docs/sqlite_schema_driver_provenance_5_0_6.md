# Public SQLite schema source provenance, Rust 5.0.6

The unchanged public SQLite schema observer now has an isolated, reproducible
source/corpus/native binding for all **735 original inputs**. This closes one
direct-entry provenance gap. It does not complete generator parity or the
G13/G14/G17 acceptance requirements.

The runtime reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and the
generator reference is `45e1572c4956ca963410d8ea436ead3c8857087a`.

## What executes

The source project at
`tests/parity/rust_sqlite_schema_driver_source_5_0_6/Cargo.toml` points directly
to the existing `rust_values/src/bin/sqlite_schema_oracle.rs` driver. Its source
and the two Rust fixture helpers are unchanged. The driver calls the actual
`SqliteMutationExecutor`, `SqliteProviderExt` and Context `ensure_schema` APIs.
It does not implement a second DDL algorithm.

The runner copies the complete **610-file locked runtime archive** and
**10 project inputs** into its owned temporary directory. Cargo metadata proves
that all six local TeaQL packages resolve inside that archive. The dependency
identities and checksums remain a subset of the reference lock. The build uses
`--offline --locked`, Debug assertions and a private Cargo target directory.
No shared build cache is removed.

The input helper projects only `input` from the original
`observe_sqlite_schema.cases()` recipe. It reads no expected outputs. All inputs
must also equal the retained 5.0.6 corpus. The runner preserves complete raw
source and native stdout, then compares their JSON values with the corpus,
including value types and result cardinality. No output normalization is added.

The existing cases cover table and column creation/reconciliation, type families,
length/precision/nullability checks, quoted and Unicode identifiers, index and
foreign-key metadata, event callbacks, rollback and caller-owned transactions.
The observer records SQLite metadata, query results, callback events and
autocommit state. These finite cases retain their original scope; they do not
establish arbitrary schema migration support.

## Verification

- **735/735** source/corpus/native observations agree.
- **19/19** corrupt-evidence controls are rejected.
- **4/4** affected native CTests pass, including the existing schema unit test,
  schema comparator and the two new source-evidence checks.
- **2/2** retained runtime-bridge checks pass, including their 23 controls.
- The disjoint source registry now covers **301 corpora / 322,731 observations**;
  **4 other direct entries** remain. Registry counts **930/305/195** mean total
  tests/direct entries/custom dispatchers; enumeration is not a full-suite run.

All **1,814 C++ inputs**, producer implementations and **80 generated SDKs /
3,560 files** are preserved in this checkpoint. Capability states remain
**14 qualified / 3 partial**. Complete metadata/configuration review, custom
and protected-governance callers, current full integration and the separate
column-attribute compatibility decision retain their independent requirements.
No sanitizer, MIPS/QEMU or physical-device execution is added here.

## Retained evidence and reproduction

Compact records live in `docs/parity/sqlite_schema_driver_*_5_0_6.json.gz`:
the before snapshot, preparation, complete execution, bindings, controls, gates,
inventory, disjoint partition, guards and final milestone. The task-owned Rust
target/archive and gate scratch directory are removed after their children exit.
The milestone records their measured sizes and retained receipt bytes.

The registry is `tests/parity/sqlite_schema_driver_contracts_5_0_6.json`.
After activating the pinned toolchain and configuring the native preset:

```bash
source scripts/toolchains/activate.sh
cmake --preset native
python3 scripts/generated_driver_provenance_5_0_6.py verify \
  --contracts tests/parity/sqlite_schema_driver_contracts_5_0_6.json \
  --execution docs/parity/sqlite_schema_driver_execution_5_0_6.json.gz
ctest --preset native --output-on-failure \
  -R '^(provider_sqlite_schema_contracts|provider_sqlite_schema_rust_parity|parity_sqlite_schema_driver_(contracts|controls))$'
```

A fresh source replay uses the same script's `replay` command with a fresh
`--record` path. Preserve historical receipts rather than overwriting them.
