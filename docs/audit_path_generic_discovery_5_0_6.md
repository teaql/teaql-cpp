# Associated generic path correction against Rust 5.0.6

Status: AG01-AG02 is installed and qualified for the recorded finite contracts.
The complete 893-test native epoch finishes before installation. Its C++ inputs
remain unchanged through the metadata-only continuation. AP02 and G14 remain
partial; the staged proposal receipts below retain their original identities.

The references are runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Source responsibility and reproduced differences

In the locked `syn` 2.0.118 `path.rs`, `GenericArgument::parse` first parses a
type. An unqualified, single-segment type path with no arguments or angle
arguments can then become an associated type, associated constant or bound
constraint. This includes arguments spelled with `::<...>`, as well as the
special path heads `self`, `super` and `crate` without arguments. These special
heads retain their own argument restrictions. A multi-segment or qualified path
does not acquire the same associated-binding eligibility.

The current Java recognizer instead trials an ordinary binding identifier or
`Self` followed by direct `<...>` arguments. It stops before subsequent metadata
effects on some source-accepted inputs, including `A<N::<X>=T>` and `A<self=T>`.
That changes the resulting entity/table/service names, privacy settings and
relationship descriptors rather than merely reporting a different syntax error.

The [initial inputs](parity/audit_path_generic_discovery_inputs_5_0_6.json.gz)
cover 93 shapes in six actual metadata contexts: closure parameters, typed
locals, closure returns, casts, nested expressions and string-encoded reverse
relation types. The [actual source replay](parity/audit_path_generic_discovery_5_0_6.json.gz)
finds 12 differences among 558 new observations. All preceding 9,494 complete
metadata results and 270 AST witnesses remain unchanged.

The [expanded preceding-parser replay](parity/audit_path_generic_expanded_before_5_0_6.json.gz)
reproduces **216 differences** across 36 of 223 shapes, with 1,338 new
observations. It includes empty and nonempty turbofish arguments, associated
types/constants/constraints, special and raw identifiers, qualified and
multi-segment paths, invalid argument forms and binder boundaries. Every expected
result comes from the unchanged locked source; no expected result was rewritten.

## Isolated candidate and remaining gates

The [candidate capsule](parity/audit_path_generic_candidate_capsule_5_0_6.json.gz)
retains the complete preceding and proposed parser sources, their hashes and all
expanded inputs. The proposal recognizes eligible special heads and optional
turbofish arguments while preserving their distinct restrictions.

The [candidate source replay](parity/audit_path_generic_candidate_5_0_6.json.gz)
matches **10,832/10,832** complete metadata observations and all 270 retained
AST witnesses. The separate
[candidate regression run](parity/audit_path_generic_candidate_java_5_0_6.json.gz)
passes all **121 Java tests** and reproduces all 9,494 preceding source results.
The checked crate archive and unpacked parser source hashes are retained in the
source receipts. Each replay uses an owned temporary Rust/Java build directory
and removes it after process completion; the candidate overlay and input files
are also removed after their compressed sources are retained.

The [twelve fresh generated caller pairs](parity/audit_path_generic_candidate_callers_5_0_6.json.gz)
compile the complete original entity files and candidate-generated C++ facades.
All **36 descriptors / 168 privacy tuples** agree. Default PMR is disabled and
each UserContext is actually destroyed; retained input hashes remain unchanged.
This proves the selected expression-metadata callers, not complete relation-type,
SQL or application execution. The owned compilation directory is removed after
success, reclaiming 182,507,099 bytes.

The [independent comparison controls](parity/audit_path_generic_candidate_caller_controls_5_0_6.json.gz)
check the actual raw source/native stdout before replaying both actual comparison
expressions. All sixteen corruptions reject: twelve Boolean-to-number changes
that the archived comparison expression accepts, plus missing/extra fields and
partial/duplicated descriptor rows. This does not relabel the compiled run as
execution of the archived controller. The receipt also preserves an initial
wrapper failure before any shell command and its corrected execution.

