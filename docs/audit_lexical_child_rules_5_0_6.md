# Literal and source-line boundaries against Rust 5.0.6

Status: LC01-LC08 are installed and reconciled for the named lexical rules,
source-value observations and generated descriptor/privacy callers. Complete
AP02, G14 and the generator goal remain open.

The later [DM01-DM08 continuation](default_model_literal_metadata_5_0_6.md)
qualifies full default model container attributes and the actual StringTemplate
writer's removal of CR before Rust compilation. Its installed model entry
supersedes LC08's CRLF-only implementation. The LC receipts below retain their
original helper identities and scope.

The references remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
producer `45e1572c4956ca963410d8ea436ead3c8857087a`. The direct observer executes
the unchanged attribute parser with `syn` 2.0.118 and `proc-macro2` 1.0.106.
The [dependency receipt](parity/audit_lexical_child_dependency_sources_5_0_6.json.gz)
checks the lexer archive against the reference Cargo checksum and verifies its
unpacked `parse.rs` member. The regular source gate independently checks the
locked `syn` archive and recorded source members.

## Source responsibilities

| ID | Locked responsibility and shared adapter correction or preserved rule |
| --- | --- |
| LC01 | All six cooked/raw UTF-8, byte and C string lexers reject an unescaped bare CR. A CR followed by LF is lexically valid. Ordinary cooked string values normalize that pair to LF; direct raw string values preserve it. Explicit escaped `\r` remains a CR value. |
| LC02 | `trailing_backslash` consumes only ASCII space, tab, LF and paired CRLF. General token whitespace is a different set. The adapter stops consuming Unicode whitespace as continuation padding, retains it as ordinary string content where permitted, and rejects invalid CR sequences. |
| LC03 | Byte strings retain their ASCII content restriction and full byte-escape range. C strings retain Unicode content and nonzero byte/Unicode escape rules. Both retain source-specific escape validation instead of becoming ordinary UTF-8 string metadata. |
| LC04 | Raw string delimiters permit at most 255 hashes. Excess hashes cannot fall back into an identifier plus punctuation. Reserved `br#` and `cr#` prefixes cannot become ordinary identifiers when their literal spelling is invalid. |
| LC05 | Literal suffixes, numeric word breaks and raw identifiers retain the locked identifier rules. The matrix preserves ordinary/Unicode suffixes, combining characters, digit starts, reserved raw identifiers and numeric fallback behavior. No new host-JDK Unicode classification is introduced. |
| LC06 | Whole-stream lexical validation still precedes metadata effects. Balanced macro groups, ordinary/nested comments and converted doc attributes retain their separate rules. Invalid literal tokens remain invalid even inside ignored macro contents. The existing comment/doc and character responsibilities are preserved. |
| LC07 | The shared cooked-string decoder used by column metadata and audit strings now normalizes CRLF and rejects bare CR, including continuation padding. Numeric metadata, UTF-8 views and explicit escaped control values retain their existing behavior. |
| LC08 | Rust source files are normalized before a derive receives tokens. The model metadata entry therefore normalizes source CRLF before calling the shared parser. Direct parser callers retain `syn::parse_str` raw-value behavior. This distinction is a producer-stage responsibility, with actual compiler and complete generated-model evidence. |

The correction changes three shared Java helpers. Generated C++ methods do not
contain another parser or newline algorithm. No deployed dependency, C++
ownership mechanism, public API or module export changes.

## Direct source and compiler boundaries

The [matrix](../tests/parity/audit_lexical_child_contract_inputs_5_0_6.json)
contains **794 shapes / 5,877 new contracts** over string payloads, continuations,
suffixes, delimiters, token boundaries and comments. Numeric, local-expression,
opaque-macro and string-valued metadata consumers are observed separately.

The [preceding parser](parity/audit_lexical_child_before_5_0_6.json.gz) reproduces
**502 differences**. An isolated two-helper candidate and the
[actual multi-source verifier](parity/audit_lexical_child_actual_candidate_5_0_6.json.gz)
match all 63,918 original/current outcomes. The
[final three-helper candidate](parity/audit_lexical_complete_candidate_5_0_6.json.gz)
also preserves all prior outcomes and 270 retained AST witnesses.

An [actual Rust compiler probe](parity/audit_lexical_child_rustc_normalization_5_0_6.json.gz)
shows that ordinary and raw source literals both see LF after source-file CRLF
normalization. A separate [complete generated-model probe](parity/audit_lexical_child_compiler_boundary_5_0_6.json.gz)
reproduces two raw-string descriptor differences after direct parsing already
matches. The three original models and their complete outputs remain retained;
the ordinary cooked-string control agrees. LC08 fixes the model entry rather
than changing the direct parser's raw-string contract.

One [initial caller attempt](parity/audit_lexical_complete_candidate_callers_5_0_6.json.gz)
stops at a literal vertical tab that XML does not permit in an attribute. It is
an incomplete caller gate, not a passing native result. The
[input-boundary proof](parity/audit_lexical_complete_xml_rejection_5_0_6.json.gz)
retains the original rejection and independently executes the proposed C++
producer's rejection. The vertical-tab literal contracts remain in the direct
source corpus. The positive caller cohort uses XML-valid inputs; no runtime
compatibility exception is introduced.

