# Physical mutation statement identity: Rust 5.0.6

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, particularly
`teaql-runtime/src/data_service/base.rs`. This is a focused correction of the
compatibility execution adapter. It does not accept the complete base or Context
source responsibility, advance a generator capability, or close a P0-P7 phase.

## Source contract and reproduced difference

The locked `sql_statement_trace` identifies the last Entity frame, ignoring
trailing comments and other diagnostic frames. If that frame has the physical
command's entity type, its ID is replaced by the command ID. Otherwise a new
Entity frame is appended. An earlier ancestor of the same type cannot stand in
for the current statement. The source takes insert identity from `values["id"]`
and other command identities from `id`, using `Value::try_u64` in both cases.
An unknown ID clears an outdated last-frame ID; a valid zero remains a physical
statement ID. Audit reasons, their identities, comments and preceding lineage
are retained.

The previous C++ adapter searched the entire lineage for a matching entity type
and never assigned the physical ID. Request construction and the separate
`execution_trace_chain` projection do not repair that raw command lineage.
Both permanent frontend probes reproduced **1,028 differences in 1,638 cases**
before the correction. The unchanged cases provide controls for unknown IDs,
absent insert IDs and existing compatible lineage.

The correction is confined to `mutation_item` in
`teaql_runtime/include/teaql/runtime/detail/data_service/base_mutation.hpp`.
All ten mutation entry points continue through the existing shared operation,
with their current intent, privacy, executor, batch, error and cleanup protocols.
The source's last-frame search and ID extraction replace the earlier any-frame
test. Nested trace storage still uses the bound Context resource.

## Actual-source fixture

`scripts/verify_runtime_statement_trace_5_0_6.py` prepares a complete reference
runtime view, preserves every original source and test body, and appends one
private caller test to `base.rs`. This caller executes the actual insert,
update, delete, recover, batch-insert, batch-update and four many-operation
methods. It also runs the original statement-identity test unchanged.
The view retains the reference registry versions and checksums. The only omitted
development dependencies are the view utility's documented, disabled external
OpenTelemetry exporters.

The fixture varies fourteen Value identities, nine lineage shapes, all ten
entry points and absent/present insert IDs. It observes raw executor command
lineage, request execution lineage, affected rows and unchanged caller lineage.
Two-row batch cases deliberately supply only one row trace, exercising the
second row's default lineage independently. Many-operation cases exercise both
commands. Expectations are generated from these original methods rather than
from a second implementation of the trace algorithm.

The C++ probe executes the real `RuntimeDataService` and records requests at a
fixture executor boundary. Every input has a fresh GlobalContext/UserContext;
the default PMR resource is disabled. Both direct headers and the Runtime-only
module import execute the same complete probe body. This is an immediate mock
executor observation, not a physical database or asynchronous cleanup test.

## Evidence and remaining acceptance

The retained
[native pre-repair receipt](parity/runtime_statement_trace_before_5_0_6.json.gz)
contains actual source-view provenance, original test output and all observed
results. The separate
[module pre-repair receipt](parity/runtime_statement_trace_module_before_5_0_6.json.gz)
binds that receipt, retains the old implementation text and reproduces the same
1,028 differing indices. Earlier evidence is not rewritten to imply that the
old implementation passed.

The [installed replay](parity/runtime_statement_trace_installed_5_0_6.json.gz)
is complete: actual source, native and imported execution match **1,638/1,638**
cases each. The original test and added caller pass. All **10 native** and
**2 imported** scoped CTest checks pass without failures or skips. The selected
builds finish with no work; module-consumer validation checks canonical bodies
and Runtime-only imports. The receipt retains reproduction sources, actual
commands, complete outputs, JUnit results and unchanged input hashes.

The native gate includes the existing runtime mutation corpus, SQLite mutation
and reentrant contracts, base and Context query/intent corpora, and foreign
mutation-work cleanup. The imported gate additionally executes the complete
generic-completion program. These remain scoped checks. The preceding 784-test
native checkpoint is historical and is not relabeled as validation of this
correction. Source counts remain 82 accepted, 40 pending and two external
exclusions; all seventeen generator capability verdicts retain their values.

To reproduce the original-body and installed frontend comparison after building
both permanent probe targets:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_runtime_statement_trace_5_0_6.py \
  --record /tmp/teaql_statement_trace_fresh.json.gz \
  --cpp build_native_current/teaql_runtime_statement_trace_probe \
  --module build_modules_native/modules/teaql_module_runtime_statement_trace_probe \
  --check-gates
```

Use a fresh receipt path. The committed corpus is checked against the live
original source; ordinary verification never overwrites its expectations.
