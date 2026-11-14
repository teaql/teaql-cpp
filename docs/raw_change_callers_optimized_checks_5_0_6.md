# Decoded-value corruption in shared source controls

This SQ02/G13 repair covers the lifecycle, generated-helper, query-input and
query-surface source-control entry points. References remain Rust 5.0.6 runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. It changes Python control programs;
there is no new C++ or Rust runtime execution in this checkpoint.

All four callers imported the historical `raw_change` function. Its value
mutation was inside `assert change_text(...)`. Under `-O`, the mutation is
removed, but JSON serialization still changes whitespace in the raw stdout
string. Comparing the whole record therefore reports a difference even though
the decoded observation is unchanged. The source validator correctly accepts
that semantically unchanged record; the corruption control did not perform
the change its name promised. Ordinary Python performs the value mutation and
the validator rejects it. This is not an ordinary-mode source-validator gap.

The [pre-edit witness](parity/raw_change_callers_preedit_5_0_6.json.gz)
executes the actual helper and each complete source validator in both modes.
Its eight outcomes distinguish raw-byte differences from decoded-value changes.
The complete runner source and hash are captured before its owned directory is
removed. No retained observation is edited in the repository.

The [transition manifest](parity/raw_change_callers_hardened_transition_5_0_6.json.gz)
keeps all four original control scripts and the historical helper unchanged.
Four active copies replace six assertions with unconditional checks and make
exactly one import change each, selecting the already-qualified unconditional
helper. Lifecycle's positive protocol checks and negative-adapter decisions
remain active under optimization; the other three terminal result counts also
remain active. The sole change to each existing CTest command is its script
path. The original registries and source receipts retain their original hashes.

The [permanent guard](../tests/parity/verify_raw_change_callers_hardened_5_0_6.py)
checks exact source AST transitions, the shared helper's existing transition,
dependencies, current commands and four source/registration corruptions.
Its [focused control receipt](parity/raw_change_callers_hardened_controls_5_0_6.json.gz)
records **16** processes across ordinary and optimized Python: each active
caller changes an actual decoded value and its source validator rejects it;
each also rejects an input with no string to modify, preserving that input.
The owned runner is removed after all subprocesses finish.

The [selected native receipt](parity/raw_change_callers_selected_native_5_0_6.json.gz)
passes **9/9** source/control CTests under ordinary Python and **9/9** under
`PYTHONOPTIMIZE=1`, with zero failures, errors or skips. Each mode rejects all
**149** existing substitutions: lifecycle 42, helper 35, query-input 37 and
query-surface 35. All **36** frozen source/evidence hashes remain unchanged.
These selections replay complete saved source records; they do not execute
new Rust or C++ business programs. The owned runner and JUnit files were
removed after completion, reclaiming **7,226 bytes** across the focused and
selected runs.

The registry has **1,094** tests and **612** Python entries; **45** retain
**133** direct assertions. Imported and embedded executable checks still need
separate review. No full current native epoch has run. Complete SQ02,
G13/G14/G17 and P0-P7 remain open at **14 qualified / 3 partial**.
