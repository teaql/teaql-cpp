# Audit expression paths, ranges and delimiter boundaries

References: Rust 5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. AP02 remains partial. This continues
the [operand correction](audit_expression_operands_5_0_6.md); it does not claim
a complete Rust expression, type, pattern, statement or token parser.

## Actual source responsibilities

The original unchanged parser constructs the complete attribute token stream
before applying nested metadata. A delimiter error after an unknown assignment
therefore rejects the whole input, even when nested parsing would stop at that
assignment. A well-tokenized but invalid expression instead stops nested parsing
and preserves earlier assignments. These two failure boundaries stay distinct.

Successfully parsed nonliteral expressions are ignored, but must be fully
consumed before a later limit or mask assignment. The installed Java helper now
handles the following additional forms without evaluating them:

| Form | Preserved boundary |
| --- | --- |
| Prefix/open/inclusive ranges | Open bounds may be absent; inclusive bounds and present operands must parse. |
| Generic and qualified paths | Consume turbofish arguments, qualified paths and following calls without treating their commas as attribute separators. |
| Labeled break/continue/loop/block and yield | Distinguish a lifetime label from a character literal; preserve optional values and required bodies. |
| Raw references | Require the source const/mut marker and an operand. |
| Explicit closure return types | Consume the supported named/generic/tuple/reference/pointer type forms and require a block body. Full type validation remains separate. |
| Complete delimiter preflight | Reject mismatched/unclosed groups before any metadata effect; quoted delimiters and macro lifetime tokens remain opaque tokens. |

Actual string-valued metadata still uses the existing Rust string decoder.
Ignored byte/C strings retain their own literal validation; their byte escapes
may exceed 0x7f. They are not decoded as ordinary UTF-8 string values. Macro
contents are tokenized but never expanded or executed.

The shared correction changes the producer's existing `CppRustLiteralMetadata`
and source-derived Java tests. No C++ runtime, ownership, privacy or allocator
algorithm changes, and no dependency is added. Existing handwritten examples
and the original producer checkout remain unchanged.

## Source comparisons and preserved regression

The [before archive](parity/audit_expression_paths_before_5_0_6.json.gz) preserves
the complete earlier producer/test bodies and active ledgers. The new
[196-input path matrix](../tests/parity/audit_expression_paths_5_0_6_inputs.json)
and [51-input literal matrix](../tests/parity/audit_expression_delimiter_literals_5_0_6_inputs.json)
extend the unchanged 793-input matrix. Names containing valid/invalid describe
initial candidates, not acceptance: actual Rust classifies every input. In
particular, the candidate `&'a value` stops expression parsing and retains the
earlier limit; it is not accepted merely because it resembles a type reference.

The [initial replay](parity/audit_expression_paths_initial_5_0_6.json.gz) records
133 differences before this correction. The first installed path correction
reduces its 989 observations to twelve differences. Extending the witness to
byte/C strings exposes an intermediate bug: the new preflight uses the ordinary
string decoder on valid high-byte escapes. The
[failed regression guard](parity/audit_expression_paths_literal_regressions_initial_5_0_6.json.gz)
records 124 improvements and **three regressions** against the actual archived
producer. The [intermediate source replay](parity/audit_expression_paths_literal_initial_5_0_6.json.gz)
retains all nine byte/C discrepancies, including six preexisting ones.

The [repaired replay](parity/audit_expression_paths_literal_repaired_5_0_6.json.gz)
matches **1,028/1,040** actual source outcomes: **1,000 exact valid outputs** and
all **28 actual source rejections**. The separate
[before/current guard](parity/audit_expression_paths_regressions_5_0_6.json.gz)
records **130 improvements / zero regressions**. Both versions are compared
with actual unchanged Rust, never with an earlier Java result as a golden value.

All 247 added observations match. Within the original 793 inputs, matching
advances from 768 to **781**. The remaining **twelve declared differences** are:

- Four recognized container effects: entity, table, data-service and reverse
  relation overrides. Merely skipping them to reach a later limit would not
  implement their identity/runtime effects.
- Eight expression forms: if-let, while, while-let, for, match, struct construction,
  struct update and standalone let expressions.

These are the declared witnesses, not an exhaustive inventory of Rust grammar.
Full type/pattern/block/lexical validity remains an AP02 responsibility. The
invalid wire-naming AP03/R08 decision is still unanswered and independent.

