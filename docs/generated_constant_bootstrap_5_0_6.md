# Generated constant bootstrap checkpoint: Rust 5.0.6

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Status: P3/G05 remains partial; normal constant generation is still guarded.
The qualification fixture now uses actual generated bootstrap, replacing the
explicit C++ root/constant seed setup in the earlier candidate probe. The runtime
and producer commits remain those in the active locks.

## Responsibilities and implementation

The shared `run_generated_bootstrap_model<Recipe>()` owns the asynchronous typed
query/create/reconcile/audited-save lifecycle. Generated recipes supply model
types, fixed IDs, query entries, field values, comparisons and authentic purpose/
comment/audit strings. The shared sequence from the preceding checkpoint owns
ordering and retries. Neither generated layer emits a second SQL write engine,
transaction loop, graph-save implementation or blocking wait.

The initial root query is the public generated Q entry with self fields and ID 1.
An existing root is installed without field reconciliation. A missing root is
created in the executing Context, bound to its mutation runtime, assigned ID 1,
reserved through `initialize_generated_bootstrap_entity`, populated, and saved
through AuditedSaveExt. The saved root is then installed through the existing
idempotent bootstrap root API.

Each constant queries its own identity. Missing rows use the same ID-reservation
and audited-save path. Existing rows compare model-defined values and save only
when a value changes. Unchanged versions remain unchanged. An existing row does
not acquire a new ID floor merely because it was read: the source also reserves
only newly created rows, and the corpus retains this distinction.

Recipes preserve source string values, explicit and hash-fallback IDs, foreign
ID 1, native field types and optional wrapping. Root execution precedes constants
regardless of declaration order. Source `Domain.getRootDescriptors()` excludes
constant descriptors: a constant-only domain has no Domain Root. An earlier
C++ generation regression incorrectly accepted that configuration; source review
and complete original-crate compilation disproved it. Qualification generation
now rejects it. A standalone constant catalog remains valid alongside a distinct
ordinary root, including a candidate at ID 1 in its own entity type.

Private Context-root/constant assignment stays behind a befriended generated
recipe, and that assignment checks the existing bootstrap scope before mutation.
The const callback uses a local C++ bridge for the existing Context-bound Q facade;
the real UserContext remains the sole runtime argument and allocation owner.
The runtime polls/cancels typed query and save children, forwards wakes, consumes
completed results, and rejects foreign-resource operations before retaining them.

All three generated checker/behavior module variants register the bootstrap.
The qualification `configureRuntime(user_context, filename)` installs metadata,
SQLite transport/executor and the existing SQLite ID-space generator without
executing schema work. `setup()` delegates to it and completes `ensure_schema()`.
This split makes initialization failure directly observable and allows controlled
existing-database fixtures. Other generated fixture domains retain their previous
setup behavior while general assembly remains G12 work.

## Actual original-runtime evidence

The unmodified complete frozen candidate crate runs its public service runtime
and `ensure_schema` against in-memory SQLite. C++ runs the generated module and
the same public Context protocol. The corpus contains 99 cases and 162 rounds:
90 successful rounds and 72 failures. It compares complete database values,
versions, ID-space rows, active roots, trigger-call counts and full errors without
normalization.

Cases cover empty initialization, existing rows, stale values, missing constants,
soft-deleted constants, a foreign-root row, extra custom rows, incompatible
nullable schema and conflicting preinstalled roots. Repeated rounds demonstrate
no-op versions, preservation of existing root values and retained custom rows.
Real SQLite triggers inject one/four/five/many primary-key-shaped failures,
secondary-key errors and ordinary write errors for inserts and reconciliation.
Primary conflicts retry with the source limit; other errors stop immediately.
Failure recovery and repeated initialization retain earlier successful writes
exactly as source does; the bootstrap sequence is not one encompassing transaction.

The 388 candidate model/Q/E/query/audited-save observations still match after
using generated C++ initialization. The independent typed-model lifecycle test
uses a controlled deferred query to verify nested wake forwarding, restoration
of ambient scope, actual Context deletion, foreign-child cleanup and an existing
root that needs no ID generator or save. It disables default PMR allocation and
checks that generated private assignment cannot bypass the bootstrap scope.
This protocol test is distinct from the real SQLite corpus.

## Remaining guard-removal gates

The initial actual paired model has string fields and required foreign IDs.
[The field/topology checkpoint](generated_constant_variants_5_0_6.md) adds 36
paired cases for numeric/optional/Date fields, late roots, dependent constants
and standalone catalogs, and records the original boolean/JSON/invalid-root
failures. Do not treat these as full bootstrap field/configuration acceptance. Complete the
[constant qualification matrix](constant_qualification_matrix_5_0_6.md), including
scalar/optional/JSON/temporal families, standalone constants, late root and
multiple/dependent constant configurations, source metadata/compile failures,
governance/audit observations and remaining asynchronous write cleanup. Preserve
the inherited Business-ID source disposition separately. P3/G05, G12 assembly,
G13 and the complete P0-P7 objective remain open.

## Reproduction

Final native gates pass 5/5 (public bootstrap corpus, candidate corpus, candidate
lifecycle, sequence protocol and typed-model lifecycle). Targeted ASan/UBSan gates
pass 3/3 with `detect_leaks=0`; module typed-model/import gates pass 2/2.
Selected Java generation/naming tests pass 31/31; six focused
compiler contracts pass (one positive, five negative), and eleven C++ artifact
sets reproduce 121 files. The public bootstrap corpus also matches both live
executables. Formatting, source locks, module paths and the 1,412-entry export
catalog pass. These remain scoped checks under the open qualification matrix.

```sh
python3 scripts/verify_upstream.py
python3 scripts/generate_examples.py --check
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_generated_bootstrap_probe teaql_generated_candidates_probe teaql_generated_bootstrap_model_tests
ctest --test-dir build_native_current --output-on-failure -R '^(generated_bootstrap_rust_parity|generated_bootstrap_model_contracts|generated_candidates_rust_parity|generated_candidates_contracts)$'
```

Build `rust_field_layout/src/bin/generated_bootstrap_5_0_6.rs` against the locked
reference. `generate_generated_bootstrap.py --rust ... --cpp ...` reproduces the
corpus; `compare_generated_bootstrap.py --rust ... --cpp ...` verifies it against
both live executables. The Source schema fixture declares its required NOT NULL
and foreign-key constraints; the incompatible-schema case removes nullability
constraints intentionally and is expected to fail before bootstrap.
