# Core root upgrade audit: Rust 5.0.6

Status: pending complete current-source qualification. The historical
[5.0.5 root review](core_root_source_acceptance.md) is not current acceptance.
The active reference `teaql-core/src/lib.rs` has 1,030 lines and twenty-six root
tests at commit `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`c1f1f8b32f3a70f241e7f689ce3387e41d42979b14dd02477eae30a0979d781a`.

## Reviewed baseline delta

A direct diff from the locked 5.0.5 commit shows these changes:

- Public dynamic_fields and dynamic_properties modules, plus private load_state
  and request_intent module declarations.
- Root reexports for EntityJsonTraversal, FieldLayout, LoadedSnapshot,
  RelationShapeCache, CompactRowLayout and the four request-intent models.
- A hidden serde_json crate reexport used by Rust-generated code. C++ does not
  acquire this Rust dependency; generated/runtime JSON contracts use the existing
  native JSON/value infrastructure. This language-specific binding needs explicit
  treatment in the full root review.
- Expansion of one existing test, `java_style_string_match_builders_expand_like_patterns`:
  both `tea` and `%tea_\%`, each through all six string-match builders, must retain
  distinct original and expanded pattern strings. A raw `like(name, "%tea%")`
  separately retains a Value operand rather than LikePattern provenance.

The other twenty-five root tests retain their source bodies in this diff. Their
current macro/helper dependencies still require execution and review; unchanged
test text alone is not acceptance of new dependency behavior.

## Current C++ evidence and gap

`verify_public_exports.py` passes eight core public module namespaces and
seventy-eight root reexport names. Its lexical inventory does not establish
member behavior or cover the bare `pub use serde_json;` statement. Root
EntityGraph types continue to use their distinct core namespace rather than the
runtime identity graph. The final eval module remains included in the scan.

The installed functional fixture already uses LikePattern for the six `tea`
cases, but omits the escaped/wildcard input and the raw-LIKE Value distinction.
The [isolated corrected candidate](parity/core_root_like_candidate_5_0_6.json.gz)
retains the entire existing functional fixture, replaces only that test body,
and adds all twelve original-pattern checks plus the direct raw-LIKE comparison.
The complete functional executable passes against unchanged current native
archives. No production Expr behavior was changed for this result.

The candidate remains outside the worktree during the frozen SDK integration.
After installation, verify the full original-root functional, entity-mapping,
composition and presentation responsibilities on native and canonical imported
consumers. Reconcile the current root exports and dependency behavior with the
complete source before promoting its disposition. Existing complete Expr source
acceptance is a dependency, not proof of this whole root test suite.

## Current original execution

The [fresh original-core receipt](parity/core_root_original_5_0_6.json.gz) records
69/69 unchanged library unit tests passing, with zero ignored or filtered cases.
All twenty-six root test names are extracted from the current source and checked
individually in the successful output. Source, crate manifest, reference lock and
baseline lock hashes are unchanged across execution. The command uses the exact
reference checkout with offline/locked resolution and an external target directory.
This is original-side evidence, not a claim that all sixty-nine tests are root
contracts or that the C++ root is already accepted.

The native counterparts for the twenty-six root tests are distributed across
`tests/parity/teaql_core/entity_mapping.cpp` (ten model/mapping contracts),
`tests/parity/teaql_core/functional_contracts.cpp` (six root expression contracts
among its seventeen total checks), `tests/parity/teaql_core/composition_contracts.cpp`
(five root query/list/SafeExpression contracts plus other checks), and
`teaql_core/tests/presentation_api_tests.cpp` (five root presentation contracts
among its broader API assertions). The current queued installer refreshes the
functional body only. Whole-root qualification must also execute the other
canonical bodies against the current native/imported implementation and reconcile
helper/macro behavior; the queued functional gate alone is insufficient.
