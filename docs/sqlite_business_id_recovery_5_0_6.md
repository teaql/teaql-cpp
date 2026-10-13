# SQLite business-ID commit-failure recovery

Status: source replay and targeted recovery/lifetime gates pass. The subsequent
complete native refresh exits successfully (492/492); full CTest/P7 acceptance
remains open. Runtime baseline:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` (5.0.6).

## Defect and original behavior

Fresh replay of the historical SQLite business-ID inputs initially matches
197 of 198 observations. The failing deferred-foreign-key case commits an
allocation that violates the constraint. Original Rust returns the native error
while leaving SQLite's transaction open: another allocation reports nested BEGIN,
and the caller can explicitly ROLLBACK through the parent provider.

C++ used its asynchronous SqliteNativeTransaction as the allocation's Context
cleanup object. After failed COMMIT, that object kept the exclusive transaction
owner identity. Parent operations consequently reported a connection lock error
instead of allowing the original recovery. This was a production mismatch, not
a stale expectation or missing request intent.

## Runtime correction

Synchronous allocation now registers a separate cleanup object with a recovery
identity. It does not acquire the public asynchronous connection lease. The
identity remains only after an allocation actually starts its native transaction;
failed nested BEGIN does not claim or roll back someone else's save.

Successful provider statements returning to autocommit clear both applicable
ownership identities. The synchronous cleanup rolls back only when its own
recovery identity is still current. Explicit caller ROLLBACK therefore disarms
old cleanup; destroying that Context cannot roll back a later transaction.
Unrecovered failed commits still roll back during Context quiescence, before
returning arena blocks. Public asynchronous lease rules remain unchanged.

The original allocator's `COMMIT` error/transaction timing is preserved. No
implicit rollback is added at that error point, no error is normalized and no
extra runtime dependency or ownership domain is introduced.

## Evidence

All **198/198** historical input calls now match actual locked Rust plus C++.
Complete errors, allocation/storage rows and transaction/autocommit observations
are saved separately as `sqlite_business_id_5_0_6_cases.json.gz`. The historical
driver/corpus remain unchanged; the active business-ID comparator uses the new
artifact and exact active commit. The reproducible provider replay script now
includes transport, ID-space and business-ID families.

Native regression additionally checks failed COMMIT followed by a native nested
BEGIN error, explicit provider recovery, Context deletion while a newer leased
transaction stays active, and automatic cleanup of another unrecovered failure.
The existing save-isolation, independent allocator, range, storage, Global-copy
and concurrent allocation assertions remain.

Relevant native checks pass **5/5**. Targeted ASan/UBSan passes **5/5** with leak
scanning disabled, including the business-ID lifecycle/source tests and unchanged
lease/asynchronous scheduling/full historical transaction comparisons. Fresh live
Rust replay matches **198/198**. Pinned format/check passes **986** handwritten
C++ files. Host SQLite is not instrumented; lifetime gates qualify project code.

## Full native refresh

The first complete refresh was deliberately interrupted (exit 130) after this
provider mismatch was found. All compiler processes stopped before editing.
That interrupted run is not a successful build result. Complete native refresh
was restarted after the correction and targeted gates and exits successfully
after 492 build steps. Full CTest remains outstanding; prior whole-suite failure
records are not replaced by this build or the five selected passes. Later
generated readonly-property changes require their own build/integration gates.

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_sqlite_transport_probe teaql_provider_sqlite_business_id_tests
python3 tests/parity/compare_sqlite_business_id_5_0_6.py --cpp build_native_current/teaql_sqlite_transport_probe --rust /tmp/teaql_cpp_field_layout_rust_506/debug/sqlite_transport_5_0_6
python3 tests/parity/replay_sqlite_provider_5_0_6.py --cpp build_native_current/teaql_sqlite_transport_probe --rust /tmp/teaql_cpp_field_layout_rust_506/debug/sqlite_transport_5_0_6
```

Complete provider/error/interleaving, generator capability and P7 gates remain
open. No new module, MIPS/physical-device, GPU or external integration acceptance
is inferred.
