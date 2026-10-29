# Current full native integration

New full native epochs should use
[`scripts/run_native_integration_epoch_hardened_5_0_6.py`](../scripts/run_native_integration_epoch_hardened_5_0_6.py).
Its ten former assertions are unconditional checks, including frozen-input and
locked-reference validation under `python -O`. The original controller path is
retained for source-bound historical receipts. The current 1,066-test registry
has split 1,066/1,066 name coverage on unchanged inputs, as documented in
[the source-check continuation](registered_python_optimized_checks_5_0_6.md#full-native-epoch-boundary);
no uninterrupted full epoch has run with the hardened controller yet.

## Terminal 1,063-test epoch and installed AP02 continuation

The [frozen input snapshot](parity/current_full_native_1063_inputs_5_0_6.json.gz)
and [terminal receipt](parity/current_full_native_1063_terminal_5_0_6.json.gz)
record **1,063/1,063** native CTests passing with zero failures, errors or
skips. JUnit contains the exact 1,063 unique names in the frozen registry.
All 11,808 runtime and producer input hashes and both locked references remain
unchanged, all eight controller steps exit zero, and the final build reports
no work. The task-owned log directory was removed after the terminal receipt.
This is full native execution evidence for the producer inputs before the
162-case AP02 corpus was installed; it does not accept G13/G14/G17 or P0-P7.

The [AP02 installation](audit_grammar_branch_review_5_0_6.md#installed-type-and-pattern-continuation)
adds one producer test resource, one JUnit method and one source-verifier corpus
entry after that epoch. A [new frozen input snapshot](parity/ap02_type_pattern_full_native_inputs_5_0_6.json.gz)
was captured for a fresh full native epoch; its terminal outcome is pending.

## Current 1,058-test native epoch and focused recovery

The [full native build receipt](parity/current_full_native_build_5_0_6.json.gz)
records a successful no-work build. The [initial full CTest receipt](parity/current_full_native_ctest_5_0_6.json.gz)
preserves the terminal 1,058-test run: 1,048 passed, three failed and seven
were not run because their expression source fixture failed. That run exited 8.

Both failure causes were in qualification wiring. The resource-lifetime admission
test's isolated repository omitted two helper modules now imported by its
comparator. The expression source registry retained the old digest of the
query-alias comparator after its complete-corpus and output-count checks were
added. The isolated test now copies both helpers, and the registry binds the
current comparator digest. Original corpora and expected results were not
changed. The [focused recovery receipt](parity/current_full_native_ctest_recovery_5_0_6.json.gz)
records 14/14 passing CTests: the three previously failed checks, all seven
blocked expression checks, the query-alias and resource-binding comparisons,
and their required source fixtures. The resource admission's 26/26 controls
pass. The [complete native retest](parity/current_full_native_ctest_retest_5_0_6.json.gz)
uses the same 1,058-test registry and passes **1,058/1,058**, with zero failures,
zero skips and exit code 0. Its disposable JUnit directory was removed after
the result was captured. This closes the current full native execution gate;
it does not close G13/G14/G17, AP02 or P0-P7 semantic acceptance.

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

Previous field-column input checkpoint: [FC01-FC04](field_column_model_metadata_5_0_6.md)
qualifies eight actual model inputs, 24 ordinary/inherited columns and 72 typed
JSON hydration outcomes, with 29 strict controls and actual Context deletion.
The proposed production change was unnecessary and was never installed.
Current source **63,918/63,918**, Java **134/134**, source guards 17 and scoped
native/imported tests two each pass. Producer implementation, all 80 SDKs,
3,560 generated files, 2,467 protected artifacts and 1,813 C++ inputs retain
their preceding identities. SDK generation and the full native893 run are
explicitly reused with byte/hash checks. Complete grammar, other model
consumers, protected execution, SQ02 and G13/G14/G17/P0-P7 remain open.
Status stays **14 qualified / 3 partial**.

Previous default-model checkpoint: [DM01-DM08 reconciliation](default_model_literal_metadata_5_0_6.md)
repairs eleven of thirteen complete generated-model comparisons, including
undeclared versus empty masks and the resulting Unknown versus Plain
privacy policies. Actual StringTemplate CR removal is qualified separately
from direct Rust source parsing. Current source **63,918/63,918**, Java
**133/133**, 270 retained AST witnesses, thirteen model pairs, 39 descriptors,
182 privacy classifications and seventeen strict comparison controls pass.
Source guards pass 17 tests; scoped native and imported-module gates pass two
each. All 80 SDKs, 3,560 files and 2,467 protected artifacts reproduce unchanged;
manifests refresh only three helper fingerprints and retain all four shared
metadata helpers. All 1,813 C++ inputs retain the
[full native 893-test identities](native_provenance_integration_5_0_6.md), with
separate current source/caller proofs. This closes the named default container
consumer responsibilities. Complete delegated grammar, other model consumers,
protected execution, SQ02 and G13/G14/G17/P0-P7 signoff remain open. Status stays
**14 qualified / 3 partial**. Earlier checkpoints retain their original inputs.

Previous literal/source-line checkpoint: [LC01-LC08 reconciliation](audit_lexical_child_rules_5_0_6.md)
repairs 502 source-parser differences and two actual generated raw-string
metadata differences. Direct raw parsing and compiler source normalization
retain distinct contracts. Current source **63,918/63,918**, Java **132/132**,
270 retained AST witnesses, twelve generated caller pairs and sixteen comparison
controls pass. Source guards pass 17 tests; native and imported-module gates
pass two each. Two JSONL boundaries preserve Unicode content through twelve
controls; direct verification dependencies are fingerprinted. All 80 SDKs,
3,560 files and 2,467 protected artifacts reproduce unchanged. Twenty-three
legacy manifests gain the missing decoder identity; all record four shared
metadata helpers, and 31 provenance controls reject. All 1,813 C++ inputs retain
their [full native 893-test identities](native_provenance_integration_5_0_6.md),
with separate current source/caller proofs. Complete delegated grammar,
default model string consumers, SQ02 provenance and G13/G14/G17/P0-P7 signoff
remain open. Status stays **14 qualified / 3 partial**. Earlier checkpoints
retain their original evidence and scope.

Previous generic/bound checkpoint: [GB01-GB08 source reconciliation](audit_generic_child_rules_5_0_6.md)
qualifies declaration/binder ownership, const-number trivia, ordinary versus
strict nested-bound completion and preserved capture/path rules. The preceding
parser reproduces 2,371 differences in 7,607 new contracts across 903 shapes,
eight ordinary contexts and 383 additional encoded Type cases. Current source
**58,041/58,041**, Java **128/128**, 270 retained AST witnesses, twelve fresh
generated caller pairs and sixteen comparison controls pass. Source guards
pass 17 tests; scoped native and imported-module gates pass two each. All 80
SDKs, 3,560 generated files and 2,467 protected artifacts remain byte-identical;
manifests update only parser hashes. All 1,813 C++ inputs retain their
[full native 893-test identities](native_provenance_integration_5_0_6.md), joined
to explicit current source/caller proofs. Complete delegated AP02 grammar,
configurations/callers, SQ02 provenance and G13/G14/G17/P0-P7 exits remain open.
Status stays **14 qualified / 3 partial**. Earlier checkpoints retain their own
inputs, outcomes and scope.

Previous nested item checkpoint: [IT01-IT08 source reconciliation](audit_item_child_rules_5_0_6.md)
qualifies the named direct signature/modifier/field/where/container rules. The
preceding parser reproduces 2,200 differences in 8,776 new contracts across
1,097 shapes and eight metadata contexts. Current source **50,434/50,434**,
Java **127/127**, 270 AST witnesses, twelve fresh generated caller pairs and
sixteen comparison controls pass. Source guards pass 17 tests; scoped native and
imported-module gates pass two each. All 80 SDKs, 3,560 generated files and 2,467
protected artifacts remain byte-identical; manifests update only parser hashes.
All 1,813 C++ inputs retain their [full native 893-test identities](native_provenance_integration_5_0_6.md),
joined to explicit current source/caller proofs. Remaining delegated AP02
grammar/configurations, G13/SQ02 provenance and complete G17/P0-P7 exits remain
open. Status stays **14 qualified / 3 partial**. Earlier checkpoints below retain
their historical scope.

Previous statement checkpoint: [ST01-ST06 source reconciliation](audit_statement_child_rules_5_0_6.md)
fixes compound != macro lookahead and reconciles the named direct statement
rules. The preceding parser reproduces 216 differences in 8,120 new contracts
across 1,015 shapes and eight metadata contexts. Current source **41,658/41,658**,
Java **126/126**, 270 AST witnesses, twelve fresh generated caller pairs and
sixteen comparison controls pass. Source guards pass 17 tests; scoped native and
imported-module gates pass two each. All 80 SDKs, 3,560 generated files and 2,467
protected artifacts remain byte-identical; manifests update only parser hashes.
All 1,813 C++ inputs retain their [full native 893-test identities](native_provenance_integration_5_0_6.md),
with separate current metadata/caller proofs. Delegated AP02 grammar/configurations,
G13/SQ02 provenance and complete G17/P0-P7 exits remain open. Status stays
**14 qualified / 3 partial**. Earlier checkpoint statements below retain their
historical scope.

Previous block/array checkpoint: [BC01-BC04 source correction](audit_block_child_rules_5_0_6.md)
repairs 1,944 actual metadata differences in 6,544 new contracts across 818
expression shapes and eight metadata contexts. Current source **33,538/33,538**,
Java **125/125**, 270 AST witnesses, twelve fresh generated caller pairs and
sixteen comparison controls pass. Source guards pass 17 tests; scoped native and
imported-module gates pass two each. All 80 SDKs, 3,560 generated files and 2,467
protected artifacts remain byte-identical; manifests update only parser hashes.
All 1,813 C++ inputs retain their [full native 893-test identities](native_provenance_integration_5_0_6.md),
with separate current metadata/caller proofs. Full G14/AP02 grammar, G13/SQ02
provenance and complete G17/P0-P7 exits remain open. Status stays **14 qualified /
3 partial**. Earlier checkpoint statements below retain their historical scope.

Previous operator/trailer checkpoint: [OP01-OP08 source correction](audit_operator_child_rules_5_0_6.md)
repairs 2,368 actual metadata differences in 11,522 new contracts, including all
32-by-32 operator pairs in five contexts. Current source **26,994/26,994**,
Java **124/124**, 270 AST witnesses, twelve fresh generated caller pairs and
sixteen comparison controls pass. Source guards pass 17 tests; scoped native and
imported-module gates pass two each. All 80 SDKs, 3,560 generated files and 2,467
protected artifacts remain byte-identical; manifests update only parser hashes.
All 1,813 C++ inputs retain their [full native 893-test identities](native_provenance_integration_5_0_6.md),
with separate current metadata/caller proofs. Full G14/AP02 grammar, G13/SQ02
provenance and complete G17/P0-P7 exits remain open. Status stays **14 qualified /
3 partial**. Earlier checkpoint statements below retain their historical scope.

Previous pattern-child checkpoint: [PT01-PT08 source correction](audit_pattern_child_rules_5_0_6.md)
repairs 1,731 actual metadata differences across 580 pattern shapes / 4,640 new
contracts. Current source15,472/Java123/270 AST witnesses, twelve fresh generated
caller pairs, sixteen controls, native source17/native2/imported2 and all80 SDKs
pass. All3,560 generated files and2,467 protected artifacts remain byte-identical;
each manifest updates exactly one parser hash. All1,813 C++ inputs retain their
[full native893 identities](native_provenance_integration_5_0_6.md), with explicit
separate current metadata proofs. G13/SQ02, full G14/AP02 grammar and complete
G17/P0-P7 exits remain open. Status stays14 qualified /3 partial. Earlier
checkpoint statements below retain their historical scope.

Current integrated checkpoint: [complete native893 and associated generic continuation](native_provenance_integration_5_0_6.md)
qualifies the frozen native execution and explicitly joins the installed
metadata-only change on 1,813 unchanged C++ inputs. The
[AG01-AG02 correction](audit_path_generic_discovery_5_0_6.md) repairs 216 actual
differences. Current source10,832/Java122/270 AST witnesses, twelve generated
caller pairs, sixteen controls, native source17/native2/imported2 and all80 SDKs
pass. Generated code/guidance/data and2,467 protected artifacts are unchanged;
only80 parser hash literals refresh. The full execution retains its original
parser identities. G13/SQ02 (247 other direct entries/177 custom registrations),
remaining G14/AP02 grammar and G17/P0-P7 signoff stay open. Status remains
14 qualified /3 partial. Earlier checkpoint statements below retain their
historical scope.

## Completed native epoch after character lexing correction

The [character lexing continuation](character_lexing_5_0_6.md) passes installed
Java107, 3,147 exact original/current parser observations and complete 78-SDK
reproduction with all prior code/guidance/data bytes unchanged. Its seven new
compiled character models, eleven retained descriptor models and ten typed-JSON/
compiler cases all pass. The compiler controller is terminal with exit zero;
all 28 complete original/current observations agree, with unchanged frozen inputs,
references and all six completed native archives.

The [pipeline record](parity/native_after_character_pipeline_5_0_6.json.gz)
confirms the verified compiler controller is terminal and has launched the fresh
full native controller. Its independent
[input snapshot](parity/native_current_character_inputs_5_0_6.json.gz) and
[execution receipt](parity/native_current_character_terminal_5_0_6.json.gz)
belong to this epoch. No previous failed or superseded epoch is relabeled. Its prefix is
`/tmp/teaql_native_current_character_5_0_6`. Source dispositions at that frozen
checkpoint were 67/55/2; the later installed source audit below is separate.

The complete native consumer build exits zero, followed by a successful no-work
rebuild. Actual registry discovery and terminal JUnit both contain **784**
distinct tests. All **784/784** pass, with zero failures, errors or skips, in
4,737.98 seconds. This includes the complete generated API compiler gate,
which takes 3,381.95 seconds. The final selected-free native rebuild reports
no work. All **10,668** frozen execution inputs and both reference identities
remain unchanged. The controller is terminal and its complete flag is true.
This acceptance belongs to the preceding frozen epoch; subsequent installations
have separate scoped gates and are not relabeled as part of this full run.

The [post-native coordinator](parity/event_policy_installed_cohort_5_0_6.json.gz)
has verified this complete native terminal, frozen inputs/references and every
proposed before hash, then installed the twelve-file merged Event/Policy package.
Both configurations pass. The Event portable helper now completes its default
native/imported gates, matching 5,611 source outcomes on each selected frontend;
Policy's default gate also completes, matching 2,606 source outcomes per frontend
with native9/module9 and original6/caller1. Event's native10/module10 and five
original-public caller tests pass. Canonical/path/generator checks finish
successfully and the merged cohort is terminal and complete. Formal source
acceptance remains a separate audit.

The subsequent [key-snapshot package](runtime_service_helpers_source_review_5_0_6.md#permanent-package-and-guarded-installation)
is staged and its permanent input-driven probe is qualified against actual
source outcomes on native and imported frontends. Its
[coordinator](parity/relation_key_snapshot_installed_cohort_final_5_0_6.json.gz)
waits for the Event/Policy coordinator's successful terminal state, preserving
the frozen epoch and both cohorts' registrations. It then installs the minimal
helper repair and requires the default source/native7/module7 gates. A queued
package or completed provisional comparison is not an installed-source result.

The [shared-type package](runtime_service_types_source_review_5_0_6.md#permanent-package)
follows the key-snapshot cohort. Its
[coordinator](parity/service_types_installed_cohort_final_5_0_6.json.gz) requires
that cohort's installed source/native/imported gates before adding the permanent
258-observation verifier and missing module consumers. Both coordinators
use independent before-input snapshots after a documented pre-installation
[correction](parity/installation_snapshot_copy_correction_5_0_6.json.gz).

The [EntityStatus source replay](entity_status_source_review_5_0_6.md) and
[business integration source replay](business_integration_source_review_5_0_6.md)
provide additional scoped whole-file reconciliation without changing production
or observer bodies. These and the installed core/locking cohort are accepted
at the later installed source audit below.

The [runtime ID source review](runtime_ids_source_review_5_0_6.md) adds 3,262
exact actual-source/native/imported observations, native22/module5 checks and
82 original Release versus isolated C++ Release-ID observations. The scoped
controller is terminal and successful; its mixed-profile variant is not a full
Release runtime. Its initial Unicode JSON Lines harness failure is preserved.
The two unchanged ID source files are accepted at the later installed source
audit below, retaining that explicit profile boundary.

The [runtime service shared-type review](runtime_service_types_source_review_5_0_6.md)
adds 96 actual private-source calls and 162 retained RelationLoadPlan cases,
matching all 258 observations independently on native and imported frontends.
Three complete existing C++ lifetime/scope programs pass per frontend with
stable binaries and 1,048 verified canonical bodies. Those provisional witnesses
remain separate from the later installed gates and source acceptance below.

## Installed follow-ups and formal source acceptance

All three follow-up coordinators are now terminal with successful installed gates:
Event/Policy passes source5,611/native10/module10/caller5 and
source2,606/native9/module9/original6/caller1. The key-snapshot repair passes
source/native/imported64 each, original2/caller1 and native7/module7. Shared
service types pass source/native/imported258 each, caller1 and native5/module5.
Pinned format apply/check, canonical/path/generator validation and selected
no-work rebuilds pass. Their changes and their scoped results remain distinct
from the earlier 784-test frozen epoch.

The [formal installed audit](source_cohort_acceptance_5_0_6.md) accepts fifteen
complete named source responsibilities after joining their detailed body reviews
and actual-source/lifetime/integration evidence. Source status is **82 accepted /
40 pending / 2 excluded_external**. Capability status remains eleven qualified
and six partial, with all complete P0-P7 exits still requiring their independent
remaining obligations. No sanitizer, MIPS, GPU, external-provider or physical
device result is inferred from this checkpoint.

## Superseded 784-test core and transaction epoch

The fresh full native epoch follows the installed
[transaction source acceptance](transaction_scope_source_review_5_0_6.md#complete-installed-source-acceptance)
and [core cohort's passing scoped gates](core_sources_installed_5_0_6.md).
The current registry has 784 distinct native tests. Reference, generator and
configure steps pass. Its
[input snapshot](parity/native_current_784_core_inputs_5_0_6.json.gz) and
[execution receipt](parity/native_current_784_core_terminal_5_0_6.json.gz)
belong to this independent epoch; logs/state use
`/tmp/teaql_native_current_784_core_5_0_6`.
The snapshot freezes 10,665 execution inputs and both exact references. During
the consumer build, an independent original-source comparison reproduced nine
generator closure-parser differences: valid const/static/higher-rank closure
forms stopped parsing before subsequent metadata took effect. A separate staged
repair passed 1,763 exact source comparisons and 104 complete Java tests.

The [supersession receipt](parity/native_current_784_core_supersession_5_0_6.json.gz)
records SIGINT sent only to the verified task-owned native Ninja process.
The build exits **130** and its controller exits **1**; CTest never starts.
All 10,665 frozen inputs and both references remain unchanged before and after
termination. This is an interrupted build, not a compiler rejection or a
passing full-suite result. Completed work is retained for incremental rebuilding.

The verified two-file parser/test repair is subsequently installed in the
independent producer and passes the actual Maven 104-test selection. A new
complete native epoch must follow installed parser and SDK reproduction gates;
this unsuccessful epoch is preserved unchanged.
Current source counts are 67 accepted, 55 pending and two external exclusions.
The five core source verdicts explicitly await this full integration gate.
All previous unsuccessful epochs below remain unchanged.

## Preserved failed 782-test provider epoch

The fresh full native epoch has built all current consumers after the
[complete-root replay](sqlite_provider_source_review_5_0_6.md#complete-root-replay-and-module-registration-repair).
Its [input snapshot](parity/native_current_782_provider_inputs_5_0_6.json.gz)
freezes 10,653 execution inputs and both exact reference identities. Logs/state
use `/tmp/teaql_native_current_782_provider_5_0_6`; the
[terminal record](parity/native_current_782_provider_terminal_5_0_6.json.gz)
records a failed complete run: **778/782 pass**, with four failures, no errors
and no skips. CTest exits 8 after 3,417.95 seconds. Reference, generator,
configure, complete build, no-work rebuild and registry steps all pass. The
generated API compiler contract passes in 2,400.92 seconds. All 10,653 frozen
inputs and both references remain unchanged, and the final rebuild has no work.
The receipt's `complete: false` records failed qualification, not a running job.

The failed tests are `business_id_lookups_rust_parity`,
`sql_mutation_execution_contracts`, `entity_source_rust_parity` and
`eval_indexed_rust_parity`. They remain unresolved in this recorded epoch;
subsequent focused repairs require separate evidence and cannot relabel it as
a passing full-suite run.

The separate [four-failure follow-up](native_782_followup_5_0_6.md) now passes
native 9/9 and imported 9/9. It replays the original Entity/Eval observers before
refreshing only their manifest anchors, removes an ignored-build dependency from
the lookup comparator and corrects the stale guarded-readback assertion while
adding a positive ordinary-readback control. The original failed epoch and all
remaining full-suite/source/capability requirements retain their scope.

The later [installed transaction Send source gate](transaction_scope_source_review_5_0_6.md#complete-installed-source-acceptance)
qualifies only its complete Context source group, with separate native4/imported4
and exact-source/compiler results. Current source dispositions become 67 accepted,
55 pending and two external exclusions. Subsequent core registration work belongs
to a new input state; neither change supplies a new complete native suite result.

Before this epoch, the scoped provider gate passes native 27/27 and imported
27/27, with 15,676 exact source outcomes per frontend, all 42 original root
tests and selected final no-work builds. An initially missing imported
asynchronous-lease registration is repaired; its unsuccessful gate remains
preserved. There is no new production change, generator change or sanitizer
result. Discovery is 782 native/587 imported, independently of execution.
The whole provider remains pending and source counts remain 66 accepted,
56 pending and two external exclusions. All six partial capabilities and
complete P0-P7 exits retain their original scope.

## Current scoped cursor-capacity repair

The [SC01-SC06 checkpoint](sqlite_chunk_capacity_source_review_5_0_6.md)
repairs 250/858 actual capacity differences. Native 7/7, imported 6/6 and selected
ASan/UBSan 6/6 pass, with every capacity/result observation and all 87 preceding
binding controls. Actual UserContext deletion, pending transaction wakeup and
real dynamic-stream integration pass; leak scanning is disabled. Selected
post-test builds have no work.

One complete allocation-test source advances, to 66 accepted/56 pending/2
external. The whole provider, other source contracts and complete current native
integration remain open. Current discovery is 782 native/573 imported; these
are inventories at that checkpoint. The subsequent current whole native epoch
is recorded above; the interrupted 778 epoch remains unsuccessful.

## Preceding scoped provider lease repair

The [lease/binding checkpoint](sqlite_provider_lease_binding_review_5_0_6.md)
repairs twelve reproduced provider differences. Native 9/9, imported 4/4 plus its
separate lease comparator, and selected ASan/UBSan 4/4 pass, with all 231 exact source
outcomes per frontend and selected no-work builds. The original root tests pass 42/42
and the native selection runs their four complete ports. Leak scanning is disabled.

The interrupted 778-test epoch below remains unsuccessful. The
[whole-provider review](sqlite_provider_source_review_5_0_6.md) identified SP17's
explicit native chunk-capacity comparison, subsequently qualified above.
At that checkpoint no new full native epoch was running; complete integration
remains open until the subsequent epoch above reaches its terminal gate.
All source and capability verdicts retain their scope.

## Superseded 778-test execution epoch

The 778-test epoch scheduled **1,209** compile/link tasks. It was explicitly
superseded during its consumer build after an independent compiled-module
comparison reproduced twelve provider lease/binding differences. SIGINT was
sent to the verified native Ninja process; the build exited **130** and its
controller exited **1**. CTest was never started. This is an interrupted build,
not a complete suite result or a compiler rejection of the repaired provider.

The [input snapshot](parity/native_current_778_inputs_5_0_6.json.gz) freezes
**10,633** runtime, test, module, script, JSON-ledger and independent producer
inputs, together with both exact reference identities. Narrative documentation
and compressed evidence receipts remain outside execution inputs. Logs and the
controller state use `/tmp/teaql_native_current_778_5_0_6`. The original
[execution receipt](parity/native_current_778_terminal_5_0_6.json.gz) retains its
unsuccessful terminal result. The separate
[supersession receipt](parity/native_current_778_superseded_5_0_6.json.gz)
records the reason, unchanged frozen inputs, verified process and terminal
statuses. No compiler output was used before its process terminated.

During the build, a separate original-source observer reproduces the
[lease-before-binding requirement](sqlite_provider_lease_binding_review_5_0_6.md)
in fifteen cases. A separately compiled observer uses the idle, previously
completed module archives and BMIs; twelve actual outcomes differ. Those
artifacts and all 10,633 execution inputs remain unchanged until supersession.
The ensuing shared-provider repair and new source controls require fresh gates
and a new whole native epoch. All capability verdicts and P0-P7 requirements
remain unchanged.

## Preserved 736-test execution epoch

The previous frozen epoch is terminal: **735/736 pass**, one provenance-guard
failure, no errors/timeouts/skips. CTest exits 8 after 4,160.82 seconds. The full
generated compiler contract passes in 2,666.91 seconds. The complete consumer
build, stable no-work build, all 10,329 input/reference checks and final no-work
build pass. Every JUnit name matches the discovered registry. This remains a
failed whole-suite epoch; its `complete: false` is preserved, not a live job.

The sole failure, generated_sample_fields_rust_parity, rejects its stored clock
fixture hash before comparing outputs. The current helper adds timespec_get to
its unchanged clock_gettime prefix for the modern standard-library clock path.
The [requalification](parity/generated_sample_fields_clock_requalification_5_0_6.json.gz)
matches all 85 retained outputs against fresh reference-locked original and
native execution before refreshing only the clock provenance hash. All inputs,
expected values, baseline and producer artifacts remain unchanged; the
[previous corpus](parity/generated_sample_fields_clock_before_requalification_5_0_6.json.gz)
is preserved. The comparator guard remains intact. Its
[targeted follow-up](parity/generated_sample_fields_clock_followup_5_0_6.json.gz)
records the separate current result without relabeling this 735/736 run.

The [new input snapshot](parity/native_current_736_inputs_5_0_6.json.gz) and
[terminal receipt](parity/native_current_736_terminal_5_0_6.json.gz) retain
this independent epoch. The checked-in controller is
`scripts/run_native_integration_epoch_5_0_6.py`; its task-owned process/state/log
prefix is `/tmp/teaql_native_current_736_5_0_6`. Narrative documentation and
compressed evidence were written while it ran; execution inputs remained
frozen through the final check. Subsequent reviewed edits belong to new input
states. A process/session must be checked before treating any later job as live.
This is a native integration gate, not full G13/P0-P7 or sanitizer/MIPS acceptance.

After this epoch ended, the [native SQLite transaction-stream repair](sqlite_transaction_source_acceptance_5_0_6.md)
changed the transaction tracking helper and private SQLite provider implementation
and added a full source/lifecycle port, bringing the native registry to **737**.
Its installed native **8/8**, imported **4/4**, current original **3/3** and
**510/510 per-frontend** source replays are separate scoped follow-ups. The first
module configure failed on incomplete probe package metadata; the repaired
configuration and subsequent compilation/tests pass. Neither this repair nor
the clock follow-up turns the earlier frozen 735/736 epoch into a current full
suite pass. Current complete SQ05/P7 integration remains open.

The later [audit module/source continuation](commit_audit_source_review_5_0_6.md#durable-source-qualification)
adds the native source comparator, producing registry **738**. Installed export
checks pass 3/3, focused native3/imported2 and the extended canonical kernel
native1/imported1 pass. Fresh exact-reference private-source execution passes
the original five tests plus one observer and all 99 retained outcomes per
frontend. Seven module exports and test coverage change; audit runtime function
and header bodies do not. This remains scoped source/import qualification, not
a complete current native, sanitizer, MIPS or P7 integration result.

The later cache/pagination and Language/I18n qualifications add only permanent
canonical module registrations and development source-reproduction fixtures.
Existing C++ implementation and contract bodies stay unchanged. Store gates pass
native 6/imported 3 and 477 exact-source observations per frontend, plus two tests
of the actual complete Rust Context. I18n passes original 4, native 4/imported 4 and
4,635 exact-source observations per frontend, including generated vocabulary
installation. The native registry remains 738; these scoped source acceptances
bring dispositions to 32/90/2 without changing any complete capability status or
relabeling the historical failed full epochs. See the
[store qualification](query_stores_source_review_5_0_6.md#installed-source-qualification)
and [I18n qualification](i18n_language_source_review_5_0_6.md#installed-source-qualification).

Disk pressure during this build required bounded cache recovery. Only inactive
Rust `debug/incremental` directories under task-created `/tmp/teaql_cpp_*`
targets with locators in the current repository's verification scripts were
selected. No active Cargo target, source, dependency archive or native build
output was removed. The [priority recovery](parity/native_current_736_cache_priority_recovery_5_0_6.json.gz)
first reclaimed the private-save incremental children while preserving their
parent for the final cleanup. The [complete recovery receipt](parity/native_current_736_incremental_cleanup_5_0_6.json.gz)
records **8.32 GiB total** across 17 caches, including that priority subset;
all **364** linked outputs retain their SHA-256, size and symbolic-link target.
This is storage recovery during an ongoing build, not additional runtime test
or source acceptance. The execution inputs remain frozen.

The later [complete error-source qualification](runtime_error_source_review_5_0_6.md#durable-source-and-frontend-qualification)
adds one native source comparator, producing registry **739**. Current native
**7/7** and imported **6/6**, **2,362 exact source observations per frontend**,
three authored Rust trait tests and four compile refusals pass. All existing C++
implementation/test bodies and earlier corpora remain unchanged. Source status
advances to **33 accepted / 89 pending / 2 excluded_external**. The initial
JUnit-path driver failure and successful fresh retry remain separate. These are
scoped value/trait/ownership results, not a complete 739-test suite or full
G13/G17/P7 acceptance; all historical failed epochs retain their outcomes.

The later [SQL provenance/privacy source qualification](runtime_privacy_source_review_5_0_6.md#durable-source-and-frontend-qualification)
adds one native comparator, bringing the discovered registry to **740**. Current
native **19/19** and imported **19/19** checks, 3,091 actual source observations
per frontend, original SQL2 and privacy/masking40 pass. Both frontends complete
successful builds and final no-work checks; all 922 canonical bodies verify.
No existing implementation/test/corpus byte changes. The initial duplicate
module-test configure failure and successful fresh configuration remain separate.
Exactly three source dispositions advance to **36 accepted / 86 pending /
2 excluded_external**. This scoped follow-up does not relabel either failed
whole-suite epoch or establish a complete current 740-test pass. Complete
G14/G17/P7 integration remains open.

The later [Context logging/formatter source qualification](context_logging_source_review_5_0_6.md)
adds one native original-vector contract, bringing discovery to **741** tests.
The installed scoped selection passes **15/15 native and 15/15 imported**;
original Context2/formatter4 and 3,373 source observations per frontend pass.
The exact twelve-vector fixture, five-operation root reason and independent
query controls run through public Context capture. Both frontend builds finish
with no work; all 925 canonical bodies verify. Thirty-two prior implementation/
test/corpus/header files remain unchanged. Initial configuration/selection/path
failures retain separate records beside the passing final retry. Source status
is now **39 accepted / 83 pending / 2 excluded_external**, with no change to
the seventeen capability statuses. This is not a complete 741-test run and
does not relabel either failed whole epoch. G14/G17/P7 remain open.

## Preserved previous full epoch

Status: the complete frozen native epoch is terminal: **718/721 pass**, two
timeouts and one stale failure assertion. No tests were skipped. All **193**
generated compiler contracts pass. The
[terminal receipt](parity/p7_native_intent_epoch_terminal_5_0_6.json.gz)
records CTest exit 8, all JUnit cases, unchanged input hashes/reference identities
and a successful no-work rebuild. The original 1,168-task consumer build also
exited zero. The original registry had 719 tests; the reviewed comparison and
raw-request additions produced the actual 721-test epoch. SQ05, G13 and P7
remain open; this failed run is not relabeled as a full-suite pass.
This checkpoint uses the active Rust 5.0.6 lock and the committed native header
frontend and toolchain preset. It adds no sanitizer or MIPS profile run.

The subsequent [core primitive qualification](core_primitives_acceptance_5_0_6.md)
adds one native comparison (registry **722**) and corrects the shared system-clock
range implementation. Its current native20/imported15, targeted sanitizer2 and
source/clock gates are separate from the terminal 721-test epoch. They do not
turn that old result into a current full-suite pass. Full current SQ05/P7
integration remains open after these explicit production changes.

The later [allocation-contract checkpoint](load_state_allocations_5_0_6.md)
adds one native registration (registry **723**) and corrects repeated temporary
metadata allocation in same-shape CompactRow merge. Its current native10/imported10,
targeted sanitizer4, four original allocation tests and complete source replays
qualify that kernel repair. Full current 723-test SQ05/P7 integration remains open;
none of these targeted results changes the historical full-suite outcome above.

The [shared-state source checkpoint](shared_state_source_tests_5_0_6.md) adds the
724th native registration and repairs immutable dictionary/selection sharing
and latest-object arena lookup. Current native16/module15/sanitizer6 and finite
G16 native46/module45 pass. Actual-source G16 and adjacent state corpora also
pass for both frontends. These results qualify two complete source-test files,
bringing dispositions to 18 accepted/106 pending. The independent three-name
typed alias witness leaves G15 partial. Full current 724-test SQ05/P7
integration is still open; the complete earlier 721-test outcome is unchanged.

The [generated member-alias continuation](generated_member_aliases_5_0_6.md)
adds two native checks, producing registry 726. Its initial finite native 44
selection passes 43 and retains an infrastructure failure: the full compiler
gate could not write a PCH on the exhausted root filesystem. A fresh complete
selection passes 44/44 with the unchanged inputs in a task-owned tmpfs directory,
including all 54 positive and 139 negative generated compiler contracts. The
initial failure is preserved separately from that terminal successful retry.
Imported 32 and sanitizer 3 plus 260 actual generated observations per profile
pass. All 78 current SDKs reproduce with a restricted decoder-only change proof.
No full current 726-test SQ05/P7 result follows from this selection.

The [typed-batch/source continuation](generated_load_state_bulk_5_0_6.md) adds
two registrations, producing registry728. Its actual ordinary/inherited model
matrix compares12 complete batches and40,404 row values per profile. Native6,
module6 and targeted sanitizer3 pass, including cold metadata, foreign-thread
refusal, explicit Global retention and actual User deletion. The complete
nineteen-test source is accepted, bringing dispositions to19/105. All previous
production/producer/SDK bodies are preserved; earlier Java/compiler/reproduction
gates are explicitly reused. G15 quoted-FK5 and full current SQ05/P7 remain open.

## Execution sequence

1. Configure with the pinned native preset and finish the entire consumer build.
   The original build scheduled 1,168 compile/link tasks; previous focused
   consumer builds do not establish that all current consumers are up to date.
2. Require a successful second build reporting no work and check that the
   2,619 recorded source/build input hashes remain unchanged.
3. Discover the current registry and run the complete native CTest preset with
   its existing parallelism, processor requirements and timeouts. This includes
   the registered compiler/type-error contract rather than silently excluding it.
4. Preserve terminal build/test exit codes, logs and JUnit results. Account for
   every failure, timeout and skip before accepting the integration obligation.

The original controller polled the actual compiler process to terminal completion
before starting dependent work. Compilation failure stops the sequence. A
successful no-work build and reviewed input checks precede the current CTest run.
An active log or state file alone is not proof that a process is still running.

The subsequent [comparison-contract repair](comparator_result_contracts_5_0_6.md)
fixes two reproduced false passes and adds a host-side mutation-control test.
It changes exactly three existing test/configuration files and adds one Python
helper; C++ input bodies are unchanged. The original controller is expected to
stop at its source-input check after compilation. Preserve that result and the
original snapshot and review the exact test-only deltas. At that checkpoint the
queued registry was **720**; the next addition below increases it to **721**.
Do not silently rebase the old snapshot.

The subsequent [explicit TransactionScope check](transaction_scope_intent_5_0_6.md)
adds one native test and one canonical module consumer. Its 64 actual original
SQLite observations match both completed consumers in the separate module build.
Existing C++ input bodies remain unchanged. Native/module test configuration
changes are recorded explicitly; the current queued native registry is **721**.
The original controller can stop on the second-build no-work requirement or
the input guard after these additions. Neither stop is full-suite acceptance:
preserve its actual terminal result, review the test/configuration transition,
then require a stable complete build before executing the new registry.

The [complete intent source review](request_intent_acceptance_5_0_6.md) adds the
missing canonical ownership consumer and one existing-function module export.
Exactly two original C++ input files change: the runtime facade and its isolated
lookup each add the same `using`. Removing those lines restores their complete
prior bytes; all runtime function bodies remain unchanged. The original guard
must also account for this intentional frontend/configuration transition. Native
header compilation does not consume those two module/lookup files. The queued
native registry remains **721**; imported-only registrations do not increase it.

## Independent source and inventory checks

The [original terminal receipt](parity/p7_native_original_epoch_terminal_5_0_6.json.gz)
preserves the complete successful compiler/linker build and the subsequent
stability step's exit **1**. That step built the added raw-scope consumer and
does not report no work; no original-epoch CTest started. The recorded status is
not rewritten as successful integration or labeled a new runtime/compiler defect.

The [new immutable input epoch](parity/p7_native_intent_epoch_inputs_5_0_6.json.gz)
records the actual **721-test** registry, successful no-work build, exact frozen
runtime/test/example/module/script and producer inputs, reference identities
and seven reviewed differences from the original snapshot. All existing runtime
function bodies are unchanged; two C++ frontend files only add one existing
function export. The complete native preset runs without a test filter and
retains `generated_api_type_errors` with its four-processor requirement.

The controller preserved terminal CTest/JUnit results, verified unchanged code/data
input hashes and reference identities, and completed another no-work build.
Changes to frozen execution inputs would have invalidated this epoch. Narrative Markdown
under `docs/` and new compressed evidence receipts are outside its input scope;
generated Assist, fixture guidance and producer files remain frozen inputs.

The initial current run records `constant_temporal_rust_parity` timing out at
120 seconds and `bootstrap_extensions_rust_parity` at 180 seconds. Preserve both
failures and their committed limits. The complete compiler contract subsequently
passed in 2,544.86 seconds; full CTest finished in 3,631.17 seconds. A
[small-cohort timing diagnosis](parity/native_timeout_small_cohort_diagnosis_5_0_6.json.gz)
compares sixteen unchanged cases from each family: both return complete matching
outputs in 1.58 and 1.85 seconds. Those observations do not accept either whole
matrix or diagnose the timeout cause. Serial full-command diagnosis retains the
original inputs and limits; it is separate from the full CTest epoch.

The [serial diagnosis](parity/native_timeout_serial_diagnosis_5_0_6.json.gz)
reproduced both committed-limit timeouts while the compiler gate remained active.
Separate streaming profiles preserve complete inputs and order: all
[1,728 temporal observations](parity/native_temporal_full_profile_5_0_6.json.gz)
match in **117.90 seconds**, and all
[2,304 bootstrap-extension observations](parity/native_bootstrap_full_profile_5_0_6.json.gz)
match in **225.97 seconds**. The temporal profile preserves all three original
clock groups. These profiles use an explicit larger diagnostic allowance; they
do not pass the original CTest limits. They establish complete observed behavior
and workload timing before any later timeout/scheduling repair. Do not omit cases,
normalize timestamps or relabel the original failed run as passing integration.

The actual frozen core library passes **69/69** original unit tests with its
locked dependencies. This is source evidence, not a successful C++ full-suite
run or acceptance of all core source files.

The original 719-test registry has **235** direct active-baseline corpus bindings
and **128** custom dispatcher registrations unresolved by the simple static
inventory. An active-baseline label is not proof of correct oracle dependencies,
normalization or complete comparison coverage. Those remain SQ02 work.

The [input checkpoint](parity/p7_native_current_inputs_5_0_6.json.gz) preserves
the registry, static binding report, source hashes and original core-test output.
The [cache recovery receipt](parity/p7_native_incremental_cache_recovery_5_0_6.json.gz)
records deletion of only 14 GiB of rebuildable Rust incremental data and unchanged
hashes for all 120 linked observers in that target directory. Source, dependency
archives, linked outputs, migration edits and prior evidence are retained.

## Failure repairs and targeted follow-up

The two full matrices exceeded their former 120/180-second test budgets despite
complete matching diagnostic results. Their budgets are now **300/600 seconds**,
respectively. Corpus bytes, three temporal clock groups, comparison rules and
native preset parallelism are unchanged. Other test timeouts are unchanged.

`runtime_entity_query_contracts` expected missing setup to throw before returning
an operation. That assertion predates the source-qualified
[missing-service result repair](execution_hot_cache_and_lookup_5_0_6.md): original
Rust and the current runtime return a Graph error through the operation. The test
now waits and checks the Runtime/Graph kinds and exact Product error message.
Production execution bodies are unchanged. The existing 32-observation source
corpus remains the independent multi-model/eight-entry check.

The [targeted follow-up](parity/native_integration_followup_5_0_6.json.gz) passes
**5/5**: all three original failures and both existing lookup-result/cancellation
tests. The complete temporal and bootstrap matrices take **95.83/146.86 seconds**
in this run. Its separate immutable input snapshot records the exact transition,
including the independent generator literal repair; all captured inputs and
reference identities remain unchanged through the run. There are no failures or
skips. It is not another full 721-test run or permission to discard any original
failure.

The [canonical module check](parity/native_integration_followup_module_5_0_6.json.gz)
passes **1/1**. After generation/reproduction, all **835** canonical consumer and
fixture bodies verified, and the complete native build reported no work. At that checkpoint every
runtime/SDK C++ input from the original epoch remained byte-identical except the
reviewed test assertion; see the
[terminal validation](parity/generated_audit_literal_validation_5_0_6.json.gz).

## Acceptance boundary

A successful native integration gate will establish only its current execution
scope. It cannot approve G08's quoted-column behavior difference or G12's naming
profile difference, close the remaining source reviews, or qualify G14/G17.
The [whole-objective requirements](semantic_qualification_requirements_5_0_6.md)
and [capability ledger](cpp_generator_parity_ledger.json) retain those obligations.
The source ledger now has fourteen accepted files and 110 pending files.

The [complete evaluation-source review](eval_source_acceptance_5_0_6.md)
accepts all EV01-EV08 responsibilities in `eval.rs`, bringing the current source
dispositions to **20 accepted / 104 pending**. A reproduced Indexed Debug gap
affects all 18 new scenarios; the six-line repair restores the full snapshot
formatter and checked expired-borrow access. Native 6/module 6/SAN 4 pass, with
1,136 retained and 18 complete actual-source scenarios/180 mutations per profile
and all three original eval tests. The initial fixture compile and native/module
5/6 failures remain separate from the successful retries. All other eval header
bytes, producer and all 78 SDKs remain unchanged. Native registry is 730 and
canonical imported bodies 860; neither count establishes complete semantic
acceptance. G15 quoted-FK differences, other source responsibilities and full
current SQ05/G13/P7 and P0-P7 remain open; no historical full run is relabeled.

The [complete core Entity review](entity_source_acceptance_5_0_6.md) accepts
EN01-EN12 for `entity.rs`, bringing dispositions to **21 accepted / 103 pending**.
Its 230 new actual-source cases reproduce 111 differences; independent metadata
defaults, typed MutationIntent auditing and distinct boxed JSON dispatch repair
those paths. All 2,344 actual-source outcomes match in native 16/module 16/SAN 8.
The original non-Clone program, pure intent-header body preservation, explicit
Global retention and actual User deletion also pass. Initial compile/fixture/
include-cycle/combined-lookup failures remain preserved. Generator source and all
78 SDKs are unchanged. New core erased-state defaults do not qualify nonempty
generated adapters or every runtime caller; those remain explicit G13 work.
Native registry is 732; complete current SQ05/P7 and the whole P0-P7 objective
remain open, without relabeling the historical 718/721 full-suite result.

The [generated borrowed-root checkpoint](generated_runtime_state_bridge_5_0_6.md)
adds a terminal current compiler gate:55 positive/141 negative contracts pass.
Its native9/module6/SAN4 selection and six complete source cases remain focused
evidence. The current native registry is736; no new full-suite execution or
SQ05/P7 acceptance follows from this compiler-only completion. Historical full
native outcomes and timeouts remain unchanged.

## Later core descriptor metadata source qualification

The [metadata source review](core_metadata_source_review_5_0_6.md) adds one
native64 comparator, bringing discovery to **742**. The complete existing
metadata ownership/API body and probe also become permanent imported consumers.
Focused native **5/5** and imported **5/5**, original **3/3**, and **424** exact
source observations per frontend pass with final no-work builds. All927
canonical consumer/fixture bodies verify; existing C++ implementation/test and
SDK bytes remain unchanged. Exactly one source disposition advances to
**40 accepted / 82 pending / 2 excluded_external**. No capability status changes.
The earlier 735/736 and 718/721 full epochs remain failed checkpoints; this is
not a complete 742-test run or new sanitizer/MIPS/device qualification.

## Later data-service and request source qualification

The [terminal data-service acceptance](parity/data_service_source_acceptance_5_0_6.json.gz)
adds two native checks and three canonical imported consumers. Native 12/12 and
imported 12/12 pass after their exact target builds, with no skips and final
no-work builds. Ten unchanged original tests and 1,308 source observations per
frontend reproduce under an exact reference lock. All 930 canonical bodies,
existing production/test bodies, six native archives and 78 SDKs retain their
required identities. Source dispositions advance to 43 accepted/79 pending/2
external; all seventeen capability statuses stay unchanged. Discovery is 744.
This is not a complete 744-test run; historical 735/736 and 718/721 remain failed
full checkpoints. No new sanitizer/MIPS/device qualification is inferred.

## Complete core Expr source qualification

The [EX01-EX12 review](core_expression_source_review_5_0_6.md) accepts all 420
lines and fifty methods in the current core Expr source, including 5.0.6's
separate typed-LIKE pattern/original leaf. Actual original calls retain twelve
variants, twelve operators, fourteen functions, ordered NaN-aware composition,
the strict twenty-value threshold and all-operator subquery preparation. All
69 original core unit tests and 1,403 actual-source outcomes per frontend pass.
Native 6/imported 6, four new complete canonical consumers, exact reference
registry identities, stable builds and 1,682-file formatting pass. All 934
canonical bodies, existing C++ bodies, six native archives and 78 SDKs retain
their required identities.

Source status is now 44 accepted/78 pending/2 external and native discovery 746.
All capability statuses remain unchanged. This accepts one complete source, not
SafeExpression, complete SelectQuery, protected SQL/runtime callers or whole
G13/G14/G17/P7. Historical 735/736 and 718/721 failed full epochs and unanswered
source-behavior decisions remain. No runtime/generator implementation or SDK
body changes, new Java/generation, full-suite, sanitizer/MIPS or device result
is claimed.

## Complete SafeExpression source qualification

The [SE01-SE16 review](safe_expression_source_review_5_0_6.md) accepts all 308
lines, twenty-four methods, six built-in TeaqlEmpty implementations, public
custom predicates and the four unchanged original tests. All 580 actual
original/native/imported outcomes per frontend agree. One positive original
compiler example executes; eight negative examples preserve exact Clone/type
bounds, including each list adapter independently. Native 5/imported 5 and all
938 canonical bodies pass, with stable builds and pinned formatting of 1,683
files. Existing production/test/observer/helper bodies, six native archives and
all 78 SDKs retain their hashes.

The initial 132 mismatches were an observer JSON-macro token defect, preserved
separately; only the observer boolean identifier changed. No runtime or generator
algorithm was changed. Source status is 45 accepted/77 pending/2 external and
discovery is 747. All seventeen capability statuses remain unchanged. Historical
735/736 and 718/721 full epochs remain failed; this is not complete current
integration. Whole generated E, protected runtime/query/save callers,
G13/G14/G17/P7 and unanswered source-behavior decisions remain open. No new
Java/generation, full-suite, sanitizer/MIPS or physical-device result is inferred.

## Complete core mutation model source qualification

The [MU01-MU14 review](core_mutation_source_review_5_0_6.md) accepts the complete
291-line source: three separate map roles, six command types, MutationKind, all
fourteen methods and derived/map protocols. Actual original/native/imported
outcomes agree for all 2,048 inputs per frontend, including 240 new real-NUL,
NaN, builder and independent-batch boundaries. One compiler positive executes;
fourteen exact negatives preserve role/iteration/Default boundaries. All 69
original core unit tests and native 5/imported 5 pass. Native assertions now
cover conversion and assignment in all six role directions; production and
existing function bodies remain unchanged.

All 942 canonical bodies and six native archives/78 SDK identities verify.
Source status is 46 accepted/76 pending/2 external and native discovery 748.
All capability statuses remain unchanged. Disk pressure after testing was
resolved by removing only three task-owned incremental caches; qualified
inputs and binaries were preserved. Historical 735/736 and 718/721 full epochs
remain failed. Whole mutation/provider/governance/protected execution,
G13/G14/G17/P7 and unanswered source-behavior decisions retain their complete
requirements. No new generator/runtime algorithm, Java/generation, full-suite,
sanitizer/MIPS or device result is claimed.

## Complete SQL binding provenance source qualification

The [SB01-SB14/BT01-BT11 review](sql_bindings_source_review_5_0_6.md) accepts
the complete 137-line binding source and 342-line original test file. All eleven
original tests and 1,209 exact source/native/imported observations per frontend
pass. The full semantic port now includes typed-LIKE original privacy. Native
6/imported 6, focused ASan/UBSan 1 with leak scanning disabled, immutable corpus
replay, 944 canonical bodies and 1,684-file formatting pass.

The new matrix reproduced 120 scenarios/720 pretty tuple differences while all
semantic policy/value/secret/rebind outputs agreed. The production repair uses
the existing structural formatter for the original-operand pair only. New
nonempty-original retention and allocation-failure contracts preserve explicit
Global/User ownership. Other production/test function bodies, six native
archives and all 78 SDKs remain unchanged.

Source status is 48 accepted/74 pending/2 external and native discovery 749;
all capability statuses remain unchanged. Historical 735/736 and 718/721 full
epochs remain failed. Complete SQL/executor/provider/protected runtime callers,
G13/G14/G17/P7 and unanswered behavior decisions retain their original scope.
No Java/generation, full-suite, MIPS or physical-device result is inferred.

## Complete SQL type and SQLite renderer source qualification

The [ST01-ST15/SR01-SR06 review](sql_types_source_review_5_0_6.md) accepts
the complete 332-line SQL type source and 118-line rendering test responsibility
within the standing SQLite profile. All six original tests pass; native coverage
retains all SQLite branches in five tests and the explicit PostgreSQL-only/other
backend exclusions. All 2,210 original/native/imported observations per frontend
agree, including 632 new NUL/callback/finiteness/error/model/width boundaries.
Native 6/imported 6, exact corpus replay and 946 canonical bodies pass with stable
builds. C++ production/test/observer bytes, six native archives and all 78 SDKs
remain unchanged. No handwritten C++ or runtime/generator algorithm changed.

Source status is 50 accepted/72 pending/2 external and native discovery 751.
All capability statuses stay unchanged. New maximum-usize comparisons are
native64-only. Historical 735/736 and 718/721 full epochs remain failed; current
whole integration, full SQL/provider/protected callers, G13/G14/G17/P7 and
unanswered behavior decisions retain their full scope. No new Java/generation,
full-suite, sanitizer/MIPS or physical-device result is inferred.

## Query purpose source continuation

The [current query review](core_query_source_review_5_0_6.md) records native 28
and imported 26 passing checks after moving Purpose trace creation from the
core setter to the runtime constructor. The original core 69/shared 19 and 2,602
per-frontend inputs pass with explicit cache/lifetime boundaries. Source status
remains 50/72/2; the query schema builder is still missing. Current native
discovery is 753. These focused results and the repaired module fingerprint
metadata do not relabel either failed full epoch or establish a 753-test pass.

## Readonly query schema builder continuation

The [schema-builder qualification](query_schema_builder_5_0_6.md) records native
15/imported 15 and selected ASan/UBSan 3 passing checks, with final no-work builds.
All 480 complete original/native/imported outputs agree. These are focused
metadata/ownership/shutdown checks; source status remains 50/72/2 and native
discovery is 755. Neither the new helper nor these results turn either failed
full epoch into a current whole-suite pass.

## Complete query state and row diagnostics continuation

The [complete-state qualification](query_full_state_5_0_6.md) records native
21/imported 21 selected checks and final no-work builds. All 472 original,
native and imported inputs agree: 124 complete query states, 300 unchecked
row-extension outcomes and 48 direct error diagnostics. It preserves every SDK
byte and changes only the row exception text and missing Debug writer.
Native discovery is 759; source status remains 50/72/2. These are focused
results, not a current whole-suite run or whole-query source acceptance. Both
historical failed full epochs keep their outcomes; no specialized profile is
rerun for this diagnostic-only implementation repair.

## Read-result sidecar continuation

The [carrier-presence checkpoint](row_read_sidecar_5_0_6.md) adds 972 exact source
inputs and repairs 24 merge differences while preserving projected values and
JSON. Native 22/imported 22 pass, including every previous complete-state/error
check, with final no-work builds. Source replays add the retained 472 exact
inputs and original core 69/69. Native discovery is 760; imported discovery is
515. Source status stays 50/72/2. These remain selected gates; neither failed
historical full epoch is relabeled, and no full current integration is claimed.

## Complete core query source gate

The [complete model-source review](core_query_acceptance_5_0_6.md) accepts
CQ01-CQ17 after native 25/imported 25 and successful final no-work builds.
Both frontends exercise all sixteen corpora and ownership/fault contracts.
Fresh original/native execution covers 2,602 retained inputs; 1,405 new inputs
also run through the original and imported frontend. Another 1,924 unchanged
original observations are reused with source/observer hashes verified; this
distinction is retained in the qualification receipt. Source status is 51/71/2;
native discovery is 762 and imported discovery 517. This is whole model-source
acceptance, with no whole-suite, sanitizer/MIPS or physical-device promotion.

## Complete memory engine source gate

The [memory-engine source review](inmemory_source_acceptance_5_0_6.md) accepts
IM01-IM14 and all nineteen original tests without changing runtime code.
Native 4/imported 4 pass with final no-work builds. All 8,732 retained/new
outputs reproduce on the actual original and both current C++ frontends.
Native discovery is 763; imported discovery is 519. Source status advances to
52/70/2, while every capability verdict and both historical full-suite failures
remain unchanged. No complete current integration is inferred from this gate.

## Complete UserContext source checkpoint

The [UC01-UC25 review](context_source_review_5_0_6.md) accepts only the complete
1,631-line context.rs and its six embedded tests. Current native17/imported17
core checks and native23/imported23 delegation checks pass, with 3,362 exact
core outcomes and 1,088 freshly compiled original audit-dispatch outcomes per
frontend. The original runtime view preserves all 67 source/test files and its
273 original test names. A source-copy timestamp repair prevents Cargo from
reusing an appended observer from another disposable view.

Only this source disposition advances: **54 accepted / 68 pending /
2 excluded_external**. Existing schema, logging, language, telemetry,
commit-audit and document/reference evidence is reused at its checked unchanged
body and original scope. Ten canonical imports reuse existing test/probe bodies;
no production C++ function/header, generator or SDK changes. Full baseline,
capability, child-source, protected-publication and P0-P7 integration remain
active. The historical failed native epochs retain their outcomes.

## Complete registry and module assembly source checkpoint

The [RG01-RG24 review](registry_source_review_5_0_6.md) accepts the complete
837-line registry.rs. All 27,257 exact actual-source/native/imported observations
per frontend agree, including 1,296 new SmartList metadata/failure/replacement
cases. Current native11/imported11 pass with final no-work builds. Five canonical
module entries close previously missing decoder/graph execution; all existing
contract/probe bodies and production functions/headers remain unchanged.

The byte-checked complete original view retains 67 source/test files and its
273-test registry, with only the existing JSON access bridges appended. There
are no embedded registry tests. Existing hydration/state/Context evidence is
reused only for its explicit scope and checked unchanged bodies; changed
historical dependencies are recorded rather than silently relabeled.

Only this source disposition advances: **55 accepted / 67 pending /
2 excluded_external**. All seventeen capability verdicts stay unchanged. Current
source qualification does not accept the remaining underlying/caller files,
behavior decisions or full P0-P7 integration. The historical failed whole-suite
epochs retain their outcomes; SDKs and protected artifacts remain byte-identical.

## Complete Checker source and constructor boundary checkpoint

The [CK01-CK22 review](checker_source_review_5_0_6.md) accepts the complete
745-line checker.rs and all five original tests. A Rust-positive/C++-negative
compiler witness reproduces early generic constraints. The runtime now permits
metadata carriers while requiring Entity, allocator-aware copy and matching
typed logic before Checker erasure/registration. Its valid materialization,
Fix, required findings and sparse merge body is byte-identical.

All 2,716 exact observations per frontend, native11/imported11 and four paired
negative compiler programs pass. The paired positive program exercises five
construction combinations. Targeted native5/imported5/sanitizer5 ownership and
dispatch checks pass, with leak scanning disabled; the native/imported five
checks overlap their eleven-check selections. Existing generated compiler
1-positive4-negative and native flow1,620/governed SQLite Save540 regressions
also pass. These latter comparisons reuse their retained source corpora.

Only checker.rs advances: **56 accepted / 66 pending / 2 excluded_external**.
All capability verdicts stay unchanged. Four canonical imports reuse existing
model/original bodies and add the new constructor contract. Generator, SDK and
protected reference artifacts remain unchanged. Earlier constructor/source-fixture
failures and failed full native epochs retain their outcomes. No new whole-suite,
MIPS, external-provider, GPU or physical-device result follows from these gates.

## Consuming loaded-view checkpoint

The [loaded-view continuation](loaded_view_move_5_0_6.md) reproduces 160 differences
among 512 actual source inputs before changing the runtime. The C++ consuming
overload now preserves the receiver's graph publication role and lazy ledger
binding. Its lvalue copying body remains unchanged. All 512 outputs agree on
native headers, imported modules and targeted ASan/UBSan, including the separate
native ownership fixture on each frontend. Leak scanning stays disabled.

Retained native9/imported9 checks compare 3,712 existing outcomes and exercise
model/ownership contracts. These replays retain their historical source corpora;
only the new 512-case source observer is freshly compiled here. The failed Rust
fixture build, native fixture execution and duplicate module registration remain
recorded. The final successful registrations preserve the original baseline test.

Source counts stay **56 accepted / 66 pending / 2 excluded_external** and all
capability verdicts remain unchanged. Complete entity_runtime.rs review and
P0-P7 integration remain active. No new whole-suite or physical-device result
is claimed.


## Complete entity-runtime source checkpoint

The [ER01-ER32 review](entity_runtime_source_review_5_0_6.md) accepts the complete
entity_runtime.rs source with explicit Context/owner-thread/exception adaptations.
All 21 unchanged original tests and 4,226 exact original/native/imported outcomes
pass, including 770 new equality/dynamic-diagnostic observations. Value-first
NaN equality and delayed LedgerEntity receiver bounds repair actual source
mismatches; the two default method bodies remain byte-identical.

Native17/imported17, paired compiler1-positive3-negative and existing generated
native2/imported1 Context-retention/SQLite Save/Q/E callers pass. Final selected
builds report no work. The initial metadata collector and duplicate module-test
configuration failures remain preserved, together with historical full-suite
failures. Only entity_runtime.rs advances, to **57 accepted / 65 pending /
2 excluded_external**. All seventeen capability verdicts and complete P0-P7 exits
remain unchanged. All 78 existing SDKs and 5,741 protected artifacts retain their
starting bytes. No new full-suite, sanitizer, MIPS, physical-device or generation
reproduction result is claimed.


## Complete SQL terminal-diagnostic checkpoint

The [DX01-DX16 review](sql_diagnostics_source_review_5_0_6.md) accepts complete
diagnostic_execution.rs and diagnostic_stream.rs after exact-reference replay.
The historical SQL protocol workspace has 44 registry identity differences;
an independent source view preserves all 12 original files, runs the seven
original stream tests and reproduces 7,035 outcomes per native/imported frontend.
All retained query/mutation/transaction/stream/kind outputs stay unchanged.

Native11/imported11 and selected ASan/UBSan2 pass, including 352 new guard/journal
cases with actual User deletion, normal/unwinding close, late recorder lifetime
and throwing sinks. Leak scanning is disabled. Production functions/headers,
78 SDKs / 3,457 files, 5,741 protected artifacts and the old observer remain
unchanged; final selected builds report no work. Source dispositions advance only
the two diagnostic files, to **59 accepted / 63 pending / 2 excluded_external**.
All capability verdicts and full P0-P7 requirements remain unchanged. Executor,
mutation, runtime/governance/privacy and complete current integration stay open;
no historical failed epoch is relabeled as passing.


## Complete SQL execution diagnostic test-source checkpoint

The [EQ01-EQ06/EM01-EM21 review](sql_execution_tests_source_review_5_0_6.md)
qualifies both complete source test files. Rust 5.0.6 has twenty-one mutation
tests, six more than the historical port. New coverage retains repeated-root
identity, whole-batch old-value privacy, successful transaction/batch/guarded
readbacks and the absence of invented readbacks for zero-match/physical-delete
writes. The target-ID fixture now puts its canary in owned intent as well as trace.

All 27 unchanged original tests and native2/imported2/selected ASan-UBSan2 pass,
with final no-work rebuilds and leak scanning disabled. Production C++ sources
and headers, all 78 SDKs / 3,457 files and 6,091 protected artifacts are unchanged.
Canonical module coverage is 1,002 bodies / 1,516 exports. Native discovery stays
775 and the module preset discovers 561 imported tests; neither is a full run.
Pinned format/check passes 1,704 files. The failed nonexistent-verifier invocation
is preserved with its successful corrected follow-up.

Only execution_tests.rs and mutation_execution_tests.rs advance, to **61 accepted /
61 pending / 2 excluded_external**. All seventeen capability verdicts and complete
P0-P7 requirements remain unchanged. Whole executor/mutation/provider/governance/
privacy qualification and complete current integration remain open. Historical
735/736 and 718/721 full-suite epochs remain failed. No generator, Java, full-suite,
MIPS or physical-device acceptance is newly claimed.


## Complete SQL mutation orchestration checkpoint

The [ME01-ME20 review](sql_mutation_execution_source_review_5_0_6.md) accepts
the complete 523-line mutation_execution.rs source. New original-source inputs
reproduce 276 guarded-sidecar and 16 descriptor-cache differences. The runtime
now preserves the guarded entry's absent inherited sidecars and uses the same
executor descriptor cache for ordinary batch classification and compilation.

All 6,191 exact original/native/imported/sanitizer outcomes, 27 unchanged original
tests and native9/imported9/selected ASan-UBSan9 pass. Actual generated SQLite
Q/E and ledger composition pass native2/imported1; selected final builds report
no work. Leak scanning is disabled. New fixture/import, corpus-baseline metadata
and JUnit-collector failures remain preserved with explicit successful follow-ups;
no original expected outcome was edited.

The complete production diff covers two files; the other 6,089 protected artifacts
and all 78 SDKs / 3,457 files retain their bytes. Module verification covers 1,003
canonical bodies and 1,516 exports. Native discovery is 777 and the module preset
discovers 563 imported tests; these are inventories, not full runs. Pinned format/
check passes 1,705 files. Only this source file advances, to **62 accepted /
60 pending / 2 excluded_external**. All capability statuses and full P0-P7 exits
remain unchanged. Whole executor/provider/runtime/governance/privacy, complete
current integration and standing behavior decisions remain required. Historical
735/736 and 718/721 full-suite epochs stay failed. No new Java/generation, MIPS
or physical-device acceptance is claimed.


## Complete SQL executor source checkpoint

The [SX01-SX27/XT01-XT16 review](sql_executor_source_review_5_0_6.md) qualifies
complete executor.rs: independent transport, conditional stream/transaction
interfaces, typed errors, descriptor/shape/FIFO caches, binding recollection,
partition probes, mutation forwarding and native/buffered/dynamic stream timing.
Four missing native ports complete all sixteen embedded source tests. A 720-case
dynamic stream fixture reproduces 24 actual metadata/optional-observer ordering
differences; one shared header now captures on the flag and wraps only when
metadata and observer both exist. Initial fixture errors/output correction remain
preserved without changing any original expected outcome.

All 8,243 exact original/native/imported outcomes, 50 original tests and native19/
imported19 pass. Targeted ASan/UBSan3 covers 720 dynamic outcomes and two existing
stream/move-only lifetime checks; leak scanning is disabled. Actual SQLite dynamic
stream and generated Q/E checks pass native2/imported1, with final no-work builds.
Pinned format/check passes 1,706 files. The other 6,090 protected artifacts and
all 78 SDKs / 3,457 files are unchanged; the production diff is one header.

Module verification covers 1,007 canonical bodies and 1,516 exports. Native
discovery is 778 and the module preset discovers 567 imported tests; these are
inventories, not complete runs. Only executor.rs advances, to **63 accepted /
59 pending / 2 excluded_external**. All seventeen capability statuses and full
P0-P7 exits remain unchanged. Pending provider/runtime/governance/privacy callers,
complete current integration and standing behavior decisions remain required.
Historical 735/736 and 718/721 epochs stay failed. No new Java/generation, MIPS
or physical-device result is claimed.


## Complete SQL dialect/root source checkpoint

The [DC01-DC31/root review](sql_compiler_source_review_5_0_6.md) accepts complete
dialect.rs and lib.rs: seven helpers, SchemaIndexSpec, all 43 trait methods,
private validation/identifier bodies, 22 direct aliases and all 33 original
root/dialect test dispositions. The historical root port lacked the new grouped
partition/HAVING regression; its complete literal SQL and binding assertion is
now retained. Three non-SQLite renderer-only tests remain explicitly excluded.

A new standalone observer uses 129 exact reference registry identities; 44 old
observer differences remain historical. All 6,728 source/native/imported outcomes,
33 unchanged original tests and native13/imported13 checks pass, including actual
grouped SQLite execution. Final selected builds report no work. Production and
6,091 protected artifacts, including all 78 SDKs / 3,457 files, remain unchanged.
Pinned format/check passes 1,706 files. Module checks cover 1,009 canonical bodies
and 1,516 exports; native discovery stays 778 and the module preset discovers
569 imported tests. Inventories are not full integration acceptance.

Only the two compiler/root source files advance, to **65 accepted / 57 pending /
2 excluded_external**. All seventeen capability verdicts and complete P0-P7 exits
remain unchanged. Provider/runtime/caller qualification, current full integration
and standing behavior decisions retain their full requirements. Historical
735/736 and 718/721 full-suite epochs remain failed. No new sanitizer/MIPS/device
or Java/generation-reproduction result is claimed.
