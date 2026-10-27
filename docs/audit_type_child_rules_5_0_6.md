# Audit type child rules against Rust 5.0.6

Status: TC01-TC06's corrections are installed and qualified in the recorded
scope. Current source metadata, Java regressions, SDK reproduction and scoped
privacy gates pass. Complete AP02 and G14 remain partial.
The active references are runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Source responsibilities

The preceding [variant inventory](audit_grammar_branch_review_5_0_6.md) exposed
the difference between a witnessed enum variant and its complete child rules.
This continuation inspects `syn` 2.0.118 `ty.rs::ambig_ty`,
`TypeBareFn::parse`, `parse_bare_fn_arg` and
`generics.rs::TypeParamBound::parse_single` / `TraitBound::do_parse`.
The locked crate checksum and archived/unpacked source equality are checked
before execution, including `generics.rs` and `path.rs` in this cohort.

| ID | Locked behavior and Java correction |
| --- | --- |
| TC01 | A parenthesized unqualified path or an eligible single bare trait bound can become an unprefixed trait-object bound list. A parenthesized explicit `dyn`, `dyn*` or `impl` type cannot. The cursor preserves that distinction instead of classifying every one-bound object as promotable. |
| TC02 | The special `(?Trait)` branch produces a trait object, so an enclosing parenthesis may promote it again. Its following bounds are consumed even in the source's `without_plus` path. Parenthesis-promotion loops require a bound after every `+`; ordinary bare bound lists retain their distinct trailing-`+` behavior. Bound-list state now records trailing punctuation. |
| TC03 | Parenthesized bounds parse a trait bound, not another arbitrary type-parameter bound. They do not admit recursively parenthesized bounds, lifetimes or precise captures through a general recursive shortcut. A shared trait-bound helper retains the source's const/binder/polarity restrictions. |
| TC04 | Function-style trait arguments are permitted only when the final path segment has no arguments. Earlier segments may have generic arguments. The cursor records final-segment argument state independently of whole-path macro eligibility. |
| TC05 | A first `mut self: Type` bare-function argument still consumes its type before being represented as source Verbatim. The preceding cursor skipped that type and stopped later metadata effects. Unnamed and later receiver restrictions retain their separate paths. |
| TC06 | All corrections apply both to ignored audit expressions and to the actual string-encoded reverse-relation `json_type` parser. Validating one entry point does not qualify the other. |

These are generator-side interpretation rules. No C++ runtime, public API,
ownership algorithm or deployed dependency changes.

## Observed differences and candidate qualification

The immutable fixtures retain progressively inspected source branches:

- [285 initial type cases](../tests/parity/audit_type_children_inputs_5_0_6.json)
  reproduce 36 metadata differences.
- [675 type/bound cases](../tests/parity/audit_type_bounds_inputs_5_0_6.json)
  reproduce 89 differences. The first candidate still differs on 31 inputs;
  its failure and removed disposable directories remain recorded.
- [750 expression-entry cases](../tests/parity/audit_type_rules_inputs_5_0_6.json)
  reproduce 127 differences. The corrected isolated candidate matches all
  9,344 observations, including the preceding 8,594 unchanged source results.
- [900 complete contracts](../tests/parity/audit_type_rule_contract_inputs_5_0_6.json)
  cover 150 source-derived shapes in five expression contexts and one encoded
  relation-type context. The [preceding parser](parity/audit_type_contract_initial_5_0_6.json.gz)
  reproduces 156 differences; the
  [candidate](parity/audit_type_contract_candidate_5_0_6.json.gz) matches all
  9,494 outcomes: 9,392 complete metadata effects and 102 checked source refusals.

Every expectation in the new permanent 900-case Java corpus comes from the
unchanged original attribute parser. The four preceding corpora are immutable.
The old parser independently
[fails only the new regression among 121 tests](parity/audit_type_child_previous_control_5_0_6.json.gz),
while preserving every source expectation. This is a required negative control,
not an unresolved failure of the installed runtime.

