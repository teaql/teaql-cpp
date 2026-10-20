# SQLite dynamic field storage source review: Rust 5.0.6

Status: accepted for this complete named source after original5 and installed
native2/module2/focused-ASan+UBSan2. Each profile includes all five source
counterparts plus the retained SQLite storage regression executable.

## Locked source

Reviewed source: all 550 lines of
`teaql-provider-sqlite/tests/dynamic_field_storage.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`988e10b88fd6d8b99949b614e3e4cb9308959c3a7d22ba76ec88772b5b80beec`.
The counterpart is
`tests/parity/teaql_provider_sqlite/dynamic_field_storage.cpp`, with the existing
`fixture.hpp`. It supplements the retained
`teaql_provider_sqlite/tests/dynamic_storage_tests.cpp`; those broader C++ cache,
lease and error regressions are not removed or counted as original Rust tests.

## Shared helpers and setup

The fixture uses an empty metadata provider, a real in-memory SQLite connection,
SQLite dialect and independent SQL data-service executor. Definitions retain
owner type, exact field types and revision `test-v1` unless the test explicitly
requests `test-v2`. Writes retain namespace, U64 owner identity and typed changes.
The exact original query comment/purpose and mutation reason are used:
`read typed extensions`, `verify storage invariants`, and
`framework graph persistence regression`.

C++ definitions and field carriers are registered through the fixture's explicit
UserContext. A returned field is a checked borrow of its carrier; constructing a
carrier as a temporary value is insufficient for publishing that borrow.
Rust's Arc-owned selected-code identity maps to the existing Context-owned
`selection_identity()`. Moving each batch carrier into the Context retains that
identity and does not deep-copy the shared selection geometry.

## Complete five-test mapping

| Original test | Direct C++ assertions |
| --- | --- |
| context_schema_initializes_storage_and_ambient_store_cannot_write | Install database provider for School.note in extension-demo; Context schema runs twice; ambient store is unbound; PRIVATE-CANARY write rejects with exact binding-required code; provider load returns both requested IDs with empty values; write Debug omits the canary |
| native_transaction_owns_dynamic_commit_rollback_and_typed_identity | Transaction store is bound and shares source/binding identity; cloned transport retains source identity, independently constructed transport differs; write School and Platform in first namespace and School in second; validate then apply; same native transaction executes ordinary parameterized DML; rollback removes native and dynamic writes; commit preserves all three typed namespace values; test-v2 rejects with exact definition mismatch |
| all_scalar_types_roundtrip_exactly_and_json_null_is_not_sql_null | Commit and reload all 13 exact original operands under U64 maximum owner ID; complete map/type equality; both floating-point bit patterns exact; typed JSON null state is Value while SQL NULL state is Null |
| null_delete_and_missing_remain_distinct_and_nonfinite_rejects_before_any_write | Valid first write plus NaN second write rejects the whole batch with the exact error; both requested owners remain empty; commit NULL and text, deduplicate IDs 1/2/3/1; NULL differs from NotLoaded; shared selected-code identity matches actual present keys; committed Delete removes the stored NULL row |
| dropped_transaction_rolls_back_and_large_id_batch_is_bounded | Apply abandoned value in real transaction, explicitly cancel the Context-owned transaction, then load all 1,200 IDs and require 1,200 empty owner entries |

### Exact scalar operands

The 13 values are false, I64 minimum, U64 maximum, F64 bits
`0x3fd5555555555555`, negative zero, decimal
`12345678901234567890.12345678`, the original SQL-looking Unicode text,
empty LargeText, JSON object containing code é/count 42, JSON null,
date `1995-09-01`, timestamp `1750000000123`, and SQL NULL.
Every returned Value must equal its expected variant and payload, with separate
bit equality for both F64 values. JSON null is tested through the public typed
field-state API, not inferred solely from the serialized payload.

### Native transaction and ownership boundaries

The transaction test enters through `SqlDataServiceExecutor::begin`, obtains its
actual `SqlDataServiceTransaction` store, and uses that transaction's native
transport for ordinary DML. Thus rollback is checked across both ordinary and
dynamic storage on the same transaction; an ambient transport cannot substitute
for it. The separately constructed executor shares the native connection but
must not acquire the original provider's source identity or binding key.

C++ Context results are borrows. The original lexical Drop test is represented
by the existing explicit `Transaction::cancel()` abandonment operation, which
rolls back before the subsequent ambient read. It does not call commit or the
normal rollback API to manufacture a passing Drop result, and does not extend
the transaction lifetime through shared ownership.

## Evidence

- `parity/dynamic_field_storage_original_5_0_6.json.gz`: exact original5, zero
  failed/ignored/filtered tests, with source and manifest/lock hashes.
- `parity/dynamic_storage_source_candidate_5_0_6.json.gz`: retained initial
  failure. The fixture tried to publish field borrows from unregistered carrier
  values. A debugger identified `DynamicFieldValues::field` and `ArenaBorrow` as
  the rejection path; this is the existing allocation-domain rule.
- `parity/dynamic_storage_source_candidate_v2_5_0_6.json.gz`: registers scalar
  and batch carriers through UserContext before field access, then passes all
  five counterpart tests without production changes.
- `parity/dynamic_storage_source_installed_5_0_6.json.gz`: native2/module2/
  focused-ASan+UBSan2 pass, including the retained storage regressions. Canonical
  consumer and fixture preservation, no-work builds, module paths and final
  source/reference guards pass. Leak scanning is disabled per project policy.
  The pinned formatter and check pass for all 1,768 handwritten C++ files.

Full G16 capability qualification is separately documented in
`dynamic_storage_acceptance_5_0_6.md`. Accepting this test source does not qualify
other pending provider/runtime source files, remaining capabilities, physical
router execution or P0-P7 completion.
