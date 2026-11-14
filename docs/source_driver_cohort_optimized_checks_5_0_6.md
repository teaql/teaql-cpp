# Seven source validators under optimized Python

This SQ02/G13 repair covers seven active source validators, their seven control
entries and two imported recipe helpers. References remain runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`, the locked Rust 5.0.6 baseline.
Complete SQ02, G13/G14/G17 and P0-P7 retain their other requirements.

The [pre-edit execution](parity/source_driver_cohort_optimized_preedit_5_0_6.json.gz)
reproduces seven false acceptances. Each historical verifier accepts its valid
saved execution under ordinary Python and rejects a copy whose only change is
`complete=false`. Under `python -O`, all seven incorrectly accept that incomplete
copy. These are actual saved-evidence verification runs; no new Rust or C++
observations are inferred from them.

| Source validator | Replaced direct checks |
| --- | ---: |
| [Runtime protocol and debug/release profiles](../scripts/runtime_protocol_driver_provenance_hardened_5_0_6.py) | 87 |
| [Runtime access bridges](../scripts/runtime_bridge_driver_provenance_hardened_5_0_6.py) | 84 |
| [Pagination](../scripts/pagination_driver_provenance_hardened_5_0_6.py) | 68 |
| [Query stores and Context supplement](../scripts/verify_query_store_driver_contracts_hardened_5_0_6.py) | 67 |
| [Registry and local dependency provenance](../scripts/verify_registry_driver_contracts_hardened_5_0_6.py) | 61 |
| [Private runtime](../scripts/verify_private_runtime_driver_contracts_hardened_5_0_6.py) | 57 |
| [Generated entity API](../scripts/verify_entity_api_driver_contracts_hardened_5_0_6.py) | 48 |
| Seven existing control entries | 23 |
| Two imported recipe helpers | 75 |
| Total | 570 |

The [imported-helper witness](parity/source_driver_cohort_helper_preedit_5_0_6.json.gz)
also reproduces two optimized-mode false acceptances: an archive containing a
symbolic-link member and an observer source missing required adaptation
markers. The checks run entirely in memory or in a task-owned temporary tree;
they do not extract the archive or change repository sources.

Sixteen active source copies now use unconditional `if`/`AssertionError`
checks. The [transition manifest](parity/source_driver_cohort_hardened_transition_5_0_6.json.gz)
contains all complete original, assertion-only and active sources. Its guard
requires exact AST equivalence for every assertion replacement, followed by
exactly 25 declared import, historical-path or replay-lifecycle edits.
Historical source bytes, registries, original executions and corpora remain
hash-bound to their original paths. The CTest source/control registrations now
use the active copies, including the two hardened helpers on those call paths.

The active validators check the historical self/helper hashes recorded by the
original executions. Five optimized replay entry points refuse execution before
creating an output, because shared build recipes still require ordinary Python.
This does not establish a fresh archive build or source execution under the
new replay scripts.

The [permanent guard](../tests/parity/verify_source_driver_cohort_hardened_5_0_6.py)
checks every source transition, immutable contract/execution hash and current
CTest command. Five source or registration corruptions reject. For every one of
the seven validators, valid evidence passes and the incomplete copy rejects
under both Python modes. Both invalid imported-helper cases also reject in
both modes. Its [control receipt](parity/source_driver_cohort_hardened_controls_5_0_6.json.gz)
records these boundaries, all replay refusals and removal of task-owned
temporary evidence.

The [selected native receipt](parity/source_driver_cohort_selected_native_5_0_6.json.gz)
retains exact CTest/JUnit membership, outputs and unchanged source/evidence
hashes. The ordinary run passes **15/15** tests: seven source contracts, seven
existing negative-control suites and the permanent source guard. The
`PYTHONOPTIMIZE=1` run also passes **15/15**, with exact JUnit/registry names
and zero errors, failures or skips. The existing suites reject **180** corrupted
records and **two** actual legacy locks in each mode. All **52** source/evidence
hashes remain unchanged. The owned JUnit directory is removed after both CTest
processes terminate; compact logs and XML remain in the receipt.

The current registry contains **1,089** tests and **607** distinct Python entry
scripts; **63** retain **185** direct `ast.Assert` nodes. Imported helpers and
the remaining entry points still require review. These inventories do not
qualify complete SQ02 or all optimized Python paths. No full 1,089-test native
epoch has run. The project guidance now requires unconditional Python
acceptance checks and positive/negative verification in both Python modes.
