# SQLite ledger allocation trace source review (Rust 5.0.6)

Status: accepted. This review covers the entire
teaql-provider-sqlite/tests/ledger_allocated_trace.rs source, not only an
allocated-token helper. The locked source has 15 tests and 1363 lines.

## Original source execution

The [original replay receipt](parity/sqlite_ledger_allocated_trace_original_5_0_6.json.gz)
retains 15/15 unchanged tests passing with the actual locked Rust crates and
macros. Source hash, exact test-name set, resolved lock and commands are retained.
The reusable SQLite source replay controller now accepts
--source ledger_allocated_trace; its default remains dynamic_field_query.
Registry identities must remain a subset of the locked reference, and execution
uses offline/locked resolution. The source and original checkout are not edited.

The probe assigns both CARGO_TARGET_DIR and TMPDIR to its owned temporary
directory, waits for Cargo completion and removes the directory after saving
compact evidence. No original database or build tree remains in that owned path.

## Required source responsibilities

The [reconciliation ledger](parity/sqlite_ledger_allocated_trace_reconciliation_5_0_6.json)
lists SLAT01-SLAT15 with exact original names, source lines and requirements.
The work divides into four related groups:

- Allocated root and child identities must reach lower-ledger requests, actual
  physical SQL metadata, safe SQL logs, authoritative readback and committed
  safe audits. Trusted reasons and masked exported reasons have distinct checks.
- Database allocation must occur inside the actual SQLite transaction. Root and
  child floors, persisted rows, foreign keys, ordinary numeric values, forward/
  reverse relation variants, overflow refusal and rollback/retry all matter.
- Explicit transaction scopes must keep audits unpublished inside the body and
  publish only after commit. Failure retains assigned diagnostic traces while
  rolling back rows and retryable sequence state.
- Blank local reasons inherit, while U+001C..U+001F retain their non-whitespace
  semantics. Both logging modes still validate owned intent. A separate SQLite
  connection inside the safe sink verifies committed row visibility.

Observed<E> must delegate real execution unchanged and capture both success and
failure metadata while forwarding observers. Synthetic trace injection,
preassigned IDs, raw-audit-only probes and mock-only dispatch cannot qualify
these source tests.

## Existing C++ evidence and implementation order

Existing ledger execution, SQLite ID, commit-audit and request-scope tests supply
reusable mechanisms and regression coverage. Their precise limitations are
recorded in the ledger; none is relabeled as a complete source counterpart.

First construct a shared real-SQLite capture fixture and source-shaped typed
root/child carriers. Qualify the ordinary allocated root and child through all
four observation boundaries. Then add transaction-owned ID generation and the
three relation directions, failure/overflow/retry and explicit commit gates.
Finally qualify the exact local-reason vectors and independent-connection safe
audit checks with logging on and off. Fix shared runtime mechanics before any
thin adapter if a source discrepancy is found.

Keep documentation and new identifiers English, source ownership explicit and
Context-owned data within its declared lifetime. Generated artifacts are not
hand-edited. Scope remains SQLite and core runtime behavior.

## Current verdict

Original-source execution, the responsibility inventory and the observer
infrastructure controls below are qualified.
SLAT01/02/03/08/09/11 are qualified below. The historical SLAT03 failure is repaired.
Complete C++ source acceptance, new capability or
phase completion does not follow from these partial results. Source totals remain 114 accepted / 8 pending / 2 excluded_external;
capabilities remain 11 qualified / 6 partial and P0-P7 stays incomplete.

Original disposable outputs removed: 217,306,039 bytes. Retained compressed receipt: 64,056 bytes.


## Real SQLite observer infrastructure checkpoint

`teaql_runtime/tests/sqlite_ledger_allocated_trace_tests.cpp` now wraps the
production SQL executor and its actual transactions. Context-owned asynchronous
result wrappers forward wake notifications, propagate cancellation and retain
successful returned metadata. Failure observers retain metadata and forward the
upstream callback. The infrastructure test executes actual SQLite inserts,
a duplicate-key failure, commit, rollback and a query; it checks physical row
counts and transaction state, command/metadata counts and callback delivery.
It does not inject trace frames or replace the provider with a mock.

The [observer receipt](parity/sqlite_ledger_allocated_trace_observer_5_0_6.json.gz)
retains the source checkpoint and native, imported-module and ASan/UBSan JUnit
results (1/1 each). Leak scanning was disabled. Canonical module verification
passed for 1,136 bodies; pinned formatting passed for 1,800 handwritten files.
The first native run rejected the fixture query's missing intent; adding its
comment and purpose fixed the test without changing runtime validation.

