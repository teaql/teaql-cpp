# Core SmartList source review: Rust 5.0.6

Current source disposition: **accepted** at the [installed cohort checkpoint](source_cohort_acceptance_5_0_6.md). Earlier staged/pending paragraphs retain their historical scope.

Status: complete-body pre-acceptance review. The full `teaql-core/src/list.rs`
remains `pending_review`; permanent module/reproduction and current full native
integration are still required. Complete G13/G15/P0-P7 acceptance is separate.

The reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. All 506 lines,
including the two original tests, are unchanged from the exact retained 5.0.5
reference. The C++ declarations/template bodies, typed Boolean storage, ID-key
helper and existing semantic/ownership tests have been reviewed. Historical
[SmartList acceptance](smart_list_api_acceptance.md) remains a distinct checkpoint.

## Complete responsibility mapping

| ID | Original responsibility | C++ disposition and observation |
| --- | --- | --- |
| SL01 | Six public fields | Preserve ordered data, optional total count, independent aggregation/summary maps, recursive facets and loaded state. Neither the data vector nor metadata alone is the whole list. |
| SL02 | Empty/Default versus owned construction | Explicit-owner empty construction is unloaded; owned-vector/range construction is loaded even when empty. No element Default bound is added. |
| SL03 | Clone/PartialEq/Debug | Explicit destination copies preserve every nested field; equality follows element capability and keeps NaN non-reflexivity. Raw diagnostics retain complete recursive fields without inventing a serde model. |
| SL04 | Scalar metadata builders | Preserve explicit zero/maximum totals, typed Value variants and replacement semantics. Mutating data does not silently recompute total count or loaded state. |
| SL05 | Facet access and extraction | Keep const/mutable named lookup, map access, replacement/removal and take_facets. Missing names remain absent; extraction does not transfer the owning Context. |
| SL06 | Push/extend/FromIterator | Accept owned move-only elements and single-pass ranges. Consume each owned item before requesting the next, preserving applied prefixes after iterator failure. |
| SL07 | Set/get/first/last | Preserve optional lookup/replacement and ordered data. Invalid set leaves no replacement result; native rvalue arguments remain inspectable under the documented C++ calling convention. |
| SL08 | Index/IndexMut and borrowed iteration | Provide checked indexing, const/mutable spans and iteration over real elements. Out-of-bounds exceptions map the source panic boundary; ordinary C++ iterator invalidation remains the caller's responsibility. |
| SL09 | Stable retain | Preserve survivor order, release rejected ownership before the next callback and retain unvisited tail after a callback failure without calling the filter again. |
| SL10 | Count and metadata JSON views | Explicit total overrides length. Aggregation/summary lookup remains separate from their JSON projections; presentation conversion does not authorize persistence. |
| SL11 | Consuming vector conversion | Move data out without Clone. The returned vector still belongs to its allocator domain; a moved-from wrapper is valid but does not extend that domain's lifetime. |
| SL12 | Consuming map | Move each item into its callback, preserve callback/drop order and transfer all five metadata fields to the new element type. Do not add Clone/assignment requirements to mapped output. |
| SL13 | Borrowed to_list/to_set | Preserve callback order, ordered deduplication and new output ownership. Owned callback results move into the list domain; borrowed results require the explicit copy/borrow contract. |
| SL14 | identity_map/group_by | Retain ordered keys and source element Clone responsibility. Last duplicate wins in identity_map; groups preserve source element order. New owned keys require no Clone. |
| SL15 | merge_by | Retain key callback order, last existing duplicate position, one appended position per new key and final repeated incoming replacement. Iterator/key failure preserves the original applied prefix. |
| SL16 | Entity into_value | Evaluate new/dirty/deleted with source short-circuit order, convert every entity and emit only when loaded or changed. Some(empty dirty fields) still indicates change; an unloaded unchanged list still runs conversions before returning None. |
| SL17 | Entity into_values | Keep distinct MutationValues output and complete list metadata. This conversion is not a graph Save or an executed database mutation. |
| SL18 | Identifiable and versioned helpers | Keep ids/map_by_id and versions under their separate source constraints; do not add unrelated entity capabilities. Preserve conversion call order. |
| SL19 | Complete ID-key helper | Retain prefixes and exact formatting for every Value variant. NULL/TypedNull share the null key; objects/lists retain their original coarse keys and duplicate behavior rather than becoming unsupported IDs. |
| SL20 | Generic language traits | Preserve vector/range construction, owned/borrowed traversal and extension without accidental element Clone/Default bounds. C++ spans and explicit rvalue conversion adapt Rust Deref/IntoIterator ownership. |
| SL21 | Native Boolean storage | Rust Vec<bool> provides actual Boolean references. Scoped BoolVector supplies contiguous bool objects, pointers/spans and checked growth instead of C++ vector<bool> proxies. It is a storage adapter within the existing owner, not a new lifetime domain. |
| SL22 | Nonassignable replacement | Destroy/reconstruct with noexcept movement when assignment is unavailable. Preserve observed element cleanup and consumed-mapper failure order without promising transactional behavior for arbitrary throwing native moves. |
| SL23 | Aliased C++ ranges | Snapshot borrowed self/subrange/reverse inputs before mutation or growth. Reject consuming the destination's own backing vector; Rust's exclusive borrow prevents that case. Owned lazy input remains incremental. |
| SL24 | Explicit cross-domain retention | Prepare complete destination copies before foreign movement; reject noncopyable foreign retention before consuming the source. Propagate nested strings, Values, facets and row layouts; no shared Context ownership is introduced. |
| SL25 | Two original tests | Preserve complete replacement/appending order and repeated incoming-key replacement. The C++ functional source port contains both bodies alongside its unrelated existing core regressions. |

