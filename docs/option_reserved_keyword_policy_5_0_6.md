# Option is a reserved model identifier

The [default source-entry repair](default_source_configuration_guard_5_0_6.md)
closes the subsequently diagnosed generic-service bypass. Both public source
overloads now require a configured callback, so they cannot emit interpreted C++
for a reserved model name. Configured factory/host/offline routes retain their
existing keyword checks; explicit domain-only maintenance remains separate.

The historical [default generic-source diagnosis](compiler_protected_configuration_join_5_0_6.md#remaining-default-source-entry-bypass)
found two public `DomainGenerationService` source overloads that used the
unconfigured emitter and skipped the configured keyword boundary. The later
source-entry repair described above closes that bypass. The original diagnosis
retains its historical scope; no Option-specific compiler adaptation is required.

The user's later decision on 2026-11-20 reserves `option` through the existing
model keyword list. This supersedes the earlier plan to support an entity named
Option through a C++ compatibility exception.

The shared `ReservedKeywordRule` reserves the exact spellings `option` and
`Option` alongside the existing Rust name restrictions. Rust's `Option<T>` is a
standard prelude type; the restriction protects generated model identifiers
from shadowing it. The model error remains the existing `KSML-KEYWORD-001` for an
entity or `KSML-KEYWORD-002` for a field, with source location and a concrete
rename suggestion.

For example, rename an entity to `option_record` or `payment_option`, or a field
to `option_code`. The rule checks model identifier names. Optional fields,
ordinary string values such as `Option`, and Rust type expressions keep their
existing semantics.

Configured formal C++ generation invokes the existing shared keyword rule before
the compiler adapter. This covers direct source-service calls
and the normal two-argument offline entry, in addition to the upload and dynamic
routes that already validate models. Reserved-name input cannot start the
compiler worker or publish an SDK. Trusted compiler configuration remains
required. Explicit low-level emitter diagnostics retain their historical scope.

The complete model that produced the original Rust collision is retained as
`generator/src/test/resources/models/cpp_option_reserved.xml` in the independent
generator workspace. It is now a negative model-validation fixture. The locked
Rust failure and earlier C++ compilation result remain historical evidence;
neither is relabeled as current model acceptance.

No Option-specific compiler-error bypass or reference-source rewrite is needed.
The locked Rust 5.0.6 reference remains unchanged. Complete configuration,
protected-consumer and final integration obligations remain independent.

## Verification

The [fresh Java execution](parity/option_keyword_java_5_0_6.json.gz) passes all
98 shared evaluator tests and 295 C++ generator tests. Isolating the old keyword
rule causes six regression failures; isolating the previous factory without the
keyword boundary causes three. This distinguishes list coverage from actual
source/offline enforcement. The original complete collision model is rejected
with `KSML-KEYWORD-001` and zero compiler-worker invocations.

Entity and field cases cover both reserved spellings, source locations and
rename guidance. Compound names, ordinary `Option` string data and nullable
fields remain valid. Configured source, upload, dynamic and ordinary offline
requests reject reserved names before SDK publication; renamed requests reach
the existing compiler adapter.

Ordinary and optimized Python each reject twelve execution-receipt corruptions
and twelve source/policy corruptions. The first
[overbroad validation attempt](parity/option_keyword_java_initial_failure_5_0_6.json.gz)
is retained. The final implementation delegates only keyword checking at the
source/offline boundary and preserves the upload/dynamic full-validation gate.

The [artifact identity check](parity/option_keyword_artifact_reuse_5_0_6.json.gz)
retains all eighty SDKs, 3,560 code/guidance files plus eighty manifests, twenty
emitter/provenance inputs and 1,824 C++ source inputs unchanged. Existing
generation/compiler evidence is reused explicitly; these identity checks do not
claim new C++ compilation or SDK generation.

The [affected native source/receipt checks](parity/option_keyword_native_5_0_6.json.gz)
and their required fixtures pass 40/40 in ordinary Python and 40/40 in optimized
Python. Both JUnit files have complete membership and no failures, errors or
skips, with identical before/after source frames. Historical compiler and
compatibility evidence keeps its original scope. The Option naming blocker is
resolved; G13/G14/G17 and the final P0-P7 integration remain partial.

The [checkpoint](parity/option_keyword_checkpoint_5_0_6.json.gz) binds the current
source closure, actual JUnit outcomes and retained evidence hashes. All three
owned temporary paths are absent, with 2,046,031 measured bytes removed from
successful runs. Compact reports and the original failure retain 2,494,895 bytes
before the checkpoint file itself; stable build directories and shared caches
are preserved.

```sh
source scripts/toolchains/activate.sh
python3 -B scripts/verify_option_keyword_policy_5_0_6.py --check \
  --record docs/parity/option_keyword_java_5_0_6.json.gz
python3 -B scripts/option_keyword_source_admission_5_0_6.py
PYTHONOPTIMIZE=1 python3 -B scripts/option_keyword_source_admission_5_0_6.py
```
