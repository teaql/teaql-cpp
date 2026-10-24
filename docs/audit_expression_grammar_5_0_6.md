# Audit expression grammar continuation (Rust 5.0.6)

The later [postfix continuation](audit_postfix_grammar_5_0_6.md) records further
source discrepancies and a verified uninstalled correction. It preserves the
current frozen producer inputs; the installed 138-case correction below and its
4,681-case regression remain separate from that proposal. Full AP02 stays open.

This review concerns the C++ producer's interpretation of container metadata.
The runtime reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; the original
generator reference is `45e1572c4956ca963410d8ea436ead3c8857087a`. The actual
dependency used by the original observer is locked `syn` 2.0.118. Neither
reference nor the original attribute parser is modified.

## Reproduced differences and shared correction

The [initial 222-case probe](parity/audit_grammar_current_probe_5_0_6.json.gz)
found 18 differences. Nine involve valid nested attributes on struct-expression
fields, struct-pattern fields and match arms: Rust reaches the later metadata
assignment, while Java stops early. The other nine have the opposite direction:
unparenthesized comparison/range chains stop Rust before later assignments, while
Java incorrectly continues. The prior checkpoint's description of this second
direction is corrected; its raw observations remain unchanged.

The [expanded pre-repair probe](parity/audit_expression_expanded_before_5_0_6.json.gz)
compares **2,016** complete source/Java observations and finds **138 differences**:
18 initial cases, 46 operator cases, 63 attribute cases and 11 expression-context
cases. It includes every pair of 23 selected arithmetic, comparison, logical,
bitwise, assignment and range operators, with flat/left-grouped/right-grouped
forms; valid, malformed, orphan and inner attributes at ten nested positions;
and unary, return, closure, cast and range contexts. Later table and indexed-layout
settings expose continuation effects beyond the numeric limit itself.

The shared Java parser now follows source infix binding order, assignment
associativity, comparison-chain rejection and range stopping boundaries. Unary
operands keep their own binding strength. Attribute parsing reuses the existing
validated helper for fields and arms, and the inner-attribute helper at match
body entry. Pattern-rest attributes and struct-expression-rest attributes retain
their different source admission rules. No generated method implements another
parser, and no new dependency is introduced.

The [candidate replay](parity/audit_expression_expanded_candidate_5_0_6.json.gz)
matches **2,016/2,016** observations against freshly executed unchanged Rust.
Every source input and result is identical to the pre-repair probe. Both owned
compiler/controller directories were removed after their processes exited.

## Permanent source regression

The independent producer retains those 2,016 original observations in
`generator/src/test/resources/cpp_expression_metadata_5_0_6.json.gz`. Its source
receipt and hash identify the pre-repair original execution; expected results
are not derived from the corrected Java parser. The existing 2,665-case corpus
remains byte-identical.

`CppAuditMetadataCorpusTest` uses one complete-metadata comparison helper for two
separate test methods. Accepted observations compare every field; original
refusals require checked rejection without claiming identical language-specific
error messages. `scripts/verify_audit_metadata_source_5_0_6.py` now checks both
corpus identities, freshly executes the complete original parser over all
**4,681** inputs, recompiles all C++ producer helpers and runs the existing Java
selection plus the new corpus test.

The [installed source gate](parity/audit_expression_installed_source_5_0_6.json.gz)
passes **4,681/4,681** observations and **113/113** Java tests. The historical
parser control is recorded separately, rather than weakening the new corpus or
rewriting the original observations to make the old implementation pass.
There are 4,591 accepted full-metadata observations and 90 original refusals.
The [historical parser replay](parity/audit_expression_historical_parser_control_5_0_6.json.gz)
freshly compiles the parser from producer commit
`ff3a7df5032a07465962c5bfc1ad49c2ca991797` with the current helpers and tests.
It preserves every original observation, reproduces exactly 138 differences and
fails the new permanent corpus test. Its `complete=false` and failed JUnit result
are the expected negative control, not current-parser failures. Both source-gate
directories were removed after terminal execution: 48,849,390 and 48,844,643
bytes respectively. The owned historical overlay source was removed as well.

## Generated entity and privacy callers

The [fresh compiler gate](parity/audit_expression_generated_compiler_5_0_6.json.gz)
extends the nine retained caller cases with all six newly reproduced expression
families. It recompiles all 261 original generator/model Java sources, generates
15 complete original and C++ model workspaces, and compiles their actual entity
descriptors. Original generated files remain unchanged during compilation.

All **45 entity observations / 210 property-policy tuples** match. These retain
the numeric limit, declaration state, complete masks, table/service identity,
property/column names and actual shared `SqlIntentRedactions::field_policy`
result. Valid nested attributes reach the later limit 29; invalid chains retain
17. Native execution disables the default PMR resource, actually deletes each
UserContext and requires zero remaining User bytes. The combined native compiler
and executable both exit successfully before their outputs are consumed. The
owned source, SDK, class, Cargo and binary directory is removed afterward
(185,451,459 bytes).

This is actual generated descriptor/privacy execution, not a SQL, runtime
assembly, full generated-request surface or complete grammar gate.

## Existing SDK reproduction and current integration boundary

The [artifact gate](parity/audit_expression_artifact_closure_5_0_6.json.gz)
regenerates all 25 example destinations, 50 contract destinations and five
independent full SDKs, then runs all three strict checks again. Every generated
implementation byte stays identical. Exactly **80** provenance records change,
and each complete JSON comparison permits only the literal-parser source hash
to change. No original Rust or historical C++ SDK changes. Generator baseline
and module-path verification also pass.

The artifact controller's own temporary directory (106,815 bytes) is removed;
the called generators also remove their owned source/class/output directories.
The stable native/module builds and canonical producer build outputs are reused.
Pinned formatting checks all 1,805 handwritten C++ files, and the resolved-source
inventory still verifies all 75 definitions in 26 groups. No runtime C++, module
export or generated implementation is changed by this continuation.

The earlier full-native epoch remains **876/877**, with its obsolete stream
comparator failure retained. Its separately repaired native and imported stream
tests each pass all 13,104 raw original observations. Complete current native
integration must be qualified against the final parser, corpus, verifiers and
provenance checkpoint; these focused gates do not substitute for that result.

The [final continuation audit](parity/audit_expression_final_continuation_5_0_6.json.gz)
checks **11,413** execution inputs against the preceding full epoch. Exactly 87
inputs differ: 80 provenance records, the stream comparator, two source/compiler
verifiers, scoped capability evidence, the Java parser, its corpus test and the
new source-observed corpus. No C++ implementation or original reference changes.
Every owned continuation directory is absent after terminal execution.

A subsequent complete native epoch uses
`parity/audit_expression_full_native_inputs_5_0_6.json.gz` and
`parity/audit_expression_full_native_terminal_5_0_6.json.gz`. Its configure,
complete build, no-work checks and **877/877** tests pass with zero
failures/errors/skips and unchanged inputs/references. The owning wrapper reaps
its child and removes 1,115,108 temporary log/state/JUnit bytes. The later
[installed syntax continuation](audit_postfix_grammar_5_0_6.md) has separate
source/compiler/artifact evidence; it does not relabel this frozen epoch.

These are finite source-based grammar and caller contracts. Complete AP02,
G14/G13/G17 and P0–P7 qualification still require their independent exits. The
approved JSON payload exception remains separate; these syntax differences do
not introduce another exception. No source disposition or capability status is
promoted here.
