# Current query comparator and registration review

Post-install native integration checkpoint:
[The 1,063-test terminal epoch](parity/ap02_type_pattern_full_native_terminal_5_0_6.json.gz)
passes with no failure, error or skip on **11,809** unchanged frozen inputs;
the JUnit names equal the registry and the post-build has no work. The owned
temporary directory was removed. This epoch includes the installed AP02
type/pattern corpus and precedes the scoped comparator changes below; it does
not qualify those later source edits or close G13/G14/G17/P0-P7.

The [next full epoch](parity/ap02_cross_composition_full_native_terminal_5_0_6.json.gz)
includes those comparator edits and the installed cross-branch corpus. Its
1,063 registered tests complete with one
[isolated admission-fixture failure](custom_case_admission_fixture_5_0_6.md)
on 11,811 unchanged inputs. The fixture repair passes 60/60 controls and one
selected native CTest. A new complete native epoch is running; the failed
epoch remains failed evidence and G13/SQ02/P7 retain their full requirements.

Current logging-normalization counterexample:
[The pre-edit control](parity/log_manager_header_normalization_candidate_5_0_6.json.gz)
reproduces a false equality when the Rust and C++ help-header phrases occur
inside ordinary text: the installed log-manager comparator replaces the Rust
phrase globally. A candidate restricted to the actual banner line and its
following section preserves all **77** reconstructed banner substitutions in
the retained **94** cases while distinguishing the embedded phrases. This is
a controlled candidate, not a source edit or a new accepted logging gate. The
same in-memory candidate also matches [94/94 current C++ log-manager
observations](parity/log_manager_banner_candidate_native_5_0_6.json.gz) against
the retained normalized corpus, with the comparator, corpus, contract and
built probe hashes unchanged. The comparator now requires the complete help
banner context, including the preceding audit-help line. A [fresh locked Rust
raw replay](parity/log_manager_fresh_raw_source_post_edit_5_0_6.json.gz) and
[current native replay](parity/log_manager_full_banner_scope_post_edit_5_0_6.json.gz)
both match **94/94** retained outcomes; the raw source has **77** actual banner
headers. The ordinary phrase and a copied section without the preceding line
stay unchanged. The pinned source contract and four selected logging CTests
pass. These are post-edit gates; current G13/SQ02 signoff remains open.

Current quoted-FK normalization counterexample:
[The pre-edit descriptor control](parity/quoted_fk_descriptor_normalization_candidate_5_0_6.json.gz)
shows that a global `column_name` replacement can also change ordinary query
text. Restricting it to the `order_id` `PropertyDescriptor` preserves all
**424** current replacements across **7,122** relation cases and distinguishes
the embedded-text counterexample. The approved quoted-FK behavior remains the
target; this candidate had no source edit or native execution. The relation
comparator now uses a shared exact-descriptor adapter and pins its source and
scope control. An [independent compatibility replay](parity/quoted_fk_scoped_adapter_compatibility_5_0_6.json.gz)
checks **6,576** source matches and **546** approved exceptions, rejects eight
invalid manifests and three corrupted result variants, and removes its owned
71,876,195-byte temporary directory. Selected post-edit execution remains
separate from the preceding full native epoch and any G13/SQ02 promotion.
The [companion dynamic-JSON control](parity/quoted_fk_dynamic_json_normalization_candidate_5_0_6.json.gz)
finds the same broad replacement in its filter adapter: narrowing to the same
descriptor context preserves **189** replacements across **1,181** existing
cases and rejects the embedded-text counterexample. Both comparator adapters
now use the scoped correction; neither candidate changed the approved exception.
The dynamic-JSON source contract pins the changed comparator, shared adapter and
scope control. Execution and policy import the same adapter, so selected post-edit
execution includes those consumers. All **15/15** selected post-edit CTests
pass across logging, relation and dynamic JSON; the later logging-only hardening
passes another **4/4** selected CTests. The
[post-edit scope receipt](parity/dynamic_json_adapter_scope_post_edit_5_0_6.json.gz)
records **18** passing controls.
The old [adapter-scope checker also accepted](parity/dynamic_json_adapter_scope_false_acceptance_5_0_6.json.gz)
a rewrite of the marker inside ordinary filter text. The current checker
requires the approved `order_id` descriptor context and directly rejects this
ordinary-text rewrite. The pre-edit false-acceptance receipt remains historical.

Current nonempty-partial-result review surface:
[The 1,063-test registry join](parity/current_custom_partial_result_surface_5_0_6.json.gz)
matches all **160** custom comparison locators to their exact current CTest
commands, source fixtures and explicit setup dependencies, and hashes all
126 distinct comparator scripts. **110** locators directly call the shared
`compare_values.observe`; **50** use another transport or delegate to another
comparator. This AST and wiring inventory selects the next partial-result
audit; it does not qualify either group's normalization, nonempty partial
results or complete SQ02/G13/GP13 semantics. The one non-`_rust_parity` clock
contract is included in the 160.
The locator receipt predates the six comparator edits below, so its changed
script hashes are historical; the selected source contracts pin current bytes.

The [optimized-interpreter partial-result controls](parity/comparator_optimized_partial_state_controls_5_0_6.json.gz)
now exercise **62/62** saved-output cases. A pre-edit one-record replay showed
that `compare_explicit_save_scope_5_0_6.py` and
`compare_eval_indexed_5_0_6.py` both reported success under `python -O` after
the remaining results were withheld; ordinary Python rejected the same input.
Their result checks now execute regardless of Python optimization. The same
hardening covers the generated lookup Context check and the three direct
state/entity comparators. Normal and optimized replay each accepts complete
saved output and rejects a nonempty one-record prefix for the six targeted
paths. The broader replay also checks complete result types and LF framing in
nine typed/delegated families. Five selected explicit/eval/source CTests and
four selected state/entity/source CTests pass after the edits. This is a bounded
partial-result check; the other dispatcher paths and complete SQ02/G13 remain
open. The earlier 44-control receipt is an intermediate checkpoint.