These are infrastructure controls, not counterparts for SLAT01-SLAT15. The
root/child typed carriers, identity allocation, exact trusted and safe traces,
committed safe sink and source-specific failure/retry vectors remain required.
No source disposition or capability was promoted. Builds reuse the committed
native, modules and sanitizer directories; no disposable build tree was added.


## Qualified allocated root counterpart (SLAT01)

The C++ source now invokes the original SLAT01-shaped operation through public
`save_audited_ledger_entity`: a typed `AllocationRoot` decoded with ID 0,
version 0 and name `late root`, marked new and staged in its mutation ledger,
with audit reason `create late root`. Setup includes the original reverse
`children` relation and child descriptor (including nullable `quantity`).

Assertions inspect the actual lower-ledger INSERT recursively through batches,
physical SQL metadata, authoritative query metadata, committed safe Created
audit and safe SELECT logs. Assigned identity and trusted reasons must match
at each internal boundary; exported reasons must be `create [REDACTED]` and
readback purpose must remain nonblank. A physical row count independently
confirms persistence. No allocated ID or trace is injected by the fixture.

The first compile exposed test-only span/container and allocator-string
comparison mismatches; the helper now accepts a trace span and compares string
views. The first execution exposed an overbroad test assertion on schema
lifecycle events; the post-commit assertion now applies to the root Created
event under test. Schema behavior and runtime execution were not changed.

The [root checkpoint](parity/sqlite_ledger_allocated_trace_root_5_0_6.json.gz)
retains the final source and native/module/sanitizer JUnit results: 1/1 in each
profile, with both observer controls and SLAT01 executed by that CTest entry.
Pinned format/check (1,800 files), canonical consumers (1,136 bodies), active
module paths and whitespace checks passed. Leak scanning remains disabled.
SLAT01 is qualified; SLAT02-SLAT15 and whole-source acceptance remain pending.
No runtime or generator implementation changed in this checkpoint.


## Historical child checkpoint and failing database allocation gate

SLAT02 now passes in native and imported-module builds. The typed test carriers
store local comments by entity key, matching the locked macro and the current
C++ generated support contract. The child starts at ID 0 under existing root42;
its object comment, shared-ledger comment and composed-ledger comment are checked
before allocation. An actual post-save child query checks one row and root_id42.
The lower-ledger INSERT, physical SQL and committed safe audit preserve the
ordered parent/child IDs and local reason; exported child reason is masked.

The [child and failure receipt](parity/sqlite_ledger_allocated_trace_child_gap_5_0_6.json.gz)
retains both profiles' complete failed CTest reports and the source checkpoint.
Observer controls and SLAT01-SLAT02 pass before SLAT03 fails. Independent
`--slat02` invocations also pass. The default CTest entry runs all implemented
cases and remains red; no expected-failure annotation or skip was introduced.

SLAT03 installs the real SQLite generator with floors500/800 and confirms that
allocation occurs inside the actual save transaction before any safe audit.
It fails when ordinary ID-space access cannot acquire that transaction's lease.
See the [confirmed gap and repair requirements](sqlite_ledger_id_transaction_gap_5_0_6.md).
This requires a shared runtime/provider fix, not a fixture-specific allocator.

Formatting (1,800 files), canonical module bodies (1,136) and active path checks
passed. No production runtime/generator implementation changed in this checkpoint;
no new sanitizer or MIPS result is claimed. Stable native/module build directories
were reused. Copied JUnit reports were removed after retaining compressed evidence.
Whole-source status remains pending_review and P0-P7 remains incomplete.


## Transaction scope repair and explicit commit checkpoint

The [repair receipt](parity/sqlite_ledger_id_scope_repair_5_0_6.json.gz) qualifies
SLAT03 and SLAT11 and rechecks SLAT01-SLAT02 after the runtime change. The
[scope contract and historical gap](sqlite_ledger_id_transaction_gap_5_0_6.md)
describe the data-service/SQL/provider adaptation. The configured generator is
still called; ID allocation is authorized only in the actual transaction and
its owning Context, for the synchronous ledger allocation loop.

