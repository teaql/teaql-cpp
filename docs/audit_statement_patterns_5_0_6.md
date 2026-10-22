# Audit local-statement parsing continuation (Rust 5.0.6)

Status: temporary producer candidate only. The actual cpp_support workspace is
frozen by the running full native integration and has not been modified for this
continuation. G14/AP02 and the overall parity goal remain partial.

References remain Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, generator
`45e1572c4956ca963410d8ea436ead3c8857087a`, and the original parser's locked
`syn 2.0.118`. This work preserves the complete container-metadata responsibility,
including the later effects of an ignored expression's parse result.

## Source contract and demonstrated discrepancy

The actual source parser consumes a complete expression before deciding whether
it is a literal. Successful nonliterals are ignored; a nested parse failure stops
later attributes while retaining prior assignments. It does not evaluate the
expression or expand a macro body.

For a local declaration, the locked syn implementation parses one pattern,
an optional type, an optional ordinary expression initializer and optional
let-else block, then requires a semicolon. The source separately distinguishes
initializers whose expression ends in a brace before accepting let-else.
Ordinary local initialization and statement-expression parsing use different
expression-boundary rules.

The current Java cursor's local-binding branch instead scans nonempty tokens
until an equals sign or semicolon. Its later permissive block fallback can admit
invalid patterns/types or trailing initializer tokens. The
[64 actual original/current observations](parity/audit_statement_patterns_initial_5_0_6.json.gz)
reproduce **22 differences**, across both audit-limit keys. Examples include a
plus sign used as a pattern, two adjacent binding names, a missing type and an
unparenthesized top-level or-pattern. Each input has an earlier limit of 17,
a discarded expression, and later mask/limit assignments. Rust retains limit 17
with no mask declaration; the current producer applies the later limit 21 and
mask declaration. Those are complete parsed-effect differences, not only a
boolean syntax classification.

## Temporary candidate and regression scope

The [temporary candidate](parity/audit_statement_patterns_candidate_5_0_6.json.gz)
reuses existing pattern/type parsers, parses local initializers with the normal
expression rule and requires the local semicolon. Const and macro patterns retain
their distinct syntax. Tracking the last significant token preserves let-else
classification across whitespace/comments. Nonlocal statement/item handling
retains its earlier behavior and remains a full-grammar obligation.

All **1,583** compared outcomes agree: the 1,519 unchanged retained container
inputs plus these 64 controls. The candidate yields **22 improvements and zero
regressions** under that finite matrix. For source-accepted inputs every parsed
effect is compared exactly. For original parser rejections, the existing
cross-language rejection contract is preserved; Rust and Java error text/stages
are not relabeled as identical.

The candidate is compiled from an owned copy of the current Java source and
existing unmodified helpers. It does not run Maven or replace producer classes.
The original observer builds with the exact retained Cargo lock in an owned
nonincremental target. Source hashes and cleanup remain in the receipts. Initial
owned probe outputs (48,096,297 bytes) and temporary candidate classes/source
(132,729 bytes) are removed after retaining compact evidence and the complete
candidate source.

## Further boundary qualification

The [additional boundary experiment](parity/audit_statement_boundaries_initial_5_0_6.json.gz)
is running against original expression, pattern and type forms beyond those
first controls. It compares the installed and proposed parsers separately to
avoid accepting an improvement that rejects another source-accepted form.
A live receipt is not a final result.

Installation must wait for the frozen native epoch to finish. It then needs
producer regression tests, source/candidate comparisons, generated metadata
compiler/use checks and strict artifact reproduction at the resulting checkpoint.
Neither the temporary matrix nor the ongoing native integration promotes AP02,
G14 or P0-P7 by itself. The unanswered invalid wire-naming and quoted-FK behavior
questions remain independent; the approved JSON payload exception does not
cover them.

## Additional boundary result and revised temporary candidate

The additional eighty original/installed/candidate observations are now terminal.
They exposed **four regressions** in the first temporary proposal: `become`
initializers and higher-ranked trait-bound types, each under both audit keys.
The [boundary receipt](parity/audit_statement_boundaries_initial_5_0_6.json.gz)
retains these differences without changing its results after a follow-up.