A [read-only AST review](parity/optimized_assert_review_surface_5_0_6.json.gz)
hashes the **126** distinct scripts behind the 160 active custom comparison
locators. It finds **56** scripts with assertions, **49** with an original
observer call inside an assertion, and one with the final result-value
comparison inside an assertion. These are review locations: the AST inventory
does not prove a false acceptance for each source call, and ordinary native
CTest does not run Python with optimization. It identifies a finite SQ02
follow-up without changing the current frozen epoch.
The [registry join](parity/optimized_assert_registry_scope_5_0_6.json.gz)
narrows the active impact: these 49 scripts serve **51** custom CTest
registrations, none of whose commands passes `--rust`; all 51 have a separate
source setup dependency. The demonstrated `python -O` source skip affects the
optional direct live-source CLI path, not an already requested Rust observer
inside those registered custom comparison commands. Their separate source
setups retain their own provenance requirements.

A [derived-query source replay](parity/derived_query_source_optimized_candidate_5_0_6.json.gz)
demonstrates the distinct source-call risk: a deliberately failing Rust
observer is rejected by ordinary Python, but `python -O` skips the observer
inside `assert` and reports **576/576** matches against a retained-output
stub. An [isolated candidate](parity/derived_query_source_optimized_candidate_validation_5_0_6.json.gz)
uses an explicit conditional and passes four good/failing-original ×
normal/optimized controls. These temporary stubs were removed. The installed
comparator remains frozen; the other static source-call locations require
their own review before broad SQ02 claims.

A separate [JSON graph-limit candidate](parity/json_graph_limit_optimized_candidate_5_0_6.json.gz)
demonstrates a remaining optimized-interpreter false acceptance: with all
eleven output records present and one value deliberately corrupted, ordinary
Python rejects the comparator result, while `python -O` reports eleven matches.
The shared observer already rejects a missing record; this witness concerns
the value comparison guarded by `assert`. Its temporary observer was removed.
An [isolated candidate](parity/json_graph_limit_optimized_candidate_validation_5_0_6.json.gz)
replaces only that assertion with an explicit error and passes all four
complete/corrupt × normal/optimized controls. Its copied files and observer
were removed. The installed comparator and current frozen native inputs are
unchanged; installation, source-contract hashes and a permanent control remain
SQ02 work after that epoch.

A focused [nonempty partial-result control](parity/runtime_examples_nonempty_partial_control_5_0_6.json.gz)
passes one valid schema example result and withholds the relation example
result. The current two-example comparator rejects that output with its
missing-result error; its 177-byte owned fixture directory was removed. This
checks one delegated comparator branch, not the other 159 custom selections.
Two more [delegated nonempty-partial controls](parity/delegated_comparator_nonempty_partial_controls_5_0_6.json.gz)
feed exactly one valid observation into the registered query-key and runtime-check
CLI paths. Both reject the missing remainder (**1/451** and **1/1,134**),
including the runtime-check pointer-width handshake. Their owned 2,782-byte
fixture directory was removed. These controls establish result-count admission
for those two selections only; source provenance and the other custom paths
retain their separate requirements.

Current runtime examples CLI source checkpoint:
[The two installed SQLite business examples](runtime_examples_cli_source_binding_5_0_6.md)
match **2/2** freshly rebuilt original Rust and fresh C++ business results,
and the original **34 ordered SQL entries**. Fifteen negative controls and
four selected CTests pass. The incremental lookup maps **160/160** installed
comparison locators; **1,058** CTest registrations are enumeration. This
does not close the three partial capabilities or P0–P7.

Current SQLite transaction CLI source checkpoint:
[The installed historical transaction comparison](sqlite_transaction_cli_source_binding_5_0_6.md)
matches **510/510** freshly rebuilt original Rust and fresh C++ outcomes:
183 synchronous, 183 asynchronous and 144 lease cases. Fifteen negative
controls and four selected CTests pass. The incremental lookup maps
**159/160** comparison locators, with **1** remaining; **1,055** CTest
registrations are enumeration. Full G13/G14/G17 and P0–P7 exits remain open.

Current SQLite raw Debug CLI source checkpoint:
[The installed native-width raw Debug comparison](sqlite_raw_debug_cli_source_binding_5_0_6.md)
matches **36/36** freshly rebuilt original Rust outputs and fresh C++ results
without normalization. Fifteen negative controls and four selected CTests
pass. The incremental lookup maps **158/160** comparison locators, with
**2** remaining; **1,052** CTest registrations are enumeration. Full
G13/G14/G17 and P0–P7 exits remain open.

Current logging CLI source checkpoint:
[The installed log-manager and audit-event comparisons](logging_cli_source_binding_5_0_6.md)
match **94/94** normalized log outputs and **1,427/1,427** audit outputs
from freshly rebuilt original Rust observers and current C++ execution.
Fifteen negative controls and five selected CTests pass. The incremental
lookup maps **157/160** comparison locators, with **3** remaining; **1,049**
CTest registrations are enumeration. Full G13/G14/G17 and P0–P7 exits remain
open.

Current JSON graph-limit CLI source checkpoint:
[The installed large graph comparison](json_graph_limits_cli_source_binding_5_0_6.md)
matches **11/11** freshly rebuilt Rust outcomes and fresh C++ results.
Fifteen negative controls and five selected CTests pass. The incremental
lookup maps **155/160** comparison locators, with **5** remaining; **1,046**
CTest registrations are enumeration. Full G13/G14/G17 and P0–P7 exits remain
open.

