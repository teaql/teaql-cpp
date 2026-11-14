# Executed import boundary review for Rust 5.0.6

This review continues the [installed helper repair](executed_helper_validation_candidates_5_0_6.md#installed-continuation).
It records actual current caller behavior and distinguishes imported utilities
from unused historical entry bodies. It does not qualify the complete import
graph, executable strings, SQ02 or G13/G14/G17. Capability status remains
**14 qualified / 3 partial**.

## Source inventory and actual calls

The [primary observation receipt](parity/executed_import_boundary_observations_5_0_6.json.gz)
starts from **615 registered Python entry files** and resolves **686 local
Python files** by static imports. Eighteen imported files contain 267 assertion
nodes. The receipt retains their exact source hashes, assertion locations and
importing callers. Imports inside unused functions are included; dynamic
loading and executable strings require separate review. This inventory is not
a complete execution graph.

Seventeen current registered `verify` or `--source-only` commands run under
ordinary and optimized Python, with a trace observer that records functions
and assertion lines in those eighteen files. All **34 commands pass** and
paired stdout is identical. All **719 input hashes** remain unchanged. The
observer does not rewrite the inspected source or replace acceptance results.
These are direct command observations, not a new CTest/JUnit epoch.

| Source role | Observed or reviewed boundary |
| --- | --- |
| Historical replay, generator and compiler entry bodies: thirteen files / 94 assertions | Imported functions include `versions`, `adapt`, input builders and metadata helpers; other callers import constants such as `FAMILIES`, `RUST_MAIN` and `CPP_MAIN`. Their historical `main` bodies and the unimported configuration `variants` function are outside those declared import paths. Not all thirteen files are executed by the selected seventeen commands. |
| Historical composite assets: one file / 21 assertions | The remaining parent imports `expected_manifest` from the old source verifier. The asset module initializes, but none of its assertion-bearing functions run in the selected commands. Active composite generation/rendering uses the qualified replacement. |
| Historical generated-driver helper: one file / 77 assertions | The inherited-business caller invokes only function recipes through `inputs`; ordinary execution reaches two assertion sites, which optimization removes. The caller independently checks the source/recipe hash, function kind, exact count and complete corpus vector. The earlier ten-process outer-boundary witness checks valid recipes and four corruptions in both modes. This does not accept arbitrary use of the old helper. |
| Historical archive driver: one file / 63 assertions | Several source verifiers call `archive_files` on a freshly reconstructed, locked Git archive and compare its digest and complete file map. Only its member-class assertion runs on those paths. The separately registered SQL mutation command uses the frozen-source `optimize=0` loader described below. Unregistered replay branches are outside this observation. |
| Historical integration controller: one file / ten assertions | Its module initializes, but neither `references` nor its `main` runs in the selected verification commands. Replay and full-epoch execution require their own boundary. The final integration runner is the qualified hardened controller. |
| Fixed mutation input builders: one file / two assertions | The SQL mutation continuation below observes both builders and verifies that the enforced outer comparator rejects truncated vectors even when their internal count assertions disappear. |

The inherited-business trace makes **two fewer `archive.digest` calls** under
optimization because the old helper's repeated hash assertions disappear.
The matching output does not conceal that change: the caller's independent
checks and [ten-process witness](parity/inherited_input_outer_boundary_5_0_6.json.gz)
provide the bounded admission.

## Enforced SQL mutation archive boundary

The [archive continuation receipt](parity/executed_import_archive_boundary_observations_5_0_6.json.gz)
runs the actual registered `parity_sql_mutation_driver_contracts` command in
both modes. Its frozen source is hash checked before compilation with
`optimize=0`. Both runs pass and reach the same **52 archive-driver assertion
sites**, including recipe, dependency, archive and raw-result checks. Direct
optimized execution of the historical driver remains outside this boundary.

The current reconstructed archive contains **610 regular files, 145
directories and no other member classes**. Its SHA-256 is
`d0b0e7c1fec41a352be8375f82629eaea47074998ac7b275a991a285f98aa9c0`.
The two fixed builder count assertions run under ordinary Python and disappear
under optimization; the enclosing complete-vector checks remain active.

Six actual controller cases run in each mode. The valid record passes; wrong
archive hash, missing archive member, substituted helper hash, truncated
mutation recipe and truncated cache recipe all raise `AssertionError` at the
intended outer checks. Hash/count/vector failures are observed from the actual
driver, not inferred from the fixture label. The truncation controls replace
only the helper return value inside their disposable observer process. No
retained helper, source file or corpus is edited.

Both registered command runs and all **twelve valid/invalid controller
processes** pass their expectations, with **722 unchanged inputs**. The
continuation retains the exact wrapper/control sources, commands, optimization
flags, traces and error locations.

## Cleanup and remaining work

The two probes remove **92,135 bytes** of owned observer/trace output after
their children exit. The two compact reports total **81,155 bytes**. No Rust
or C++ compilation, cache eviction or build-directory cleanup occurs.

Continue SQ02 by joining each remaining active dispatcher and executable-string
path to its complete source observations, selection, normalization and result
cardinality. These finite observations resolve specific caller questions;
they do not turn a static import count or matching valid output into full
acceptance. The current registry remains **1,097 tests / 615 Python entries**,
and the last broad native epoch remains the historical 1,075/1,075 run.
