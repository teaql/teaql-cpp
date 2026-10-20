# SQLite provider lease and binding order: Rust 5.0.6

Status: original and compiled-module failure reproduction complete; repaired
native, imported and targeted sanitizer gates pass. This is a scoped repair,
not whole-provider source acceptance.

The reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` from
`upstream.lock.json`. Complete provider qualification still requires all of
`teaql-provider-sqlite/src/lib.rs`, its tests and the independent transaction,
dynamic-storage, runtime and publication responsibilities. This finding does
not reduce that scope or change any capability verdict.

## Source order and the uncovered combination

The original root `SqlTransport::execute_sql` and `fetch_all_compact_sql`
first await `transaction_lease.lock()`, then call their synchronous methods.
Those synchronous methods convert binding values before locking the native
connection. The original native stream similarly awaits its root lease before
`bind_values` and the synchronous connection lock.

The previous C++ `SqliteCall::poll` and `SqliteStream::poll_next` in
`teaql_provider_sqlite/src/provider.cpp` convert bindings before calling
`cursor.lease.acquire`. An unsupported List binding can therefore take their
early error return before encountering a held transaction. The
[compiled-module receipt](parity/sqlite_provider_lease_binding_module_before_5_0_6.json.gz)
reproduces **12/15** differences: every held-transaction case returns a binding
error on its first poll, including the three cases the source cancels while
still Pending. The three idle controls agree. Compilation and linking finished
before execution; all linked archives and BMIs retain their hashes afterward.

The existing 144-case lease corpus exercises valid bindings and real transaction
release. Its successful results do not cover this invalid-binding combination.
No old input or expected outcome has been modified.

## Actual original observations

The [original-source receipt](parity/sqlite_provider_lease_binding_original_5_0_6.json.gz)
retains the complete observer and manifest, resolved lock, registry identities,
provider source hashes, commands, compiler terminal result and all observations.
Every resolved registry identity matches the reference lock. The observer calls
the actual original provider; it contains no local lease or query implementation.

All **15** original observations succeed:

| Inputs | Source observation |
| --- | --- |
| Execute, compact fetch and native stream, each with a held transaction released by commit, rollback or drop | Both initial polls are Pending. After release, the next poll returns the unsupported List-binding error. |
| The same three operations, cancelled while the transaction is held | Both initial polls are Pending. Dropping the future produces no completed error result; dropping the transaction restores autocommit. |
| The same three operations on an idle connection | The first poll returns the binding error immediately. |

Autocommit is restored in every case. A new task-owned observer under `/tmp`
uses the existing exact-reference compiler target cache; no runtime dependency,
generator, generated SDK or frozen native execution input changes.

## Shared provider repair

`SqliteLease` now separates acquiring the asynchronous lease from checking the
native connection's poisoned lock. Root asynchronous execute/fetch carries a
private sequencing flag; synchronous and transaction-bound calls keep their
existing bind-before-native-lock order. Native root streams wait before binding;
transaction streams keep their unleased source order. Repeated fetch retains
native lock/prepare before binding conversion.

A stack-local native-guard marker prevents a binding-stage C++ exception from
poisoning a connection whose native lock was not entered. The dynamic-stream
wrapper preserves that distinction and still poisons on exceptions during actual
chunk enhancement. Queued/owned leases retain existing cancellation and Context
cleanup. No public API, allocation domain, shared owner, generator or SDK changes.

The permanent oracle project is
`tests/parity/rust_sqlite_lease_binding_5_0_6`. Its original fifteen-case driver
and corpus preserve the earlier observations. A separate actual-source driver
adds **72** controls:

| Inputs | Required boundary |
| --- | --- |
| Four asynchronous entries and four binding forms on a poisoned idle connection | Ordinary execute/fetch/stream binding errors precede native lock failure; repeated fetch locks and prepares before conversion. A valid binding exposes the lock error. |
| Idle synchronous execute/fetch, four binding forms and poisoned/unpoisoned connections | Preserve original synchronous precedence and complete successful rows or errors. |
| Held root execute/fetch/stream with Object and overflowing U64 bindings, four terminal/cancel paths | Preserve pending polls, release, precise error and cancellation outcomes beyond List. |
| Held repeated fetch with all four binding forms and four terminal/cancel paths | Preserve lease waiting, valid rows, binding errors and cancellation. |

The guard fixture deliberately triggers the original checked-Debug timestamp
panic and the C++ exception inside a real native connection guard. It does not
set a synthetic poison flag. All **87** outcomes match actual locked Rust; both
observers also reproduce all original fifteen cases. The
[source receipt](parity/sqlite_provider_lease_binding_guard_original_5_0_6.json.gz)
retains commands, hashes, the exact dependency check and original outputs.
Held-poisoned connections and release/MIPS arithmetic remain outside this matrix.

The repaired [native gate](parity/sqlite_provider_lease_binding_native_repaired_5_0_6.json.gz)
passes **9/9**: both new comparisons, the existing 144-case lease comparison,
native transaction-stream cleanup, provider transport and all four original
root-test ports. This includes **231** source observations and all **42** root
test responsibilities. The unchanged actual original root tests separately pass
[42/42](parity/sqlite_provider_root_tests_current_5_0_6.json.gz).
The selected post-test build reports no work. Complete provider acceptance still
requires its other full-source obligations and current integration.

The [imported/sanitizer gate](parity/sqlite_provider_lease_binding_module_sanitize_repaired_5_0_6.json.gz)
passes **4/4 imported** plus the standalone 144-case lease comparator, and
**4/4 ASan/UBSan** with leak scanning disabled. Each frontend agrees with all
**231** source outcomes and reports no selected build work afterward. Checked
poison controls are registered only in Debug; the fifteen ordinary lease cases
retain the other profiles. No new release/MIPS result is inferred.

## Verification boundaries

1. Preserve the complete pre-repair twelve-difference observation and original
   fifteen-case matrix. Wait for all compiler/linker processes before using outputs.
2. Qualify asynchronous root execute/fetch/stream ordering without changing
   synchronous error precedence. The same `SqliteCall` implementation serves
   both interfaces, so moving one statement unconditionally is insufficient.
3. Preserve binding-error versus poisoned-native-lock precedence. Original
   asynchronous lease acquisition and native connection locking are separate
   stages; current `SqliteLease::acquire` also checks poisoning. A naive move
   ahead of binding conversion can introduce another difference.
4. Preserve repeated-fetch prepare-before-bind behavior, transaction-bound
   operations, synchronous leased-connection rejection and native cursor cleanup.
   Query-stream compilation errors and dynamic stream-plan/chunk validation
   retain their existing pre-cursor failure boundaries.
5. The interrupted native epoch is terminal, with an explicit supersession
   receipt, before changing its frozen inputs. Imported and targeted sanitizer
   lifetime checks preserve actual transaction Context deletion/cancellation.
   The [whole-provider review](sqlite_provider_source_review_5_0_6.md) identifies
   the separate mandatory SP17 chunk-capacity contract for the next source work.
   Complete necessary source repairs before starting another full native epoch;
   never relabel the interrupted epoch as passing.

Reproduce selected gates with the locked tools; routine use defaults to native:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_sqlite_lease_binding_5_0_6.py \
    --profiles native \
    --record /tmp/sqlite_lease_binding_native_fresh.json.gz
```

