# Shared corpus admission and default binding review

Status: three actual malformed-corpus false passes are repaired, and the default
corpus locator follows the current source declaration. Complete SQ02 provenance,
GP13, G13/G14/G17 and P0-P7 remain open.

References remain Rust 5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Actual admission defect and repair

The shared `compare_values.main` previously accepted `cases=[]`, `cases={}` and
`cases=""`. Each supplied zero inputs, observed zero outputs from the real native
Value executable, printed `0/0` and exited successfully. The
[initial execution](parity/corpus_admission_initial_5_0_6.json.gz) retains the
exact unmodified comparator source and all eleven subprocess outcomes. It uses
an owned miniature repository layout with a byte-identical comparator and lock;
the production corpus is never modified.

The entry point now requires a nonempty list of cases before collecting inputs
or invoking either implementation. A valid case may still expect NULL or an
empty collection. The lower-level `observe` API retains its existing empty-input
behavior for protocols that explicitly exercise zero work.

[Installed admission controls](parity/corpus_admission_installed_5_0_6.json.gz)
pass **11/11**: the complete 722-case source corpus, one legitimate observation
and nine invalid corpus/record shapes. The three earlier false passes now reject.
The other malformed forms already failed; they are not counted as new repairs.
All real source cases and expected values remain unchanged.

One admitted observation proves admission only. It cannot establish that a
replaced or truncated corpus contains every required source observation. Exact
source/dependency/observer identity, expected cardinality and normalization
remain independent SQ02 obligations.

## Source-derived default bindings

The old locator did not recognize the shared comparator's own entry and mapped
an imported `main()` with no arguments to a hardcoded `value_cases.json`. The
actual source default is `value_5_0_6_cases.json.gz`.

The inventory now reads the explicit default string from the shared `main`
declaration, recognizes that shared entry and applies the same default to an
imported or aliased call without arguments. Explicit corpus names remain
unchanged. Locally shadowed functions, dynamic arguments and multiple ambiguous
calls remain unresolved instead of receiving fabricated bindings.

The [inventory controls](parity/corpus_inventory_controls_5_0_6.json.gz) execute
eight binding cases against the exact archived preceding implementation and the
current locator; three default bindings change as required. Four actual
inventory CLI selections separately check valid and malformed corpus copies.
All three malformed selections previously passed `--check` as active-baseline
bindings. They now report `invalid_corpus_shape` and fail that check.

## Current registry and permanent checks

The [current locator audit](parity/corpus_inventory_current_5_0_6.json.gz) records
**879 native registrations**, **305 direct active-baseline bindings** and
**170 custom registrations requiring review**. Every direct binding has a
nonempty case list; none points to a missing or historical corpus. The shared
Value entry resolves its actual 722 observations. These are locator/admission
facts, not live source provenance or semantic acceptance of all registrations.
The earlier 877/304/169 snapshot retains its old membership and scope.

Two permanent native checks are registered: `parity_corpus_admission_contracts`
and `parity_corpus_inventory_contracts`. The custom-output controller recognizes
the inventory check as an existing invariant. The admission check uses the real
Value probe and remains subject to actual result mutation.

[Seven affected native checks](parity/corpus_admission_native_gate_5_0_6.json)
pass, including existing typed-result controls, Value/format and two governed
adapters. The existing [imported Value check](parity/corpus_admission_module_gate_5_0_6.json)
passes after its actual target finishes building. The
[record/replay controller](parity/corpus_admission_dispatcher_controls_5_0_6.json.gz)
accepts the actual unchanged observations and rejects five effective corruptions:
dropped/duplicated records, a dropped member and Boolean/integer/float kind
substitutions. The separate inventory invariant also passes. No changed output
is accepted by these controls.

No C++ runtime, generated SDK, module export, observer algorithm or dependency
changes. Existing typed JSON comparison and LF framing remain unchanged. No new
Java, full native, sanitizer, MIPS or physical-router result is claimed.

## Evidence and lifecycle

The [milestone reconciliation](parity/corpus_admission_milestone_5_0_6.json.gz)
binds the exact entry-point change, locator sources, active corpus hashes,
registered commands and terminal checks. Pinned formatting and reference/path
guards have their own recorded output. Capability counts remain 14 qualified /
3 partial; the 879-test registry has not been executed as a new whole-suite gate.

The [cleanup receipt](parity/corpus_admission_cleanup_5_0_6.json.gz) verifies
removal of exact owned control directories. Stable preset outputs, source and
compact evidence are retained; shared caches are not evicted.
