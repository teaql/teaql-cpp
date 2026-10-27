# Strict JSON types in parity comparators

Eight custom comparison boundaries now distinguish Boolean, integer and floating
JSON values. The C++ runtime, generated libraries and locked Rust sources are
unchanged. This corrects test acceptance rather than eight runtime defects.

## Reproduced false passes

Python's ordinary container equality accepts `True == 1` and `1 == 1.0`.
The existing shared `compare_values.same` deliberately rejects both. Eight
custom comparators bypassed that helper for some or all of their output.

The [first actual-native controls](parity/dispatcher_json_type_controls_initial_5_0_6.json.gz)
and [additional controls](parity/dispatcher_json_type_controls_additional_5_0_6.json.gz)
first pass each unmodified probe's complete output through its actual registered
comparator. They then change only JSON primitive kinds, preserving records,
keys, cardinality and numerically equal values. All eight original comparators
accept at least one corrupted output; bulk decoding accepts both corruptions.

| Comparator | Previously accepted corruption |
| --- | --- |
| Indexed evaluation | Boolean load/clone/wrapper flags become integers |
| Generated member aliases | Boolean loaded/dirty/new flags become integers |
| Generated bulk load state | Boolean flags become integers; 40,404 integer row values become floats |
| Explicit public-save scope | Integer observations become floats |
| Graph outcome models | Integer observations become floats |
| Unpolled lookup ownership | Sixteen Boolean outcomes become integers |
| Relation probe boundaries | Integer rows/counters/telemetry become floats |
| Private generated query sinks | Integer result IDs/versions become floats |

The observers invoke the real native executables. They do not fabricate a C++
defect, modify a Rust result or change the business operation. Original process
success and positive controls are recorded alongside each corruption.

## Shared correction and permanent controls

The affected full-output comparisons and field invariants now use the existing
strict helper. The indexed wrapper's completeness flag requires actual `True`.
The affected JSON-lines readers use LF framing, preserving Unicode separators
inside strings. Existing diagnostic representation adaptations remain explicit.

The permanent comparator contract test grows from thirteen to **32 controls**.
Six complete valid saved-output replays, seven kind corruptions and six blank
record corruptions supplement the existing resource/deferred controls. It uses retained original corpora and
source receipts, with no C++ execution claim for those replay controls. The
[permanent gate](parity/comparator_json_type_framing_regressions_5_0_6.json.gz) passes
32/32, including every added positive control and expected rejection. The earlier
26-control receipt retains its intermediate scope.

The [installed actual-native gate](parity/dispatcher_json_type_controls_installed_5_0_6.json.gz)
passes twelve current registrations through both original recording and
unmodified replay. Its **37 effective corruptions all reject**, including all
nine previously accepted kind corruptions and record/member controls. No changed
output is accepted. Twenty-three other requested transformations have no effect
on their actual output and are explicitly not counted as rejections.

Textual progress/privacy markers and termination status have their own
invariants; duplicate progress text is not a malformed JSON result. Clock
failures likewise retain their expected nonzero native status. A formatter's
directory and admission helpers remain separate from its result records.

The control tool records exact commands, properties, native identities, inputs,
original/emitted hashes and actual effects. Its mode lives in an owned control
file so privacy fixtures can clear environment configuration without disabling
the control. Replay keys include the working directory; an unmodified replay
must pass before corruption. Unspecified timeouts use the actual native
`DartConfiguration.tcl` setting, not a shorter invented budget.

## Initial broad attempt and artifact ownership

The [preflight](parity/dispatcher_result_controls_preflight_5_0_6.json.gz)
rejects nineteen effective record/member controls. The subsequent
[broad attempt](parity/dispatcher_result_controls_current_5_0_6.json.gz) preserves
448 rejections but is **incomplete**. Its sample-progress positive run hits the
controller's incorrect 120-second fallback; the successful native checkpoint
had taken 505.923 seconds for that same registration. Privacy environment
clearing and file-producing replay also exposed controller limitations. One
duplicated textual progress marker is outside JSON-record scope. None of these
is relabeled as a complete broad pass or a runtime regression.

The [disposition](parity/dispatcher_result_controls_initial_disposition_5_0_6.json.gz)
retains that controller source and archives ten diagnostics created by deliberate
corruption. Those files are removed from the ordinary initial-gap locations;
they are not genuine source/native discrepancies. Four exact private-sink
temporary directories are also removed after their children terminate.

Seventeen legacy diagnostic destinations now honor the explicit development
directory supplied by the control tool. Removing only that import/wrapper
restores each complete prior AST; the first shared-helper checkpoint retains
unchanged `same`, `observe` and `main` bodies. The [preservation proof](parity/comparator_diagnostic_destination_preservation_5_0_6.json.gz)
records every before/after hash. Normal comparison behavior and its default
diagnostic destination are preserved. The subsequent
[framing extraction](parity/comparator_lf_framing_extraction_5_0_6.json.gz)
moves the existing LF decoder into a shared helper used by six custom readers.
Inlining it restores the complete original `observe` AST; `same`/`main` remain
unchanged. Interior blank records reject, while Unicode separators remain data.
Private-sink tests now use a temporary
directory with automatic cleanup while optional reports retain the observations.

## Current acceptance and reproduction

The [final affected native gate](parity/comparator_json_type_final_native_gate_5_0_6.json.gz)
passes **9/9 registered CTests**, including all eight affected boundaries and
the 32-control contract test. Builds before and after report no work. Source
and generated C++ implementations stay identical to the completed 877-test
checkpoint; this is not another full-suite run. No sanitizer, MIPS or generator
regeneration is required for these Python-only changes.

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/verify_comparator_result_contracts_5_0_6.py
python3 scripts/verify_dispatcher_result_controls_5_0_6.py \
  --test generated_member_alias_rust_parity \
  --record /tmp/alias_comparator_controls.json.gz
```

Use a fresh receipt path. Every owned controller/cache/log directory is removed
after terminal execution. The current twelve-registration controller removes
7,191,129 bytes; the incomplete broad attempt removes 634,417,402 bytes.
Stable build directories and unrelated caches are retained.

Source status remains **122 accepted / 0 pending / 2 excluded_external** and
capabilities remain **11 qualified / 6 partial**. These controls strengthen SQ02;
they do not establish every oracle dependency/normalization rule or complete
G13, G14, G17 or P0–P7. The user subsequently approved the
[quoted-FK](quoted_fk_compatibility_exception_5_0_6.md) and
[invalid-naming](invalid_wire_naming_compatibility_exception_5_0_6.md) exceptions;
their capability signoff and current gates remain separate work. The earlier
C++ JSON exception is unchanged.
