# Core query and row source review: Rust 5.0.6

The later [complete CQ01-CQ17 review](core_query_acceptance_5_0_6.md) now accepts
the whole core model source with current native 25/imported 25, new Record and
constructor diagnostics, complete enhancement/shape sequences and explicit
source-evidence reuse. This earlier purpose checkpoint and its pending verdicts
below retain their historical scope. Source status is now 51/71/2; whole runtime
execution and G13/G17/P0-P7 remain separate.

References are runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`. This review continues G13
source reconciliation and G17 request-intent qualification. It does not accept
the whole query source, complete runtime execution, or P0-P7.

The later [schema-builder qualification](query_schema_builder_5_0_6.md)
implements and qualifies the helper missing at this purpose checkpoint. Its
source/lifetime/imported evidence is separate from the historical counts below.
Whole query.rs acceptance remains pending.

The later [complete-state continuation](query_full_state_5_0_6.md) qualifies
all 31 fields with independent mutation, full Debug/equality/Clone, recursive
list preparation and actual source-Context deletion after destination copies.
It also repairs the unchecked row-extension error boundary and adds the missing
DynamicFieldError Debug protocol. Its 472 exact inputs per frontend and
native 21/imported 21 selections remain separate from the historical counts
below. Row sidecars, initialized caches and conversion bodies still require
their complete whole-file reconciliation.

The [read-sidecar continuation](row_read_sidecar_5_0_6.md) now qualifies absent,
empty and populated relation/dynamic carriers through 972 exact source outcomes
and explicit Global retention. Its one-line merge fix preserves empty right
carriers after take operations. The full combined cache, checked-enhancement and
shared-shape/conversion body review remains pending.

## Reproduced purpose responsibility defect

The original `teaql-core/src/query.rs` distinguishes two operations:

| Entry point | Original responsibility |
| --- | --- |
| `SelectQuery::purpose` | Replace the optional purpose text, including empty text. Preserve the existing trace exactly. Construction alone does not authorize execution. |
| Runtime `PurposedSelectQuery::new` in `generated_support.rs` | Reject blank purpose, retain the untrimmed text, and append one Purpose frame with the query entity and no entity ID. Existing frames remain. |

C++ previously appended the Purpose frame inside the core setter and delegated
the runtime constructor to it. Ordinary runtime construction could therefore
look correct while direct setter calls accumulated extra execution lineage.

The [actual-source mismatch](parity/core_query_intent_gap_5_0_6.json.gz)
reproduces all 48 inputs before changing production. For an uncommented query
with three purpose assignments, Rust keeps trace counts `0, 0, 0`; C++ reported
`1, 2, 3`. Full model Debug, equality with direct field assignment, preparation,
wrapper cloning and repeated runtime wrapping expose the difference independently.

The repair removes trace insertion from the core setter and performs it at the
runtime constructor. It preserves the API, validation, existing trace frames and
Context ownership. Rewrapping deliberately adds another frame; copying a wrapper
does not. No deduplication or hidden lifetime is introduced. The generated
EntityQuery facade continues to authorize through this runtime boundary.

The witness executes the original public implementations rather than copying
their algorithms. Three entities, absent/present comments and eight purpose
values include empty and Unicode whitespace, NUL, zero-width space and long
multibyte text. Each input applies three setters and compares all nine Debug
modes for the complete core and runtime models. Rust assertion failures map to
the existing C++ exception boundary with the same message.

## Source replay and remaining whole-file review

The isolated `rust_core_query_source_5_0_6` workspace requires exact reference
registry versions and checksums. It builds unchanged retained observers, plus
the new purpose witness. Existing manifests, locks, corpora and generated
artifacts are preserved.

| Matrix | Inputs | Comparison boundary |
| --- | ---: | --- |
| Query helpers, builders and expressions | 1,089 | Complete retained outputs |
| Query/Trace diagnostics | 120 | All retained formatting modes |
| CompactRow API and JSON projection | 401 | Complete retained outputs |
| Row/list diagnostics | 169 | All retained formatting modes |
| Layout schema and presentation | 200 | Complete retained outputs |
| Dynamic row payloads and checked merges | 176 | Complete retained outputs |
| Initialized layout/snapshot cache | 81 | Eighty complete public semantic views; one explicit Arc/Weak versus Context-retention contract |
| New helper/validation boundaries | 318 | Complete outputs, including validation order, Unicode trimming, maximum integers and embedded NUL |
| New core/runtime purpose boundary | 48 | Complete outputs, including nine Debug modes |

The cache observer retains both raw initialized Debug outputs. Allocator
identities and randomized private-map presentation are not compared as stable
wire values. The existing explicit lifetime adaptation remains separate from
the exact comparisons; these 81 inputs must not be described as 81 exact raw
Debug matches.

The current SelectQuery has **31 fields**, including purpose, facets, relation
aggregates, readonly schema and persistent dynamic selection. The historical
26-field/5.0.5 acceptance remains historical. A further
[compiler witness](parity/core_query_schema_builder_gap_5_0_6.json.gz)
confirms that the original `with_dynamic_property_definitions` builder executes,
retains schema identity through Clone, and neither selects fields nor adds trace.
At this checkpoint the corresponding C++ helper was absent: its consumer failed specifically
on that missing member. Direct access to the metadata field does not qualify
this helper's source responsibility.

The later schema-builder continuation supplies implementation and schema identity,
replacement, explicit destination-copy and actual Context-destruction evidence. Whole-file
acceptance must also join the complete new-state, row-sidecar, cache and
conversion responsibilities. Passing retained builders alone does not do this.
The query source remains `pending_review` and all capability statuses remain
unchanged. A receipt's `complete` flag records that its selected controller
finished; it must not be read as whole-source acceptance.

The earlier initial controller used the wrong source-test namespace; a separate
installed attempt started before the new corpus was available. Both failures
are retained and excluded from passing evidence. The witness's first two builds
also lacked required diagnostic headers; only its successful third build supplied
the semantic mismatch. See the
[pre-repair witness record](parity/core_query_intent_witness_before_repair_5_0_6.json.gz).

Moving the unchanged layout ownership test into the canonical module registry
exposed a producer fingerprint defect: it stripped includes but retained an
existing import, whereas the strict verifier strips both. The producer now uses
the same rule. The verifier and all consumer bodies remain unchanged. Both
frontend test selections finished successfully before that metadata check;
the continuation separately verifies the corrected fingerprint and no-work
builds without relabeling the failed controller.

## Verification

The [qualification record](parity/core_query_intent_qualification_5_0_6.json.gz)
joins the [final source replay](parity/core_query_source_final_5_0_6.json.gz),
[frontend test selections](parity/core_query_source_final_installed_5_0_6.json.gz)
and [module metadata continuation](parity/core_query_source_module_qualified_5_0_6.json.gz).
All compiler/linker/test processes are terminal. Native **28/28** and imported
**26/26** pass. All **949** canonical consumer/fixture bodies remain unchanged;
the producer correction updates 61 metadata fingerprints, including the one
existing C++ import and sixty headers. Both builds finish with no work.

The original core suite passes **69/69**, including the six query tests; the
separate shared-load-state suite passes **19/19**. All **2,602** source inputs
replay on the original, native and imported frontends: **2,521** exact complete
outputs, eighty cache semantic views and one explicit lifetime adaptation.
All retained and new corpus bytes reproduce. Formatting/check passes for
**1,685** C++ files. The production delta is limited to the core setter and
runtime constructor; all other pre-existing core/memory C++ bytes are preserved.

The [artifact check](parity/core_query_intent_artifact_preservation_5_0_6.json.gz)
preserves 78 SDKs/3,457 generated files and 2,201 protected artifacts. This checks
bytes without rerunning generation or Java. Source status stays **50 accepted /
72 pending / 2 excluded_external**. Capability statuses stay eleven qualified
and six partial. Native discovery is **753**, not a whole-suite pass; historical
735/736 and 718/721 full epochs retain their failed outcomes.

No full native suite, sanitizer, MIPS, external integration or physical-device
result follows from this focused purpose correction. The later schema-builder
record has its own selected sanitizer evidence. Whole query/runtime/G13/G17/P0-P7
acceptance remains open.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --parallel 2 --target \
    teaql_core_query_intent_source_probe teaql_query_api_probe \
    teaql_query_debug_probe teaql_compact_row_api_probe teaql_model_debug_probe \
    teaql_core_shape_presentation_probe teaql_dynamic_carrier_probe \
    teaql_initialized_snapshot_cache_probe
replay_directory=$(mktemp -d)
python3 scripts/verify_core_query_source_5_0_6.py \
    --record "$replay_directory/source.json.gz"
ctest --preset native --no-tests=error --output-on-failure \
    -R '^core_query_(boundaries|intent_source)_rust_parity$'
```