The locked syn expression parser requires a complete expression after `become`.
Its type-bound parser retains bound generic parameters before the trait path.
The temporary follow-up adds the former expression branch and reuses the cursor's
existing bound-generic parser for both standalone and trait-bound type forms.
No application expression is evaluated and no C++ runtime feature is introduced.

The [revised temporary comparison](parity/audit_statement_patterns_candidate_v2_5_0_6.json.gz)
now matches all **1,663** observations: 1,519 retained inputs, 64 statement
controls and 80 additional boundary inputs. It provides **44 improvements and
zero regressions** against the installed implementation under that combined
matrix. Each receipt retains the exact source outputs, full Java candidate,
commands and cleanup. The revised candidate is still uninstalled; producer
source hashes remain unchanged. Broader statement/item/type/lexical grammar and
all existing AP02 identity obligations continue to apply.

Before installation, retain both the first candidate's narrow success and the
later four-regression result, add permanent deterministic producer regressions,
and perform current source/compiler/artifact gates. The current frozen native
epoch qualifies its own original inputs, not this proposed generator revision.
A passing temporary matrix is not a producer release or capability promotion.

## Block items and function-body continuation

The [120 original block-item observations](parity/audit_block_items_initial_5_0_6.json.gz)
add declaration bodies and statement-semicolon boundaries. The preceding
candidate differs on **52** inputs; it has no regressions relative to the
installed producer on this new matrix. Function/type/module/trait/impl bodies,
imports, macro statement boundaries and attributed local declarations remain
observable requirements, rather than opaque token buffers.

Actual source results also retain its permissive AST boundaries: a bodyless
function declaration, a declaration-only type alias, an empty where bound and a
brace macro named `macro_rules` are consumed successfully in these discarded
expressions. Candidate labels do not establish syntax validity; only source
outputs determine the expected metadata effects.

The [function/attribute temporary follow-up](parity/audit_block_functions_candidate_5_0_6.json.gz)
adds function-name/parameter/type/where/body parsing, consumes outer statement
attributes before local declarations and retains initial inner block attributes.
It reuses the existing nested block and type mechanisms. The combined matrix
now matches **1,741/1,783**, with **54 improvements and zero regressions** against
the installed producer. The **42 remaining differences** are explicitly retained;
this candidate's complete-parity flag remains false.

Remaining item/semicolon cases must be implemented and requalified. No parser
fallback is accepted as a substitute for the locked source's body validation.
No temporary source/class outputs are retained outside the compact receipts,
and no frozen producer source was edited. All earlier candidate successes and
subsequent failures keep their original checkpoints and scope.


## Data/container items and statement boundaries

The [data-item candidate](parity/audit_block_data_items_candidate_5_0_6.json.gz)
adds constants/statics, named/tuple/unit structures, enumerations and aliases,
retaining the source AST parser's declaration-only forms. It matches
**1,757/1,783**, with **70 improvements and zero regressions** relative to the
installed producer. Its 26 remaining differences are preserved in that receipt.

The [container-item candidate](parity/audit_block_containers_candidate_5_0_6.json.gz)
adds distinct module, trait, implementation and foreign declaration lists, plus
imports and item macros. Ordinary statements cannot substitute for an item in
those lists. It matches **1,773/1,783**, with **86 improvements and zero
regressions**. Ten macro/semicolon differences remain at that checkpoint.

The [macro/semicolon candidate](parity/audit_block_semicolons_candidate_5_0_6.json.gz)
then distinguishes anonymous brace macro statements from expression macros,
retains dot/question-mark continuation, validates named macro boundaries and
requires semicolons between ordinary non-block expressions. It matches all
**1,783** retained observations, with **96 improvements and zero regressions**
relative to the installed producer. The candidate receipt's complete flag
applies only to this finite comparison, not to AP02 or the Rust grammar.

