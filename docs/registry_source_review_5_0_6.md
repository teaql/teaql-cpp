# Complete registry and runtime assembly source review

RG01-RG24 qualify the complete named source under the established C++ Context,
owner-thread and explicit-retention adaptations. Current native11/imported11
checks and all 27,257 exact source observations per frontend pass, with final
no-work builds. Complete G12, G13, G15, G17 and P0-P7 acceptance retain their
independent requirements.

The reference is all 837 lines of Rust 5.0.6
`teaql-runtime/src/registry.rs`, SHA-256
`9a64562cf1f61c16f561c006df7be0eec9f1548b1dc1022dd7f748539da0b4bc`,
at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. There are no embedded tests in
this file; the original runtime's 273-test registry is checked independently.
The earlier [registry contract](registry_api_contract.md) remains a historical
5.0.5 source review and does not acquire a new baseline label.

## Complete responsibility mapping

| ID | Original responsibility | C++ mapping and qualification boundary |
| --- | --- | --- |
| RG01 | Public bounded read-hydration adapter | `decode_compact_rows_with_read_metadata` shares row layouts, reserves the final vector, decodes in input order with one shape cache, and returns the first error without publishing a partial vector. The accepted dynamic-loading and typed-batch evidence supplies actual sidecar/payload/sharing observations; it is distinct from graph installation. |
| RG02 | ReadHydrationShapes identity and retention | Rust retains the source base/selection Arcs alongside augmented availability so reused addresses cannot alias. Native scoped allocation identities are never reused, so the pass-local map needs no retained row payload. Its key contains both base and selection identities; values retain geometry alone. Explicit Context retention replaces shared lifetime extension. |
| RG03 | Private dynamic-sidecar decode | Consume the sidecar before the typed callback; reject unsupported carriers before decoding, propagate typed decode errors, require indexed loaded state, augment/check availability, and propagate installation errors. Private values remain row-specific. Exact existing dynamic source observations, accepted core dynamic/state sources and unchanged C++ fault contracts retain these branches. |
| RG04 | Decoder registry models and registration | Default and Clone retain five independent maps, including JSON capability/edge callbacks. Registration uses the static Entity name, independently of descriptor names. No source Debug/default behavior is invented. Existing decoder and JSON matrices observe replacement, missing entries and independent copies. |
| RG05 | JSON capability guard | Lookup by installed name, then check property/relation lengths and source membership equality before calling field_layout. Table/privacy metadata are not extra equality inputs. Missing/failed layout and dynamic-property support remain separate outcomes. All 24,576 original capability/edge cases retain exact errors and guard precedence. |
| RG06 | JSON edge callbacks | Decode many rows before installing a loaded list in the parent's view. To-one uses only the first projection, including an explicit absent relation for no rows. Each child receives its coordinate and a fresh weak graph state. Native read-view reservation is the established coordinate-allocation adaptation; it creates no persistent owner. |
| RG07 | Compact single decoding | Use a fresh weak graph state, consume sidecars through the shared helper, validate convertible u64 identity, then install the typed entity. Zero is not independently forbidden. Decode or identity failure occurs before installation. |
| RG08 | Compact batch decoding | Share layouts, use one weak state and one hydration cache for the batch, and install each successful entity immediately. An error retains earlier installed entities and prevents later decoding. This partial-install contract is deliberately distinct from list atomicity. |
| RG09 | List and complete SmartList decoding | The Vec entry constructs source SmartList defaults. The metadata-preserving entry decodes the complete vector before installation, then carries total_count, aggregations, summary, facets and is_loaded unchanged. New direct observations distinguish every field, nested facets, default Vec metadata, empty/failed/duplicate rows, missing identities, registry Clone and pre-existing relation replacement. |
| RG10 | Optional compact relation | Decode only the first row, ignore later rows, preserve absent versus present-None, and install at the ordinary view-zero boundary. No positive-ID or extra cardinality constraint is introduced. |
| RG11 | Registry dispatch and missing errors | Compact, batch, list and option retain their distinct missing-decoder messages. JSON entries retain their own error family. Registered native calls additionally validate root/builder ownership before dispatch; missing callbacks preserve their source-first error precedence. |
| RG12 | MetadataStore SPI and observer defaults | Entity lookup/enumeration, log recording, query-debug capture and execution capture remain separate. Query observers depend only on execution capture; mutation observers are always returned. Both forward complete ExecutionMetadata to the actual store. Native callbacks check their borrowed store's owner on invocation and explicit copy. |
| RG13 | InMemoryMetadataStore roles and derives | Sorted replacement by descriptor.name, independent Clone/schema snapshots, public descriptor-store registration, exact Default/Debug and all three SPI roles remain distinct. Native descriptor arrays are borrows and cannot outlive the store; get_entity creates an explicit scoped snapshot. |
| RG14 | EntityRegistry and in-memory membership | Empty/default/new, replacement/idempotent registration, with_entity, Clone and exact ordered map Debug are preserved. Native membership uses an ordered set; its diagnostic writer retains the source name-to-same-name map representation. |
| RG15 | RequestPolicy defaults | All five select/insert/update/delete/recover defaults return success without modifying their input. Policy installation and actual protected execution remain separate responsibilities; this file alone supplies neither tenant rules nor persistence authorization. |
| RG16 | Behavior SPI and registry | Five permissive before hooks and empty relation_loads remain independent. Registry Default/Clone, key replacement, missing lookup and fluent registration preserve the selected callback. Native managed ResourceBorrow replaces Arc sharing without cloning callback behavior or extending its owner. |
| RG17 | RuntimeModule state | All ten fields remain distinct: public metadata, membership, behaviors, checkers, sinks, optional language, initial graphs, root graphs, bootstrap callbacks and graph decoders. Default and explicit destination Clone preserve their separate snapshots. No Debug derive is invented. |
| RG18 | Typed entity/descriptor/behavior registration | entity and entity_with_behavior register descriptor-name membership/metadata and static-name decoders. A descriptor-only entry does not invent a decoder; behavior-only does not manufacture membership. Native validates callback domains before mutation, retaining the approved fail-closed ownership adaptation. |
| RG19 | Checker, event and language registration | Delegate to their separate registries, append sinks, replace same-key checkers and install language only when Some. An empty module replaces its registry roles without resetting Context language. Underlying Checker/event algorithms retain their own source exits. |
| RG20 | Initial and root graph registration | Single/bulk methods append in order, retain duplicates and preserve separate graph roles. Clone, apply_to and into_context retain independent graph snapshots. Root create-if-absent versus initial reconciliation belongs to the separate bootstrap algorithm; registration does not execute it. |
| RG21 | Generated bootstrap callbacks | Retain callback insertion order independently of invocation. Context installation replaces its callback vector. The 384 unchanged actual schema observations exercise real Context assembly and ordered success/error/cancellation; the separately accepted dynamic-schema lifecycle retains its own wider scope. |
| RG22 | apply_to and into_context | Install all module roles, initial/root graphs, callbacks and decoders; apply optional language last. Native prepares scoped destination copies before replacing borrowed Context services. Its const fluent module and explicit GlobalContext factory replace consuming Rust ownership without changing installed responsibilities. |
| RG23 | module macro | Recursive and terminal entity/behavior arms preserve registration order, duplicates and trailing commas. Native typed builder chains replace macro syntax. The retained 198-case decoder/module corpus includes sixteen original macro invocations and independent Clone/into_context observations. |
| RG24 | Context and resource ownership | Borrow metadata/results/callbacks only from a valid domain; reject sibling/shorter-lived callback retention. Explicit Global copies preserve nested storage after actual User deletion. Existing decoder/module/registry and graph contracts retain quiescence, replacement, resource-close and cyclic graph-copy checks. No new ownership domain is introduced. |