SLAT03 now returns root 501/version 1, records exactly the real root allocation,
commits floor 501 and one physical row, and passes command/SQL/readback/safe-audit
trace checks. SLAT11 uses the exact `explicit database root` name and
`explicit root allocation` reason: the explicit body sees saved 501 without
published safe audits, while commit publishes the matching event and durable
floor/row. Supplemental explicit business rejection and suspended-body
cancellation restore floor 500, remove the uncommitted row and publish no audit.

Provider controls retain the existing unbound-generator rejection and add
nested/moved scopes, exception restoration, wrong-Context rejection, ordinary
parent SQL exclusion, active transaction cursor exclusion, cancellation,
consumed handles and Context-close rollback after scope release. These controls
exercise the C++ lease adaptation; they do not replace the remaining source cases.

Final focused gates pass native 8/8, modules 4/4 and ASan/UBSan 3/3; the source
fixture entry runs observer controls, SLAT01/02/03/11 and scope cleanup together.
Format/check covers 1,800 handwritten files, canonical verification 1,136 bodies,
and isolated exports 1,520 names. No full-suite, MIPS or physical-device result
is claimed. Source totals stay 114 accepted / 8 pending / 2 excluded_external.

Builds reused the committed profile directories (approximately native 4.4 GiB,
modules 3.0 GiB and sanitizer 2.5 GiB after this checkpoint, versus 4.1/2.9/2.2 GiB
before expanded builds). No disposable build tree was added. Build logs and
JUnit copies are removed after being retained in the compressed receipt.


## Qualified rollback/retry and overflow counterparts

The [retry/overflow receipt](parity/sqlite_ledger_retry_overflow_5_0_6.json.gz)
qualifies SLAT08 and SLAT09, with the earlier four source cases and observer/scope
controls rerun in the same entry. Native, imported modules and ASan/UBSan each
pass 1/1; these are compound CTest entries, not a count of original source cases.
Format/check (1,800 files), canonical bodies (1,136) and active paths also pass.

SLAT08 uses the original SQLite rejection trigger and names/reasons. The failed
save actually allocates root 501 but restores floor 500 and leaves zero rows and
no safe audits, with autocommit restored. The caller's cloned/shared ledger
retains its zero-ID new marker and changes. After an independent floor advance
to 700, the same pending root is edited to `accepted` and saved with
`retry reviewed allocation`. Actual command, SQL, readback and safe-audit
boundaries use 701 and the new reason; the committed ledger is cleared.

SLAT09 advances the actual floor to `i64::MAX` and requires an overflow error.
There is no successful allocation observation, lower-ledger command, row or
safe audit. The floor remains unchanged and the transaction releases. This
verifies the configured database generator cannot silently fall back to a
process-generated identity on failure.

No production runtime or generator implementation changed in this checkpoint.
The earlier repair regression results remain historical; no new broad-suite or
MIPS result is claimed. Stable build directories were reused and copied JUnit
reports were removed after compressed retention. Nine source cases remain open:
SLAT04-SLAT07, SLAT10 and SLAT12-SLAT15. Whole-source status remains pending_review.


## Qualified existing-parent and new-graph relation variants

The [graph receipt](parity/sqlite_ledger_allocated_graph_5_0_6.json.gz) qualifies
SLAT04-SLAT07. Native and imported-module CTest entries each pass 1/1, running
all ten implemented source cases plus observer and transaction-scope controls.
The earlier six-case sanitizer checkpoint remains historical; this checkpoint
changes only the test fixture and does not rerun sanitizer or MIPS profiles.

SLAT04 persists root 42 before installing the actual database ID generator.
Only the child receives ID 801; root floor 500 is unchanged. The actual child
foreign key is 42, and its local reason and parent reason survive command,
physical SQL and committed safe audit boundaries.

SLAT05-SLAT07 use the original reverse-only, forward-only and bidirectional
metadata variants. Actual allocation order is root 501 then child 801. The
persisted foreign key becomes 501 while ordinary quantity remains INTEGER 0.
Both exact assigned reason chains reach the lower-ledger requests and physical
SQL metadata. Safe audits mask private INSERT names, contain the assigned IDs,
and occur exactly once per entity after commit. The actual safe child INSERT
log retains the masked root reason, successful outcome and root-to-SQL path,
without raw natural-language intent frames.

All four cases use real SQLite, the configured database generator and public
ledger Save. No production or generated implementation changes were needed.
Format/check covers 1,800 files; canonical module verification covers 1,136
bodies; active paths and diff whitespace checks pass. Stable native/module
build directories are reused. Copied JUnit reports are removed after compressed
retention; no disposable build directory was added.

