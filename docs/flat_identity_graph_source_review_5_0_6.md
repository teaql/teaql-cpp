# Flat identity graph test source review: Rust 5.0.6

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Complete source: `teaql-runtime/tests/flat_identity_graph.rs`.
SHA256: `56a771469d24460ff3b5bd9a8d69e7b03479bed400efe325a0014546b39f214f`.

This review covers all six test bodies and their model/setup helpers. It does
not accept the implementation of the complete graph or resolved data service,
all macro/generated APIs, graph Save, or the P0–P7 goal.

## Complete responsibility mapping

| Rust test | Counterpart and assertions |
| --- | --- |
| `macro_hydration_shares_identity_graph_without_reusing_mutation_ownership` | `hydration_keeps_independent_mutation_ownership` preserves the two-row setup and unchanged decoding-source graph. `generated_flat_identity_isolation` additionally uses actual generated `Owner::from_compact_row_with_context`, `entity_runtime_state`, `updateName`, `changedName`, `evalName` and entity comments. Independent roots resolve the same frozen entity address; versions 3 and 8 remain local; neither sibling nor graph source acquires the other's changes or comments. |
| `roots_share_flat_entities_without_sharing_mutation_ledgers` | `roots_share_entities_without_sharing_ledgers` constructs the roots before freezing the graph, verifies identical vendor addresses, and verifies one root's change is invisible to the other. |
| `shared_read_only_reference_preserves_snapshot_version_and_root_local_intent` | `reference_preserves_snapshot_and_local_intent` preserves three distinct owners, root versions, reference version 11, snapshot equality, empty reference changes/new/deleted keys, and both request and entity intent isolation. The generated continuation repeats these assertions using generated `Owner` and `Platform`, also checking positive entity/request comment values and generated field reads after updates. |
| `the_same_u64_id_is_namespaced_by_entity_type` | `same_id_is_namespaced_by_type` installs both vendor and string at ID 7 and resolves their distinct values. |
| `frozen_graph_exposes_a_stable_typed_to_many_view` | `stable_typed_to_many_view` verifies stable list identity, length, ordered contents and absence for the wrong payload type. |
| `frozen_graph_distinguishes_loaded_null_from_missing_to_one_view` | `loaded_null_is_distinct_from_missing` distinguishes an installed empty typed option from an uninstalled relation and checks relation presence. |

The original `Vendor` helper maps to the allocator-aware handwritten `Vendor`.
The original macro-generated `LoadedOrder` has id/version/name scalars; the
retained generated `Owner` carries those scalars and an additional platform
foreign key. This is an explicit model-name/shape adaptation, not a claim that
the two SDKs have identical declarations. `Owner` exercises actual generated
scalar decoding, snapshot installation, hydration callback, field updates and
entity-intent storage. Generated `Platform` supplies the immutable reference.
The separate handwritten tests retain the original graph construction order
and typed vendor/string/list/optional payload cases.

All C++ state is scoped to UserContext. Stable pointers are borrows within that
Context. Graph sharing does not prolong its lifetime. Existing tests in the
same generated executable additionally check explicit destination copies,
actual source Context destruction, stale source expressions, retained Global
intent, and provider-backed save; those are supporting regressions rather than
additional responsibilities attributed to this Rust test file.

## Qualification gate

`scripts/verify_flat_identity_source_5_0_6.py --generated --record <fresh.json.gz>`:

1. Checks the exact locked Rust reference and captures source, scripts, fixtures,
   producer inputs and reference identities.
2. Builds both generator workspaces and reproduces complete Rust/C++ SDKs for
   `relation_identity_contracts` and `query_alias_contracts`, requiring file-set,
   byte and provenance equality. Generated files are not patched.
3. Runs the six unchanged original integration tests against a byte-checked
   complete runtime view with the locked dependency identities. Only disabled
   external exporter development dependencies are omitted.
4. Builds and runs both C++ counterparts in native and imported-module modes:
   two CTest executables per profile, with six handwritten scenarios and the
   generated isolation scenario plus retained composition regressions.
5. Checks canonical module consumers, binary hashes, no-work rebuilds, and
   unchanged captured inputs/references before recording success.

The terminal installed receipt and a separate source-disposition receipt are
required for acceptance. The earlier handwritten-only receipt remains a
historical intermediate result. The earlier registry-parser failure passed
Rust's tests but did not qualify the complete gate; it remains preserved.

No production implementation or generator template changes are part of this
continuation. No fresh sanitizer, MIPS, physical-router, performance or complete
cross-language parity claim is made.

The first generated gate stopped before SDK comparison because its reproduction
script read an obsolete `/tmp` classpath cache. The script now resolves each
workspace's classpath into its own disposable directory through Maven. The
failure receipt is retained as
`parity/flat_identity_generated_classpath_failure_5_0_6.json.gz`; no generated
artifact or production source was changed to address it.

## Terminal result

[Installed gate](parity/flat_identity_generated_installed_5_0_6.json.gz):
**original6, native2/module2**, complete SDK reproduction, canonical module
consumers, no-work rebuilds and unchanged-input/reference guards all pass.
[Source acceptance](parity/flat_identity_graph_acceptance_5_0_6.json.gz)
records only this complete test file. Active totals become **88 accepted,
34 pending, two external exclusions**; capability totals stay **11 qualified,
six partial** and the full goal remains incomplete.