Current protected Behavior CLI source checkpoint:
[The installed original SQLite deletion/recovery comparison](protected_behavior_cli_source_binding_5_0_6.md)
matches **1,296/1,296** freshly rebuilt Rust outputs and fresh C++ results.
Fifteen negative controls and four selected CTests pass. The incremental
lookup maps **154/160** comparison locators, with **6** remaining; **1,043**
CTest registrations are enumeration. Raw autocommit is the checked scope;
full G13/G14/G17 and P0–P7 exits remain open.

Current inherited Business-ID CLI source checkpoint:
[The installed adapted original-method comparison](inherited_business_cli_source_binding_5_0_6.md)
matches **450/450** freshly rebuilt adapted Rust outputs and fresh C++
results; untouched producer acceptance remains false. Fifteen negative
controls and five selected CTests pass. The incremental lookup maps
**153/160** comparison locators, with **7** remaining; **1,040** CTest
registrations are enumeration. Full G13/G14/G17 and P0–P7 exits remain open.

Current entity constructor CLI source checkpoint:
[The installed original generated entity comparison](entity_constructor_cli_source_binding_5_0_6.md)
matches **512/512** freshly rebuilt Rust outputs and fresh C++ results.
Fifteen negative controls and five selected CTests pass. The incremental
lookup maps **152/160** comparison locators, with **8** remaining; **1,037**
CTest registrations are enumeration. Full G13/G14/G17 and P0–P7 exits remain
open.

Current Indexed LoadState CLI source checkpoint:
[The installed six-variant comparison](eval_indexed_cli_source_binding_5_0_6.md)
matches **18/18** fresh common semantic outcomes across 180 operations;
whole raw diagnostics remain unequal. Fifteen negative controls and five
selected CTests pass. The incremental lookup maps **151/160** comparison
locators, with **9** remaining; **1,034** CTest registrations are enumeration.
Full G13/G14/G17 and P0–P7 exits remain open.

Current generated member-alias CLI source checkpoint:
[The installed original generated decoder comparison](generated_member_alias_cli_source_binding_5_0_6.md)
matches **260/260** fresh C++ outcomes against retained Rust source execution.
Fifteen negative controls and five selected CTests pass. The incremental
lookup maps **150/160** comparison locators, with **10** remaining; **1,031**
CTest registrations are enumeration. Full G13/G14/G17 and P0–P7 exits remain
open.

Current inherited Checker CLI source checkpoint:
[The five installed model comparisons](inherited_checker_cli_source_binding_5_0_6.md)
preserve the original 7,200-case Rust corpus and match 7,200 fresh C++
observations. Fifteen negative controls and eight selected CTests pass. The
incremental lookup maps **149/160** comparison locators, with **11** remaining;
**1,028** CTest registrations are enumeration. Full G13/G14/G17 and P0–P7
exits remain open.

Current derived-query source CLI checkpoint:
[The installed composition and diagnostics comparisons](derived_query_cli_source_binding_5_0_6.md)
bind **360 + 576 = 936 original outcomes** to fresh exact native results,
including privacy and intent-log boundaries. Fifteen substitutions and all
six selected checks pass. Incremental source lookup is **144/160**, leaving
**16**; **1,025** CTest registrations are enumeration. Full G13/G14/G17 and
P0–P7 exits remain open.

Current query-key pointer-width source CLI checkpoint:
[The installed key comparison](query_keys_cli_source_binding_5_0_6.md)
binds **451 original inputs** at each width. Fresh C++ execution matches
**451/451** on native 64-bit and **451/451** on the retained MIPS binary under
QEMU; 46 source outputs differ by width. Fifteen substitutions and all six
selected checks pass. Incremental source lookup is **142/160**, leaving
**18**; **1,022** CTest registrations are enumeration. Full G13/G14/G17 and
P0–P7 exits remain open.

Current generated Business-ID source CLI checkpoint:
[The installed wrapper and Checker comparison](generated_business_ids_cli_source_binding_5_0_6.md)
binds **1,044 original outcomes** to fresh native execution: **916** raw
equalities and **128** cases under the one declared resource type-name adapter.
Fifteen substitutions and all four selected checks pass. Incremental source
lookup is **141/160**, leaving **19**; **1,019** CTest registrations are
enumeration. Full G13/G14/G17 and P0–P7 exits remain open.

Current typed Business-ID E source CLI checkpoint:
[The installed E comparison](expression_business_ids_cli_source_binding_5_0_6.md)
binds **96 original outcomes** to current native execution: **48** raw exact
and **48** under the declared diagnostic presentation adapter. Fifteen
substitutions and all five selected checks pass. Incremental source lookup is
**140/160**, leaving **20**; **1,016** CTest registrations are enumeration.
Full G13/G14/G17 and P0–P7 exits remain open.

Current typed Business-ID Q source CLI checkpoint:
[The installed generated-factory comparison](business_id_lookup_cli_source_binding_5_0_6.md)
binds **528 original outcomes** to **528 exact fresh native results**, including
288 SQLite operations with safe SQL records. Fifteen substitutions and all
four selected checks pass. The incremental static lookup is **139/160**,
leaving **21**; **1,013** CTest registrations are enumeration. Full
G13/G14/G17 and P0–P7 exits remain open.

Current strict stream-shape source CLI checkpoint:
[The installed strict comparison](stream_shapes_cli_source_binding_5_0_6.md)
binds **13,104 raw original outcomes** to fresh native results; the 6,012
earlier startup adaptations are retained only as historical evidence. Fifteen
substitutions are rejected and all four selected checks pass. Static source
lookup is **138/160**, leaving **22**; the **1,010** registered tests are an
inventory. Broader G13/G14/G17 and P0–P7 exits stay open.

