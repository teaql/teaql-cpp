# Generated bootstrap sequence support: Rust 5.0.6

Historical prerequisite checkpoint: this runtime change implemented the shared scheduling and retry mechanics
needed by P3 constant initialization. At that checkpoint it did not emit model initialization
recipes or remove the constant-generation guard. The subsequent model integration
is recorded in generated_constant_bootstrap_5_0_6.md; the default guard remains. The active locks govern both
runtime and producer source.

## Source responsibility

The frozen generator's `RustDomainNaming.rustGeneratedBootstrap()` executes one
complete root/constant sequence per attempt. On success it returns immediately;
on a specified conflict it yields before restarting the complete sequence. There
are at most five attempts. A nonretryable error or the fifth failure returns the
original error. Earlier successful writes are not enclosed in a new transaction
by this retry loop; model recipes must remain idempotent.

The conflict predicate accepts `OptimisticLockConflict` directly. For Graph
errors it checks the source's exact primary-key message substrings, including
case. Other error kinds do not become retryable merely because their text matches.
Preserving the original text predicate does not add database providers: C++
continues to support SQLite only. Even its substring behavior for `_data.identifier`
and `_data.id_other` is retained; this checkpoint does not silently tighten it.

## Shared C++ mechanics

`run_generated_schema_bootstrap(user_context, recipes)` copies the supplied
callback list into the UserContext arena and returns an existing
GeneratedSchemaBootstrapFuture. Each callback returns that same protocol; the
runner invokes and polls callbacks in order. Completed child results are consumed
before advancing. Retryable errors reset the recipe index and return Pending
with a wake notification, so synchronous wait and explicit polling both progress.
No generator-specific state machine, blocking wait, singleton or dependency is
required by these mechanics.

The runner participates in the existing UserContext::ensure_schema bootstrap
scope. It does not retain ambient privileges across Pending, and direct invocation
outside that scope fails before any recipe runs. A child operation must use the
same Context resource; a foreign operation is cancelled before rejection.
Wake forwarding is removed before completion or cancellation. Parent cancellation
joins the active child through the existing operation protocol before arena
reclamation; throwing recipes preserve that cleanup path.

These are new kernel support functions, exported through the canonical header
and named-module facades. No directory migration or module-owned type change is
part of this checkpoint.

## Evidence and limits

The narrow retry oracle compiles the exact private classifier extracted from the
complete unchanged frozen candidate runtime. Its extracted bytes and source
hash are retained and verified by
`scripts/generate_bootstrap_classifier_reference.py --check`. This provides direct
classifier observations; it is not a public schema/bootstrap integration oracle.

Native protocol tests exercise sequential ordering; zero/one/four/five/many
failures; complete-sequence restart; the fifth-error boundary; nonretryable
failure; cooperative yielding and nested wake forwarding; exceptions; foreign
children; explicit Context destruction with a pending recipe; and direct calls
outside the bootstrap scope. Default PMR allocation is disabled. Existing schema
tests retain provider/business/bootstrap ordering, scope restoration and close
precedence.

Next, model recipes must implement source-defined root ID 1 creation, ID-floor
reservation, active-root installation, constant creation/reconciliation and
no-op detection through the typed data-service and audited-save protocols.
Qualify their real SQLite behavior and public original generated runtime before
removing the guard. The scheduler tests alone do not prove constant initialization,
governance/audit outcomes, root validation, or full P3/G05 acceptance.

## Reproduction

Checkpoint validation passes native 3/3, targeted ASan/UBSan 2/2 with
`detect_leaks=0`, and module sequence/import checks 2/2. All 85 extracted original
classifier observations match C++. The seven-module export/isolated-lookup
catalog verifies 1,410 declarations; active module paths and the pinned formatter
also pass. No physical-device, MIPS or public model-bootstrap acceptance is
claimed by these checks.

```sh
python3 scripts/verify_upstream.py
python3 scripts/generate_bootstrap_classifier_reference.py --check
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_generated_bootstrap_tests teaql_schema_runtime_tests teaql_bootstrap_classifier_probe
ctest --test-dir build_native_current --output-on-failure -R '^(generated_bootstrap_contracts|schema_runtime_contracts|bootstrap_classifier_rust_parity)$'
python3 scripts/verify_module_exports.py
```

Build the standalone `bootstrap_classifier_5_0_6` Rust oracle against the locked
reference, then run `tests/parity/generate_bootstrap_classifier.py --rust ... --cpp ...`
to reproduce the retained classifier corpus. Targeted sanitizer tests and the
module-based sequence consumer verify the same lifetime and API contract.
