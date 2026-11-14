# Generated sample parity checks under optimized Python

This is a bounded SQ02/G13 repair for four registered generated sample
comparators. It does not change the locked Rust 5.0.6 observations, the C++
runtime, the generator, or the generated SDKs. It does not close SQ02, G13,
G14, G17, or P0-P7.

The [pre-edit witness](parity/generated_sample_optimized_preedit_5_0_6.json.gz)
retains the complete former sources and source hashes for the type, custom,
catalog, and business sample comparators. A child program replaced only the
selected input list with an empty list while using each registered C++ probe.
Ordinary Python rejected all four empty comparisons at the expected cardinality
check. `python3 -O` removed those `assert` checks: all four programs exited
successfully and printed `Verified 0 ... scenarios`. The intended case counts
are 360, 140, 240, and 128, respectively. This is a real zero-observation
false acceptance, not a claim that the retained Rust or C++ outcomes differ.

The installed sources replace exactly 13, 21, 18, and 14 direct `assert`
statements with unconditional `if`/`AssertionError` checks. The
[permanent control](../tests/parity/verify_generated_sample_optimized_controls_5_0_6.py)
reconstructs each whole-source transition from the retained predecessor and
checks that no direct `ast.Assert` remains in those four comparators. It runs
16 negative processes: empty input under ordinary and optimized Python for
each comparator, wrong provenance under optimized Python for each, and an
empty executable result under optimized Python for each. All 16 reject. The
[control receipt](parity/generated_sample_optimized_controls_5_0_6.json.gz)
records the current source hashes and cleanup of the 47-byte task-owned fake
observer and its temporary directory.

The four real comparators and their original-source fixture passed the
selected native CTest run **5/5**. With `PYTHONOPTIMIZE=1`, the four real
comparators, the permanent control, and their source fixture passed **6/6**.
The current native registry has **1,082** tests and **600** distinct registered
Python entry scripts; **84** of those scripts still contain **943** direct
`ast.Assert` nodes. This is an inventory of optimization risk, not 943
demonstrated false acceptances. No full 1,082-test native epoch has run for
this current source state. The four comparator checks and their negative
controls support only this bounded SQ02 repair.
