# Adapter, logging and configuration checks under optimized Python

Three registered corruption-control gates could report success while Python
optimization removed their actual checks. The
[pre-edit execution witness](parity/adapter_logging_configuration_preedit_5_0_6.json.gz)
retains twelve actual source-script executions with correct and deliberately
broken dependencies under ordinary Python and `python -O`:

| Gate | Controlled dependency regression | Historical optimized result |
| --- | --- | --- |
| Quoted-FK Debug adapter scope | Rewrite a protected SQL sentinel outside the allowed descriptor paths | Reports eighteen successful controls |
| Logging normalization | Rewrite a Rust header inside ordinary application text | Skips all three normalization calls and reports success |
| Configuration source controls | Accept every configuration record, including corrupted records | Skips the corruption checks and reports twenty rejections |

Each regression fails with `AssertionError` under ordinary Python. Valid
dependencies pass in both modes. These are in-memory dependency substitutions
within the real control scripts; the witness does not claim a new Rust or C++
business execution or a defect in the unchanged production dependencies.

The [exact source transition](parity/adapter_logging_configuration_hardened_transition_5_0_6.json.gz)
preserves all three historical scripts and installs separate active copies.
Only sixteen assertions become unconditional `if`/`raise` checks: seven
adapter checks, four logging checks and five configuration checks. There are
no additional function changes. Each existing CTest command changes only its
script path; fixture and timeout properties remain identical.

The [permanent guard](../tests/parity/verify_adapter_logging_configuration_hardened_5_0_6.py)
pins the three full source transitions, eleven imported/corpus dependencies,
the original witness and all three registrations. Its
[execution receipt](parity/adapter_logging_configuration_hardened_controls_5_0_6.json.gz)
records twelve valid/regression processes across both Python modes and four
rejected source/registration corruptions. Valid executions now make the same
imported calls in both modes: fifteen adapter calls, three normalization calls
and twenty-eight configuration validation calls. Each broken dependency is
rejected at the original ordinary-Python failure with the same call sequence.

The [selected native receipt](parity/adapter_logging_configuration_selected_native_5_0_6.json.gz)
passes **6/6** CTests per Python mode, with zero failures, errors or skips and
all **25** frozen inputs unchanged. Each selection retains eighteen adapter
controls, fifteen logging substitutions, three normalization boundaries and
twenty configuration corruptions, plus their source fixtures. These checks
consume complete saved source observations; they do not execute new Rust or
C++ business programs. All owned observer sources, traces and JUnit files
were removed after their processes completed, reclaiming **31,486 bytes**.

The current registry contains **1,095** tests and **613** Python entry scripts;
**42** retain **117** direct assertions. Imported helpers and executable strings
remain separate review obligations. The generator helper's generation-only
`main` is not executed by this adapter gate and retains its historical source.
No full current native epoch has run. Complete SQ02, G13/G14/G17 and P0-P7
remain open at **14 qualified / 3 partial**.