Current generated relationship-analysis source CLI checkpoint:
[The approved quoted-FK source contract](relation_analysis_cli_source_binding_5_0_6.md)
retains **7,122 original observations** and verifies current C++ results as
**6,576** source comparisons plus **546** explicit approved exceptions. Fifteen
substitutions are rejected and all four selected CTest checks pass. Static
source lookup is **137/160**, leaving **23**; the **1,007** registered tests
are an inventory. Broader G13/G14/G17 and P0–P7 exits stay open.

Current composed-query and graph-save source CLI checkpoint:
[Two accepted source matrices](composed_graph_cli_source_binding_5_0_6.md)
bind **5,472 original outcomes** to fresh native results. Fifteen source and
CTest substitutions are rejected; all five selected installed checks pass.
Static source lookup is **136/160**, leaving **24**; the **1,004** registered
tests are an inventory. Broader G13/G14/G17 and P0–P7 exits stay open.

Current execution-model source CLI checkpoint:
[Three accepted source streams](execution_model_cli_source_binding_5_0_6.md)
bind **4,448 scenarios** and **6,480 hot-cache steps** to fresh native
results. Sixteen substitutions are rejected and all six selected CTest checks
pass. Static source lookup is **134/160**, leaving **26**; the **1,001**
registered tests are an inventory. Broader G13/G14/G17 and P0–P7 exits stay
open.

Current state/entity source CLI checkpoint:
[Four accepted original streams](state_source_cli_binding_5_0_6.md) now bind
**272 observations**, including 12 load-state batches with **40,404 typed
rows**, to fresh current native results. The entity-source historical and
current corpora have identical inputs and expected outputs; their sole
metadata difference is a recorded Cargo manifest hash, validated against the
retained Git blob. Eighteen controls and seven selected native checks pass.
Static source lookup is **131/160** with **29** remaining; the **998**
registered tests are an inventory. Full G13/G14/G17 and P0–P7 remain open.

Current adapted inherited-business source-provenance checkpoint:
[Inherited identifiers, governed saves and updates](inherited_business_driver_provenance_5_0_6.md)
binds **2 complete corpora / 80 observations**, all matching adapted
source/corpus/native results. A fresh reference-locked original-source project
reproduces **20 expected compiler errors**; the legacy lock's **46 differing
registry identities** remain historical. The unchanged adapter creates **43
derived files**, with exactly **5 changed entity files**; untouched producer
acceptance stays false. The **610-file** archive and **75 copied inputs** are
verified. Five-model helpers, native **5/5**, prior key/classifier checks **2/2**
and negative controls **35/35** pass. Source records cover all **305 disjoint
direct-corpus locators / 323,568 primary observations** under their declared
private-function, adaptation and profile scopes. Registry **934/305/197** is
enumeration. All **1,814 C++ inputs**, producer implementations and **80 SDKs /
3,560 files** remain unchanged. Complete custom/protected execution, metadata
and current integration remain open: **14 qualified / 3 partial**.
The column-attribute compatibility decision remains pending.

Previous private key/classifier source-provenance checkpoint:
[Query metadata keys and bootstrap retry classification](private_key_classifier_driver_provenance_5_0_6.md)
binds **2 complete corpora / 2 unchanged drivers / 757 observations**, all matching
source/corpus/native results. The **610-file** archive, **55 copied inputs** and
reference-locked dependencies are verified. Native **5/5**, retained SQLite schema
checks **2/2** and negative controls **31/31** pass. Cumulative bindings cover
**303 disjoint corpora / 323,488 observations**; **2 adapted inherited-business
direct entries** remain. Registry **932/305/196** is enumeration. Original key
identity shells, native 64-bit scope and private classifier extraction remain
explicit; the complete 31-file generated SDK is an artifact, not a compiled
package in this checkpoint. All **1,814 C++ inputs**, producer implementations
and **80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom/protected
execution and G13/G14/G17/P0-P7 remain open: **14 qualified / 3 partial**.
The column-attribute compatibility decision remains pending.

Previous public SQLite schema source-provenance checkpoint:
[Public schema creation and reconciliation](sqlite_schema_driver_provenance_5_0_6.md)
binds **1 complete corpus / 1 unchanged driver / 735 observations**, all matching
source/corpus/native results. The **610-file** archive, **10 copied inputs** and
reference-locked dependencies are verified. Native **4/4**, retained runtime-bridge
checks **2/2** and negative controls **19/19** pass. Cumulative bindings cover
**301 disjoint corpora / 322,731 observations**; **4 other direct entries** remain.
Registry **930/305/195** is enumeration. All **1,814 C++ inputs**, producer
implementations and **80 SDKs / 3,560 files** remain unchanged. Complete metadata,
custom/protected execution and G13/G14/G17/P0-P7 remain open:
**14 qualified / 3 partial**. The column-attribute decision remains pending.

Previous runtime-bridge source-provenance checkpoint:
[Context events, typed aggregates and loaded graphs](runtime_bridge_driver_provenance_5_0_6.md)
binds **3 complete corpora / 3 complete driver bodies / 4,802 observations**,
all matching source/corpus/native results. The full **610-file** archive and
**19 copied inputs** remain unchanged. Native **5/5**, previous private-protocol
checks **2/2**, current controls **23/23** and retained private-protocol controls
**26/26** pass. Source bindings cover **300 disjoint corpora / 321,996
observations**; **5 other direct entries** remain. The **928/305/194**
registry/direct/custom counts are locators, not a full-suite run.
All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. Original runtime bodies and test bodies retain their exact prefixes;
declared bridge constants and wrapped complete driver bodies are appended.
Existing observation projections and fixture-path adaptation remain explicit. Complete
metadata, custom provenance, protected execution and G13/G14/G17/P0-P7 remain
open: **14 qualified / 3 partial**. The separate column-attribute compatibility
decision remains pending.

