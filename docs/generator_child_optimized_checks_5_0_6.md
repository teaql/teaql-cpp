# Generator verifier child checks under optimized Python

This SQ02 continuation repairs an actual child-program acceptance defect found
during the [named custom-comparison join](custom_comparison_binding_join_5_0_6.md).
It preserves the locked Rust 5.0.6 references and the historical scripts.
G13, G14 and G17 remain partial at **14 qualified / 3 partial**.

## Actual pre-edit omission

The runtime-configuration and sample-observation source prerequisites launched
`scripts/verify_generator_parity.py` as a Python child. Its **21 assertions**
checked generated provenance, source/model identities and the complete
80-manifest inventory. Child processes inherited `PYTHONOPTIMIZE`, which
removed those checks.

The [six-process witness](parity/generator_verifier_child_preedit_5_0_6.json.gz)
runs the actual verifier with an isolated read of the retained Rust query-alias
provenance. A hand-edited marker and an incorrect model hash each reject under
ordinary Python and pass under optimization. Valid input passes in both modes.
Only the observer process's manifest read is replaced; repository sources and
metadata files remain unchanged. Its 2,796 bytes of owned observer output are
removed after process exit.

## Exact source repair and actual parent coverage

The [source transition](parity/generator_verifier_child_transition_5_0_6.json.gz)
retains three original files and qualifies three separate copies. Exactly
21 assertions become unconditional checks. Two caller adaptations change only
the generator-verifier child path. Whole-file AST comparison and source hashes
exclude undeclared behavior changes.

The runtime-configuration and sample-observation fixtures keep their existing
test/setup names and use the qualified parent copies. A new required
`generator_child_checks` fixture runs the
[permanent source/control verifier](../tests/parity/verify_generator_child_checks_5_0_6.py)
before either source prerequisite.

The [current control receipt](parity/generator_verifier_child_registered_controls_5_0_6.json.gz)
executes **18 actual child/parent processes**: direct verifier and both actual
parent programs, three valid/invalid inputs, and two optimization modes.
Invalid child provenance raises `AssertionError`; each parent observes that
nonzero child exit and raises `CalledProcessError`. Parent traces retain the
actual upstream-verifier and hardened-generator commands, in order. The
observer replaces only the selected child's manifest read, while executing
the original parent control flow. Valid output is identical across modes.

Five source/registration controls reject wrong historical identity, changed
active code, missing child-call admission, reverted parent registration and a
missing required guard. Owned observer processes are stopped and reaped before
their temporary output is removed if a timeout occurs.

P0 separately invokes its historical verifier through `-E -B`, with ordinary
checks explicitly required by the P0 entry. That enforced historical boundary
remains intact. Direct optimized use of the raw historical generator verifier
retains its documented counterexample; active source prerequisites use the
unconditional replacement.

## Native continuation

The [native controller record](parity/generator_verifier_child_native_5_0_6.json.gz)
joins both repaired source prerequisites to all thirteen outside-directory
startup, sample, schema, error and runtime-configuration comparisons, the
existing source/helper controls and the new guard. Both ordinary and optimized
selections pass **18/18**, followed by **1/1 ordinary P0**. Exact registry/JUnit
names contain no failures, errors or skips. All **11,962 input hashes**, the
three admitted evidence files, both references and **twelve affected native
files** remain unchanged.

The guard removes **29,381 bytes** of owned observer output; the coordinator
removes **12,267 bytes** of JUnit output. The transition/control/native reports
total **468,486 bytes**. No disposable build directory or copied native
executable is created. The current registry has **1,100 tests / 618 Python
entry scripts**, with zero direct assertions in those registered entries.

These scopes remain independent of full current native execution, complete
SQ02, G14 metadata, G17 governed execution and the P0-P7 exit criteria. No
runtime C++, generated SDK or generator implementation is changed by this
verifier repair.