All three candidates were compiled and executed to successful terminal exits
from temporary Java source copies. Their owned source/class outputs were
removed; producer source hashes remain unchanged. The latest receipt retains
the full proposed Java source and removes 156,330 bytes of disposable outputs.
No running native integration input was changed.

Explicitly unqualified paths remain: qualified/visible declaration dispatch,
union declarations, complete nested item rules, broader type/expression/token
boundaries and the original complete AP02 identity obligations. The latest
candidate intentionally retains the previous permissive fallback for those
unparsed qualified items; that fallback is not accepted as source parity.
Installation, permanent regressions and current compiler/generated-artifact
qualification remain pending until the native input freeze ends.


## Qualified declaration dispatch and newly exposed regressions

The [174 additional source observations](parity/audit_qualified_items_initial_v2_5_0_6.json.gz)
cover visibility, ordered function qualifiers, unions, trait aliases, const
implementation headers, declaration-only constants/statics, inner item attributes
and nested declaration restrictions. They expose **98 differences**, including
**42 regressions** relative to the installed producer, in the preceding candidate.
The earlier finite 1,783-case success does not override these failures.

The initial source observation run succeeded, but its Java installed-probe launch
failed because the probe class is not installed in producer classes. That
[failed receipt](parity/audit_qualified_items_initial_5_0_6.json.gz) remains intact
with the failure disposition. The successful follow-up reuses only the completed
original observations and separately compiles the installed and proposed Java
parsers into owned class directories. No Maven output or producer class is
replaced.

The [qualified-dispatch follow-up](parity/audit_qualified_items_candidate_5_0_6.json.gz)
replaces the permissive qualified-item fallback with explicit item dispatch.
Functions consume qualifiers in source order. Module, trait, implementation and
foreign bodies retain their different allowed items and macro forms. The
candidate handles visibility, union fields, trait aliases/empty bounds,
const-implementation headers, generic-versus-qualified-path lookahead, source
verbatim declaration forms and initial inner attributes. It does not evaluate
ignored expressions or expand macro token groups.

All **1,957** retained outcomes now agree, with **160 improvements and zero
regressions** relative to the installed producer. The candidate is still
uninstalled, and both the preceding 98 differences and 42 regressions remain
recorded at their original checkpoint. Full type/pattern/expression/token and
nested item rules, complete AP02 identity evidence, permanent producer tests and
current compiler/artifact gates remain obligations. This finite result does not
qualify all Rust syntax, AP02, G14 or P0-P7.


## Nested declaration rules and current temporary checkpoint

The [186 nested-item observations](parity/audit_nested_item_rules_initial_5_0_6.json.gz)
expose **44 further differences**, including **18 regressions** relative to the
installed producer, beyond the previous finite 1,957-case success. They cover
receiver position/reference rules, variadic argument parsing, generic whitespace,
the declaration-specific position of alias where clauses, empty bounds,
restricted import/macro path names and nested root imports, restricted visibility
and the special module name `try`.

The [nested-rule follow-up](parity/audit_nested_item_rules_candidate_5_0_6.json.gz)
now matches all **2,143** retained observations, with **236 improvements and zero
regressions** relative to the installed producer. Receivers follow source argument
position rules, aliases retain the module/trait/implementation/foreign where-clause
contracts, imports retain their distinct root-group grammar, and module-style
paths validate their allowed identifier/keyword segments. A variadic argument
also preserves the locked parser's observed early-return boundary rather than
substituting later compiler validation for AST parsing.

The follow-up is still a temporary Java source copy. It does not modify or
replace the producer, a generated SDK, a C++ source or the running native epoch.
Both the initial 44 differences and 18 regressions remain recorded at their
original checkpoint. The retained complete flag applies to this finite matrix
only; full syntax/identity obligations and permanent/current producer qualification
remain open.

The [current read-only freeze audit](parity/native_service_graph_freeze_audit_followup_5_0_6.json.gz)
checks all 11,399 native execution inputs and exact reference commits after the
probes. No frozen input changed. Its controller and build child are still live;
this audit is not a successful full native build or CTest result. All owned probe
source/class/target directories from the successful follow-ups were removed.
