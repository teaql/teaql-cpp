# Private memory mutation and poisoned-lock checkpoint: Rust 5.0.6

Status: focused native/module/sanitizer qualification. Full private `memory.rs`
source acceptance and P0-P7 remain incomplete.

## Original observations

The [original observer](parity/memory_mutation_original_5_0_6.json.gz) preserves
every locked runtime body and invokes its actual private memory service. It
records 1,144 cases: two metadata configurations, ten stored-version forms,
three command identities and nineteen mutation forms (1,140 cases), plus four
insert cases. The metadata configurations use either fallback `id`/`version`
names or explicitly flagged `pk`/`revision` properties.

Versions include absent, NULL, signed negative/zero/positive, U64, text,
`INT64_MAX`, `INT64_MIN` and `INT64_MAX - 1`. Command identities distinguish
matching signed IDs, numerically equal unsigned IDs and absent IDs. Operations
cover checked/unchecked update, an explicit version assignment after automatic
increment, soft/hard delete and recovery. Insert cases cover ordinary, duplicate,
missing-entity and empty-value insertion. Complete post-operation rows and full
error text are retained; matching a count alone is insufficient.

Fourteen original cases panic under the reference test profile: eight addition
overflows and six negation overflows. `catch_unwind` observes the original panic
without replacing any mutation body. After each panic, the observer verifies
that fetch remains poisoned, seed does not restore the service, all four write
operations report poisoned state, and missing-entity validation still precedes
lock-state checking. Panic-hook diagnostics remain in the original receipt and
are expected evidence, not an unobserved successful run.

## C++ repair and explicit adaptation

Source inspection found unchecked signed addition and `abs(INT64_MIN)` in the
old C++ helper. Those paths were not executed as a pre-repair comparison because
they could invoke undefined behavior. The new helper checks bounds before
arithmetic and throws the test-only `MemoryArithmeticPanic`, carrying the exact
observed source message. This is an explicit Rust-panic/C++-exception adaptation,
not a claim that both languages have identical unwinding mechanisms.

The helper now owns a mutex and poisoned-state flag. A scoped lock records
exception unwinding during protected operations. Fetch holds the lock while
copying stored rows, then evaluates the query outside it; mutation holds it while
checking and modifying state. Seed silently skips writes when the lock is
poisoned, matching the original helper. Metadata validation remains before locking. These objects
belong to the service's UserContext and are destroyed before arena reclamation.
No shared ownership extends the Context lifetime.

Ordinary version conflicts remain returned errors and do not poison the service.
Identity equality remains strict for mutation, independently of relationship
identity normalization. Successful checked updates increment first, then apply
explicit mutation values, including an explicitly supplied version.

This is a private test-helper repair. Public runtime/provider bodies, production
authorization and generated APIs are unchanged. The checked overflow observation
uses the original debug test profile; no release-profile overflow equivalence,
general concurrent Context allocation safety, or clone-sharing guarantee is
claimed by this checkpoint.

## Verification

- [Native](parity/memory_mutation_native_repaired_5_0_6.json.gz): 1,144/1,144.
- [Canonical module](parity/memory_mutation_modules_repaired_5_0_6.json.gz): 1,144/1,144.
- [ASan/UBSan](parity/memory_mutation_sanitize_repaired_5_0_6.json.gz): 1,144/1,144,
  empty diagnostic stderr, `UBSAN_OPTIONS=halt_on_error=1` and leak scanning off.
  The focused probe also creates and destroys a UserContext for each scenario.
- Native CTests: 6/6, retaining mutation, grouping, aggregates, pipeline, filters
  and original callers. Module comparisons also preserve the prior 1,459/208/18/16
  observations; original module callers pass 1/1. Sanitizer CTest passes 1/1.
- Canonical verification checks 1,114 consumer/fixture bodies; module paths and
  locked-source provenance pass.
- Pinned formatting/check pass for 1,782 handwritten files; affected builds have
  no work remaining. The [CTest checkpoint](parity/memory_mutation_ctest_checkpoint_5_0_6.json.gz)
  retains actual logs, current source identities, sanitizer settings, fourteen
  original panic observations and verified temporary-build cleanup.

Broader relation shapes, error precedence, recursive scalar boundaries and the
complete helper responsibility audit remain open. This checkpoint does not
promote a source disposition or a full migration phase.

## Reproduction and cleanup

```bash
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_memory_mutation_probe
python3 tests/parity/diagnose_memory_mutation_5_0_6.py \
  --cpp build_native_current/teaql_memory_mutation_probe
```

Fresh original observations use
`scripts/observe_memory_mutation_5_0_6.py --output <new-receipt.json.gz>`.
The runner refuses to overwrite receipts, verifies locked sources and isolates
its temporary runtime view/Cargo target from shared build caches. After the
original child exited successfully it removed 161,007,991 bytes and verified
directory removal. Shared downloads and stable preset builds remain available.
