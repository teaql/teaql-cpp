# Flat identity graph: Rust 5.0.6 continuation

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Source: `teaql-runtime/tests/flat_identity_graph.rs`, SHA256
`56a771469d24460ff3b5bd9a8d69e7b03479bed400efe325a0014546b39f214f`.

The previous C++ counterpart retained four tests from the historical baseline.
The current source has six. The two added scenarios protect the distinction
between shared read-only identity and independent mutation ownership.

## Responsibility mapping

| Original scenario | C++ counterpart |
| --- | --- |
| Macro hydration shares graph without mutation ownership | `hydration_keeps_independent_mutation_ownership`: decode two rows through `EntityDefaults::from_compact_row_with_context`; share the frozen vendor pointer while retaining separate ledger owners, original versions, changes and comments. The C++ model adapter is handwritten. |
| Independent roots resolve the same flat entity | `roots_share_entities_without_sharing_ledgers`: the same vendor address is visible from both roots; a change on one root stays local. |
| Shared reference preserves snapshot and root-local intent | `reference_preserves_snapshot_and_local_intent`: two order roots reference one platform with its own version and snapshot; root changes, general comments and per-entity comments do not contaminate the sibling or reference. |
| Equal numeric IDs are namespaced by type | `same_id_is_namespaced_by_type`: vendor and string payloads coexist at ID 7. |
| Stable typed to-many view | `stable_typed_to_many_view`: repeated access returns the same ordered list; a mismatched payload type is absent. |
| Loaded NULL differs from missing to-one | `loaded_null_is_distinct_from_missing`: an installed empty option is present, while an uninstalled relation is absent. |

All payload storage uses the UserContext arena. The fixture delegates graph
sharing, snapshot storage and mutation tracking to the runtime; it does not
implement an alternative graph or ledger. The model's hydration callback joins
its independently created state to the supplied graph, as the retained generated
model support does. No production or generated code changes are required by
this continuation.

## Verification and boundary

Run with the pinned development toolchain:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_flat_identity_source_5_0_6.py \
  --record docs/parity/flat_identity_source_installed_5_0_6.json.gz
```

The installed receipt passes **original6, native1/module1**, including all six
C++ scenarios, canonical consumers, no-work rebuilds and unchanged-input guards.
The first attempt passed the six Rust tests but its registry parser rejected the
digits in `u64`; the corrected parser and the retained
`flat_identity_source_registry_failure_5_0_6.json.gz` distinguish that verifier
error from runtime behavior.

The verifier requires a fresh receipt path. It executes all six original Rust
tests, preserving source bytes and locked dependency identities, then builds
and runs the native and imported-module C++ counterpart. It checks the exact
CTest selection, canonical module consumers, a no-work rebuild, and unchanged
captured inputs and references. One CTest executable contains all six C++
scenarios; test counts must not be described as six CTest registrations.

The Rust test uses a procedural macro to derive `LoadedOrder`; this C++
counterpart uses a handwritten model adapter. Consequently, this evidence
qualifies runtime behavior through that adapter, not generated-model acceptance
of the new hydration scenario. The source disposition remains `pending_review`
until that generator boundary has its own evidence. It does not accept the
whole graph, graph Save, resolved service or the P0–P7 goal. No new sanitizer,
MIPS or physical-device claim is made.

## Generated continuation

The [complete source review](flat_identity_graph_source_review_5_0_6.md) now
closes the handwritten-only gap using real generated Owner/Platform models.
The separate generated receipt passes original6/native2/module2 and complete
SDK reproduction. The pending disposition above describes the earlier
handwritten-only checkpoint; the complete test source is now accepted.
