# Expression operator and trailer child rules against Rust 5.0.6

Status: OP01-OP08 are installed and qualified for the recorded source matrix,
permanent regression and generated descriptor/privacy callers. Complete AP02 and
G14 remain partial.

The references are runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`. Source execution checks
the locked `syn` 2.0.118 crate checksum and archived/unpacked source equality.
The inspected owners include `expr.rs`, `precedence.rs`, `op.rs`, `path.rs` and
`ty.rs`; the existing observer includes the unchanged original attribute parser.

## Source responsibilities

These rules govern how an ignored Rust expression ends before subsequent
metadata. A nested parsing error preserves preceding assignments; a successfully
parsed non-string expression supplied to a string setting triggers a separate
refusal. Collapsing these paths changes names, routes and privacy metadata.

| ID | Source responsibility and Java correction or preserved contract |
| --- | --- |
| OP01 | `parse_expr`, `parse_binop_rhs` and `Precedence::of_binop` retain all 28 binary operators, assignment associativity, non-chainable comparisons and range-LHS stopping. The matrix covers all 32-by-32 pairs including assignment, cast and both modern range forms. `expr_let` parses its initializer from comparison precedence. |
| OP02 | Type-path angle arguments are not selected on `<=` or `<<=`. A type-path macro requires `!` rather than `!=`. Casts followed by these operators retain the actual expression boundary instead of entering malformed type syntax. |
| OP03 | `trailer_helper` requires method-call parentheses after named-member turbofish arguments. A generic-looking field followed by no call is not silently accepted. |
| OP04 | Numeric fields and `.await` stop before a following `::`. They retain the distinction between successful partial expression parsing and a nested parse error, including string-valued metadata consumers. |
| OP05 | `unary_expr` recognizes raw addresses only when `raw` is immediately followed by `mut` or `const`. Otherwise `raw` is an ordinary referenced value and its field/call/index/try/turbofish trailers remain available. |
| OP06 | `Expr::peek` determines optional jump values using source token categories and operator exclusions. Return/yield/break values restore ordinary struct parsing; break retains its special leading-brace restriction in a condition. Become requires an ordinary expression. |
| OP07 | `expr_break` rejects an unparenthesized labeled-loop value before string decoding. That error must not become a later non-string panic. |
| OP08 | `trailer_expr` rejects outer attributes on a prefix range while permitting the distinct parenthesized, unary and bounded-range cases. Attribute presence and range kind are checked at the same expression level. |

The code retains the source's three layers: expression dispatch, trailers/unary
forms and precedence-driven infix parsing. The source's explicit ambiguities and
error boundaries are not replaced with operator-name heuristics.

## Original differences and isolated correction

The [initial corpus](../tests/parity/audit_operator_contract_inputs_5_0_6.json)
contains 6,021 new observations. It covers all operator pairs in bare, grouped,
local, condition and call contexts, plus string-setting consumers for relevant
unary/trailer/attribute branches. The
[preceding parser](parity/audit_operator_initial_5_0_6.json.gz) reproduces 344
differences among 21,493 complete observations; all prior 15,472 results remain
unchanged.

The expanded [11,522-case corpus](../tests/parity/audit_operator_complete_contract_inputs_5_0_6.json)
adds jump-start/operator boundaries, labeled values, method trailer combinations,
struct-context propagation and numeric/string metadata consumers. The
[expanded preceding parser](parity/audit_operator_complete_before_5_0_6.json.gz)
reproduces **2,368 differences**. The first expanded candidate retains four
actual differences at the labeled-break/string-decoding boundary; its receipt
and source are preserved. An earlier proposal also contains an unreachable extra
backslash in its expression-start character set; the final source uses the exact
set and has its own execution.

The [final isolated source](parity/audit_operator_final_proposal_5_0_6.json.gz)
and [source replay](parity/audit_operator_final_candidate_5_0_6.json.gz) match
**26,994/26,994** complete metadata observations and the unchanged 270 AST
witnesses. The [new/old regression gate](parity/audit_operator_complete_candidate_regression_5_0_6.json.gz)
passes **124/124** Java tests with the candidate; the preceding parser fails only
the new test, reproducing all 2,368 differences while preserving the other 123.
Every new expectation comes from unchanged original Rust execution.

## Installed verification

The [installed source replay](parity/audit_operator_complete_installed_metadata_5_0_6.json.gz)
matches **26,994/26,994** observations: 24,936 metadata effects and 2,058 checked
refusals. Its complete focused Java suite passes **124/124** tests. The independent
[grammar replay](parity/audit_operator_complete_installed_grammar_5_0_6.json.gz)
also preserves the 270 AST witnesses and every retained source expectation.

Twelve [fresh original/generated C++ caller pairs](parity/audit_operator_complete_installed_callers_5_0_6.json.gz)
match 36 entity descriptors and 168 privacy classifications. They include the
corrected method-call, raw-address, attributed-range, cast/operator, jump-value
and labeled-break boundaries, with a null default PMR resource and actual
`UserContext` destruction. The [sixteen comparison controls](parity/audit_operator_complete_installed_caller_controls_5_0_6.json.gz)
reject Boolean/number substitutions, missing or extra fields, partial results
and duplicate rows. These controls execute the actual current and archived
comparison expressions against the fresh outputs.

Both isolated and [installed 80-SDK reproduction](parity/audit_operator_complete_installed_sdks_5_0_6.json.gz)
preserve all 3,560 generated code, guidance and data files and 2,467 protected
artifacts. Each manifest changes exactly one parser-hash literal. The
[installation delta](parity/audit_operator_complete_installed_delta_5_0_6.json.gz)
checks those bytes and the four installed source changes explicitly.

The [current source guards](parity/audit_operator_complete_installed_source_guards_5_0_6.json.gz)
pass all 17 comparator, admission, inventory and provenance tests. The
[frontend gate](parity/audit_operator_complete_installed_gates_5_0_6.json.gz)
passes two native and two imported-module tests, pinned formatting and its check,
locked upstream/generator verification and module-path verification. All 1,813
C++ inputs retain their identities from the [complete native 893-test checkpoint](native_provenance_integration_5_0_6.md).
That full execution keeps its original parser identities; this Java-only
continuation has separate current metadata, caller and reproduction evidence.

The [milestone receipt](parity/audit_operator_complete_milestone_5_0_6.json.gz)
joins those scopes and the exact source/provenance delta. Disposable probe
compilations are removed after their source, commands, outputs and results are
retained in compact receipts. Stable preset builds and shared caches are kept.

## Acceptance boundary

Full expression/statement/item dispatch, remaining type/path/binder/pattern and
lexical dependencies, every model configuration and complete metadata callers
remain AP02/G14 exits. Source observations and descriptor/privacy callers do not
independently qualify protected SQL or whole application operations. G13/SQ02,
G17 and full P0-P7 signoff remain open, with 14 qualified / 3 partial capabilities.
Runtime source dispositions and the three approved compatibility exceptions are
unchanged.
