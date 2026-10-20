# SQLite guarded mutation source review: Rust 5.0.6

Status: accepted for this complete named source after original Rust, installed
native and canonical module verification.

## Locked source and scope

The reviewed source is `teaql-provider-sqlite/tests/guarded_mutation.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`e782dd19798abe5e90fe139458d1d4a42395e13530ef89bbf2a13bb1692d2336`.
This review covers all 229 lines, including schema lookup, entity metadata,
seeding, the nine ordered guarded mutations and final database assertions.
The source contains one test. It does not establish acceptance of the complete
SQLite provider or the generator parity plan.

The 5.0.6 change constructs each request from a `MutationCommand` with the
nonempty reason `audited provider conformance test`. The retained C++ test now
uses that exact reason and checks it before every guarded execution. C++ uses
its existing `MutationRequest` construction API; this is a semantic counterpart,
not a claim of identical language syntax.

## Responsibility mapping

The implementation is `tests/parity/teaql_provider_sqlite/guarded_mutation.cpp`;
its shared setup is `tests/parity/teaql_provider_sqlite/fixture.hpp`.

| Original responsibility | Installed C++ counterpart |
| --- | --- |
| Schema lookup returns the descriptor only for its entity name | Registered `Order` descriptor in the executor metadata provider |
| `Order` maps to `orders`; I64 ID/version/tenant and Text name are non-null | `order_entity` retains all four types, roles and nullability flags |
| In-memory SQLite connection and explicit table creation | Real SQLite transport and the same four-column table |
| Compile and execute three inserts with version 1 | `fixture.seed` invokes the dialect and native transport for IDs 1, 2 and 3 |
| Tenant 1 update of row 2 is rejected; update of row 1 succeeds | Affected-row assertions 0 and 1; names `stolen` and `updated` |
| Tenant 1 delete of row 2 is rejected; tenant 2 delete succeeds | Affected-row assertions 0 and 1 with expected version 1 |
| Tenant 1 recovery is rejected; tenant 2 recovery succeeds | Both use expected version -2; affected rows 0 and 1 |
| Cross-tenant hard delete fails; same-tenant hard delete succeeds | Row 3, expected version 1, affected rows 0 and 1 |
| Guarded cross-tenant update inside an actual transaction fails | Row 2, expected version 3, affected rows 0, followed by rollback |
| Row 2 survives with tenant 2, version 3 and original name | Typed I64 assertions and exact `tenant-two` text after rollback |
| Row 1 retains its authorized update; row 3 no longer exists | Exact `updated` text and I64 zero count |

The supplementary C++ `tenant_changes_at_the_write_boundary` test remains
unchanged. Its transport changes a row's tenant immediately before the UPDATE;
the statement's own predicate rejects the write, returns no persisted snapshot,
and leaves the new tenant, original version and name intact. This is additional
C++ coverage, not a second original Rust test.

## Ownership and layers

The fixture retains separate metadata, dialect, SQL executor and native SQLite
transport responsibilities. Requests, expressions and results use the explicit
fixture Context resource. The native connection and transaction retain their
existing cleanup contracts. Neither the test update nor the module consumer
introduces a new allocator, ownership domain or production execution path.
The module consumer is generated from the canonical test and fixture bodies;
it imports `teaql_provider_sqlite` instead of project declaration headers.

## Evidence

- `parity/guarded_mutation_original_5_0_6.json.gz`: exact locked Rust provider
  integration test, 1 passed, no failed, ignored or filtered tests.
- `parity/guarded_mutation_native_candidate_5_0_6.json.gz`: standalone C++
  candidate with the current reason and assertion, including the supplementary
  write-boundary case. This is candidate evidence, not installed module evidence.
- `parity/guarded_mutation_installed_5_0_6.json.gz`: installed native1/module1
  pass with both cases in each executable, canonical consumer/fixture body
  verification, no-work rebuilds, module path checks and final source/reference
  input guards. The pinned formatter and formatter check both passed for all
  1,764 handwritten C++ files before acceptance.

No physical-router execution or new sanitizer result is claimed for this
request-reason/test-registration change.