This is a production-body review, not a symbol inventory. Metadata, policies,
behavior, graph decoding, module assembly, Context and SQL/provider execution
remain independent layers.

## Exact-reference execution

The independent `rust_registry_source_5_0_6` project uses unchanged drivers via
manifest paths. Its registry versions/checksums are checked against the frozen
reference. A byte-checked complete runtime view appends only the existing JSON
private-access bridges in registry.rs and entity_runtime.rs. All 67 original
source/test/fixture bodies remain unchanged; fresh copy mtimes force compilation
of this view. No alternate graph decoder or module implementation is installed.

| Observation family | Exact inputs |
| --- | ---: |
| Metadata, schema snapshots and observer defaults | 321 |
| Complete registry Default/Clone/Debug scripts | 18 |
| Typed decoder and module/macro registration | 198 |
| Typed JSON capability and edge callbacks | 24,576 |
| Graph/module initial and root registration | 464 |
| Schema callbacks through real module assembly | 384 |
| New complete SmartList decoding boundary | 1,296 |
| **Total per frontend** | **27,257** |

The new observer supplies metadata and rows, invokes the actual registry, freezes
its graph and reads the result. It does not recreate installation or decoding.
It checks all six SmartList fields, recursive facet metadata, maximum u64 totals,
NULLs, Unicode summaries, success/error ordering, and preservation of an earlier
relation after a failed replacement. Default PMR is disabled in its C++ process;
all inputs, outputs and graphs use UserContext storage.

