# Protected Behavior deletion and recovery: Rust 5.0.6

All **1296/1296 original/native outcomes match** through actual SQLite execution.
This supplements G02 and the runtime integration requirements; it does not
complete G02/G03/P1 or claim audited graph-save equivalence for raw operations.

## Complete original implementation

The test builder archives the exact active Rust runtime commit into an independent
derived build directory. Every original source file remains a byte-identical
prefix. One appended impl exposes two methods that bind a validated MutationIntent
when supplied and call the original delete_internal/recover_internal methods.
No original method, executor, provider, policy, event or logging algorithm changes.

The generated original NoteBehavior and complete original Note/Tenant domain
compile unchanged against that runtime. The Rust custom behavior composes the
generated default; C++ subclasses its generated NoteBehavior. Both use the real
provider and registered RequestPolicy/raw event sink. The observer dependency
versions and registry checksums match the reference Cargo.lock.

[verify_protected_behavior_5_0_6.py](../scripts/verify_protected_behavior_5_0_6.py)
checks every compiled source byte against the reference plus the exact access
suffix, builds offline with the locked dependencies and replays the live source
against C++ and the retained corpus. The [source record](parity/protected_behavior_source_5_0_6.json)
identifies the bridge and source fingerprints.

## Matrix and observed ordering

The matrix combines soft deletion, hard deletion and recovery; inspecting,
denying or version-rewriting Behavior and RequestPolicy; matching/stale/missing/
trigger-rejected rows; successful/failing raw event delivery; missing/blank/valid
mutation intent; and absent or distinct command/context trace chains.

The complete projection preserves operation outcome/error text, ordered callbacks
and their incoming versions, persisted rows, raw event identity/versions/trace,
SQL text, intent fields, operation kind, affected rows and result summaries.
There is no output normalization or substituted provider result.

Actual outcomes include 864 Behavior callbacks, 576 RequestPolicy callbacks,
144 SQL diagnostic records and 84 raw event deliveries. Forty-two operations
return row counts; the other 1254 return their actual errors. Hook rejection stops
Policy and SQL; Policy observes any Behavior version rewrite and can stop SQL.
SQL/optimistic failures retain the original row and do not fabricate an event.

Blank supplied intent rejects before callbacks because the test bridge first
constructs MutationIntent. Missing intent leaves the service unbound: the actual
private implementation can invoke Behavior/Policy before the lower runtime's
intent rejection. Logging switches do not substitute for intent validation.

Deletion prefers a nonempty command trace over the service trace. Recovery uses
the service trace for execution and leaves the recovered raw event's trace empty,
as the original implementation does. These differences remain observable.

## Autocommit and delivery failure

These fixtures execute raw entity-service operations against SQLite autocommit.
The raw event sink runs after provider execution. In 42 cases its failure returns
an error after the persisted row has already changed. Those cases are not modeled
as a rollback, and retrying them as an uncommitted mutation would be incorrect.

This contract is distinct from application mutation audits delivered after an
audited graph transaction. The fixture installs RequestPolicy; it does not invoke
whole-plan MutationPolicy or generated Checkers for deletion/recovery. Their
separate governance and transaction requirements remain intact.

## Lifetime and terminal verification

Native CTest passes 4/4 including existing generated Behavior, Checker SQLite
commit and custom extension regressions. The new module and ASan/UBSan selections
each pass 1/1 over the full 1296-input corpus. Leak scanning is disabled.

Native observers disable default PMR, explicitly copy their output into Global,
destroy stack PMR values, delete UserContext and then serialize retained data.
SQLite statements use real RAII cleanup. No production runtime or generator
function changed, so no regeneration or new Java/complete compiler gate is
required for this test-only checkpoint. No full-suite, MIPS or physical-device
result is claimed. See the [terminal record](parity/protected_behavior_5_0_6.json).

## Reproduce

```sh
set -e
source scripts/toolchains/activate.sh
python3 tests/parity/build_protected_behavior_oracle_5_0_6.py
cmake --build --preset native --target teaql_protected_behavior_probe
python3 scripts/verify_protected_behavior_5_0_6.py
ctest --preset native -R '^protected_behavior_rust_parity$'
```