Previous private-protocol source-provenance checkpoint:
[Private runtime protocols and Debug/Release behavior](runtime_protocol_driver_provenance_5_0_6.md)
binds **7 complete corpora / 6 observer modules / 23,657 observations**,
all matching source/corpus/native results. The full **610-file** archive and
**24 copied inputs** remain unchanged. Native **9/9**, previous generated-model
checks **2/2**, current controls **26/26** and retained generated-model controls
**27/27** pass. Source bindings cover **297 disjoint corpora / 317,194
observations**; **8 other direct entries** remain. The **926/305/193**
registry/direct/custom counts are locators, not a full-suite run.
All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. Original runtime bodies and test bodies retain their exact prefixes;
only declared observer modules are appended. Rust Debug/Release and the C++
probe's header-counter mode keep separate scopes. Complete
metadata, custom provenance, protected execution and G13/G14/G17/P0-P7 remain
open: **14 qualified / 3 partial**. The separate column-attribute compatibility
decision remains pending.

Previous generated-model source-provenance checkpoint:
[Generated entities, inheritance, constants and runtime assembly](generated_model_driver_provenance_5_0_6.md)
binds **26 complete corpora / 20 original targets / 16,674 observations**,
all matching source/corpus/native results. The full **610-file** archive and
**490 copied inputs** remain unchanged. Native **28/28**, previous Context
checks **2/2**, current controls **27/27** and retained Context controls
**20/20** pass. Source bindings cover **290 disjoint corpora / 293,537
observations**; **15 other direct entries** remain. The **924/305/192**
registry/direct/custom counts are locators, not a full-suite run.
All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. Nine generated packages compile completely; two source-only
models retain their unchanged entity/Checker library scope and original full-crate
failures. Historical observer projections remain explicit. Complete
metadata, custom provenance, protected execution and G13/G14/G17/P0-P7 remain
open: **14 qualified / 3 partial**. The separate column-attribute compatibility
decision remains pending.

Previous Context capability source-provenance checkpoint:
[Context, logs, I18n, telemetry and business IDs](context_public_driver_provenance_5_0_6.md)
binds **17 complete corpora / 16 original targets / 16,376 observations**,
all matching source/corpus/native results. The full **610-file** archive and
**41 copied inputs** remain unchanged. Native **19/19**, previous core-model
checks **2/2**, current controls **20/20** and retained core-model controls
**19/19** pass. Source bindings cover **264 disjoint corpora / 276,863
observations**; **41 other direct entries** remain. The **922/305/191**
registry/direct/custom counts are locators, not a full-suite run.
All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. The original logging shell and human-time observer adapter
retain their documented limits; complete Context-event bridges remain separate. Complete
metadata, custom provenance, protected execution and G13/G14/G17/P0-P7 remain
open: **14 qualified / 3 partial**. The separate column-attribute compatibility
decision remains pending.

Previous core-model source-provenance checkpoint:
[Core values, query state and graph/request models](core_model_driver_provenance_5_0_6.md)
binds **34 complete corpora / 27 original targets / 10,625 observations**,
all matching source/corpus/native results. The full **610-file** archive and
**93 copied inputs** remain unchanged. Native **36/36**, previous numeric/time
checks **2/2**, current controls **19/19** and retained numeric controls
**19/19** pass. Source bindings cover **247 disjoint corpora / 260,487
observations**; **58 other direct entries** remain. The **920/305/190**
registry/direct/custom counts are locators, not a full-suite run.
All **1,814 C++ inputs**, producer implementations and **80 SDKs / 3,560 files**
remain unchanged. Each corpus retains its existing semantic scope. Complete
metadata, custom provenance, protected execution and G13/G14/G17/P0-P7 remain
open: **14 qualified / 3 partial**. The separate column-attribute compatibility
decision remains pending.

Previous numeric/time source-provenance checkpoint:
[Numeric, time and auxiliary values](numeric_time_driver_provenance_5_0_6.md) binds
fifteen complete corpora to **nine original targets / 41,382 observations**,
all matching source/corpus/native results. The complete 610-file archive,
30 copied inputs, exact local packages and reference checksums are verified.
Native **17/17**, prior SQLite-provider tool checks **2/2**, current controls **19/19**
and retained SQL controls **19/19** pass. Source bindings cover
**213 disjoint corpora / 249,862 observations**; **92 other direct entries**
remain. The **918/305/189** registry/direct/custom counts are locators, not a
full 918-test run. All **1,814 C++ inputs**, producer implementations and
**80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.
The separate column-attribute compatibility decision remains pending.

Previous SQLite provider source-provenance checkpoint:
[SQLite provider and diagnostics](sqlite_provider_protocol_provenance_5_0_6.md) binds
eleven complete corpora to **seven original targets / 9,152 observations**,
all matching source/corpus/native results. The complete 610-file archive,
27 copied inputs, exact local packages and reference checksums are verified.
Native **13/13**, prior SQL-tool checks **2/2**, current controls **19/19**
and retained SQL controls **19/19** pass. Source bindings cover
**198 disjoint corpora / 208,480 observations**; **107 other direct entries**
remain. The **916/305/188** registry/direct/custom counts are locators, not a
full 916-test run. All **1,814 C++ inputs**, producer implementations and
**80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.
The separate column-attribute compatibility decision remains pending.

