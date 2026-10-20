# Context locking source review: Rust 5.0.6

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Status: complete-body review and current scoped replay pass; source promotion
still requires the current full native integration gate. The containing
Context, provider integration and complete G13/G17/P0-P7 exits remain separate.

All 137 lines of `teaql-runtime/src/context/locking.rs` are byte-identical to
the retained 5.0.5 commit `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The [historical acceptance](context_locking_api_contract.md) retains its original
native/sanitizer/MIPS scope. Current evidence replays the actual 5.0.6 runtime;
it does not relabel those historical results.

## Complete responsibility reconciliation

| ID | Source responsibility | Current C++ mapping and observation |
| --- | --- | --- |
| LC01 | Private entry owner and optional monotonic expiry; Copy/Clone | `LocalLocks::Entry` stores the scalar owner and optional `LockInstant`. Replacement copies this state without retaining User pointers. |
| LC02 | Default directory, mutex, condition variable and process-static ownership | `LocalLocks` starts empty and keeps separate synchronization. Its strings and directory belong to the explicit application Global; no process singleton owns Context data. Independent Globals intentionally have independent namespaces. |
| LC03 | Non-business local owner identity | Existing non-reused Global/User identities replace the source scalar atomic issuer. Recreated Users cannot release earlier owners' leases; user/trace labels do not change ownership. Bounded identity exhaustion remains a C++ ownership adaptation. |
| LC04 | Compute deadline before the acquisition loop | `LockInstant` uses monotonic seconds/nanoseconds and unsigned millisecond conversion, avoiding native signed-nanosecond narrowing for maximum-u64 leases. |
| LC05 | Free, same-owner and expired-entry acquisition | Insert or replace owner and expiry. Reentry has no recursion count; zero expiry removes automatic expiration. |
| LC06 | Contended zero timeout and elapsed deadline | Make one attempt for zero timeout; otherwise fail only after checking current ownership/expiry and the deadline. |
| LC07 | Condition-variable waiting and retry | Wait until the earlier expiry/deadline and recheck after every wake. Bounded C++ wait slices preserve the same deadline across large-duration waits. |
| LC08 | Owner-specific local release | Only a matching owner removes the key and notifies. Wrong-owner or absent release does nothing. C++ avoids allocating a directory for an absent release. Context destruction does not silently release leases. |
| LC09 | Provider-neutral remote interface | Preserve key, opaque owner, timeout, expiry and boolean acquire/release outcomes. Async operations and checked scoped resources adapt owned Rust futures/Arc under the owner-thread contract; no remote implementation is introduced. |
| LC10 | Optional provider | Missing acquire and release providers both return true, matching the original optional boundary. |
| LC11 | Lookup and delegation timing | First poll resolves the current provider and stable Context owner token. Provider replacement before first poll remains observable; business identifier changes do not change its owner token. |
| LC12 | Scoped inputs, provider borrows and pending work | Copy the key into the requesting User domain, validate provider identity before virtual use, forward child wakes and cancel/join pending work before Context reclamation. Provider resources must outlive pending borrows; no shared owner extends Context lifetime. |
| LC13 | Private/default and public trait protocols | Retain the source private scalar Copy/Clone and empty directory. The public provider requires no Clone/Debug/serde/default capability. C++ explicit allocation, quota failures and owner-thread rejection retain their existing documented adapters. |
| LC14 | Caller and namespace boundaries | Locking is distinct from query-build coordination, SQL transactions and authorization. Token construction has separate Context-construction responsibilities; only its stable opaque identity is observed here. This review does not claim source process-wide exclusion across independent C++ Globals or concurrent access to one User. |

The implementation remains in `locking.hpp`,
`detail/context/locking.hpp`, `context/locking.cpp` and the existing Context
resource/identity boundary. No runtime function, declaration or dependency is
changed by this review. The source file itself has no embedded tests.

## Current original and imported execution

The [terminal receipt](parity/context_locking_source_replay_5_0_6.json.gz)
checks the unchanged source bytes, complete observer manifest/lock, driver,
reference dependency versions/checksums, helper provenance and immutable corpus.
All **222** original and imported C++ outcomes match. They cover empty/Unicode/
NUL keys, three owners, optional and replaced providers, reentry, wrong-owner
release, Context recreation, zero and maximum-u64 durations, expiry and stable
owner identity despite business-label changes.

The provider fixtures implement only their SPI. The actual original local lease
algorithm is unchanged. Rust keys receive unique per-script prefixes to isolate
its process-static table. Both observers project opaque owner strings to
first-seen identities, retaining equality across providers and Context changes;
there is no additional output normalization.

Both actual original unit tests pass separately: local ownership/expiry and
remote delegation/ownership. The existing complete imported programs and
comparator pass **3/3**, with no failures, errors or skips. They include the
full locking lifetime/budget/wakeup/cancellation body and the unchanged Context
tail port. The C++ ownership body disables default PMR allocation and verifies
actual User deletion, zero retained User blocks, directory usability after quota
failure and wrong-thread
rejection.

The initial selection incorrectly requested two nonexistent CTest names and ran
only one of the required three. The explicit cardinality guard rejected it;
its [failed receipt](parity/context_locking_source_initial_selection_failure_5_0_6.json.gz)
remains preserved. The corrected selection uses the actual registered names.
This was an observer-selection failure, not a runtime mismatch.

All 10,665 frozen inputs and both exact reference identities remain unchanged
against the then-active [784-test native epoch](native_integration_5_0_6.md).
That epoch was subsequently superseded for the separate generator closure
repair; it provides no complete native result. The source
remains pending until its required complete native gate and formal disposition.
No new sanitizer, MIPS, physical-device or complete capability result is claimed.

## Reproduction

The receipt includes its complete controller and executed commands. With a
completed matching module build, the scoped comparison is:

```bash
source scripts/toolchains/activate.sh
cmake --build --preset modules_native --target teaql_module_locking_probe \
    teaql_module_locking_tests teaql_module_original_runtime_context_tail
ctest --preset modules_native --output-on-failure --no-tests=error \
    -R '^module_(context_locking_contracts|original_runtime_context_tail|context_locking_rust_parity)$'
```

Original build and exact test selectors are retained in the receipt's compiler
logs and observer provenance; they use the reference-locked
`rust_final_baselines_5_0_6` workspace and the unchanged runtime manifest.
