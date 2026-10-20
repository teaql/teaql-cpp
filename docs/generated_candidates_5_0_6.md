# Generated candidates checkpoint: Rust 5.0.6

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Status: P3/G05 is partial. Candidate Q/model/E semantics are qualified for the
retained fixture; generated constant bootstrap is still pending. Normal constant
entity generation remains guarded. Qualification output uses `--qualify-constants`.
References are the exact active runtime and producer locks; frozen Rust output
is complete and untouched.

## Source contract and adapters

The original `Candidate` and `ObjectDescriptor.getIdentityConstants()` determine
constant identities. Explicit numeric IDs remain exact; nonnumeric/missing IDs
use the source Java hash algorithm. The fixture includes IDs 11, 22 and the
fallback 2104194. These identities are not new enum ordinals or Context allocation
IDs. Method suffixes come from `Candidate.getRustCandidateMethodValue()` and use
the agreed lowerCamelCase C++ application spelling after centralized naming.

`RustFieldDescriptorNaming.rustCandidates()` preserves scalar candidate filter
values, including case. Q equality/inequality methods delegate to existing Expr
construction. Constant-reference predicates use the storage foreign ID. Explicit
`_cat="human"` produces `whose...`; ordinary entities produce `with...`. Both
fluent value categories preserve Context binding and can be composed in helpers.
The centralized pluralizer emits `orderStatuses` and `people`.

A literal `null` candidate means equality with the text "null". Source
`withTagIsUnknown()`/`withTagIsKnown()` denote SQL NULL/non-NULL. Existing C++
`IsNull`/`IsNotNull` aliases are retained only when no real candidate occupies
that name; candidate semantics take precedence. The corpus separately observes
the literal row and genuine NULL rows. Normalized candidate-method collisions,
including positive/negative overlaps and source Known/Unknown collisions, fail
before emitting duplicate C++ methods.

Constant-reference model methods such as `updateStatusToInProgress()` delegate to
the existing generated ID setter and runtime mutation/load protocols. Raw foreign
ID and object assignment are internal; applications use the named candidate
updates. Model boolean helpers compare the current effective ID without SQL.
Unknown loaded IDs make all three known-candidate checks false.

E candidate checks use the existing checked entity expression and require the
foreign storage field to be loaded. A missing ID is NotLoaded, not a false
predicate or a NULL fallback. The source oracle calls the actual public E API
and reads the missing node/path from its panic diagnostic; C++ retains the same
state/node/path in EvalResult. Complete diagnostic text/suggested-fix alignment
remains G11. Scalar expressions retain their existing eager snapshot semantics.

## Semantic evidence

The corpus has 388 exact original/C++ observations:

- 160 model cases: required constant references in human/non-human models,
  loaded/unloaded fields, zero/known/unknown IDs, three named updates and no update,
  repeated mutation, shared ledger observations and independent copied load state.
- 128 query cases: every retained scalar/foreign candidate, normal/minimal entries,
  positive/negative predicates and ordered repeated composition. Strings remain
  their authentic source values; no filter normalization is performed.
- 64 actual SQLite reads: all three constant-reference and phase candidates plus
  three nullable tag candidates and SQL NULL availability checks. Unknown candidate IDs are valid fixture foreign
  rows, so foreign key constraints do not conceal query semantics.
- 36 actual audited SQLite saves: six commits, six optimistic conflicts, six
  trigger failures and eighteen sparse-input Checker failures. Complete errors,
  diagnostics, returned native fields, database states and pending-ledger state
  are compared. Every failed save preserves the database as observed before save.

The native lifetime contract disables default PMR allocation, composes Q through
a value-returning helper, tests eager E snapshots, destroys the source Context,
and checks expired Q/E plus explicit scalar/model/query copies. Global references
must be rebound through the next UserContext before access; copying allocation
does not retain the old UserContext capability.

Compiler contracts require correct calls to compile and reject raw constant
assignment, unknown candidate updates/Q/E, incorrect human vocabulary and missing
purpose. Java regression also checks normalized naming collisions, punctuation/
numeric suffix fallbacks, private setters and accurate candidate Assist entries.

## Remaining P3 obligations

At the original candidate checkpoint, C++ root/constant fixture seeding was
explicit. The current probe uses generated initialization; the distinct public
bootstrap evidence is recorded in generated_constant_bootstrap_5_0_6.md. The
candidate-only corpus does not itself qualify generated initialization. The frozen Rust bootstrap, generated by
`RustDomainNaming.rustGeneratedBootstrap`, creates Domain Root ID 1, reserves
fixed IDs through the Context ID generator, binds the active root, creates or
reconciles constants with audited saves, avoids unchanged writes, and retries
specified conflicts up to five attempts. Implement these model recipes over
shared pollable runtime mechanics before removing the constant guard. Preserve
schema/bootstrap scope, governance, audit, error propagation and cancellation.

The shared retry/sequence prerequisite now exists and is verified separately in
[generated_bootstrap_sequence_5_0_6.md](generated_bootstrap_sequence_5_0_6.md).
Model recipes and the 99-case public initialization checkpoint now supplement
it in [generated_constant_bootstrap_5_0_6.md](generated_constant_bootstrap_5_0_6.md);
remaining guard-removal gates are explicit there.

Further model configurations and malformed/missing/colliding candidate metadata
need source-based disposition. Source compile failures must be recorded without
patching frozen output. P3, G05, G13 and the full P0-P7 goal remain incomplete.

## Reproduction

Checkpoint validation passes native 2/2, targeted ASan/UBSan lifetime 1/1
(`detect_leaks=0`), module lifetime 1/1, and eight compiler contracts (one positive,
seven negative). The selected Java generation/naming tests pass 29/29. All
eleven C++ fixture sets reproduce 121 files; the complete original candidate
fixture reproduces 32 files including provenance. Both frozen references remain
clean, and formatter/module-path/lock integrity checks pass. These scoped gates
do not qualify generated bootstrap or all P3 model configurations.

```sh
python3 scripts/verify_upstream.py
python3 scripts/generate_entity_references.py --fixtures candidates --check
python3 scripts/generate_examples.py --check
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_generated_candidates_probe teaql_generated_candidates_tests
ctest --test-dir build_native_current --output-on-failure -R '^generated_candidates_(rust_parity|contracts)$'
python3 scripts/verify_generated_type_errors.py --build-dir build_native_current --cases candidate_positive candidate_raw_id_setter candidate_raw_object_setter candidate_unknown_update candidate_unknown_predicate candidate_unknown_expression candidate_human_vocabulary candidate_missing_purpose
```

Build `rust_field_layout/src/bin/generated_candidates_5_0_6.rs` against the locked
reference, then use `generate_generated_candidates.py --rust ... --cpp ...` to
reproduce the corpus or `compare_generated_candidates.py --rust ... --cpp ...`
to check both live executables. Targeted sanitizer and module consumers exercise
the same native lifetime contract.
