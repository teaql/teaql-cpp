# Audit expression consumption and required operands

Reference runtime: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; reference generator:
`45e1572c4956ca963410d8ea436ead3c8857087a`.
This continues AP02. Full expression/token/container semantics, G14/G17 and
P0-P7 remain open.

The [terminal scoped qualification](parity/audit_expression_operands_qualification_5_0_6.json.gz)
joins the source comparisons, regression guard, current generated artifacts and
installed consumers. It preserves the failed intermediate checks and leaves
every source/capability status unchanged.

## Source responsibility and installed correction

The actual unchanged `parse_container_attrs` parses each value as `syn::Expr`
before classifying a literal. A successfully parsed nonliteral expression is
ignored, but must be consumed so later attributes can run. A syntax error stops
nested parsing while preserving earlier assignments. Neither case evaluates
application expressions or expands macros.

The previous [28-input witness](parity/audit_expression_current_replay_5_0_6.json.gz)
retains six nonliteral continuation differences and two invalid-operand
differences. For example, a previous limit of 17 followed by `if flag {1}` and
a later limit of 21 ends at 21 in Rust. The old adapter stopped at `if`, retaining
17. Conversely, `(17 +)` and `17 +` must stop before a later assignment; the old
adapter skipped these invalid expressions and incorrectly applied 21.

The shared Java cursor now consumes conditional, async/const/unsafe/loop,
return and closure forms. Parenthesized expressions, tuples, arrays/repeats,
call arguments and indexes validate required operands and delimiters. Binary
operators require a right operand; member access, optional range endpoints,
simple casts and postfix question marks retain distinct consumption rules.
Literal values remain separate from all of these nonliteral forms.

The [source observation](parity/audit_expression_operands_original_5_0_6.json.gz)
establishes 27 valid and 19 invalid form expectations before Java test assertions
are added. Both audit-limit keys and both later-limit/later-mask continuations
are exercised. The [operand inputs](../tests/parity/audit_expression_operands_5_0_6_inputs.json)
also retain every macro control and the complete earlier witness. Observations
are compared through the actual unchanged Rust parser and installed Java helper,
without reconstructing metadata results locally.

The [complete prior producer bodies](parity/audit_expression_parser_before_5_0_6.json.gz)
preserve earlier uncommitted changes. Only shared generator interpretation and
Java tests change; C++ runtime, ownership and privacy algorithms do not change.

## Broader grammar remains an explicit obligation

A separate [35-input grammar witness](../tests/parity/audit_expression_grammar_next_5_0_6_inputs.json)
covers prefix ranges, patterns, loop/match forms, labels, generic paths, typed
closures, raw references, invalid block statements and lexical delimiters.
Its [initial actual-source comparison](parity/audit_expression_grammar_next_5_0_6.json.gz)
finds 30 differences. Two are newly exposed closure-header regressions: a
leading empty parameter and an empty type annotation. The cursor correction
rejects these before consuming later metadata, and permanent Java controls
retain both cases. The other grammar responsibilities remain pending.

The [intermediate combined replay](parity/audit_expression_operands_current_5_0_6.json.gz)
executes 793 observations: 761 agree, including 747 exact valid outputs and 14
agreed source rejections. There are 15 source rejections in total; one remains
a lexical-boundary mismatch. All 184 operand continuations and the eight earlier
expression differences agree. Four earlier container differences and 28 broader
grammar differences remain, so `complete` deliberately stays false.

The [archived/current interpreter comparison](parity/audit_expression_before_current_5_0_6.json.gz)
identifies four regressions in that intermediate state: async closures, generic
method calls and two invalid if-body cases. That failed guard is retained.
The installed follow-up adds async closure and generic-method consumption plus
required-operand/let checks inside supported block statements. Unsupported
item/type/pattern forms retain the earlier balanced-token behavior; this is
explicitly not complete block validation.

The [current installed replay](parity/audit_expression_block_current_5_0_6.json.gz)
matches **768/793** observations: **754** exact valid outputs and **14** agreed
source rejections. Its source has **15** rejections in total, including the
still-unmatched lexical case. The independent
[before/current guard](parity/audit_expression_block_regressions_5_0_6.json.gz)
finds **113 improvements / zero regressions** across the same inputs. All four
intermediate regressions are repaired. **25 differences** remain: four container
interactions and 21 broader grammar/lexical cases. These observations retain
their failed acceptance flag rather than becoming a passing whole-parser gate.

| Remaining responsibility | Required behavior |
| --- | --- |
| Expression starts and control flow | Consume prefix ranges, pattern conditions, remaining loops/match, labels, yield and async closures using the original expression boundary. |
| Paths, types and patterns | Preserve generic/qualified paths, struct expressions, typed closure returns and raw references without evaluating them or treating their commas as metadata separators. |
| Block validity | Validate statements and nested expression operands; balanced braces alone must not allow later metadata after an actual source syntax error. |
| Lexical validity | Reject invalid complete token streams before metadata effects; this differs from an ignored nested-meta syntax error after earlier assignments. |
| Recognized container attributes | Preserve actual entity/storage/service/relation effects and their propagation, rather than only fixing later audit values. |

