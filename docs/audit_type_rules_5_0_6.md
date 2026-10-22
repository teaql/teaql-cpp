# Audit type parsing continuation (Rust 5.0.6)

Status: temporary Java producer candidate only. Producer sources, generated SDKs
and the current full native integration inputs remain unchanged. AP02/G14 and
P0-P7 remain open.

## Source contract and new failures

The reference remains Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, generator
`45e1572c4956ca963410d8ea436ead3c8857087a`, and the original locked `syn 2.0.118`.
The original complete container-metadata parser is executed without rewriting
its parsing, identity or error branches.

The [209 original/current/proposed observations](parity/audit_type_rules_initial_5_0_6.json.gz)
add string-encoded reverse-relation types, local declaration types and expression
casts. They expose **93 differences**, including **34 regressions** relative to
the installed producer, beyond the earlier 2,143-case success. Retained failures
include reserved identifiers, object/lifetime bounds, constants in generic
arguments, associated constants/constraints, turbofish paths, macro eligibility,
function-type arguments and the type grammar used after `as`.

These are complete metadata effects. A valid ignored type/expression allows later
mask/limit settings to run; an immediate nested parsing failure stops them while
retaining prior settings. A string-encoded type must additionally pass the
source's complete `parse_str::<Type>` check, including deferred unconsumed nested
tokens. An ignored outer expression can retain the source parser's earlier
successful AST result. The paired variadic function-type controls demonstrate
this distinction; it is not a license to normalize every failure or accept
arbitrary token groups.

## Unified temporary candidate

The [type-parser follow-up](parity/audit_type_rules_candidate_5_0_6.json.gz)
shares a typed parser for metadata, local declarations and casts. It distinguishes
path, qualified path, object-bound and other type shapes, and uses the source's
`without_plus` boundary for pointer/reference targets, function-type returns
and casts. Object/implementation types require a trait, and their permitted
const/capture bounds remain distinct. Generic arguments retain lifetime, type,
literal/block constant and associated binding/constraint forms. Module-style
macro eligibility does not extend to generic or qualified type paths.

String-encoded types retain their complete-consumption requirement. A raw
underscore identifier is rejected as a lexical token, including when it occurs
inside discarded code, while a string-encoded invalid type remains an ordinary
nested metadata parse failure.

All **2,352** retained observations agree, with **315 improvements and zero
regressions** relative to the installed producer. The initial 93 discrepancies
and 34 regressions remain in their original receipt. The full proposed Java
source, completed commands, original results and owned-output cleanup are
retained. No Maven build, producer class replacement or C++ runtime change is
included in this checkpoint.

## Remaining work

The finite matrix does not prove all Rust type/expression/pattern/item/token
syntax. Expression generic-argument scanning, qualified paths and remaining
lexical/identity rules continue under the original full AP02 requirements.
Installation waits for the current native epoch to finish, followed by permanent
producer regression tests, actual source/compiler/generated-metadata gates and
strict generated-artifact reproduction. The current native run qualifies its
frozen inputs, not this uninstalled parser proposal.


## Expression generic-argument continuation

The [135 expression/method/pattern observations](parity/audit_expression_generics_initial_5_0_6.json.gz)
expose **53 differences**, including three regressions relative to the installed
producer. Opaque angle-bracket scanning admits invalid arguments and qualified
paths. The first shared-parser proposal fixes those cases but introduces
**75 pattern regressions**, retained in its
[failed candidate receipt](parity/audit_expression_generics_candidate_5_0_6.json.gz).
Wildcard and boolean literal patterns need their own source branches; they must
not become ordinary identifier paths when the shared path parser is tightened.

The [pattern-preserving follow-up](parity/audit_expression_generics_candidate_v2_5_0_6.json.gz)
uses the shared generic-argument grammar, expression-specific turbofish boundary,
qualified-path and macro eligibility rules. Method arguments retain intervening
trivia; numeric fields and `await` do not acquire method-generic syntax. It
matches **2,487** observations, with **366 improvements and zero regressions**
relative to the installed producer under that finite matrix. All earlier failed
checkpoints remain unchanged.

The [lexical continuation](audit_lexical_boundaries_5_0_6.md) records the latest
proposal, its wider source comparisons and the separate existing JUnit gate.
These remain uninstalled proposals during native integration; they do not close
AP02's full syntax, identity, compiler or generated-artifact requirements.