Previous SQL/SQLite pipeline source-provenance checkpoint:
[SQL and SQLite pipeline](sqlite_pipeline_driver_provenance_5_0_6.md) binds
eighteen complete corpora to **fifteen original targets / 12,064 observations**,
all matching source/corpus/native results. The complete 610-file archive,
42 copied inputs, exact local packages and reference checksums are verified.
Native **20/20**, prior document-tool checks **2/2**, current controls **19/19**
and retained document controls **20/20** pass. Source bindings cover
**187 disjoint corpora / 199,328 observations**; **118 other direct entries**
remain. The **914/305/187** registry/direct/custom counts are locators, not a
full 914-test run. All **1,814 C++ inputs**, producer implementations and
**80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.
The separate column-attribute compatibility decision remains pending.

Previous document/reference source-provenance checkpoint:
[Document and secure-reference protocols](document_protocol_driver_provenance_5_0_6.md)
bind seven complete corpora to **six original targets / 2,941 observations**,
all matching source/corpus/native results. The complete 610-file archive,
twenty copied inputs, exact local packages and reference registry checksums
are verified. Native **9/9**, prior query-tool checks **2/2**, document controls
**20/20** and retained query controls **24/24** pass. Source bindings cover
**169 disjoint corpora / 187,264 observations**; **136 other direct entries**
remain. The **912/305/186** registry/direct/custom counts are locators, not a
full 912-test run. All **1,814 C++ inputs**, producer implementations and
**80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.
The separate column-attribute compatibility decision remains pending.

Previous generated-query source-provenance checkpoint:
[Generated queries and governance](query_governance_driver_provenance_5_0_6.md)
bind fifteen retained corpora to **13 original targets / 18,055 complete observations**,
all matching source/corpus/native results. The full 610-file runtime archive,
136 copied inputs, original School fixture, generated provenance and actual
locked dependencies are verified. Native **17/17**, previous pagination-tool
checks **2/2** and **24** effective controls pass. Source bindings cover
**162 disjoint corpora / 184,323 observations**; **143 other direct entries**
remain. The **910/305/185** registry/direct/custom counts are locators, not a
full 910-test run. All **1,814 C++ inputs**, producer implementations and
**80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.
The separate column-attribute compatibility decision remains pending.

Previous dynamic-governance source-provenance checkpoint:
[Dynamic-field governance and audit](dynamic_governance_driver_provenance_5_0_6.md)
bind nine retained corpora to **7 original targets / 5,915 complete observations**,
all matching source/corpus/native results. The full 610-file runtime archive,
73 copied inputs, original School fixture, generated provenance and actual
locked dependencies are verified. Native **11/11**, previous pagination-tool
checks **2/2** and **19** effective controls pass. Source bindings cover
**156 disjoint corpora / 172,183 observations**; **149 other direct entries**
remain. The **910/305/185** registry/direct/custom counts are locators, not a
full 910-test run. All **1,814 C++ inputs**, producer implementations and
**80 SDKs / 3,560 files** remain unchanged. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.
The separate column-attribute compatibility decision remains pending.

Previous pagination source-provenance checkpoint:
[Generated execution and pagination](pagination_driver_provenance_5_0_6.md)
bind five retained corpora to fresh original Rust/C++ execution: **4 targets /
10,888 observations**, all matching. The complete 610-file runtime archive,
65 copied source/project/model files, generated provenance, input recipes and
reference-compatible dependency graph are verified. Native **7/7**, previous
archive checks **2/2** and **19** effective controls pass. Source bindings now
cover **147 unique corpora / 166,268 observations**; **158 other direct entries**
remain. The **908/305/184** registry/direct/custom counts are locators, not a full
908-test run. All **1,814 C++ inputs**, producer implementations and SDKs remain
unchanged during this cohort. The [column-attribute diagnostic](field_attribute_boundary_5_0_6.md)
confirms a separate reference escaping defect; its compatibility decision is
pending and is not semantic acceptance. Complete metadata, custom provenance,
protected execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.

Previous scalar-operand assignment checkpoint:
[Source-shaped sample assignment](sample_operand_assignment_5_0_6.md) fixes
24 actual Boolean getter differences and mixed-type SDK compilation failures.
Shared runtime conversion retains original mutation operands, source NULL
distinctions and Context ownership. Existing runtime function bodies are
byte-identical after removing the newly inserted helpers. Actual original/C++
sample comparisons pass **432 field outcomes**; four JSON descriptor callers
retain eight explicit approved type differences. Java **141/141**, scoped native
**6/6**, imported kernel **1/1** and ASan/UBSan kernel **1/1** pass. All **80 SDKs /
3,560 files** reproduce: only two reviewed JSON sample delegates change.
Ten body-scope controls and an input-epoch exit control pass. Four old C++ files
change and one new kernel test is added; frozen native893 identities are not
reused as current whole integration. Core corpus totals remain **142 / 155,380**,
with **163** other direct entries open. Complete metadata/provenance/protected
execution and G13/G14/G17/P0-P7 stay open: **14 qualified / 3 partial**.

Previous large-text and sample-branch checkpoint:
[Large-text descriptor and sample precedence](large_text_metadata_5_0_6.md)
fixes 24 descriptor type differences and a public-SDK sample compilation
failure. Fresh Rust/C++ comparisons pass **2 models / 6 descriptors / 52
properties** and **144 deterministic scalar field outcomes**. Java **139/139**
includes replay of the **63,918 retained metadata contracts**; their original
Rust parser results reuse unchanged source/corpus identities. Scoped native
**10/10**, thirteen strict controls and pinned format/check pass. All **80 SDKs /
3,560 bodies** reproduce unchanged, with refreshed source fingerprints.
Only CppCoreGenerator production source changes; all **1,813 frozen C++ inputs**
remain unchanged. The approved JSON exception is preserved. The core source
corpus totals remain **142 / 155,380**, with **163** other direct entries open.
Complete metadata/provenance/protected execution and G13/G14/G17/P0-P7 stay open:
**14 qualified / 3 partial**. There is no new full native, module or sanitizer run.

