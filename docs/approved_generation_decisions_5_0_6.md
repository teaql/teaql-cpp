# Approved generation decisions

On 2026-11-20 the user resolved the three outstanding behavior choices:

| Decision | Required behavior |
| --- | --- |
| Missing trusted compiler settings | Formal C++ generation fails explicitly and lists the required locked compiler settings. It cannot fall back to Java interpretation. |
| Locked Rust `Option` model collision | Preserve correct C++ entity/projection behavior and record a separate [compatibility exception](option_model_compatibility_exception_5_0_6.md). |
| Column literal injecting derive attributes | Preserve the explicit C++ refusal and record a separate [compatibility exception](field_attribute_compatibility_exception_5_0_6.md). |

Formal generation means the two-argument offline entry and the Spring-assembled
source service, upload and dynamic C++ routes. The shared startup factory guards
the C++ scope even when no settings exist. Other language targets remain usable.
The error identifies `teaql.cpp.compiler.python`, `worker`, `capabilities` and
`temporary_root`. These are trusted startup settings, not model attributes.
Partial settings, changed capability artifacts, compiler refusals and unsupported
mapping outcomes retain their existing errors and cleanup contracts.

Explicit qualification flags and supplied compiler-observation documents retain
their narrower declared responsibilities. A low-level emitter test is not a
formal compiler-backed generation request. The Rust compiler is a generation
tool; the generated C++ SDK and SQLite runtime acquire no Rust dependency.

There are now five approved behavior exceptions: JSON payload classification,
quoted-FK loading, invalid wire naming, the Option model collision and column
attribute injection. Their scopes remain independent. The last two approvals
preserve the recorded mismatches; they do not relabel original failures as
successful Rust parity.

The implementation, current Java contracts, negative controls and remaining
integration obligations are tracked separately from approval. G13, G14 and G17
remain partial until the generation, protected-consumer and phase exits close.
The earlier [authority diagnosis](compiler_generation_authority_5_0_6.md) remains
historical evidence of the fallback defect.

## Verification

The [fresh Java execution](parity/approved_generation_decisions_java_5_0_6.json.gz)
passes all 289 current contracts. Compiling the exact previous factory in an
isolated classpath makes five new regression cases fail. The first collection's
working-directory and implicit qualification assumptions are retained in its
[failure receipt](parity/approved_generation_decisions_java_initial_failure_5_0_6.json.gz).

The [exact source continuation](parity/approved_generation_decisions_sources_summary_repaired_5_0_6.json.gz)
preserves all emitter bytes and the existing diagnostic, comparison and cleanup
responsibilities. Ordinary and optimized Python each reject ten approval
corruptions and eighteen source/Java receipt corruptions. The old relationship
scope remains 6,576 exact comparisons plus 546 approved quoted-FK cases; its
fifteen substitution controls remain enforced.

The [artifact identity check](parity/approved_generation_decisions_artifact_reuse_5_0_6.json.gz)
finds all eighty SDKs unchanged: 3,560 code/guidance files plus eighty manifests,
with the same twenty emitter/provenance inputs. All 1,824 C++ source inputs also
remain exact. This is explicit reuse of existing generation/compiler evidence,
not a fresh SDK generation claim.

The [final affected CTest execution](parity/approved_generation_decisions_native_5_0_6.json.gz)
passes 37 ordinary tests for the repaired summary and its required fixtures,
plus 46 optimized tests for the affected readers/controls and their fixtures.
JUnit membership is complete, with no failures, errors or skips and identical
before/after source frames. The preceding ordinary selection retains
[46 successful tests and one failed summary guard](parity/approved_generation_decisions_native_summary_failure_5_0_6.json.gz),
with a [stable source closure](parity/approved_generation_decisions_native_summary_failure_closure_5_0_6.json.gz).
The reader repair does not repeat that successful business execution or relabel
the failed selection as a complete passing epoch. Earlier source-reader failures
and exact corrections are retained. All thirteen custom-summary corruption
controls continue to reject in both Python modes.

The pinned formatter and format check, upstream lock, module-path verifier,
capability-ledger verifier and both repository whitespace checks pass. Temporary
Java compilation and CTest directories are removed; compact reports, source
images and failure diagnostics remain. Stable build directories and shared
caches are preserved. No complete native, module, sanitizer or MIPS run is claimed.

The [checkpoint](parity/approved_generation_decisions_checkpoint_5_0_6.json.gz)
binds the current source closure, locked references, terminal/JUnit outcomes and
retained report hashes. All five owned temporary paths are absent; successful
runs measured 2,014,166 removed bytes. The retained compact reports and failure
diagnostics total 3,557,908 bytes before this checkpoint file itself.

Read-only reproduction of the decision checks requires no compiler execution:

```sh
source scripts/toolchains/activate.sh
python3 -B scripts/verify_generation_decisions_5_0_6.py \
  --check-java docs/parity/approved_generation_decisions_java_5_0_6.json.gz
python3 -B scripts/generation_decision_source_admission_5_0_6.py
PYTHONOPTIMIZE=1 python3 -B scripts/generation_decision_source_admission_5_0_6.py
```

The older `governance_status_current_5_0_6.py` report belongs to its frozen
707-consumer/716-command source frame. Its pending-choice assertion is retained
as history. The new source continuation preserves that data's full preimage and
changes only the two answered choices; GP11 and GP16 remain open. It does not
claim a complete new governance or native integration epoch.
