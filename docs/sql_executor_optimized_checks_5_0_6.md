# SQL executor source checks under Python optimization

The registered SQL executor provenance verifier checks nine declared driver
contracts and their retained 8,243 original/native observations, original
`teaql-sql` source, compiled project metadata, dependency identities, input
payloads and exact output matrices. Its negative-control runner checks 22
effective corruptions and two real legacy dependency locks. Together they
contained 60 Python `assert` statements, removed under `python -O` or a
nonzero `PYTHONOPTIMIZE`.

Before the repair, changing only `complete` from `true` to `false` in a copy
of the retained execution record caused the verifier to fail under ordinary
Python but **return success under `python -O`**. The untouched record passed
both modes. This is a saved-record verification witness; it does not claim a
new Rust build or fresh C++ execution.

The [exact source transition](parity/sql_executor_optimized_assert_rewrite_5_0_6.json.gz)
stores both pre-edit sources and hashes. All 55 verifier and five control-runner
assertions became unconditional `if`/`AssertionError` checks at the same
statement positions. Whole-AST comparison preserves their conditions,
failure messages and surrounding code. The
[permanent control](../tests/parity/verify_sql_executor_optimized_controls_5_0_6.py)
checks source identities and runs valid verification, incomplete-record
rejection and the existing 24 negative controls in both normal and optimized
Python: **6/6 checks**. The selected native CTests pass **3/3** for the source
verifier, original controls and new optimized control.

The historical [complete execution](parity/sql_executor_driver_execution_final_5_0_6.json.gz)
and original Rust/source/corpus identities remain unchanged. No C++ runtime,
generated SDK or generator source changed; no new imported-module or whole
native epoch is claimed. The current native registry has **1,075** tests and
**594** Python entry scripts; **92** registered scripts still contain
**1,103** removable assertions. Complete SQ02/G13, G14/G17 and P1/P7 remain
open.