Previous property-shape normalization checkpoint:
[Generated length and numeric shape metadata](property_shape_metadata_5_0_6.md)
now reuse the locked producer's normalization. Two complete model callers,
six descriptors and 44 property rows match fresh Rust/C++ execution; the old
emitter differed in 16 rows / 22 values. Current metadata regression is
**63,918/63,918**, Java **137/137** and scoped native **4/4**. A separate scoped
ASan/UBSan replay verifies the compiler observer's Context-close correction.
All **80 SDKs / 3,560 files** reproduce unchanged bodies; producer provenance
is refreshed. All **1,813 frozen C++ inputs** remain unchanged. This is a new
generator/test-harness epoch, not a new full native893 run. The existing
142 source corpora / 155,380 observations and remaining 163 direct entries
are unchanged; these model callers are a separate metadata gate.
G13/G14/G17 and P0-P7 stay open: **14 qualified / 3 partial**.

Previous declared-driver completion checkpoint:
[the final three SQL/mutation drivers](sql_mutation_driver_provenance_5_0_6.md)
match **2,048** fresh original/corpus/native observations. All 610 archive files,
eight copied source/project files, actual dependency metadata and complete input
recipes are retained. **25** controls, native **5/5**, focused **2/2** and source
guards **29/29** pass. Native/modules configurations pass without fresh imported
execution. Source bindings now cover **142 unique corpora / 155,380 observations**.
All **107 declared-driver project/view responsibilities** are covered across
their distinct source epochs. **163 other direct registrations** and remaining
custom CLI, dependency and normalization duties stay open. The **905/305/183**
registry/direct/custom counts are locators, not a full 905 run. All 1,813 C++ inputs,
producer implementations and SDKs remain unchanged. Frozen native893 keeps its
original identities. Complete metadata, provenance, protected execution and
G13/G14/G17/P0-P7 remain open; capability status stays **14 qualified / 3 partial**.

The preceding [entity API](entity_api_driver_provenance_5_0_6.md),
[private-runtime](private_runtime_driver_provenance_5_0_6.md) and
[query-store](query_store_driver_provenance_5_0_6.md) checkpoints retain their
original scopes, failures and sealed input identities.

Previous Registry provenance checkpoint:
[all seven source-project targets](registry_driver_provenance_5_0_6.md) match
**27,257** fresh original/native observations. Both exact runtime appendices,
prepared targets, manifests, dependency locks and complete output are retained.
**31** controls, native **13/13**, focused **2/2** and source guards **21/21** pass;
native/modules configurations pass. The runtime's 273 test names are enumerated;
Registry has zero embedded tests, and enumeration is not a test execution claim.
Cumulative source bindings cover **72 corpora / 106,987 observations**;
**233 direct entries** and **68 declared-driver project/view responsibilities**
remain. The **897/305/179** registry/direct/custom counts are locators.
Frozen native893 keeps its original inputs; this is no full897 run. All 1,813
C++ inputs, producer implementations and SDKs remain unchanged. Complete metadata,
provenance, protected execution and G13/G14/G17/P0-P7 stay open.
Capability status remains **14 qualified / 3 partial**.

Previous entity-runtime provenance checkpoint:
[all seven source-project targets](entity_runtime_driver_provenance_5_0_6.md)
match **4,226** fresh original/native observations and pass all **21** original
unit tests. Exact runtime source/view, observer, project and dependency identities
are retained; **29** controls, native **19/19**, focused **2/2** and current
source guards **19/19** pass. Cumulative source bindings cover **65 corpora /
79,730 observations**; **240 direct entries** and **72 declared-driver project/view
responsibilities** remain. The **895/305/178** registry/direct/custom counts are
locators. Frozen native893 retains its original inputs; this is no full895 run.
All 1,813 C++ inputs, producer implementations and SDKs remain unchanged.
Complete metadata, provenance, protected execution and G13/G14/G17/P0-P7 exits
stay open. Capability status remains **14 qualified / 3 partial**.

Earlier Context source-provenance follow-up: [four original-execution contracts](context_corpus_source_provenance_5_0_6.md) bind5,856 current native outputs to actual archived Rust stdout,67 reference source/test/vector hashes per view, exact observer/input/producer/helper bytes, metadata-derived manifests and registry checksums. Twelve corruption controls and six registered checks pass; a focused factory comparison automatically runs its source fixture. Original Rust execution is explicitly reused and no expected values are rewritten. The other23 baseline-only corpora, remaining direct/custom provenance and complete SQ02/GP13/P0-P7 remain open; capability status stays14 qualified/3 partial.

Earlier corpus-admission follow-up: [shared entry and default locator](corpus_admission_and_inventory_5_0_6.md) repairs three actual zero-observation false passes and resolves the default corpus from its source declaration. Admission11/binding8/inventory-shape4 controls, native7/imported1 and five effective result mutations pass. Current registry879 contains305 nonempty direct active-baseline bindings and170 custom registrations requiring review. These are admission/locator facts; complete source/dependency/cardinality/normalization provenance remains SQ02/GP13 work. Runtime C++, generated SDKs and module exports are unchanged; capabilities remain14 qualified/3 partial and P0-P7 incomplete.

## Current frozen 877-test locator checkpoint

The [current locator receipt](parity/parity_dispatcher_current_checkpoint_5_0_6.json.gz)
reads the complete native registry after its successful build and no-work check.
It retains exact commands/properties, 439 distinct comparator source bodies and
hashes, the shared comparison helper and the unchanged inventory implementation.
The inventory resolves **304 active-baseline bindings**, finds no missing or
historical direct corpus, and leaves **169 custom registrations** for semantic
and provenance review. These are locator classifications, not 169 absent runtime
implementations or a claim of complete SQ02 acceptance.

