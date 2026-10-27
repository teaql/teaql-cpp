# Generated execution and pagination source provenance

The current worklist binds five retained corpora to complete original Rust
execution. References are runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

| Existing comparison | Complete observations |
| --- | ---: |
| Deferred continuous pagination | 640 |
| Deferred ordinary count-first pages | 108 |
| Deferred ID-set pages | 252 |
| Pagination modifiers and composition | 8,448 |
| Generated count, exists, first, one, list and page execution | 1,440 |
| Total | 10,888 |

The [registry](../tests/parity/pagination_driver_contracts_5_0_6.json) names four
unchanged Rust drivers, the complete ordered input recipes and each existing
native command. Pagination modifiers and generated execution intentionally use
the same original driver with separate complete input matrices. These corpora
previously lacked explicit driver fields. Their historical contents and hashes
are preserved; fresh execution establishes the current source binding instead
of adding retrospective driver claims to the old corpora.

The [dedicated source project](../tests/parity/rust_pagination_source_5_0_6/Cargo.toml)
uses the original runtime archive and unchanged generated dynamic-JSON domain
crate. The complete generated source, manifest, model and generator provenance
are fingerprinted and copied into the owned build tree. The runtime archive is
copied in full, without extracted methods or modified algorithms. SQL schema
and data fixtures are copied byte for byte. The existing drivers use actual
in-memory SQLite, rather than an external database service.

The original broad field-layout collector has newer registry dependencies than
the locked runtime. Its manifest and lock remain untouched. The new project
retains the compatible execution-shapes project's dependency graph, with its
own package identity and four explicit targets. Actual Cargo metadata verifies
all local package paths; every registry name/version/source/checksum belongs to
the reference lock. Builds use `--offline --locked --bins`, disabled incremental
compilation and a disposable target directory. These are test dependencies;
the C++ runtime gains no dependency.

The [pagination verifier](../scripts/pagination_driver_provenance_5_0_6.py)
reuses the unchanged archive replay mechanism and supplies this family's input,
generated-fixture, dependency and result checks. The earlier SQL/mutation
verifier and its sealed execution epoch remain unchanged. Routine CTest checks
validate retained execution evidence without recompiling Rust. Each of the five
existing native comparisons requires the new source fixture.

The deferred ordinary-page driver installs a test runtime/executor adapter to
suspend real count and row operations. ID-set and cursor drivers install test
storage implementations to suspend callbacks, resume or cancel, then retry.
They do not replace repository, SQL, scalar-count or decoding algorithms.

Continuous-pagination observations retain the existing bounded projection:
generated cursor IDs are checked for their `cpg_` hexadecimal shape, and absolute
expiry is checked to be in the future. Exact random IDs and timestamps are not
compared. Other callback keys, ordering, boundaries, offsets, SQL, results,
errors and cancellation observations remain exact. The generated execution
driver retains its documented Rust public-inner/re-purpose adapter for modifiers
applied after purpose selection. This does not claim identical staging syntax
between languages.

The [fresh execution](parity/pagination_driver_execution_5_0_6.json.gz) matches
all **10,888** original/corpus/native observations. All 610 archive files and
65 copied input files remain unchanged after execution. The
[binding check](parity/pagination_driver_bindings_5_0_6.json.gz) and
[19 negative controls](parity/pagination_driver_controls_5_0_6.json.gz) pass.
Controls reject missing or substituted sources, generated models, input recipes,
locks, build targets and local dependencies; failed/unlocked/partial builds;
changed input payloads; empty/missing/extra output; and Boolean/number confusion.

The [initial CTest run](parity/pagination_driver_gates_5_0_6.json.gz) retains one
failure: continuous pagination reached its existing 30-second timeout while
the new receipt-mutation checks ran concurrently. The other six tests passed.
An [isolated registered replay](parity/pagination_driver_timeout_control_5_0_6.json.gz)
passed the same continuous comparison in **9.29 seconds**, plus its source fixture.
The new mutation-control test now has `RUN_SERIAL TRUE`, preventing overlap with
timed native probes. Test contents, expected outputs and the original native
timeout are unchanged. The
[final registered gate](parity/pagination_driver_gates_final_5_0_6.json.gz)
passes **7/7** affected tests and **2/2** previous archive-tool checks, without
skips. Its input hashes remain unchanged throughout execution. The earlier
failure is preserved separately.

The [disjoint partition](parity/pagination_driver_partition_5_0_6.json.gz)
now contains **147 source-bound corpora / 166,268 observations** across their
separate execution epochs. **158 other direct registrations** remain. The
[current registry inventory](parity/pagination_driver_inventory_final_5_0_6.json.gz)
has **908 tests / 305 direct bindings / 184 custom dispatchers**. Enumeration
does not establish execution of all 908 tests.

All **1,814 C++ inputs**, producer implementations, retained SDKs and module
exports are unchanged during this cohort. The separate
[column-attribute diagnostic](field_attribute_boundary_5_0_6.md) remains a pending
compatibility decision, not semantic acceptance. Complete metadata, custom
provenance, protected execution and G13/G14/G17/P0-P7 stay open:
**14 qualified / 3 partial**. No fresh full native, imported-module, sanitizer,
MIPS or physical-device acceptance is claimed.

Owned temporary source/build trees were removed after terminal process exit;
the original source replay alone removed **385,592,427 measured bytes**. Commands,
sources, hashes and complete results remain in compact receipts. Stable CMake
profiles and shared download caches are retained.
