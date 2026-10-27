# Statement child rules against Rust 5.0.6

Status: ST01-ST06 are installed and reconciled for the reviewed statement rules,
recorded original-source matrix and generated descriptor/privacy callers.
Complete AP02, G14 and the generator goal remain open.

The references are runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`. Source execution uses
the unchanged attribute parser and locked `syn` 2.0.118. Its crate checksum and
archived/unpacked `stmt.rs`, `classify.rs`, `expr.rs`, `item.rs` and `path.rs`
members are checked before execution.

## Source responsibilities

An ignored expression can contain a block of ordinary Rust statements. A nested
parse error stops later metadata processing; a successful non-string expression
given to a string setting instead produces a checked refusal. Mistaking an
operator for a macro can therefore alter identity, route and privacy metadata.

| ID | Source responsibility and adapter disposition |
| --- | --- |
| ST01 | `parse_stmt` looks ahead for `Token![!]`, which does not match the compound `!=` token. `statementMacro` previously consumed that operator's first character and failed before ordinary expression parsing. Its shared lookahead now leaves `!=` intact. Separated or comment-delimited `!`/`=` tokens retain their own source behavior. |
| ST02 | `Block::parse_within` admits empty semicolons, complete statements and one trailing expression. A nonterminal expression requiring a semicolon cannot become the next statement implicitly. The existing statement loop retains these boundaries. |
| ST03 | `parse_stmt` chooses brace/named macros, local declarations, nested items and earlier-boundary expressions using distinct lookaheads. The reviewed item/closure/path controls preserve these choices through the existing item and expression helpers. Nested item child grammar remains a separate dependency. |
| ST04 | `stmt_mac` and `ItemMacro` preserve opaque token groups, delimiter-specific semicolons, named macros and the dot/question-mark fallback to an expression. A brace macro followed by another statement differs from a macro with a postfix expression. |
| ST05 | `stmt_local` uses one pattern, an optional type, optional initializer, conditional `else` block and required semicolon. `expr_trailing_brace` governs divergence eligibility; parentheses change that classification. Existing plain-block permissions are retained. Pattern/type children remain independently qualified dependencies. |
| ST06 | `stmt_expr`, the earlier-boundary expression entry and `requires_semi_to_be_stmt` keep block expressions, macro delimiters and ordinary expressions distinct. Outer statement attributes are consumed before that expression entry. Attribute attachment does not invent another metadata evaluation path. |

The application metadata path reaches statements through `Block::parse_within`.
The standalone `Stmt::parse` entry has a different no-semicolon permission and
is not an application metadata entry. None-delimited expression/type groups
retain the existing explicit textual-input exclusion.

## Reproduced defect and candidate

The [initial source matrix](../tests/parity/audit_statement_contract_inputs_5_0_6.json)
contains 732 statement shapes in eight numeric/string metadata contexts, for
5,856 new contracts. It includes all 39 textual expression variants, all 32
operator forms, local pattern/initializer/divergence cases, macro delimiters and
tails, nested-item dispatch and empty/attributed statements. The
[preceding parser](parity/audit_statement_before_5_0_6.json.gz) reproduces 24
differences while preserving all previous 33,538 observations.

The [expanded matrix](../tests/parity/audit_statement_complete_contract_inputs_5_0_6.json)
adds qualified/raw/member/call left operands, joined versus separated/comment
bang tokens and local cast/divergence controls for all 14 textual type variants
plus additional trailing-brace types. It contains **1,015 shapes / 8,120 new
contracts**. The [expanded preceding parser](parity/audit_statement_complete_before_5_0_6.json.gz)
reproduces **216 differences**. The [candidate source](parity/audit_statement_proposal_5_0_6.json.gz)
changes one shared macro-lookahead predicate. Its [complete source replay](parity/audit_statement_complete_candidate_5_0_6.json.gz)
matches **41,658/41,658** metadata results, with no retained-source or AST-witness
differences.

## Installed verification

The permanent producer corpus retains all 8,120 new expectations from unchanged
Rust execution. The [candidate/preceding regression](parity/audit_statement_complete_candidate_regression_5_0_6.json.gz)
passes **126/126** Java tests with the correction. The preceding parser fails
only the new test, reproducing all 216 differences while preserving the other
125 tests.

The [installed source gate](parity/audit_statement_complete_installed_metadata_5_0_6.json.gz)
matches **41,658/41,658** metadata outcomes: 35,264 metadata effects and 6,394
checked refusals. Its focused Java suite also passes **126/126** tests. The
independent [AST/source replay](parity/audit_statement_complete_installed_grammar_5_0_6.json.gz)
retains every preceding source expectation and all 270 AST witnesses.

Twelve [fresh original/installed C++ caller pairs](parity/audit_statement_complete_installed_callers_5_0_6.json.gz)
match 36 entity descriptors and 168 privacy classifications. They include the
corrected ordinary, absolute, qualified, raw and attributed `!=` statements,
separated-bang refusal, macro/postfix termination and grouped local divergence.
Execution disables default PMR allocation and actually destroys `UserContext`.
The [sixteen strict comparison controls](parity/audit_statement_complete_installed_caller_controls_5_0_6.json.gz)
reject Boolean/number substitutions, missing/extra fields, partial rows and
duplicates through the actual current and archived comparison expressions.

Both isolated and [installed 80-SDK reproduction](parity/audit_statement_complete_installed_sdks_5_0_6.json.gz)
preserve all 3,560 generated code, guidance and data files and 2,467 protected
artifacts. Each manifest changes exactly one parser-hash literal. The
[installation delta](parity/audit_statement_complete_installed_delta_5_0_6.json.gz)
checks those bytes and the four source changes explicitly. All 1,813 C++ inputs
retain their identities from the [frozen complete native 893-test checkpoint](native_provenance_integration_5_0_6.md),
whose full execution keeps its original parser identities. Current Java/source,
caller and reproduction evidence has its own scope.

All 17 [current source/comparator/admission/inventory guards](parity/audit_statement_complete_installed_source_guards_5_0_6.json.gz)
pass without skips or failures. The [frontend gate](parity/audit_statement_complete_installed_gates_5_0_6.json.gz)
passes two native and two imported-module tests, pinned formatting and its check,
locked upstream/generator verification and module-path verification. These results
reconcile the named direct statement responsibilities without promoting their
independently delegated child grammar, metadata configurations or whole
protected execution.

The [milestone receipt](parity/audit_statement_complete_milestone_5_0_6.json.gz)
joins the current scopes, exact source/provenance delta and unchanged verdicts.
The [cleanup receipt](parity/audit_statement_complete_cleanup_5_0_6.json.gz) verifies
removal of eleven task-owned probe directories containing 715,392,888 measured
file bytes after their processes exited. Sources, fixtures, compact evidence,
stable preset builds and shared caches are retained.

## Acceptance boundary

This reconciles the named statement rules and their recorded callers. It does
not accept every delegated expression/type/path/pattern/item or lexical child,
every model configuration or complete metadata/protected-SQL callers. Full
AP02/G14, G13/SQ02 provenance, G17 and P0-P7 signoff retain their independent
requirements. Capability status stays **14 qualified / 3 partial**. Runtime
source dispositions and the three approved compatibility exceptions are
unchanged.