## Compiler, artifacts and installed consumers

The [Java gate](parity/audit_expression_paths_java_5_0_6.json.gz) passes **93/93**
across the complete generation, audit-sequence, literal, layout and JSON-input
classes. Four permanent tests retain both later-limit/later-mask boundaries,
missing operands, global delimiter errors and the high-byte regression.

The [new compiler comparison](parity/audit_expression_paths_compiled_5_0_6.json.gz)
generates full original/C++ trees and compiles actual original generated entity
files through unchanged derives and the complete C++ facade. All **9/9** selected
models cover ranges, generic/qualified paths, labels, raw references, typed
closures, high-byte literals and an empty-mask continuation. The observer bodies
are unchanged; only named input models are appended to the existing helper.
All compiler/execution outcomes match, retaining **27 descriptors / 126
field-policy tuples**, including Child's independent inheritance semantics.

The earlier [thirteen-pair reproduction](parity/audit_expression_paths_prior_13_reproduction_5_0_6.json.gz)
and [seven-pair reproduction](parity/audit_expression_paths_prior_7_reproduction_5_0_6.json.gz)
independently regenerate both complete trees and require every file byte/file set
to equal the earlier actually compiled inputs. They retain those prior execution
results only on exact equality; they are not twenty new compiler executions.
Both reproduction receipts are terminal and complete. Together the twenty
retained and nine fresh configurations preserve **87 descriptors / 406
field-policy tuples** with their distinct execution scopes.

The [all-SDK gate](parity/audit_expression_paths_sdk_refresh_5_0_6.json.gz)
passes for **78 SDKs / 3,457 generated files** and **2,201 protected artifacts**.
It permits producer-fingerprint refresh only after all generated bytes reproduce.
The [native archive snapshot](parity/audit_expression_paths_native_archives_before_5_0_6.json.gz)
keeps the six existing library identities for the installed consumer checks.
The [installed regression receipt](parity/audit_expression_paths_installed_regressions_5_0_6.json.gz)
records current API/audit/private-query and actual file-sink checks after
successful builds: **native4/imported3 pass**, both final builds report no work,
and all six existing archives retain their hashes. The
[terminal scoped qualification](parity/audit_expression_paths_qualification_5_0_6.json.gz)
joins all source/compiler/artifact/consumer receipts. Source dispositions remain
**39 accepted / 83 pending / 2 excluded_external** and all seventeen capability
statuses are unchanged. Native registry741 remains discovery, not a current
full-suite pass. No runtime, dependency, sanitizer/MIPS, full-suite or physical
device change/run is claimed. Complete AP02/G14/G17/P0-P7 remain open.

## Reproduction

The observation command deliberately preserves the twelve pending differences:

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_audit_metadata_sequences_5_0_6.py \
    --extra-cases tests/parity/audit_expression_operands_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_grammar_next_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_paths_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_delimiter_literals_5_0_6_inputs.json \
    --output /tmp/teaql_audit_expression_paths_fresh_5_0_6.json.gz
python3 scripts/verify_audit_expression_regressions_5_0_6.py \
    --before docs/parity/audit_expression_paths_before_5_0_6.json.gz \
    --current /tmp/teaql_audit_expression_paths_fresh_5_0_6.json.gz \
    --output /tmp/teaql_audit_expression_paths_regressions_fresh_5_0_6.json.gz
```

Process success means observation completion; the parser receipt's `complete`
remains false while differences exist. A zero-regression guard does not establish
complete Rust parsing, G14/G17 or P0-P7 acceptance.

Reproduce the nine fresh compiler pairs separately:

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/audit_generated_privacy_contracts_5_0_6.py \
    --cases expression_prefix_range expression_qualified_path \
    expression_generic_path expression_labeled_loop expression_labeled_break \
    expression_raw_reference expression_closure_return_type \
    expression_byte_literal expression_byte_macro_mask \
    --require-match --output /tmp/teaql_audit_expression_paths_compiled_fresh_5_0_6.json.gz
```

The later [pattern/control-flow continuation](audit_expression_patterns_5_0_6.md)
qualifies the eight remaining expression witnesses and nested condition/index
and early-block boundaries. It matches 1,448/1,452 expanded actual-source results,
with 240 archived/current improvements and no regressions. Four injected
container effects and complete grammar/identity requirements remain open.