An initial new-probe compilation incorrectly passed optional<u64> to the generic
JSON fixture helper and used a recursively instantiated lambda presentation
helper. The [failure archive](parity/registry_source_initial_probe_compile_failure_5_0_6.json.gz)
preserves the invalid probe and compiler output. The presentation was corrected
before execution; no production API or result was changed to make it compile.

Five additional canonical module entries import unchanged existing decoder,
JSON and graph probes/contracts plus the new SmartList probe. Four corresponding
module comparators exercise those real imported targets. Registration/body
counts do not substitute for their compilation and execution.

The [execution receipt](parity/registry_source_execution_5_0_6.json.gz) binds the
complete original view, exact dependency lock, terminal builds, all seven
comparison families and eleven named tests per frontend. The new immutable
corpus is created only after original/native agreement; old corpora and observers
remain unchanged. The gate itself keeps whole-source acceptance separate from
its execution `complete` flag.

## Evidence joins and remaining scope

The [dynamic query hydration](dynamic_query_hydration_5_0_6.md),
[complete dynamic storage](dynamic_storage_acceptance_5_0_6.md),
[typed-batch state](generated_load_state_bulk_5_0_6.md),
[whole core query/row](core_query_acceptance_5_0_6.md),
[JSON callback](json_graph_decoder_5_0_6.md) and
[Context](context_source_review_5_0_6.md) reviews retain their individual
responsibilities. RuntimeModule's generated checker and sink installation does
not accept every implementation in checker.rs, event.rs, entity_runtime.rs,
entity_save.rs, JSON graph parsing or generated provider/sample assembly.

The [reuse record](parity/registry_source_reuse_5_0_6.json.gz) explicitly identifies
the unchanged hydration bridge and other relevant implementation hashes from
the earlier qualified dynamic-storage scope. Changed historical core/interface
hashes are recorded separately rather than claimed unchanged; the current
core/state source reviews and fresh registry observations supply their present
contracts. The complete private hydration helper is reconciled at RG01-RG03;
its existing dynamic-carrier fault and lifecycle evidence retains its original
scope. The MetadataStore, RequestPolicy and Behavior pure defaults also receive
direct body review and current native/imported default-hook tests.

The [preservation proof](parity/registry_source_preservation_5_0_6.json.gz) checks
all 3,457 files in 78 SDKs, 2,201 protected artifacts and sixteen pre-existing
registry C++ implementation/observer/contract bodies. No runtime function/header,
generator or prior observer body is changed. Pinned format/check covers 1,698
handwritten C++ files, including the new probe; 982 canonical bodies, 1,516
exports and seven acyclic packages verify. No Java or generation reproduction is
rerun for this runtime observation/registration change.

The separate [whole-source qualification](parity/registry_source_qualification_5_0_6.json.gz)
promotes only registry.rs, bringing dispositions to **55 accepted / 67 pending /
2 excluded_external**. All seventeen capability verdicts stay unchanged. The
source decision joins all twenty-four reviewed responsibilities with execution,
the explicit evidence reuse and preserved before-state; none of the numeric
totals alone establishes acceptance.

The G08/G15 quoted-FK and G12/G14 invalid naming-profile behavior decisions,
all remaining source/caller/governance reviews and full current P0-P7 integration
remain active. Historical failed whole-suite epochs retain their outcomes.
No new GPU, external integration, MIPS or physical-device result is implied.

## Reproduce

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_registry_source_5_0_6.py --modules \
    --record /tmp/teaql_registry_source_fresh_5_0_6.json.gz
```

The first creation of the new immutable SmartList corpus additionally uses
`--bootstrap-corpus`, and writes it only after original/native agreement.
Existing corpora are always checked and never regenerated by this gate.
