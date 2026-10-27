# Generic and bound child rules against Rust 5.0.6

Status: GB01-GB08 are installed and reconciled for the named source rules,
recorded metadata matrix and generated descriptor/privacy callers. Complete
AP02, G14 and the generator goal remain open.

The locked references remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`. The observer executes
the unchanged attribute parser with locked `syn` 2.0.118. The fixture records
the checked source identities for `generics.rs`, `path.rs`, `ty.rs`, `lit.rs`
and `parse.rs`; the source gate also verifies the original crate archive.

## Source responsibilities

Ignored metadata expressions still have syntax. Their parsing determines
whether subsequent entity, table, route and privacy assignments take effect.
The ordinary callback can retain assignments despite an unexpected child-buffer
error. A string passed to `json_type` instead requires a complete Type parse.
These two callers therefore need distinct completion rules.

| ID | Source responsibility and shared adapter correction or preserved rule |
| --- | --- |
| GB01 | `Generics::parse` explicitly accepts a bare `_` parameter, including outer attributes and comma placement, without bounds or a default. The six declaration consumers now select that rule. `GenericParam::parse`, used by higher-ranked binders, retains its separate rejection of `_`. |
| GB02 | Generic parameters retain source-specific lifetime, type and const dispatch, attributes, bounds, defaults and separators. The adapter does not add a lifetime-first ordering restriction or generalize the special `_` declaration branch to `_:Trait` or `_=T`. |
| GB03 | Const defaults and arguments reuse the shared numeric lexer. A following block or line comment is trivia rather than part of the number. Integer bases, suffixes, fractional/exponent forms, negative numeric literals and large unevaluated values retain syntactic interpretation without numeric range evaluation. |
| GB04 | `const_argument` distinguishes literals, a single identifier and a braced expression block. A negative string, negative identifier or unbraced qualified constant path does not gain numeric-literal acceptance. Generic associated constants and types retain their separate dispatch. |
| GB05 | `TypeParamBound::parse_single` parses a parenthesized trait-bound prefix without explicitly requiring its child buffer to be empty. Ordinary metadata parsing now preserves that callback behavior, including later assignments. The strict encoded Type caller rejects unconsumed bound tokens. |
| GB06 | The strict Type mode propagates into semantic child cursors for blocks, struct expressions, outer attributes and inner attributes. Nested const expressions, closures, local item bounds and attribute values cannot accidentally switch a `json_type` parse back to ordinary metadata completion. |
| GB07 | Precise captures retain their own permission and parameter rules: lifetime, identifier and `Self` are distinct from `_`, `self` and qualified paths. Const-bound permissions remain owned by each bound consumer, including the previously reconciled trait-alias restriction. |
| GB08 | `choose_generics_over_qpath` retains its independent lookahead. `<_>` alone does not select declaration generics in an impl, while `<#[a] _>` does. Empty, lifetime, attributed, bounded, defaulted and const parameters retain source selection; associated argument names, bounds, type values and const values keep their existing responsibilities. |

The correction stays in the generator's shared metadata cursor. It introduces
no deployed C++ dependency, ownership mechanism, public API or module change.
Parsed ignored expressions are never evaluated.

## Source comparison

The [initial matrix](../tests/parity/audit_generic_child_contract_inputs_5_0_6.json)
contains 583 shapes and 4,893 new contracts. The
[preceding parser](parity/audit_generic_child_before_5_0_6.json.gz) reproduces
659 differences; the [candidate](parity/audit_generic_child_candidate_5_0_6.json.gz)
matches all 55,327 original/current observations.

The [expanded matrix](../tests/parity/audit_generic_complete_contract_inputs_5_0_6.json)
contains **903 shapes / 7,607 new contracts**: eight ordinary metadata contexts
per shape and 383 additional encoded Type cases. It adds comment boundaries,
declaration/binder ownership, nested strict completion and qualified-path
selection controls. The
[preceding parser](parity/audit_generic_complete_before_5_0_6.json.gz) reproduces
**2,371 differences**, including 83 encoded-Type observations. The
[qualified candidate](parity/audit_generic_complete_candidate_5_0_6.json.gz)
matches **58,041/58,041** outcomes without changing prior expectations or the
270 retained AST witnesses. The exact
[source proposal](parity/audit_generic_child_proposal_5_0_6.json.gz) and both
comparison rounds remain preserved.

## Installed verification

The permanent producer corpus keeps all 7,607 new expectations from unchanged
Rust execution. The [candidate/preceding Java regression](parity/audit_generic_complete_candidate_regression_5_0_6.json.gz)
passes **128/128** tests with the candidate; the preceding parser fails only
the new test, reproducing all 2,371 differences.

The [installed source replay](parity/audit_generic_complete_installed_metadata_5_0_6.json.gz)
matches **58,041/58,041** observations: 45,855 metadata effects and 12,186 checked
refusals, with **128/128** Java tests. The independent
[AST/source replay](parity/audit_generic_complete_installed_grammar_5_0_6.json.gz)
preserves every retained source outcome and all 270 AST witnesses.

Twelve [fresh original/installed C++ caller pairs](parity/audit_generic_complete_installed_callers_5_0_6.json.gz)
match 36 entity descriptors and 168 privacy classifications. They cover
declaration parameters, binder rejection, four const-literal consumers,
parenthesized bounds and nested attributes. Native execution disables default
PMR allocation and actually destroys `UserContext`. The
[sixteen strict comparison controls](parity/audit_generic_complete_installed_caller_controls_5_0_6.json.gz)
reject type, cardinality and shape corruptions through actual current and
archived comparison expressions. Encoded Type cases qualify metadata parsing;
these callers do not establish execution of arbitrary encoded JSON types.

Isolated and [installed 80-SDK reproduction](parity/audit_generic_complete_installed_sdks_5_0_6.json.gz)
preserve all 3,560 generated files and 2,467 protected artifacts. Each manifest
changes exactly one parser-hash literal. The
[installation delta](parity/audit_generic_complete_installed_delta_5_0_6.json.gz)
checks those bytes and the four source/test/corpus/oracle changes. All 1,813
C++ inputs retain their [frozen full native 893-test identities](native_provenance_integration_5_0_6.md).
That full execution keeps its original parser identities; current Java,
source, caller and reproduction receipts supply the explicit continuation.

All 17 [current source/comparison/admission/inventory guards](parity/audit_generic_complete_installed_source_guards_5_0_6.json.gz)
pass without skips. The [frontend gate](parity/audit_generic_complete_installed_gates_5_0_6.json.gz)
passes two native and two imported-module tests, pinned formatting and its
check, locked source verification and module-path verification.

The [milestone](parity/audit_generic_complete_milestone_5_0_6.json.gz) joins these
finite scopes and exact input deltas. The
[cleanup receipt](parity/audit_generic_complete_cleanup_5_0_6.json.gz) records
removed task-owned compilation and generation outputs; stable presets, sources,
fixtures and shared caches remain retained.

## Remaining scope

The named direct generic/bound responsibilities join the preceding type,
path, pattern, operator, block, statement and item scopes. Complete delegated
grammar and lexical coverage, model configurations, metadata consumers and
protected execution still require their own source review and evidence.
Complete SQ02 provenance, G13/G14/G17 and P0-P7 signoff remain open. Capability
status stays **14 qualified / 3 partial**. Approved JSON, quoted-FK loading and
invalid wire-profile exceptions retain their existing boundaries.
