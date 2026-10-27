# Pattern child rules against Rust 5.0.6

Status: PT01-PT08 is installed after isolated source, regression, actual caller
and SDK qualification. Final installed gates are recorded below. Complete AP02
and G14 remain partial.

The locked references are runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`. The observer verifies
the checksum of `syn` 2.0.118 and equality of its archived/unpacked parser sources
before compiling the unchanged original attribute parser.

## Source responsibilities

`syn/src/pat.rs` distinguishes parsing a single pattern from parsing alternatives,
and distinguishes the outer pattern kind from the kinds of its children. The
Java cursor now keeps the outer kind needed by slice parsing and separate helpers
for source dispatch, bindings, literals and range bounds.

| ID | Source responsibility and Java behavior |
| --- | --- |
| PT01 | `Pat::parse_single` selects paths using its lookahead before identifier binding. `Self`, `super`, `crate`, qualified paths and ordinary names followed by path/constructor/macro/range syntax retain separate eligibility. Plain `self` has its own binding path. |
| PT02 | `pat_ident`, references and boxed patterns parse single children. `ref`/`mut` modify an identifier rather than arbitrary paths, literals or constructors. Only an identifier binding can introduce `@`; path heads do not become bindings through a spelling heuristic. |
| PT03 | `pat_lit_or_range` and `pat_range_bound` reuse complete literal token parsing, including signed numeric literals and exponent signs. A numeric scanner cannot stop at the minus in `1e-3`. Const blocks remain distinct from identifier bindings. |
| PT04 | Bounded patterns accept `..`, `..=` and the source's obsolete closed `...`; prefix ranges accept modern limits only. Closed ranges require an endpoint, and endpoints are literals, paths or const blocks. Arbitrary patterns, calls, macros and recursively chained ranges are not endpoints. |
| PT05 | Missing range bounds follow the source token boundaries: end of the current group, `|`, `=`, a non-path `:`, comma, semicolon or `if`. `in` does not silently terminate a range endpoint. Prefix `..` without an endpoint is a rest pattern. |
| PT06 | `pat_slice` rejects a direct range with a missing endpoint. Parentheses, references, bindings, boxes and alternatives retain their outer kinds; their range children do not turn the outer pattern into a direct range. Tuple and tuple-struct children preserve the source's leading-alternative behavior. |
| PT07 | Path macros require an unqualified module-style path and a real `!`, with balanced raw token-tree content. Their bodies are not parsed as expressions. Qualified/generic paths and `!=` retain their separate boundaries. |
| PT08 | Existing struct field attributes, shorthand modifiers, numeric members and rest-buffer behavior are preserved. Match/condition/loop/field consumers and local/closure/function single-pattern consumers retain their distinct parsing contracts. |

This mapping qualifies the recorded child rules and their metadata effects.
The complete expression/type/path/lexical dependencies, every malformed-input
configuration and all application/SQL callers remain separate AP02 exits.

## Reproduced differences and candidate

The initial [371-shape / 2,968-case corpus](../tests/parity/audit_pattern_child_contract_inputs_5_0_6.json)
reproduces 1,061 actual differences. The expanded
[580-shape / 4,640-case corpus](../tests/parity/audit_pattern_complete_contract_inputs_5_0_6.json)
uses eight actual metadata contexts: match arms, if-let, while-let, for loops,
local bindings, closure parameters, function parameters and struct fields.
The [preceding parser](parity/audit_pattern_complete_before_5_0_6.json.gz)
reproduces **1,731 differences** without changing the preceding 10,832 results.

The [isolated candidate](parity/audit_pattern_complete_candidate_5_0_6.json.gz)
matches all **15,472** complete original metadata observations and the unchanged
270 AST witnesses. The [exact parser proposal](parity/audit_pattern_child_candidate_proposal_5_0_6.json.gz)
preserves both source versions. The
[regression proposal](parity/audit_pattern_complete_regression_proposal_5_0_6.json.gz)
retains a complete source-derived corpus and the exact Java test change.
The [actual new/old regression gate](parity/audit_pattern_complete_candidate_regression_5_0_6.json.gz)
passes **123/123** tests with the candidate; the preceding parser fails only
the new test, reproducing all 1,731 differences while preserving the other 122.

The [twelve original/native candidate caller pairs](parity/audit_pattern_complete_candidate_callers_5_0_6.json.gz)
compile complete generated entities/facades and agree on 36 descriptors / 168
privacy tuples. Default PMR is disabled and UserContext is actually destroyed.
The [sixteen strict controls](parity/audit_pattern_complete_candidate_caller_controls_5_0_6.json.gz)
validate actual raw outputs and reject effective type, field and partial-result
corruptions using the actual comparison expressions. These are descriptor/privacy
observations rather than full application or SQL execution.

The [candidate SDK replay](parity/audit_pattern_complete_candidate_sdks_5_0_6.json.gz)
reproduces all 80 SDKs / 3,560 generated files with unchanged bodies, guidance and
data; 2,467 protected files remain unchanged. The existing staged SDK controller
is explicitly reused with the recorded candidate parser and classpath. Active
fingerprints are refreshed only after guarded installation.

## Installed qualification boundary

The [installation plan](parity/audit_pattern_complete_installation_plan_5_0_6.json.gz)
retains four exact development changes and all eighty preceding manifests.
The [installation receipt](parity/audit_pattern_complete_installed_source_5_0_6.json.gz)
requires complete candidate gates and unchanged checkpoint inputs before writing.
The source corpus is generated from unchanged original Rust observations.

The [installed source gate](parity/audit_pattern_complete_installed_metadata_5_0_6.json.gz)
matches 15,472 complete observations (15,370 metadata effects / 102 checked
refusals) and passes all 123 Java tests. The independent
[AST replay](parity/audit_pattern_complete_installed_grammar_5_0_6.json.gz)
matches those observations and 270 unchanged witnesses.
[Twelve fresh installed callers](parity/audit_pattern_complete_installed_callers_5_0_6.json.gz)
again agree on 36 descriptors / 168 privacy tuples with actual Context deletion;
[sixteen current controls](parity/audit_pattern_complete_installed_caller_controls_5_0_6.json.gz)
reject typed, missing/extra-field and partial-result corruptions.

The [installed SDK gate](parity/audit_pattern_complete_installed_sdks_5_0_6.json.gz)
reproduces all 80 SDKs / 3,560 files and 2,467 protected artifacts. Every manifest
changes exactly one parser-hash literal; every other manifest byte is identical.
The [delta proof](parity/audit_pattern_complete_installed_delta_5_0_6.json.gz)
admits exactly four development changes and eighty fingerprints since the
pre-installation checkpoint. All 1,813 C++ inputs remain equal to their complete
native893 execution identities.

All [seventeen current source/comparator checks](parity/audit_pattern_complete_installed_source_guards_5_0_6.json.gz)
pass. The [frontend and pinned guard gate](parity/audit_pattern_complete_installed_gates_5_0_6.json.gz)
passes native2/imported2, format/check of 1,808 C++ files and reference/generator/
module-path checks, preserving every recorded C++ input hash.

All runtime C++ implementations, ownership algorithms, public APIs and generated
bodies are preserved. The prior complete native893 execution retains its original
parser identities; current continuation proofs explicitly join unchanged C++
consumers. Capability status remains 14 qualified / 3 partial, runtime source
status 122 accepted / 0 pending / 2 excluded_external. G13/SQ02 provenance,
remaining G14/AP02 grammar and G17/P0-P7 signoff stay open.
