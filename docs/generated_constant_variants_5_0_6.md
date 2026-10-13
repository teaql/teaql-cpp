# Constant field and topology checkpoint: Rust 5.0.6

Status: P3/G05 remains partial. The default constant-generation guard remains.
References are the active runtime and producer locks. This checkpoint broadens
the original public bootstrap comparisons; it does not waive the remaining
[qualification matrix](constant_qualification_matrix_5_0_6.md).

## Complete original crates and actual initialization

`constants_numeric.xml` places its ordinary Platform root after two dependent
constant types. Priority includes negative signed integers, positive/negative
double and float values, Decimal, Date, optional strings and optional integers
(including zero). Label refers to Priority. The source foreign seed is ID 1.
`constants_catalog.xml` has a standalone Category constant and a separate
ordinary Platform root; both entity types legitimately have a row at ID 1.

The frozen original producer emits complete, unmodified crates: 26 and 20 files
including provenance, respectively. The Rust oracle builds those entire crates
against the locked 5.0.6 runtime and calls their public service-runtime assembly
and `ensure_schema()`. C++ applies the actual generated checker/behavior module
and uses the shared runtime/bootstrap/provider protocols. Both warm up their
authentic provider-created schema, release the initial Context, change stored
fixture data and repeat initialization in a fresh Context on the same connection.

There are 36 paired cases over 72 rounds: 60 successful rounds and 12 failures.
Empty, existing, corrupt, missing, soft-deleted and extra-custom-row states each
run one, two and three rounds. Observations compare every SQLite column with its
actual integer/real/text/null kind, row versions, ID-space rows, active root and
full errors. No normalization or expected-value replacement is used. Date seed
initialization uses the source epoch default even when the XML candidate carries
another date. Soft-deleted constants attempt creation and fail on the occupied
primary key; they are not silently restored. Repeated execution preserves no-op
versions and custom rows. Decimal SQLite affinity remains observable.

## Invalid roots and uncompilable original field shapes

Source `Domain.getRootDescriptors()` requires `isRoot() && !isConstant()`.
The earlier C++ generator regression that treated a constant as Domain Root was
incorrect. Qualification generation now rejects a constant-only domain. This
correction preserves standalone catalogs with an ordinary root and root-first
execution regardless of XML declaration order.

The complete frozen `constants_root.xml` output is retained: StringTemplate
reports `Rust bootstrap requires exactly one Domain Root` but swallows it and
emits incomplete runtime code. Compilation then reports the missing
`module_with_behaviors_and_checkers` function. The C++ early rejection follows
the source metadata responsibility; it does not reproduce this frontend failure
mode byte for byte.

The complete `constants_scalar.xml` original crate also remains unchanged. It
has four missing boolean predicate-method errors, eight JSON literal parsing/
suffix errors and four consequent excess-argument errors. Together with the
invalid-root error, all 17 exact primary diagnostic messages and locations are
recorded in [the failure record](parity/constant_source_failures_5_0_6.json).
`verify_constant_source_failures.py --check` rebuilds the complete negative crates
with locked dependencies and checks the stable sorted diagnostics. It does not
patch or extract working fragments from the failing output.

The separate generated C++ scalar integration exercises real initialization,
typed loading, no-op and boolean/JSON/optional reconciliation, then Context
destruction with explicit JSON expression copying to a surviving Context. It
disables default PMR allocation. This is C++ integration evidence only; the
uncompilable original configuration remains an explicit open disposition.
The original bootstrap uses the JSON string `"jsonMe()"` for this field, rather
than the object in the candidate row; the C++ integration expectation preserves
that generated seed. Candidate predicate generation has the separate JSON
literal compilation failure described above.

## Reproduction

Native gates pass 4/4 across the new paired corpus, scalar C++ integration and
the retained bootstrap/candidate corpora. Targeted ASan/UBSan gates pass 2/2
(`detect_leaks=0`), including actual Context destruction and explicit copying.
Selected Java generation/layout/naming tests pass 33/33. Four original artifact
sets reproduce 86 files; fourteen C++ sets reproduce 154 files. Formatting checks
894 C++ files. Source locks, active module paths and the 1,412-entry module export
catalog pass. The separate source root-export audit still fails for missing
`CompactRowLayout` and `EntityJsonTraversal`; its pending P0/P7 responsibility is
recorded in the parity plan. These checks do not claim complete baseline parity.

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_upstream.py
python3 scripts/verify_constant_source_failures.py --check
python3 scripts/generate_entity_references.py --fixtures constants_scalar constants_root constants_numeric constants_catalog --check
python3 scripts/generate_examples.py --check
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked --manifest-path tests/parity/rust_constant_variants/Cargo.toml
cmake --build build_native_current --target teaql_generated_constant_variants_probe teaql_generated_constant_scalar_tests
python3 tests/parity/compare_generated_constant_variants.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/teaql_cpp_constant_variants_oracle --cpp build_native_current/teaql_generated_constant_variants_probe
ctest --test-dir build_native_current --output-on-failure -R '^(generated_constant_variants_rust_parity|generated_constant_scalar_contracts|generated_bootstrap_rust_parity|generated_candidates_rust_parity)$'
```

The explicit system linker is for the Rust comparison executable: the activated
development GCC linker's libc does not provide the `fcntl64` symbol referenced
by the cached host-built bundled SQLite archive. This does not change the C++
production toolchain, runtime dependency scope or target-architecture claims.
Remaining gates include broader temporal/clock and malformed metadata shapes,
governance/audit and registration modes, and cancellation during active save.