In particular, a balanced token group does not prove that its block statements,
patterns or types form a valid Rust expression. The current correction does not
claim a complete Rust parser. The four original recognized-attribute interactions
also remain unresolved: entity/table/data-service/reverse-relation changes affect
identity and runtime responsibilities beyond the audit limit. Merely allowing
the later limit to run would not implement those effects.

The invalid wire-naming behavior decision remains separate and unanswered.
No source/capability promotion or complete phase acceptance follows from this
bounded correction. The current compiler/artifact/consumer gates below qualify
only the repaired forms and their declared comparison matrices.

## Compiler and artifact evidence

The [thirteen compiled pairs](parity/audit_expression_compiled_current_5_0_6.json.gz)
pass before the block/async/generic follow-up. The
[current byte proof](parity/audit_expression_compiled_reproduction_retry_5_0_6.json.gz)
then independently regenerates both complete SDK trees for every pair and checks
every file byte and the complete file set against those actually compiled inputs.
Their descriptor/policy execution evidence is retained because all generated
inputs are identical; this is not another thirteen current compiler executions.

The [seven new compiled pairs](parity/audit_expression_block_compiled_5_0_6.json.gz)
exercise installed async closures, generic methods, valid statement blocks and
invalid if/else/block/let bodies through the actual original derives and complete
C++ generated entities. All seven compile, execute and match. Together these
twenty paired configurations retain **60 descriptors / 280 field-policy tuples**.
Raw mask overrides and Child's distinct model inheritance remain covered.

The [current Java gate](parity/audit_expression_java_block_5_0_6.json.gz) passes
**89/89**, including the complete generation class and all sequence, literal,
layout and JSON-input classes. Permanent sequence controls retain every newly
identified regression as well as the earlier macro and literal boundaries.

The first [artifact-reproduction helper run](parity/audit_expression_compiled_reproduction_5_0_6.json.gz)
reproduced all thirteen trees, then failed its final source-identity guard because
dictionary keys were strings rather than Path objects. That incomplete receipt
and the [exact earlier helper](parity/audit_expression_reproduction_tool_before_5_0_6.json.gz)
are preserved. The explicit Path conversion repair and a separate fresh retry
qualify the proof; the original failed run is not relabeled as passing. The
[corrective input snapshot](parity/audit_expression_block_tool_fixed_inputs_5_0_6.json.gz)
preserves every producer/observer/native archive identity across this tooling
repair.

All [78 active SDKs](parity/audit_expression_block_sdk_refresh_5_0_6.json.gz)
reproduce their **3,457 generated files** without content changes; **2,201
protected artifacts** retain their hashes. Only producer fingerprints refresh.
The [installed regressions](parity/audit_expression_installed_regressions_5_0_6.json.gz)
pass **4/4 native** and **3/3 imported-module** tests after successful no-work
builds. They cover generated APIs, audit events, actual private-query comparisons
and file sinks. C++ implementation/lifetime code and native archives remain
unchanged; sanitizer, MIPS and full integration profiles are not rerun.

## Reproduce the current observations

The parser verifier accepts repeated `--extra-cases` arguments, so independent
matrices remain separate without duplicating or filtering their known failures.
For a fresh output path:

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_audit_metadata_sequences_5_0_6.py \
    --extra-cases tests/parity/audit_expression_operands_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_grammar_next_5_0_6_inputs.json \
    --output /tmp/audit_expression_operands_fresh.json.gz
```

This command intentionally records unresolved differences. Process success is
observation completion; inspect `complete` and `differences` before making an
acceptance claim. Passing and pending observations overlap older corpora and
are not additive distinct feature counts.

To check that an interpreter change introduces no new differences in a completed
source replay:

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_audit_expression_regressions_5_0_6.py \
    --before docs/parity/audit_expression_parser_before_5_0_6.json.gz \
    --current /tmp/audit_expression_operands_fresh.json.gz \
    --output /tmp/audit_expression_regressions_fresh.json.gz
```

This guard recompiles the archived real Java implementation in isolation. Both
versions are judged against actual Rust outputs, so an earlier difference never
becomes a golden expected value. Zero newly introduced differences remains a
separate claim from eliminating all existing ones.

The later [path/range/delimiter continuation](audit_expression_paths_5_0_6.md)
matches 1,028/1,040 expanded observations, including every actual source
rejection, with 130 archived/current improvements and no regressions. Within
this original 793-input matrix it advances matching to 781; twelve declared
differences remain. The intermediate high-byte preflight regression and its
separately repaired guard are preserved. Full AP02 grammar/identity obligations
and the unanswered AP03/R08 decision remain unchanged.
