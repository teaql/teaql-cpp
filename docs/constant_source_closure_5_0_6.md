# Constant source closure with reference dependencies

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

This is a P3/G05 source-evidence checkpoint, not complete constant or generator
acceptance. Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Producer reference:
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Dependency and artifact controls

The early observer workspaces resolved rusqlite 0.40.2. The reference runtime
Cargo.lock resolves rusqlite 0.40.1 and libsqlite3-sys 0.38.1. The independent
`tests/parity/rust_constant_closure_5_0_6` workspace instead starts from the
reference lock and uses the existing six observer files through Cargo bin paths.
It does not copy, repair or replace those observers or their generated domains.

The verifier rejects any nonlocal package version absent from the reference
lock and checks complete registry identities, including checksums. It verifies
original domain provenance and file hashes, builds offline with `--locked`,
and waits for a successful compiler exit before executing any observer.
Driver hashes must stay unchanged throughout the replay.

Each fresh source result must match the retained corpus exactly before the
existing C++ comparator runs. The comparators preserve their original scope:
source/corpus equality is complete JSON equality, while the resource comparator
checks semantic outcomes and the two language-specific ownership contracts
separately. Rust drops replaced resources immediately; C++ destroys registered
arena resources when their Context closes. Both must reach the same final
destructor totals. This adaptation is not raw-output equality.

## Runtime matrices

| Matrix | Retained observations | Existing C++ comparator |
| --- | ---: | --- |
| Generated candidates, Q/E and audited changes | 388 | compare_generated_candidates.py |
| Root and constant initialization | 99 | compare_generated_bootstrap.py |
| Scalar/catalog bootstrap variants | 36 | compare_generated_constant_variants.py |
| Registration, policy, approval and audit | 960 | compare_constant_governance_5_0_6.py |
| Checker, Behavior and resource overrides | 2,304 | compare_bootstrap_extensions_5_0_6.py |
| Controlled second-connection conflicts and retry | 768 | compare_bootstrap_conflict_5_0_6.py |
| Typed/named resources and Context destruction | 384 | compare_resource_bindings_5_0_6.py |

The sum is 4,939 inputs; the variants and governance matrices use the same
unchanged source executable with different input corpora. Source observations
retain the existing recorded projections; this replay adds no normalization.
The [runtime record](parity/constant_closure_source_5_0_6.json) is written only
after every source comparison and C++ comparator succeeds.

## Authentic source failures

`verify_constant_failure_closure_5_0_6.py` prepares isolated manifest-only
projects under the ignored `build_constant_failure_closure_5_0_6` directory.
Their dependencies point to the complete original generated scalar, constant-only
root and optional-Timestamp crates. Original projects, generated output and
historical error records remain unchanged.

With reference package versions/checksums, the scalar/root crates retain all
17 recorded errors, and optional Timestamp retains all four recorded E0308
errors. Code, message, primary file and line must match the original records;
compiler exit must be 101. These crates cannot supply runnable runtime oracles.
Independent C++ compilation is an explicit binding difference, not successful
execution of the broken Rust artifact. The
[failure record](parity/constant_failure_closure_source_5_0_6.json) preserves
these dispositions and the reference-lock identity.

## Reproduce

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
  teaql_generated_candidates_probe teaql_generated_bootstrap_probe \
  teaql_generated_constant_variants_probe teaql_bootstrap_extensions_probe \
  teaql_bootstrap_conflict_probe teaql_resource_bindings_probe
python3 scripts/verify_constant_closure_5_0_6.py
python3 scripts/verify_constant_failure_closure_5_0_6.py
```

`--record` writes a new evidence record only after successful verification;
neither command edits the existing corpora. Documentation and test harnesses
changed here; runtime and generator algorithms did not. No sanitizer, MIPS or
physical-device run follows from this source replay. The default constant guard
and the remaining [P3 matrix](constant_qualification_matrix_5_0_6.md) obligations
remain in force.
