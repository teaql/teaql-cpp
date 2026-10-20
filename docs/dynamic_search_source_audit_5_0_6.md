# SQLite dynamic-search source review: Rust 5.0.6

Status: accepted for this complete named source after original1/native1/module1
verification.

## Locked source

This review covers all 146 lines of
`teaql-provider-sqlite/tests/dynamic_search.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`e501706f75819d6d556619d0ae6a9d679255f8037fdafdbb184e4d31830a7c56`.
The source contains one integration test and its schema lookup helper.
Its C++ counterpart is
`tests/parity/teaql_provider_sqlite/dynamic_search.cpp` with `fixture.hpp`.

## Baseline change

The original 5.0.6 source replaces the old loose request comment with a validated
QueryIntent: comment `what: search tenant schools`, purpose `verify dynamic
search`. The query still carries a separate Purpose trace node containing
`why: verify schema drift isolation`.

The C++ fixture previously set the trace text as the core query purpose and
inferred the request intent from that query. It now leaves the base query purpose
unset, constructs the exact explicit QueryIntent, and checks its comment/purpose
before executing. The original trace node remains unchanged. Both capture flags
remain true. This changes the test input to match the locked source; it does
not change production runtime behavior.

## Complete responsibility mapping

| Source responsibility | C++ counterpart |
| --- | --- |
| Schema lookup over Platform and School by exact name | Fixture metadata registration and executor lookup |
| Platform: search_platform with I64 id/tenant_id and Text name | Exact descriptor and ID role |
| School: search_school with I64 id/tenant_id/platform_id and Text name | Exact descriptor and ID role |
| Physical schema through the runtime Context provider boundary | use_sqlite_provider and ensure_schema |
| Two Campus platforms, tenants 7 and 8 | Compiled native seed inserts with exact IDs and tenant values |
| Six school rows distinguish outer tenant, related tenant and name filtering | Exact six tuples, including foreign tenant row 4, foreign parent row 5 and Other row 6 |
| Search model fields and platform relationship | Same trusted School/Platform search model map |
| Base projection, tenant filter, descending id, limit/hard limit and intent trace | Same query fields, ordered projection and trace node; retained before-copy |
| Trusted name binder | Eq over actual name using the supplied string value |
| Trusted platform.name binder | IN subquery with its own tenant 7 predicate and Campus name condition |
| Unknown trusted field branch | Same Unbound trusted field error |
| Trusted order binder | Ascending order over the supplied trusted path |
| Exact dynamic JSON with stale fields, relation and order | Byte-identical JSON request, including ignored secret values |
| Input base remains unchanged | Full base equality with the retained copy |
| Hard limit and source trace survive merging | Exact hard limit 2 and trace equality |
| Validated request intent plus diagnostic capture | Exact 5.0.6 QueryIntent and both flags true |
| Real SQLite rows retain both tenant scopes and ordering | Exactly two rows with typed I64 IDs 3 then 2 |
| Schema drift produces four warnings | Exact warning count 4 |

The existing C++ assertion that serialized warnings do not contain `secret`
remains as supplementary coverage. It is not counted as another original Rust
test. No local replacement for the runtime dynamic-search merge or SQL executor
is introduced. Requests, metadata, models, warnings and nested expressions retain
explicit Context allocation; native SQLite cleanup remains in the fixture.

## Evidence

- `parity/dynamic_search_original_5_0_6.json.gz`: exact original test, 1 passed,
  zero failed/ignored/filtered, with source and manifest/lock input hashes.
- `parity/dynamic_search_installed_5_0_6.json.gz`: installed native1/module1
  pass, canonical consumer/fixture bodies, no-work builds, module paths and
  final source/reference guards pass. The pinned formatter and check pass for
  all 1,767 handwritten C++ files.

The module counterpart uses the canonical test and shared fixture bodies. This
review does not establish full dynamic-field storage/query support, complete
SQLite provider qualification, remaining generator capabilities or P0-P7 exit.
