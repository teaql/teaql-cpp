# Active constant bootstrap save cleanup

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This checkpoint tests the C++ ownership adaptation around actual generated
bootstrap saves. Its scheduling barrier is not a new Rust provider behavior or
an original-source deferred-execution comparison.

## A pending save after SQLite writes

The test installs the unchanged generated numeric domain's combined module,
metadata, schema provider, ID generator and domain executor. Generated Q uses
its exact `ServiceRuntimeExecutor`. A separate transaction adapter is registered
for the graph saver: it delegates real queries, writes and transactions to the
existing SQLite provider and delays only the commit operation.

At that point, the current modeled row has been written and read back inside the
transaction. Earlier bootstrap saves have already committed. A test worker holds
and reads a 32 KiB User-owned string; it does not run SQLite or Context APIs on
a foreign thread. A condition-variable handshake proves it has accessed the
buffer before the parent reports Pending. The worker stays alive until the
operation's terminal hook stops and joins it. No sleeps or timing assumptions
control the scenarios.

The fixture checks all five actual save positions: the Platform root, two
Priority constants and two Label constants. Each position executes four actions,
for **20** scenarios:

- Resume: propagate a wake through commit/save/model/sequence/schema parents,
  join the worker and commit all five rows with the active root installed.
- Terminal failure: inject a nonretryable commit error, join the worker, abandon
  the real transaction and preserve the earlier committed rows.
- Explicit cancellation: stop the schema operation, join native work, roll back
  the current row, then actually delete UserContext.
- Context destruction: delete UserContext while commit is Pending, without an
  explicit prior cancel, and require the same cleanup exactly once.

Repeated Pending polls must not create another commit, worker or write. Bootstrap
privilege must be inactive between polls and restored during cancellation. The
pending transaction sees exactly the current row plus prior commits; cancellation
or failure leaves only prior commits visible afterward.

## Reclamation witness and scope

The default PMR resource is disabled. Commit state, host bytes, query/save values,
transaction adapters and worker RAII state are allocated explicitly in UserContext.
An early registered User resource acts as a destruction witness: it requires zero
workers, SQLite autocommit restored and no busy native statements before its own
destruction. Host-buffer integrity checks run again immediately before the worker
exits. The terminal hook joins once; state destruction does not join or cancel twice.

The SQLite connection and provider are explicitly Global-owned so the fixture
can inspect the database after actual User deletion. Idle prepared statements
may remain in that Global cache; no User-owned transaction lease remains. This
is not a claim that Global connections are closed when an unrelated User closes.
Existing provider lifecycle tests cover connection destruction separately.

The initial fixture used only a differently typed executor, which generated Q
correctly rejected as a missing domain resource. Installing the original domain
executor for reads and the gated executor for saves resolves that test setup
error without changing generated APIs. Production runtime and generator bodies
are unchanged in this checkpoint. The
[source/ownership review](parity/bootstrap_save_async_5_0_6_source_review.json)
records the actual boundary and adaptation.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_generated_bootstrap_save_async_tests
ctest --preset native --output-on-failure \
  -R '^generated_bootstrap_save_async_contracts$'
```

The `sanitize` profile selects the same target/test. The `modules_native` target
and test add the `teaql_module_` and `module_` prefixes respectively. Each consumer
runs the identical assertions and all 20 scenarios. No MIPS or physical-device
run is required for this native lifecycle fixture.

The full constant qualification matrix retains scalar/time, metadata/topology,
customization and controlled concurrent-conflict obligations. This checkpoint
does not remove the default constant guard or close G05/P3/P7.

## Terminal verification

All 20 scenarios pass on native, ASan/UBSan and imported modules after successful
builds. Relevant native checks pass **3/3**, sanitizer **1/1** with leak scanning
disabled, and module checks **1/1**. Canonical consumer verification covers 157
bodies, the unchanged catalog has 1,479 exports and dependency checks retain
seven acyclic modules. Pinned format/check covers 1,018 handwritten C++ inputs.
The [terminal record](parity/bootstrap_save_async_5_0_6.json) retains exact outputs,
source hashes and the initial missing-domain-executor fixture failure. No generator
or production body changed, so Java and generation reproduction were not rerun.
