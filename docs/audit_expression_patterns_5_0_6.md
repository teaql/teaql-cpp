# Audit pattern, control-flow and struct expression boundaries

References: Rust 5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. This continues the
[path correction](audit_expression_paths_5_0_6.md). AP02 remains partial: fixing
finite expression witnesses does not establish full Rust grammar or the four
remaining injected container effects.

## Source contract and installed correction

The unchanged actual attribute parser parses an entire `syn::Expr` before
deciding whether it is a literal. A valid nonliteral is ignored but consumed;
an expression parse failure stops later nested settings while preserving prior
ones. These responsibilities apply equally to both limit keys and to a following
limit or mask assignment.

The shared Java cursor now handles the previously unresolved if-let, while,
while-let, for, match, struct construction/update and standalone-let witnesses.
Supported patterns retain tuple/struct/slice nesting, references, ref/mut,
or-patterns, bindings, ranges and rest forms. Missing patterns, operands, bodies,
field separators and match arrows stop before later metadata effects.

Condition parsing suppresses eager struct construction, matching the original
conditional/scrutinee boundary. Parenthesized expressions, arrays, call arguments
and index operands restore the nested expression scope. This distinguishes a
loop/condition body from a struct inside a nested argument or index.

Block-like match arms retain their earlier expression boundary and optional
comma; async blocks, ordinary expressions, macros and postfixed block results
keep their required-comma behavior. Statements reuse the same block-shape result
rather than guessing from a source-text prefix. Balanced token scanning alone
does not replace the remaining full statement/type/pattern validation duties.

## Locked syn behavior, including rest buffers

The actual observer locks **syn 2.0.118**, checksum
`1b9ae57f904213ebb649ce6895b8a66c66f0203b9319718f69a5612a065b1422`,
as a reference-lock subset. Source inspection uses that exact version's
`expr.rs`, `pat.rs` and `classify.rs`, then actual original execution establishes
the expectations. The [dependency record](parity/audit_expression_patterns_syn_contract_5_0_6.json.gz)
also verifies the actual teaql-macros dependency edge, rather than only checking
that this version happens to exist in the workspace lock.

The locked struct-expression helper returns after a rest value without requiring
the remaining braced buffer to be empty. Its struct-pattern helper similarly
returns after a rest pattern. Consequently the actual parser consumes and ignores
such inputs as `Thing {..value, other:}` and `Thing {..value, +}`, then applies a
later limit. The Java helper preserves those actual outcomes. A missing operand
inside the rest expression, such as `Thing {..1 +}`, instead stops nested parsing.

These are discarded attribute expressions: neither generator evaluates them or
executes their body. Their acceptance does not establish valid application Rust
code, business authorization or a new C++ runtime expression API.

## Actual-source observations

The [before archive](parity/audit_expression_patterns_before_5_0_6.json.gz)
preserves all earlier producer/test bodies, observer source and active ledgers.
The [328-input matrix](../tests/parity/audit_expression_patterns_5_0_6_inputs.json)
and [84-input boundary matrix](../tests/parity/audit_expression_pattern_boundaries_5_0_6_inputs.json)
add source-classified candidates to the unchanged 1,040 observations. Candidate
labels carry no local expected result; the actual frozen Rust parser supplies
all outputs.

The [initial replay](parity/audit_expression_patterns_initial_5_0_6.json.gz)
retains 188 differences across 1,368 observations. The first installed correction
reduces those to four. The
[additional boundary replay](parity/audit_expression_pattern_boundaries_initial_5_0_6.json.gz)
then exposes four more forms across both keys and continuations: try blocks,
try-block match arms, and struct expressions inside if/while index operands.
Their sixteen comparisons remain preserved as initial differences. The installed
follow-up adds try-block classification and restores the nested index scope.

The [final source replay](parity/audit_expression_patterns_final_5_0_6.json.gz)
matches **1,448/1,452** outputs: **1,420 exact valid results** and all **28 actual
source rejections**. All 412 new observations match. Within the original
793-input matrix, matching advances to **789**. The separate
[archived/current guard](parity/audit_expression_patterns_regressions_5_0_6.json.gz)
records **240 improvements / zero regressions**, using the actual archived Java
implementation and actual Rust outputs for both versions.