An early receipt-name collision was rejected before execution; the older
2026-10-22 type-rule experiments were preserved. The new cohort has distinct
receipt names and does not relabel those older prototypes.

## Installed source and actual callers

The [installed source replay](parity/audit_type_child_installed_source_5_0_6.json.gz)
matches all 9,494 observations and passes 121 Java tests. A separate
[AST/metadata replay](parity/audit_type_child_installed_grammar_5_0_6.json.gz)
matches those results and the unchanged 270 source variant witnesses.

The [twelve fresh original/native model pairs](parity/audit_type_child_callers_candidate_5_0_6.json.gz)
compile all 261 original Java sources, complete original entity files and the
complete native generated facades. They execute actual descriptor and field
privacy classification with default PMR disabled and actual UserContext
deletion, matching 36 descriptors and 168 field-policy tuples. This is
metadata/privacy classification, not full application or SQL
execution. Their candidate parser bytes are exactly the installed parser:
`04a5de3a42abfe48c5507692fa9370c50c82ebf8d5a2d1abe2c6d20d2c4ae4e4`.

The compiled-caller Python boundary still used raw equality. Its exact executed
source is retained before correction; all twelve real outputs also match under
the corrected strict JSON comparator. Fourteen effective corruptions now reject,
including twelve Boolean-to-number substitutions actually accepted by the
preceding comparison. The [strict replay](parity/audit_type_child_compiler_controls_5_0_6.json.gz)
executes both actual AST comparison expressions. It does not relabel the earlier
compiled run as execution of a newer verifier. Whole SQ02 provenance remains open.

## Artifact and final gates

The [fresh SDK reproduction](parity/audit_type_child_sdk_refresh_5_0_6.json.gz)
verifies all 80 active destinations: 3,560 generated files and 2,467 protected
artifacts remain byte-identical. Exactly the parser implementation fingerprint
changes in each of the 80 manifests. The independent prior-manifest snapshot
and final milestone verify every field and installed file rather than accepting
an unchanged file count.

The [final scoped gate](parity/audit_type_child_final_gates_5_0_6.json.gz) builds
the actual native and imported-module privacy consumers, then passes two tests
per frontend. Both exact test names and the successful test count are checked.
Pinned formatting and checking cover 1,808 handwritten C++ files; runtime and
generator reference guards and active module-path checks pass. All recorded C++
inputs remain byte-identical. This is not a new full native, sanitizer or MIPS
epoch; the preceding complete 877-test result retains its historical scope.

The first gate controller rejected the current CTest summary wording after both
native tests had passed. Its [failed receipt](parity/audit_type_child_gates_5_0_6.json.gz)
is preserved; the repaired controller checks the actual summary and both passed
test names. The final frontend runs are separate fresh executions.

The [milestone receipt](parity/audit_type_child_milestone_5_0_6.json.gz) reconciles
these scopes without accepting full AP02 or relabeling earlier execution.
The [cleanup receipt](parity/audit_type_child_cleanup_5_0_6.json.gz) verifies
removal of this cohort's exact disposable directories, including failed
proposals. Stable preset outputs, producer artifacts, sources and compact
evidence are retained; no shared cache is evicted.

## Remaining acceptance boundary

TC01-TC06 qualify the recorded child-rule corrections; they do not close all
paths/generic arguments/binders/type syntax or the complete expression, pattern,
statement, item and lexical reconciliation. AP02 still requires their explicit
body mapping, malformed-input boundaries and metadata-consumer reconciliation.
G14 additionally joins AP01/AP03-AP12 with G17's governed persistence, protected
publication, Context retention and cleanup. Existing approved compatibility
exceptions are unchanged. Source status remains 122 accepted / 0 pending /
2 excluded_external, capability status 14 qualified / 3 partial, P0-P7 incomplete.