Five cases remain open: SLAT10 and SLAT12-SLAT15. Whole-source status remains
pending_review, and the source/capability totals are unchanged. Earlier nine-case
remaining counts above describe the preceding checkpoint.


## Qualified partial-graph failure and retry

The [graph-failure/retry receipt](parity/sqlite_ledger_graph_retry_5_0_6.json.gz)
qualifies SLAT10 with the same actual SQLite rejection trigger, names, reasons,
sequence floors and retry inputs as the original source. The first attempt
allocates root 501 and child 801. Captured physical metadata has exactly two
INSERTs: successful root followed by failed child, with the original assigned
reason chains retained. The safe child SQL log reports Failure and the owned
`first graph attempt` reason, with no raw intent frames in the exported path.
Both tables and sequence floors roll back; no committed safe audit is delivered.

After removing the trigger and advancing floors to 900 and 1200, the retained
pending graph is saved with `reviewed graph retry`. Actual command, physical
SQL and safe audit boundaries contain root 901 and child 1201 with the new
owned reason and preserved local child reason. Both sequence floors and exactly
one root/child row commit. The safe SQL log helper is shared with the successful
relation-variant cases; it accepts an explicit expected reason and outcome.

Native and module entries each pass 1/1 and run all eleven implemented source
cases plus observer/scope controls. Format/check covers 1,800 files, canonical
module verification covers 1,136 bodies, and active path and whitespace checks
pass. This change adds test coverage without production or generator changes;
no new sanitizer, full-suite or MIPS evidence is claimed. Stable build directories
are reused and duplicate JUnit reports are removed after compressed retention.

SLAT12-SLAT15 remain pending, including their independent SQLite connection
inside the safe sink. Four source cases are open; whole-source status remains
pending_review. Earlier remaining-case counts describe historical checkpoints.


## Complete source acceptance

The [final receipt](parity/sqlite_ledger_allocated_trace_acceptance_5_0_6.json.gz)
qualifies SLAT01-SLAT15 and their transitive helpers. The reconciliation ledger
records all 106 original assertion macro sites and a complete source-range
responsibility map. That inventory is a navigation aid; acceptance rests on
reviewed original assertions and actual original/candidate execution, not on
counting names or source lines. The retained original receipt runs all fifteen
unchanged tests with the locked Rust runtime and macros. Fresh native, imported
modules and focused ASan/UBSan each pass the compound CTest entry containing all
fifteen counterparts plus the observer and transaction-scope controls.

SLAT12-SLAT15 pass original nullable/empty/ASCII/Unicode blank inputs and
U+001C..U+001F directly to runtime local-comment classification. Blank owned
intent is separately rejected by the audited constructor with the exact intent
message and by graph planning with REQUEST_COMMENT_REQUIRED, field comment and
Mutation kind, before any command, metadata, safe audit or SQL log. No test-side
parent fallback or expected trace is installed on the input ledger.

The actual root/payment/attempt/shipment graph saves root 100/version 1. Every
scalar INSERT retains the owned `submit order` reason and the exact expected
per-entity reason chain. Every physical INSERT reports one affected row and
Success; authoritative readback is required. The safe sink checks each target
ID and version through a second real SQLite connection before capturing its
audit. Exactly one matching audit and one physical INSERT exist per entity/ID.
Logging-on produces one safe log per physical statement with the owned reason
and root-to-SQL path; logging-off leaves no logs without changing these other
contracts. Both connections close before the exclusively owned temporary file
directory is reclaimed, including exception paths.

The C++ adaptations are explicit: scoped carriers and capture replace Rust
shared ownership; pollable operations replace futures; an invalid_argument
exception replaces the caught constructor panic; metadata serves its schema
protocol without collapsing executor/provider responsibilities. The locked
source itself describes these native-ledger fixtures as distinct from generated
relation/API acceptance. This record preserves that boundary.

Format/check covers 1,800 files; canonical verification covers 1,136 bodies.
Active paths and diff whitespace checks pass. Sanitizer leak scanning remains
disabled. No broad-suite, MIPS or physical-router result is claimed. Stable
profile directories are reused; compact receipts retain full test output and
copied JUnit reports are removed. No disposable build tree is added.

This source moves from pending_review to accepted: totals become 115 accepted,
7 pending_review and 2 excluded_external. Capabilities stay 11 qualified and
6 partial; P0-P7 remain incomplete. All earlier partial counts in this document
are historical checkpoints.