The [permanent regression proposal](parity/audit_path_generic_regression_proposal_5_0_6.json.gz)
retains the exact preceding/proposed Java test and a complete 1,338-case source
corpus. In the [isolated new/old gate](parity/audit_path_generic_candidate_regression_repaired_5_0_6.json.gz),
the candidate passes **122/122** Java tests; the preceding parser fails only
the new test, reproducing all 216 differences and preserving the other 121 tests.
The first [controller receipt](parity/audit_path_generic_candidate_regression_5_0_6.json.gz)
is preserved: the candidate tests passed, then an overescaped summary regex
failed before the preceding-parser phase. The repaired gate is a separate fresh
execution, with exact summary checks and no source-expectation change.

The [isolated candidate SDK replay](parity/audit_path_generic_candidate_sdks_5_0_6.json.gz)
regenerates all **80 SDKs / 3,560 files** with freshly compiled candidate classes;
every generated body, guidance and data byte remains unchanged. All 2,467
protected artifacts retain their bytes. Installed-source fingerprints are checked
as preceding identities and are not relabeled as candidate fingerprints. No
active manifest is refreshed in this proposal run. Its
[controller capsule](parity/audit_path_generic_candidate_sdk_controller_5_0_6.json.gz)
retains the original and exact staged driver, including the explicit candidate
class path and compilation commands. The owned SDK/class output directory and
outer controller/overlay directory are removed after successful execution.

## Installed source and acceptance

After successful terminal native893, the
[guarded installation](parity/audit_path_generic_installed_source_5_0_6.json.gz)
applies exactly the qualified parser/test/corpus and one source-oracle registration.
AG01 recognizes eligible `::<...>` arguments before associated binding or
constraint punctuation. AG02 recognizes the simple special path heads `self`,
`super` and `crate` while preserving their distinct argument restrictions.

The [fresh installed metadata gate](parity/audit_path_generic_installed_metadata_5_0_6.json.gz)
passes all 10,832 source observations and 122 Java tests. The separate
[installed AST replay](parity/audit_path_generic_installed_grammar_5_0_6.json.gz)
matches those metadata results and the unchanged 270 original AST witnesses.
The [fresh installed caller gate](parity/audit_path_generic_installed_callers_5_0_6.json.gz)
again matches all twelve original/native model pairs, 36 descriptors and 168
privacy tuples, with default PMR disabled and actual UserContext destruction.
Its [sixteen strict controls](parity/audit_path_generic_installed_caller_controls_5_0_6.json.gz)
check actual raw outputs and retain the distinction from the archived comparator.

The [installed SDK replay](parity/audit_path_generic_installed_sdks_5_0_6.json.gz)
reproduces all 80 SDKs / 3,560 files and 2,467 protected artifacts. Each active
manifest changes exactly one parser-hash literal; every other manifest byte and
all generated code/guidance/data are unchanged. The
[delta proof](parity/audit_path_generic_installed_delta_5_0_6.json.gz)
admits exactly four development changes and eighty fingerprints before status
updates, preserving all 1,813 C++ inputs and every other frozen execution input.

All [seventeen current source/comparator checks](parity/audit_path_generic_installed_source_guards_5_0_6.json.gz)
pass. The [final frontend/guard gate](parity/audit_path_generic_installed_gates_5_0_6.json.gz)
passes native2/imported2, pinned format/check of 1,808 C++ files and reference/
generator/module-path guards, with unchanged C++ input hashes. The
[native integration account](native_provenance_integration_5_0_6.md) explicitly
joins the preceding complete execution and the current metadata-only continuation.

Complete AP02 expression/operator/type/path/binder/
pattern/statement/item/lexical reconciliation, metadata callers, SQ02 provenance
and G13/G14/G17/P0-P7 signoff remain independent exits. Runtime source status is
122 accepted / 0 pending / 2 excluded_external; capability status stays
14 qualified / 3 partial. The approved compatibility exceptions remain unchanged.
