# SQLite LIKE intent privacy source review: Rust 5.0.6

Status: accepted for this complete named source after original5 and installed
native1/module1 verification, each C++ executable covering all five counterparts.

## Locked source and complete scope

Reviewed source: `teaql-provider-sqlite/tests/like_intent_privacy.rs`, all 451
lines at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`5d9dc4a01f188946c6c8958d0acf51ccbf74300611f3630a3fd34a91e87bf15f`.
The counterpart is `tests/parity/teaql_provider_sqlite/like_intent_privacy.cpp`,
using the existing `fixture.hpp` for explicit Context ownership and real SQLite.
All five original tests are represented; the matrix alone is not full-source
acceptance.

## Helpers, assembly and observation

| Source responsibility | C++ mapping |
| --- | --- |
| Schema lookup by entity name | Registered `Owner` and `SecretRow` descriptors in the fixture metadata provider |
| Owner table, I64 ID/version, explicitly empty mask list | `like_owner`, exact scalar roles/types and declared empty audit mask fields |
| Child table, I64 ID/version/owner_id, Text name/visible, only name masked | `like_child`, exact field types and declared mask list containing `name` |
| Owner children relation joins id to owner_id | Exact many-relation mapping |
| Provider bootstrap then transactional seeding | Native `ensure_schema`, real executor transaction, three inserts and commit |
| One owner, one operand child, one UNRELATED child, all version 1 | Same values, I64 identities 1/2, owner identity 1, exact seed reason |
| Probe copies compiled queries and delegates unchanged SQL | Context-owned `CompiledQuery` records before native `fetch_all_compact_sql`; writes delegate directly |
| Policy observes original comment and purpose without rewriting | Context-owned pairs captured by `enforce_select`, which returns success |
| Six typed LIKE builders, pattern decoration and negative classification | Six exact Expr builders; independently derived expected pattern and row ID |

Rust Arc/Mutex observation containers become Context-owned vectors under the
existing owner-thread C++ contract. No cross-thread sharing is exercised by
this source. Seeding uses the existing native transaction handle; a separate
executor with the observing transport serves the runtime repository. Production
query, Policy, privacy, SQL compilation and SQLite execution implementations
are unchanged.

## Full input and assertion mapping

### Root and future-child matrices

Each original matrix has 48 cases: two exact operands (`RUST-LIKE-PRIVATE` and
`%RUST_LIKE\_%`), six positive/negative contain/begin/end operations, masked
`name` versus plain `visible`, and logging off/on. Both matrices run in C++.
The future-child path requests `Owner.children`; the root path requests
`SecretRow` directly.

Every case checks:

- One returned root and exactly the expected child/root identity (1 for positive,
  2 for negative predicates), including one loaded child in the relation case.
- One physical read for a direct query or two for a relation query. The root
  relation read has no original child operand binding.
- The final physical query retains the exact decorated pattern, the corresponding
  Plain/Masked parameter policy and the correct LIKE/NOT LIKE SQL operator.
- The retained caller request and every Policy observation keep the original
  comment and purpose.
- The exact safe-log count, root trace entity and relation depth. `PLAIN-CONTROL`
  stays readable in comment and purpose. Complete safe-log Debug output excludes
  the original operand when masked; plain intent remains byte-identical.
- A second, independent query on the same repository restores the original
  comment/purpose, proving request-local classifications do not leak forward.

### Raw LIKE control

The exact operand `RAW_LITERAL\` and decorated raw pattern are retained.
The physical query binds the pattern as its sole parameter. The safe comment
still contains the bare operand but excludes the exact pattern. This proves
that raw LIKE does not invent typed provenance by trimming wildcard characters.

### Cached plan control

One repository executes the same query shape with `LIKE-FIRST-PRIVATE` and
`LIKE-SECOND-PRIVATE`. Each safe log excludes the current operand while retaining
the other operand in comment and purpose. Both physical SQL strings are equal,
and each query has exactly its own current decorated binding.

### Rewritten AST control

Four cases cross logging off/on with replacing the typed pattern by `UNRELATED%`
or changing the Binary operator to Eq. They check the actual result count
(one for the pattern replacement, zero for Eq), exact physical binding, exact
log count and unchanged ordinary comment when logging. C++ modifies the actual
public AST fields; it does not rebuild a fresh expression that discards the
stale provenance before the runtime can examine it.

## Evidence and boundaries

- `parity/like_intent_privacy_original_5_0_6.json.gz`: original5, no failures,
  ignored or filtered tests; both original 48-case outputs and source/lock hashes.
- `parity/like_privacy_matrix_candidate_5_0_6.json.gz`: initial 96-case candidate.
  This checkpoint lacks the three controls and is not full-source evidence.
- `parity/like_privacy_complete_candidate_5_0_6.json.gz`: all five counterpart
  tests pass, including same-repository independent requests and all controls.
- `parity/like_privacy_installed_5_0_6.json.gz`: installed native1/module1
  pass, each with both 48-case matrices and all three controls. Canonical module
  consumer/fixture preservation, no-work builds, module paths and final
  source/reference input guards pass. The pinned formatter and check pass for
  all 1,766 handwritten C++ files.

This acceptance does not imply complete SQLite request-log/batch/privacy-source
qualification, remaining generator capability acceptance or P0-P7 completion.
No physical-router result or new sanitizer result is claimed for this
synchronous test-only addition.