All **160** current commands whose local parser explicitly declares `--cpp`
supply that option. Inherited parsers, alternative flags, invariant-only checks,
normalization and actual source/corpus provenance still require their own
reviews. The shared observation boundary still checks terminal process success,
LF framing, exact record count and complete typed JSON equality.

The stream comparator now checks all 13,104 raw source observations without the
obsolete startup rewrite. Its focused registered native/module tests pass; the
earlier full epoch's failed expectation remains historical. All 11,413 current
execution inputs and both references stayed unchanged during this readback.
The owned registry/report directory (620,147 bytes) was removed after execution.
The corresponding full native epoch subsequently passed **877/877** with all
frozen input checks intact. The
[source-cohort closure](runtime_core_source_closure_5_0_6.md) records that result
separately; this locator review does not change a capability verdict.

### Executed empty-result rejection controls

The [single-binary controls](parity/custom_dispatcher_empty_result_controls_5_0_6.json.gz)
replace each custom registration's `--cpp` executable with an owned executable
that consumes its input, exits successfully and emits no results. There are 162
attempts: 161 actually invoke that executable and reject its missing output.
The formatter's `--cpp` parameter instead names a directory; the initial file
substitution fails path discovery and is retained as setup evidence, not counted
as a result-count control.

The [alternative-command controls](parity/custom_dispatcher_alternative_empty_controls_5_0_6.json.gz)
complete that formatter case with its actual directory layout, valid native
pointer-width response and valid count-admission responses, then withhold the
formatting results. It rejects the missing rows after those prerequisites.
The five inherited Checker/model selections likewise invoke an empty executable
under their existing fixed-clock driver and reject its missing observations.
The runnable-example comparator rejects the absent required result prefix. Its
empty-input rejection does not claim separate execution of every example branch.

Thus **168** current custom comparator registrations actually reject the
controlled empty result. The remaining registration is the existing comparator
invariant test itself; its **13/13** resource/deferred saved, missing, extra,
partial and corrupt-result controls pass. All **169** custom registrations have
this stated negative-control scope. There are zero accepted empty-result
controls and zero timeouts; the initial formatter path error remains separate.

These controls test the exact current CLI selections. They do not establish
every original dependency, normalization rule, nonempty partial-result branch
or capability requirement. Complete SQ02 remains open. Original comparators,
corpora and C++ functions are unchanged, the same 11,413 input/reference guards
pass, and both owned control directories are removed after execution.

## Historical 736-test locator checkpoint

Status: finite source/registration review, not complete SQ02 acceptance. The
current native registry contains 736 tests. The unchanged simple inventory
resolves 236 direct active-baseline bindings and leaves 136 custom registrations
for their actual source/provenance/adaptation review. These counts supersede
older registry counts only as locators; they do not qualify runtime semantics.

The [source and command receipt](parity/current_dispatcher_locator_review_5_0_6.json.gz)
retains the complete registry, exact CLI commands, current inventory and complete
shared/execution comparator source bodies and hashes.

## Existing replay evidence must not be repeated as missing work

The active base/Context, entity-stream and SQL-stream CMake bindings already
select separate 5.0.6 corpora. Historical missing-intent inputs remain negative
evidence; they must not be described as successful provider execution. Explicit
intent matrices supply the corresponding positive paths:

| Existing qualification | Actual scope |
| --- | --- |
| [Base/Context replay](layered_query_replay_5_0_6.md) | 724 unchanged historical observations and 2,172 separate explicit-purpose observations; fixture recording is distinct from final SQL/privacy publication. |
| [Typed query/stream replay](typed_query_replay_5_0_6.md) | 943 complete observations over five families, including 193 entity-stream inputs; separate provider/model combinations retain their wider obligations. |
| [SQL protocol replay](sql_protocol_5_0_6.md) | Actual SQL/executor/stream protocol inputs and their explicit intent controls; inspect its source/dependency and observer boundaries before whole-source acceptance. |

Some older G17 questions still describe these historical probes as awaiting
their first baseline replay. Reconcile that wording with these completed scoped
receipts, preserving the actual broader source/model/call-site obligations.
The ledger itself is an execution input of the running native epoch and remains
frozen until that epoch terminates.

## Comparison completeness and command selection

The shared `compare_values.observe` executes the actual command, checks its exit
status, frames records only on LF and rejects a different output-record count.
The shared `same` checks JSON types, complete object keys, array cardinality and
floating signed zero. A downstream zip over two already checked equal-length
streams is not by itself a reproduced false pass.

The reviewed generated execution/options/result-model/hot-cache dispatchers
use that observation boundary before their whole-output comparisons and
additional invariants. Stream-shape comparison separately applies the existing
explicit eager-C++/lazy-Rust startup disposition. Its unadapted source outputs,
adapted requirements and native results remain distinct. Reading these helpers
does not establish every corpus's dependency provenance or accept their complete
normalization/call-site responsibility.

All **127** active commands whose local parser explicitly declares `--cpp`
supply that argument. Optional source-only modes therefore are not selected by
those registered commands. This check does not resolve inherited parsers,
alternative executable flags, actual result correctness or all 136 custom
registrations. In particular, invariant-only helpers need their own review;
they are not fabricated missing C++ comparisons.

No comparator, corpus, source observer or production implementation changes.
No new false-pass defect is claimed. The prior
[reproduced resource-comparator repairs](comparator_result_contracts_5_0_6.md)
remain independent concrete defect evidence. The
[new native integration epoch](native_integration_5_0_6.md) is running with all
execution inputs frozen; its outcome and complete SQ02/G13/P0-P7 acceptance
remain pending.
