# AP02 array constant expression qualification

This is a generation-time metadata repair against locked Rust runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. The reviewed repair is installed on
`cpp_support` commit `d1ad8d40`. Production passes 194 Java tests and reproduces
193 complete SDK compiler contracts. The copied execution and explicit
production artifact reuse below retain their separate scopes.

## Responsibility and reproduced differences

The original `reverse_relation.json_type` accepts Rust types whose projected
entity depends on an array type or `std::array::IntoIter`. A valid length can
contain arithmetic, casts or a constant block. The
[initial complete-library diagnostic](parity/ap02_array_const_expression_diagnostic_5_0_6.json.gz)
reproduces twelve C++ refusals for valid original models among eighteen cases.
Invalid arithmetic, unknown names and wrong final types remain checked.

The proposed resolver uses typed integer evaluation and the existing lexer.
It preserves inference, signed and unsigned bounds, cast truncation, local
scope, mutation, branch type checking and lazy execution. Its precedence and
comparison boundaries follow the existing source parser. It uses Java standard
facilities during generation; the C++ runtime gains no dependency. The native
reference witness uses 64-bit `usize` and `isize`; this is not new evidence for
target-specific constant evaluation or physical-router execution.

## Review and actual generated consumers

The [first candidate](parity/ap02_array_const_expression_candidate_5_0_6.json.gz)
passes 192 Java tests and 49 complete-library compiler pairs. A
[subsequent standalone review](parity/ap02_array_const_expression_review_v2_5_0_6.json.gz)
finds a bitwise/comparison precedence mismatch and a different helper result
for an unparenthesized comparison chain. Those observations prevent installation
of that candidate despite its passing finite corpus. The first review attempt
is retained separately; it omitted a Java compilation dependency.

The [revised continuation](parity/ap02_array_const_expression_candidate_v3_5_0_6.json.gz)
passes all 194 Java tests and all 54 compiler contracts. It freshly executes
42 original/C++ compiler-contract pairs. Twelve earlier completed outcomes are
explicitly reused after fresh complete SDK generation and exact byte equality.
The [partial controller receipt](parity/ap02_array_const_expression_candidate_v2_5_0_6.json.gz)
preserves that completed subset and its unchanged inputs. That controller was
intentionally stopped to correct a compiler-refusal expectation for malformed
metadata; its entire planned run is not accepted.

Five additional C++ descriptor consumers are compiled, linked and executed
against actual generated libraries. Their outputs match Rust:

| Metadata expression | Relation count | Retained audit value limit |
| --- | --- | --- |
| Bitwise AND, OR or XOR followed by comparison | 1 | 21 |
| Unparenthesized comparison chain | 0 | 17 |
| Explicitly grouped comparison | 1 | 21 |

The malformed comparison chain demonstrates the original parser's partial
effect: it ignores the relation and later assignments while retaining an
earlier value. Both complete generated libraries compile. A standalone helper
refusal must not be described as a whole-generator compiler refusal. The raw
continuation's compiler-contract counters cover the 42 syntax-only invocations;
its five additional descriptor compile/link invocations are recorded separately
in the process list.

The [reviewable source patch](proposals/array_const_expression_projection.patch)
changes one Java resolver and adds eight permanent regression methods. Its
[proposal receipt](parity/ap02_array_const_expression_proposal_5_0_6.json.gz)
binds the exact patch text, copied source and preceding producer revision.
No generated artifact is hand-edited.

## Installed production continuation

The [actual SQLite matrix](parity/ap02_array_const_expression_candidate_sql_5_0_6.json.gz)
executes eight original/C++ model pairs and 40 query, E and JSON outcomes with
no differences. It includes the corrected precedence, grouped comparison,
typed casts, local mutation, lazy branches and modeled child projections.

The [production execution](parity/ap02_array_const_expression_installed_5_0_6.json.gz)
passes all 194 permanent Java tests and freshly regenerates 193 complete SDKs:
139 preserved contracts and 54 array constant contracts. All SDK bytes match
the actual compiler witnesses. Their 132 accepted and 61 refused compiler
outcomes are explicitly reused; this production recipe runs no C++ compiler.

The [retained SDK refresh](parity/ap02_array_const_expression_sdks_repaired_5_0_6.json.gz)
reproduces and checks all eighty SDKs. Their 3,560 implementation/Assist files
remain unchanged; each manifest refreshes one exact parser fingerprint. The
[production SQL continuation](parity/ap02_array_const_expression_production_sql_reuse_5_0_6.json.gz)
regenerates all eight complete formatted consumer SDKs and verifies exact
artifact identity, explicitly reusing the forty runtime outcomes above.

The [historical-source transition](parity/ap02_array_const_expression_helper_transition_5_0_6.json.gz)
changes seven imports/current-input predicates in the preceding admission
helper, retaining its other validation bodies and controls. The
[ordinary and optimized controls](parity/ap02_array_const_expression_admission_controls_5_0_6.json.gz)
reject eighteen new and 73 preserved corruptions per mode. Actual affected
[registered CTests](parity/ap02_array_const_expression_native_5_0_6.json.gz)
pass 6/6 in each mode on 11,990 unchanged execution inputs.

The [ordinary source join](parity/sq02_array_const_expression_source_join_5_0_6.json.gz)
and [optimized counterpart](parity/sq02_array_const_expression_source_join_optimized_5_0_6.json.gz)
admit 97 exact changes from the retained full native checkpoint. Both preserve
the 1,101-test registry, 716 Python commands and 707 native consumer hashes,
and reject eight source-transition corruptions. These joins extend source
provenance; they do not claim a fresh full native execution.

The first SDK refresh stopped before generation because its command omitted
the local Maven PATH; its [failed receipt](parity/ap02_array_const_expression_sdks_5_0_6.json.gz)
is preserved. The accepted refresh above supplies the pinned environment.
The initial source-join controller also assumed legacy named controls returned
objects. Its [diagnostic](parity/sq02_array_const_expression_source_join_initial_5_0_6.json.gz)
is retained; the accepted joins check the exact thirteen legacy names and
reject a substituted name in both Python modes.

Pinned formatting and its check pass for 1,813 handwritten C++ files without
changing execution inputs. The module-path verifier passes its structural
and provenance checks. All recorded owned probe directories are removed after
their children exit; source, fixtures, compact receipts and shared caches remain.

This bounded repair does not close the complete supported metadata/configuration
scope, G14/GP11, G17/GP16 or P0-P7. Capability status remains **14 qualified /
3 partial**. The Option-model and quoted-column attribute decisions remain
independent.
