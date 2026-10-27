# Source bindings for baseline-only direct corpora

Status: all **27** identified direct corpus files containing only `baseline`
and `cases` now have checked external source bindings, covering **58,920**
complete observations. This closes that finite cohort. Embedded provenance in
the other 278 direct corpora, custom dispatchers, normalization, generated
metadata and whole current integration retain their own requirements.
SQ02/GP13, G13, G14, G17 and P0-P7 remain open.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Fresh original execution for the final seven corpora

The [contract registry](../tests/parity/remaining_corpus_source_contracts_5_0_6.json)
names the input builder, source target, Cargo project and native comparison.
Existing input functions are reused; three inline matrices are evaluated from
the exact AST expressions in their existing producers. No input or expectation
is rewritten.

| Corpus | Complete observations | Original execution |
| --- | ---: | --- |
| Generated support | 822 | Private-state file observer and source interface contract |
| Memory engine | 2,200 | Original engine/evaluator through the retained source driver |
| Mutation intent/privacy | 18 | Original private Runtime/Context intent setters |
| Row construction | 32 | Original constructors with matching and mismatched shapes |
| Row enhancement | 1,373 | Original checked merges and shape/conversion sequences |
| Row read sidecars | 972 | Original relation/dynamic sidecar transitions and diagnostics |
| Dynamic schema source | 864 | Original provider/bootstrap phases and cancellation |
| Total | 6,281 | Seven complete original callers |

The [fresh driver](../scripts/replay_remaining_corpus_sources_5_0_6.py) records
actual commands, compiler identity, complete original/native stdout and the
generated-support observer's raw output file. An earlier receipt retained only
that file observer's corpus hash. The
[new execution](parity/remaining_corpus_source_execution_5_0_6.json.gz) matches
all **6,281** original outputs to the existing corpora and current native outputs.
The generated-support interface test also passes.

Three existing standalone Cargo projects compile their original targets against
the reference dependencies. Builds use offline and locked selection. A private
runtime view appends the unchanged support and intent/privacy observers while
preserving all 67 original source/test/vector bodies. Its manifest follows
original Cargo metadata, omitting only the two disabled external-exporter dev
dependencies. All 466 tracked reference Rust/manifests/lock files are hashed;
the clean reference HEAD/tag is verified before and after execution. Project
manifests, locks, resolved source targets and local dependency locations are
retained. Registry identities agree with the reference lock. All 4,372 recorded
input files remain unchanged during the fresh replay.

Rust uses one explicitly owned target directory. Incremental compilation is
disabled and debug information reduced; debug assertions, selected tests and
reference/candidate separation are preserved. Shared targets are not reused or
evicted.

## Permanent validation without routine Rust compilation

The [validator](../scripts/verify_remaining_corpus_source_contracts_5_0_6.py)
checks the retained execution against the locked reference, observer assembly,
manifests, locks, compiled targets and ordered input builders. Actual source
stdout or raw output-file bytes must match both the observation summary and
every corpus expected value. Native payloads and outputs are checked separately.
This source-record check does not build or execute Rust. Existing C++ comparators
execute the current native binaries; an earlier binary hash remains historical
evidence rather than freezing future runtime development.

[Twenty-four effective controls](parity/remaining_corpus_source_controls_5_0_6.json.gz)
reject wrong baselines, missing/changed sources, shared targets, unlocked/failed
compilation, manifest overrides, changed checksums/prefixes/observers, different
targets or local dependencies, missing/duplicate records, omitted members,
changed values, boolean/integer substitution, altered input payloads and
rehashed file observations. All seven untouched bindings pass first.

Permanent checks `parity_remaining_corpus_source_contracts` and
`parity_remaining_corpus_source_controls` preserve this contract. Each of the
seven comparisons requires the source check through a CTest fixture. The
negative registration is also recognized as a source invariant by the custom
dispatcher controller.

## Reconciliation of all four cohorts

| Cohort | Corpora | Observations | Source execution |
| --- | ---: | ---: | --- |
| [Context](context_corpus_source_provenance_5_0_6.md) | 4 | 5,856 | Identified archived runs |
| [Resolved operations](resolved_corpus_source_provenance_5_0_6.md) | 7 | 30,410 | Identified archived runs |
| [Composite observers](composite_corpus_source_provenance_5_0_6.md) | 9 | 16,373 | Pinned archived assembly and runs |
| This fresh cohort | 7 | 6,281 | New original/native execution |
| Total | 27 | 58,920 | Complete observations in each named cohort |

The [combined gate](parity/remaining_corpus_source_registered_gates_5_0_6.json.gz)
passes **35/35** affected native checks: 27 comparisons and four pairs of
source/negative checks. Exact JUnit names show no failures, errors or skips.
Selecting generated support, row construction and dynamic schema automatically
adds and runs their source fixture first (**4/4**).

The later [optimized-Python source repair](registered_python_optimized_checks_5_0_6.md#seven-corpus-source-validator)
reproduces an invalid saved execution record accepted by the old seven-corpus
validator under `python -O`. It changes only the current source verifier and
negative controls, with exact pre-rewrite source and current-source evidence;
the original Rust execution and this historical 35-test gate remain unchanged.
The repaired focused source/control checks pass 2/2. A complete native epoch
on the repaired source set remains open.

The [current inventory](parity/remaining_corpus_source_inventory_5_0_6.json.gz)
records 887 native registrations, 305 direct active-baseline bindings and 174
custom registrations requiring review. The
[milestone](parity/remaining_corpus_source_milestone_5_0_6.json.gz) joins all four
registries to the exact 27-file cohort. Labels and metadata alone do not accept
the remaining provenance scope. Verdicts stay **14 qualified / 3 partial**.

## Preservation and cleanup

The [preceding snapshot](parity/corpus_source_before_remaining_5_0_6.json.gz)
retains the earlier registration/dispatcher/documents and exact CMake input of
the fresh build. Earlier records remain intact. No production C++, generated
SDK, generator, dependency or module export changes. No routine Java, sanitizer,
MIPS or device execution is added.

The fresh replay removed **399,189,614 bytes** of owned build/source/output files
after compiler/linker and probe completion. Its compressed record keeps complete
original/native output for independent checking. The
[cleanup receipt](parity/remaining_corpus_source_cleanup_5_0_6.json.gz) also lists
exact temporary validator/control/JUnit/inventory paths removed. Stable presets,
original fixtures, databases and earlier uncommitted work remain intact.

Reproduce with a new record filename:

```bash
source scripts/toolchains/activate.sh
python3 scripts/replay_remaining_corpus_sources_5_0_6.py \
    --record /tmp/remaining_corpus_source_replay_5_0_6.json.gz
```
