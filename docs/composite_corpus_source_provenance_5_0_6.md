# Composite observer source provenance

Status: nine more baseline-only corpora have checked original-source execution
bindings and matching current native outputs. This brings the three source-binding
cohorts to **20 of 27** baseline-only direct corpora and **52,639** observations.
SQ02/GP13, G13, G14, G17 and the P0-P7 plan remain open.

The locked references remain runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Complete observations and original callers

The [contract registry](../tests/parity/composite_corpus_source_contracts_5_0_6.json)
identifies every original receipt, producer version, main fixture, auxiliary
source, selected input function, data file, external Rust module and native
comparison.

| Corpus | Observations | Original caller responsibility |
| --- | ---: | --- |
| Resolved query schema | 2,304 | Result schema and cache paths |
| Resolved typed query schema | 192 | Typed decoder and result schema |
| Resolved dynamic typed | 288 | Prepared selection and unsupported carrier checks |
| Resolved dynamic guard | 72 | Intent and provider guards |
| Resolved dynamic loaded | 288 | Indexed hydration and Value/NULL/empty/error states |
| Resolved enhanced typed | 3,456 | Enhanced indexed query entry variants |
| Resolved stream schema | 769 | Readonly result schemas and cursor boundaries |
| Resolved mutation execution | 3,518 | Bound mutations and event delivery |
| Resolved mutation privacy | 5,486 | Privacy propagation and post-provider versions |
| Total | 16,373 | Nine complete original callers |

The [current replay](parity/composite_corpus_source_current_native_5_0_6.json.gz)
successfully builds the three distinct native probes and matches all **16,373**
outputs. The [source-only replay](parity/composite_corpus_source_initial_5_0_6.json.gz)
checks the archived source chains separately. Original Rust execution is reused
from the named receipts; neither replay claims a new Rust execution.

## Assembly is checked against the actual compiled observer

The [asset verifier](../scripts/corpus_source_assets_5_0_6.py) complements the
[shared source verifier](../scripts/verify_corpus_source_contracts_5_0_6.py).
The main fixture is insufficient here: the original producers insert retained
query/stream/mutation drivers, schema helpers, an access bridge and privacy
observation code before compiling the Rust test.

Each contract pins all consumed source assets separately. There are 17 archived
asset references across these contracts, including evolved producers, input
generators and helpers. Current assets must equal their current files; archived
assets must differ from their current files and match both the independent pin
and the original receipt input hash. Existing Context and resolved-operation
contracts retain their preceding current/archived observer rules.

For an inline producer, the verifier selects its actual AST statements from the
fixture assignment through the rendered observer assignment, retaining nested
closures and branch conditions. The stream producer has an existing `render`
function, which is selected directly. Producer entry points and their build
commands are not executed. The selected renderer reads only declared archived
assets and must consume every declared assembly input.

The resulting text must equal the actual compiled observer and its retained
rendering summary byte for byte. The injected absolute input path is retained.
External Rust `#[path]` modules and literal include dependencies must match the
declared compile inputs and their original/current hashes. This prevents an
unaccounted current-file fallback from completing an older observer.

Both the archived and current selected input functions must reproduce every
corpus input in order. Their data-file reads are tracked and checked against
original/current hashes; undeclared or unused data inputs reject. Selecting a
different function cannot silently reduce cardinality.

For each caller, the shared verifier also checks all 67 original runtime source,
test and vector files, the sole appended source target, the original manifest
against Cargo metadata and all 119 registry package identities against the
locked reference. The original selected test must have recompiled the runtime,
used offline/locked selection, exited successfully and emitted the complete
marker-framed stdout. That actual stdout must match both the receipt observations
and every current expected value. No input or expectation is rewritten.

## Independent controls and permanent tests

[Twenty-four effective controls](parity/composite_corpus_source_controls_5_0_6.json.gz)
reject missing or wrong auxiliary pins, incorrect version labels, missing
original identities, rehashed replacement helpers, omitted or unused assembly
inputs, a wrong input entry function, missing or changed data-file identities,
changed corpus inputs, wrong renderer selection, changed rendering summaries,
self-consistently rehashed but incorrectly assembled observers, unaccounted
external Rust modules, the wrong privacy branch and the wrong append target.
All nine untouched original chains pass first.

Two permanent host checks preserve this contract:
`parity_composite_corpus_source_contracts` and
`parity_composite_corpus_source_controls`. Each of the nine native comparisons
requires the source-contract check through a CTest fixture. The source-control
registration is also recognized as a provenance invariant by the custom
dispatcher controller.

The [registered native gate](parity/composite_corpus_source_registered_gates_5_0_6.json.gz)
passes **26/26** affected checks: all 20 source-bound comparisons and the three
pairs of source/negative checks. JUnit names match the expected set, with no
failure, error or skip. Selecting only the dynamic-loaded and mutation-privacy
comparisons automatically adds and successfully runs their shared source fixture
first (**3/3**). The configured native registry has 885 tests; this is a targeted
gate, not a whole-suite result.

The [dispatcher replay](parity/composite_corpus_source_dispatcher_5_0_6.json.gz)
runs the actual new invariant. It does not fabricate a C++ output mutation for
an observer-provenance check. The
[current locator](parity/composite_corpus_source_inventory_5_0_6.json.gz)
retains 305 direct active-baseline bindings and 173 custom registrations requiring
full review. Locator classification alone proves neither provenance nor parity.

## Historical evidence, remaining scope and cleanup

The [preceding controller snapshot](parity/corpus_source_before_composite_5_0_6.json.gz)
preserves the previous shared verifier, dispatcher and CMake registration bytes;
their hashes match the preceding resolved-operation milestone. Its
[document snapshot](parity/corpus_document_before_composite_5_0_6.json.gz)
also retains that checkpoint exactly. Earlier execution records are not rewritten
or relabeled as current source execution.

At this checkpoint, seven baseline-only corpora remained: generated support, the memory engine,
mutation intent/privacy, row construction, row enhancement, row read sidecars and
dynamic schema source. The subsequent
[complete baseline-only cohort](baseline_corpus_source_bindings_5_0_6.md) supplies
fresh source/native evidence for those seven without relabeling this checkpoint.
Embedded provenance in other
direct corpora, custom dispatcher selection/dependencies/normalization, complete
generated metadata and current whole integration retain their own obligations.
Capability verdicts stay **14 qualified / 3 partial**.

There are no production C++, generated SDK, dependency, module export or memory
algorithm changes. No Java, sanitizer, MIPS or device execution is added. The
[milestone receipt](parity/composite_corpus_source_milestone_5_0_6.json.gz)
binds only this scope. The
[cleanup receipt](parity/composite_corpus_source_cleanup_5_0_6.json.gz)
lists the exact owned temporary directories removed after child completion.
Stable preset outputs, original fixtures, compact evidence and earlier
uncommitted work remain intact. Shared Rust targets are not built or evicted.
