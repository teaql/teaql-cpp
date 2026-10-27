# Nested item child rules against Rust 5.0.6

Status: IT01-IT08 are installed and reconciled for the named item rules,
recorded original-source matrix and generated descriptor/privacy callers.
Complete AP02, G14 and the generator goal remain open.

The references remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`. The observer executes
the unchanged attribute parser with locked `syn` 2.0.118. Its crate checksum and
archived/unpacked `item.rs`, `derive.rs`, `data.rs`, `restriction.rs`,
`generics.rs` and `path.rs` are checked before execution.

## Source responsibilities

Ignored metadata expressions can contain ordinary, trait, impl and foreign
items. A nested parsing error preserves preceding assignments; a successfully
parsed non-string expression supplied to a string setting causes a separate
refusal. Item grammar therefore affects subsequent identity, route and privacy
metadata even though these item bodies are never evaluated.

| ID | Source responsibility and shared adapter correction or preserved rule |
| --- | --- |
| IT01 | `parse_fn_arg_or_variadic` has an explicit legacy branch: an identifier immediately followed by angle arguments is parsed as a type without requiring a parameter name or colon. The adapter preserves that narrow source branch, including nested/raw generic types and argument ordering. Other unnamed forms are not generalized into it. |
| IT02 | `parse_receiver_begin` rejects receiver lookahead when `self` is followed by `::`. The ordinary pattern parser then owns qualified `self::...` and reference patterns. The adapter performs that lookahead before claiming a receiver, while retaining first/duplicate receiver and reference/colon restrictions. |
| IT03 | `default` is a declaration modifier in trait/impl contexts, has a macro exclusion in impl/root item dispatch, and is not a foreign-item modifier. The adapter distinguishes `default!` from declarations and rejects foreign `default fn/static/type` forms. Trivia and inherited visibility retain their source behavior. |
| IT04 | `Field::parse_named` treats an unnamed `union` field as an anonymous aggregate only when the following token is a brace. Otherwise `union`, its generic arguments and qualified path belong to ordinary type parsing. Anonymous `struct` keeps its separate source rule. |
| IT05 | `data_struct` permits tuple fields only when no preceding where clause was parsed. The adapter captures clause presence after trivia; a parenthesized trait argument consumed inside a bound remains part of the clause, not a tuple-field delimiter. Named and unit forms keep their existing behavior. |
| IT06 | `WherePredicate` separates lifetime predicates from type predicates. Lifetime bounds accept lifetimes and their own terminators; type predicates own an optional higher-ranked binder before an arbitrary bounded type. The adapter no longer parses a parenthesized trait as a lifetime bound or restricts the binder to trait-path types. |
| IT07 | `parse_rest_of_trait_alias` disallows const-modified bounds. Ordinary trait supertraits, associated type bounds and generic bounds retain their separately permitted const syntax. The adapter passes that permission explicitly instead of changing every bound consumer. |
| IT08 | The four item containers retain independent signature/body/semicolon, constant/static, alias/where-position, macro/visibility, data-field/discriminant, module and use-tree rules, including source `Verbatim` acceptance. Existing helpers preserve these responsibilities; source acceptance does not establish Rust type checking or execute ignored item bodies. |

The changes stay in the shared metadata cursor. Generated methods do not emit
another parser, and no deployed C++ dependency, runtime ownership mechanism or
module export changes.

## Source comparison and candidate

The [initial matrix](../tests/parity/audit_item_contract_inputs_5_0_6.json)
contains 683 shapes in eight numeric/string metadata contexts, for 5,464 new
contracts. The [preceding parser](parity/audit_item_before_5_0_6.json.gz)
reproduces 496 differences while preserving all prior 41,658 observations. The
[initial candidate](parity/audit_item_candidate_5_0_6.json.gz) matches the complete
47,122-case matrix.

The [expanded matrix](../tests/parity/audit_item_complete_contract_inputs_5_0_6.json)
adds trivia and ambiguous argument/default/union/where/bound controls. Its 1,033
shapes produce 8,264 new contracts. The [preceding parser](parity/audit_item_complete_before_5_0_6.json.gz)
reproduces 1,912 differences; the [intermediate candidate](parity/audit_item_complete_candidate_5_0_6.json.gz)
retains 40 differences at lifetime-bound and trait-alias permission boundaries.
Those observations and proposals remain preserved.

The [final matrix](../tests/parity/audit_item_final_contract_inputs_5_0_6.json)
adds lifetime/type predicates and higher-ranked binders, reaching **1,097 shapes /
8,776 new contracts**. The [final preceding parser](parity/audit_item_final_before_5_0_6.json.gz)
reproduces **2,200 differences**. The [qualified candidate](parity/audit_item_qualified_proposal_5_0_6.json.gz)
matches **50,434/50,434** [original/current outcomes](parity/audit_item_final_candidate_5_0_6.json.gz),
with no retained-source or AST-witness differences.

## Installed verification

The permanent producer corpus retains all 8,776 new expectations from unchanged
Rust execution. The [candidate/preceding Java regression](parity/audit_item_complete_candidate_regression_5_0_6.json.gz)
passes **127/127** tests with the candidate. The preceding parser fails only the
new test, reproducing all 2,200 differences while preserving the other 126 tests.

The [installed source replay](parity/audit_item_complete_installed_metadata_5_0_6.json.gz)
matches **50,434/50,434** metadata outcomes: 40,980 metadata effects and 9,454
checked refusals. Its focused Java suite also passes **127/127** tests. The
independent [AST/source gate](parity/audit_item_complete_installed_grammar_5_0_6.json.gz)
preserves every retained expectation and the unchanged 270 AST witnesses.

Twelve [fresh original/installed C++ caller pairs](parity/audit_item_complete_installed_callers_5_0_6.json.gz)
match 36 entity descriptors and 168 privacy classifications. They exercise
legacy/qualified arguments, default macros and foreign modifier rejection,
anonymous fields, tuple/lifetime/binder boundaries, and distinct trait-alias
permissions. Execution disables default PMR allocation and actually destroys
`UserContext`. The [sixteen strict comparison controls](parity/audit_item_complete_installed_caller_controls_5_0_6.json.gz)
reject Boolean/number substitutions, missing or extra fields, partial rows and
duplicates through the actual current and archived comparison expressions.

Both isolated and [installed 80-SDK reproduction](parity/audit_item_complete_installed_sdks_5_0_6.json.gz)
preserve all 3,560 generated code, guidance and data files and 2,467 protected
artifacts. Each manifest changes exactly one parser-hash literal. The
[installation delta](parity/audit_item_complete_installed_delta_5_0_6.json.gz)
checks those bytes and four source changes explicitly. All 1,813 C++ inputs
retain their identities from the [frozen complete native 893-test checkpoint](native_provenance_integration_5_0_6.md),
whose full execution keeps its original parser identities. The current Java,
source, caller and reproduction evidence retains its own scope.

All 17 [current source/comparator/admission/inventory guards](parity/audit_item_complete_installed_source_guards_5_0_6.json.gz)
pass without skips or failures. The [frontend gate](parity/audit_item_complete_installed_gates_5_0_6.json.gz)
passes two native and two imported-module tests, pinned formatting and its check,
locked upstream/generator verification and module-path verification. The named
direct item responsibilities are reconciled; the delegated grammar, complete
model configurations and protected execution retain their independent
requirements.

The [milestone receipt](parity/audit_item_complete_milestone_5_0_6.json.gz)
joins those scopes and exact source/provenance changes without promoting whole
goal acceptance. The [cleanup receipt](parity/audit_item_complete_cleanup_5_0_6.json.gz)
verifies removal of fourteen task-owned probe directories containing 860,819,504
measured file bytes after their processes exited. Sources, fixtures, compact
evidence, stable preset builds and shared caches remain retained.

## Acceptance boundary

These results reconcile the named direct item rules and recorded callers, while
preserving ST01-ST06 and the preceding child-rule contracts. Remaining delegated
expression/type/path/pattern/binder and lexical dependencies, model configurations
and complete metadata/protected-execution callers remain independent AP02/G14
requirements. G13/SQ02 provenance, G17 and complete P0-P7 signoff stay open.
Capability status remains **14 qualified / 3 partial**, with runtime source
dispositions and the three approved compatibility exceptions unchanged.