## Candidate isolation and installed verification

The three existing development verifiers now share a checked source-overlay
resolver. Every proposed helper is compiled from its identified source;
unchanged original-source compilation stays independent. The
[nine negative controls](parity/audit_lexical_child_overlay_controls_5_0_6.json.gz)
reject duplicate, unknown and missing candidates through all three entry points.
This is verification tooling, not a generator or runtime dependency.

The [candidate/preceding Java regression](parity/audit_lexical_complete_candidate_regression_5_0_6.json.gz)
passes **132/132** tests with the candidate. The preceding helpers fail exactly
the four new corpus, decoded-value, invalid-CR and model-boundary tests. The
permanent corpus retains every new expectation from unchanged Rust execution.

The [installed source replay](parity/audit_lexical_complete_installed_metadata_current_5_0_6.json.gz)
matches **63,918/63,918** outcomes: 48,620 metadata effects and 15,298 checked
refusals, with **132/132** Java tests. The independent
[AST/source replay](parity/audit_lexical_complete_installed_grammar_current_5_0_6.json.gz)
preserves all retained expectations and 270 AST witnesses.

An [initial installed source gate](parity/audit_lexical_complete_installed_metadata_5_0_6.json.gz)
fails while reading successful Rust output: Python's `splitlines()` splits valid
Unicode separators inside JSON strings. Both output boundaries now use the
existing strict LF JSONL decoder. The
[framing proof](parity/audit_lexical_complete_jsonl_framing_5_0_6.json.gz)
executes the actual old/current assignment expressions against all 63,918
unchanged Rust records and twelve framing controls. The failed receipt is
preserved. The [direct dependency closure](parity/audit_lexical_complete_verifier_closure_5_0_6.json.gz)
also adds the imported comparator/framing and overlay/lock source files to each
verifier's execution inputs. Final replays use those current verifier identities;
this does not claim complete SQ02 provenance for other corpora.

Twelve [fresh original/installed C++ caller pairs](parity/audit_lexical_complete_installed_callers_current_5_0_6.json.gz)
match 36 entity descriptors and 168 privacy classifications, including the
three original compiler-boundary cases. Native execution disables default PMR
allocation and actually destroys `UserContext`. The
[sixteen strict comparison controls](parity/audit_lexical_complete_installed_caller_controls_5_0_6.json.gz)
reject actual output corruptions through the current and archived comparison
expressions. These callers establish descriptor/privacy behavior, not arbitrary
SQL execution or complete protected publication.

Isolated and [installed 80-SDK reproduction](parity/audit_lexical_complete_installed_sdks_current_5_0_6.json.gz)
preserve all 3,560 generated files and 2,467 protected artifacts. Every manifest
now records all four shared metadata helpers. Fifty-seven update three existing
hashes; twenty-three legacy records update two existing hashes and add the
missing decoder identity. The
[actual old guard](parity/audit_lexical_complete_provenance_before_5_0_6.json.gz)
accepted those omissions. The
[corrected guard](parity/audit_lexical_complete_provenance_controls_5_0_6.json.gz)
rejects all 23 real omissions and eight additional missing/stale helper
corruptions. Both reproduction scripts retain the complete helper list, and
the current guard checks it for every active SDK. The
[installation delta](parity/audit_lexical_complete_installed_delta_5_0_6.json.gz)
checks those bytes, the producer installation and the preceding verifier changes.
All 1,813 C++ inputs retain their
[frozen full native 893-test identities](native_provenance_integration_5_0_6.md).
That execution keeps its original parser identities; current Java, source,
caller and reproduction receipts provide the explicit continuation.

All 17 [current source/comparison/admission/inventory guards](parity/audit_lexical_complete_installed_source_guards_current_5_0_6.json.gz)
pass without skips. The [frontend gate](parity/audit_lexical_complete_installed_gates_5_0_6.json.gz)
passes two native and two imported-module tests, pinned formatting and its
check, locked references and module paths. The
[milestone](parity/audit_lexical_complete_milestone_5_0_6.json.gz) joins the exact
input deltas and retains unsuccessful probes with their original scope. The
[cleanup receipt](parity/audit_lexical_complete_cleanup_5_0_6.json.gz) verifies
removal of task-owned outputs, including unsuccessful attempts; unmeasured
removed bytes are reported separately.

## Remaining scope

LC01-LC08 join the preceding reviewed type, path, pattern, operator, block,
statement, item and generic responsibilities. Complete delegated grammar,
model configurations, metadata consumers and protected execution retain their
own requirements. Default model table/service string interpretation remained
outside this LC checkpoint; the later DM01-DM08 continuation qualifies its
named container consumers. Other model consumers and complete protected
execution retain separate exits. Complete SQ02 provenance, G13/G14/G17 and P0-P7
signoff remain open. Capability status stays **14 qualified / 3 partial**.
Approved JSON, quoted-FK and invalid wire-profile exceptions remain unchanged.