The C++ metadata builders return a list reference while Rust consumes the value.
Kernel lookup `summary_value` avoids the native field/method name collision, and
the public facets field exposes the source map alongside named accessors. These
are established language adaptations, not fewer list fields or responsibilities.

## Current exact-reference gate

The [terminal source/import receipt](parity/smart_list_source_import_5_0_6.json.gz)
is complete. Both original list module tests pass in the actual locked core
crate. The unchanged public-call driver is byte-identical to the retained
historical driver and invokes actual SmartList methods; its Entity fixture only
supplies the SPI and records callbacks.

All **1,291 complete API observations** match the immutable active corpus and
current imported implementation. They retain full recursive state after each
operation, lazy iterator/callback failures, cleanup events, Boolean storage,
generic element behavior and distinct entity conversions. The lossless corpus
is neither sampled nor projected down to final successful data.

All **169 model diagnostic inputs** also match actual original/imported outputs,
including complete row/list formatting. Together these provide **1,460 exact
paired observations**, with no expected-value or diagnostic normalization.
Raw formatting is not a substitute for the method/failure controls above.

The existing SmartList API ownership program and complete seventeen-contract
core functional program both pass. Their assertions include the two original
list bodies, null default PMR, move-only/nonassignable elements and keys,
single-pass and aliased ranges, failed foreign copy without source loss,
actual User deletion, explicit nested Global retention and all-block return.
They do not claim safety for an arbitrary caller-owned pointer graph.

All four probe/unit programs compile and link before execution. **Seven canonical
entry/helper bodies** retain their original handwritten implementations and
assertions, differing only in include prologues/paths. All **87 registry
identities** match the exact reference lock. The module archives/BMIs used and
all **10,653 frozen native inputs** remain unchanged after the gate.

## Remaining integration boundary

Temporary manifests, canonical mirrors and compiled consumers live under
`/tmp/teaql_smart_list_source_review_5_0_6`. The receipt preserves the controller,
sources/fixture hashes, manifest/lock, compiler/linker commands and terminal
outcomes. No output from the running native compiler is used.

After the frozen epoch ends, add the missing permanent SmartList API probe/unit
and original functional-source module registrations, add the comparison entry
and provide repository-owned reproduction. The existing model diagnostic module
entry keeps its independent registration. Full native/source acceptance remains
required before promoting this file. Broader loaded-state/model combinations, generated graph
execution and protected runtime publication retain their own gates.

This review changes no production function, source disposition, producer,
generated SDK, capability verdict or reference and claims no new sanitizer,
MIPS or physical-device result.