Use `--profiles modules_native sanitize` for the explicitly selected imported
and lifetime gates. Receipt paths must be fresh; the verifier compares original
results before using compiled C++ consumers and leaves historical corpora intact.

## Terminal checkpoint and preservation

The [preservation proof](parity/sqlite_provider_lease_binding_preservation_5_0_6.json.gz)
finds exactly one changed file among the 6,091 protected artifacts:
`teaql_provider_sqlite/src/provider.cpp`. The other **6,090**, independent
producer inputs and both reference identities are unchanged. Ten new execution
files install the observers, exact-reference project, corpora and verification
tool; seven existing execution inputs change as explicitly recorded, including
build registration and the partial evidence ledgers.

Pinned formatting and checking pass **1,707** handwritten C++ files. All module
paths, **1,516** exports, **1,010** canonical bodies, Python syntax, generator
ledger and whitespace checks pass. Native discovery is **780**, imported
discovery **571**; these are inventories, not full-suite results.

The first post-reconfigure module build exits zero while regenerating dependency
metadata; the initial no-work assertion fails and remains in the
[initial static receipt](parity/sqlite_provider_lease_binding_static_5_0_6.json.gz).
The [separate stability follow-up](parity/sqlite_provider_lease_binding_static_followup_5_0_6.json.gz)
confirms no work for native, imported and sanitizer selections without rerunning
already passing runtime tests. The
[scoped qualification](parity/sqlite_provider_lease_binding_qualification_5_0_6.json.gz)
joins these records and explicitly leaves whole-provider and full-goal acceptance
false. Source status stays **65 accepted / 57 pending / 2 external**; all six
partial capabilities remain open.

The complete provider, current full integration, six partial capabilities and
all P0-P7 exit requirements remain open. This receipt is not physical-router,
MIPS, GPU, external-provider or complete cross-language acceptance.