Four declared differences remain: injected entity, table, data-service and reverse
relation settings. They affect actual identities/relationships, so merely skipping
their syntax to reach a later audit assignment would not implement them. Full
expression/type/pattern/block/lexical validity beyond these finite matrices also
remains required. AP03/R08's invalid wire-naming behavior decision is separate.

## Compiler, artifact and installed gates

The [Java receipt](parity/audit_expression_patterns_java_5_0_6.json.gz) records
**97/97** tests across the complete generation, sequence, literal, field-layout
and JSON-input classes.
Four permanent source-derived tests retain valid/invalid patterns and bodies,
condition/index scope, block-arm comma rules and the exact struct-rest behavior.

The [new compiler comparison](parity/audit_expression_patterns_compiled_5_0_6.json.gz)
generates complete original/C++ trees, compiles actual original generated entity
files through unchanged derives and the complete C++ facade, then compares real
descriptors and field policies. Its twelve selected models cover all repaired
categories, guarded/block match arms, nested indexes, rest-tail behavior, try
blocks and an empty-mask continuation. All **12/12** pairs compile, execute and
match, retaining **36 descriptors / 168 field-policy tuples**. Both observer
bodies remain unchanged; only named model inputs are appended.

The [prior compiled input join](parity/audit_expression_patterns_prior_compiled_inputs_5_0_6.json.gz)
preserves all 29 previously successful compiler/execution pairs and their exact
receipts. Their [current complete-tree reproduction](parity/audit_expression_patterns_prior_reproduction_5_0_6.json.gz)
must reproduce every original/C++ generated byte and file set before retaining
those earlier results. This reproduction is terminal and complete; it does not
claim 29 fresh compiler executions. The 29 retained and twelve new configurations
join **123 descriptors / 574 field-policy tuples** with their distinct scopes.

The [all-SDK gate](parity/audit_expression_patterns_sdk_refresh_5_0_6.json.gz)
passes **78 SDKs / 3,457 generated files**, preserving all **2,201 protected
artifacts**. It permits fingerprint-only updates after complete body equality. The
[native archive snapshot](parity/audit_expression_patterns_native_archives_before_5_0_6.json.gz)
and [installed checks](parity/audit_expression_patterns_installed_regressions_5_0_6.json.gz)
retain current implementation identities and actual API/audit/protected-query
regressions. **Native4/imported3 pass**, both final builds report no work and
all six existing archives retain their hashes. The
[terminal scoped qualification](parity/audit_expression_patterns_qualification_5_0_6.json.gz)
joins actual source/compiler/artifact/consumer evidence. Source dispositions
remain **39 accepted / 83 pending / 2 excluded_external**; all seventeen
capability statuses are unchanged. Native registry741 is a discovered count,
not a current full-suite pass. No C++ runtime, allocator, ownership, privacy
algorithm, dependency, sanitizer/MIPS/full-suite or physical-device change/run.

## Reproduction

Use fresh output paths; the observation command intentionally retains the four
pending differences:

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_audit_metadata_sequences_5_0_6.py \
    --extra-cases tests/parity/audit_expression_operands_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_grammar_next_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_paths_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_delimiter_literals_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_patterns_5_0_6_inputs.json \
    --extra-cases tests/parity/audit_expression_pattern_boundaries_5_0_6_inputs.json \
    --output /tmp/teaql_audit_expression_patterns_fresh_5_0_6.json.gz
python3 scripts/verify_audit_expression_regressions_5_0_6.py \
    --before docs/parity/audit_expression_patterns_before_5_0_6.json.gz \
    --current /tmp/teaql_audit_expression_patterns_fresh_5_0_6.json.gz \
    --output /tmp/teaql_audit_expression_patterns_regressions_fresh_5_0_6.json.gz
```

Process success is observation completion. The parser's `complete` remains false
while differences exist; a zero-regression guard is not complete AP02/G14/G17
or P0-P7 acceptance.

Reproduce the twelve fresh compiler pairs separately:

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/audit_generated_privacy_contracts_5_0_6.py \
    --cases expression_if_let expression_while expression_while_let \
    expression_for_pattern expression_match_guard expression_match_block_arms \
    expression_struct_fields expression_struct_rest_tail expression_let \
    expression_nested_struct_index expression_pattern_mask expression_try_block \
    --require-match --output /tmp/teaql_audit_expression_patterns_compiled_fresh_5_0_6.json.gz
```
