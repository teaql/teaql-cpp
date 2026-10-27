# In-memory CLI source binding, Rust 5.0.6

The historical in-memory comparison now binds its actual CTest command to the
previously accepted complete Rust 5.0.6 source replay. That replay contains
**8,732 original observations** across two matrices and 19 original runtime
tests. The historical CLI consumes the **6,532-case** matrix; a fresh current
C++ run matches all **6,532** results. Selected installed native checks pass
**5/5**. This reuses the accepted original execution and does not claim a new
Cargo or imported-module run.

The locked runtime is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; the
generator is `45e1572c4956ca963410d8ea436ead3c8857087a`.

The [complete earlier source review](inmemory_source_acceptance_5_0_6.md)
qualifies the engine/evaluator source, nineteen original tests, 6,532 retained
query/evaluator inputs and 2,200 additional pattern and metadata inputs. The
[current binding contract](../tests/parity/inmemory_cli_source_contracts_5_0_6.json)
pins that execution and terminal acceptance receipt. It checks all seventeen
original/source and C++ implementation snapshots against current files, the
locked references, both complete corpora and the raw original JSONL streams.
The prior temporary Rust executables were removed after their original run;
their raw results remain in the retained receipt.

The [before](parity/inmemory_cli_admission_before_5_0_6.json.gz) and
[installed](parity/inmemory_cli_admission_installed_5_0_6.json.gz) controlled
CLI replays reproduce **three false acceptances**: an extra retained-case
field and short or long native result streams. The installed comparator now
requires the exact 6,532 case rows and complete native result count. Its
comparison and pointer-width logic remain intact. The
[preservation checker](../tests/parity/verify_inmemory_cli_preservation_5_0_6.py)
recovers the preceding comparator and CMake registration byte for byte.
Controlled replay output is not original or native execution evidence.

The [source controls](parity/inmemory_cli_source_controls_5_0_6.json.gz)
reject **18/18** substitutions in the source receipt, raw outputs, case rows,
current source identities and CTest command/fixture. The
[fresh native binding](parity/inmemory_cli_source_native_5_0_6.json.gz)
executes the present native64 probe on all 6,532 retained inputs; historical
32-bit expectations and earlier MIPS evidence retain their separate scopes.
The [selected installed CTest receipt](parity/inmemory_cli_installed_native_5_0_6.json.gz)
passes **5/5** checks: the actual comparator, admission, source contract,
source controls and current native binding.

The registered **995** native tests are an inventory, not a full-suite run.
Static source lookup covers **127/160** custom comparison locators; **33**
remain. The C++ runtime, generator and generated SDK implementations are
unchanged. Complete G13/G14/G17, GP13/GP16, AP02 and P0–P7 exits remain open.
No new sanitizer, MIPS, GPU, imported-module or physical-router acceptance is
claimed here.
The [artifact lifecycle receipt](parity/inmemory_cli_artifact_lifecycle_5_0_6.json.gz)
records exact task-owned JUnit and Python bytecode cleanup. Shared caches and
other sessions' files were left untouched.
