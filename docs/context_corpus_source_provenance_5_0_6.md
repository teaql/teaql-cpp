# Context corpus source and dependency provenance

Status: four baseline-only corpora now have independently checked source-execution
bindings and current native comparisons. This is a bounded SQ02/GP13 cohort;
complete provenance and P0-P7 remain open, with 14 qualified / 3 partial capabilities.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Exact source-execution bindings

The [contract registry](../tests/parity/corpus_source_contracts_5_0_6.json)
associates each corpus with its actual original-execution receipt, producer,
observer, input generator, marker, output prefix and selected Rust test.
No corpus input, expected value or baseline label is rewritten.

| Corpus | Complete observations | Original test responsibility |
| --- | ---: | --- |
| Context helpers | 1,128 | Comment folding and instrumented FnOnce observation names |
| Context factory | 96 | Default capabilities, lookup/error priority and executor registration |
| Context mutation/cache | 4,608 | Success/error, mutation intent and cache boundaries |
| Context privacy preparation | 24 | Privacy forwarding, clearing and execution without invented intent |
| Total | 5,856 | Four separately identified original callers |

The [verifier](../scripts/verify_corpus_source_contracts_5_0_6.py) checks the
following chain against each real archived receipt:

1. The fixed runtime/generator references, current observer/input-generator/
   producer bytes and the original runtime-view helper identity match.
2. All 67 original runtime source/test/vector files match the clean locked
   reference. The appended observer matches its current template except for its
   single injected absolute input-file path; that path is retained as evidence.
3. The private-view manifest matches the captured original Cargo metadata,
   including dependency features and paths. Only the two disabled external
   exporter development dependencies are omitted. Package versions and registry
   version/source/checksum identities agree with the reference lock.
4. The exact selected Rust test ran offline and locked, actually recompiled the
   original runtime, exited successfully and produced the required complete
   marker-framed observations. Those raw stdout observations agree with the
   receipt summary and every current corpus expected value.
5. The current input generator reproduces every corpus input in order. The
   successfully built native probe is executed and its complete typed output
   matches the source observations.

The [final replay](parity/context_corpus_source_bindings_final_5_0_6.json.gz)
matches all **5,856** current native outputs. Original Rust execution is explicitly
reused from the identified receipts; no new Rust run is claimed. A source-only
mode verifies the archived chain without building or executing native probes.
The earlier initial/strict verifier receipts retain their own source versions.

## Independent rejection controls and permanent gates

[Twelve effective controls](parity/context_corpus_source_controls_5_0_6.json.gz)
reject wrong baselines, missing observations, replaced expected values, a
simultaneously changed corpus and receipt summary, altered original sources,
self-consistently rehashed observer changes, dependency overrides, changed crate
checksums, failed original execution, missing locked selection, missing runtime
compilation and partial actual Rust stdout. The untouched binding passes first.

Two permanent host checks preserve this scope:
`parity_corpus_source_contracts` verifies all four archived chains;
`parity_corpus_source_controls` exercises the corruption matrix. The
[registered gate](parity/context_corpus_source_registered_gates_5_0_6.json.gz)
passes both checks and all four existing native comparisons, **6/6**, with exact
JUnit names and no failures/errors/skips. The corruption invariant is recognized
by the custom-dispatcher controller as a source invariant, not a fabricated C++
output comparison.

The four native comparisons require the source-contract check through a CTest
fixture. The [focused fixture gate](parity/context_corpus_source_fixture_gate_5_0_6.json.gz)
selects only the factory comparison and verifies that CTest first adds and runs
the source check automatically. Focused execution therefore retains the required
source/corpus binding.

No production C++, generated SDK, original source/corpus, runtime dependency,
module export or memory/lifetime algorithm changes. No routine Java, sanitizer,
MIPS or physical-router execution is introduced.

## Remaining scope and cleanup

The locator identified 27 direct corpora whose files contain only baseline and
cases. At this checkpoint, four had this external source-execution binding and
the other 23 needed their own evidence reconciliation. The subsequent
[resolved-operation cohort](resolved_corpus_source_provenance_5_0_6.md) adds seven
bindings without relabeling this checkpoint's evidence. Embedded provenance in
the other direct corpora also requires verification. A baseline-only file is
not automatically unsupported or invalid, and embedded metadata is not proof
of actual source execution.

Complete active-dispatcher CLI selection, observer/dependency/cardinality and
normalization review remains SQ02/GP13 work. Full current integration remains
SQ05/GP16; generated metadata completeness remains GP11/G14/AP02. The
[milestone receipt](parity/context_corpus_source_milestone_5_0_6.json.gz)
retains the exact cohort and current source hashes without promoting these
remaining requirements.

All exact owned probe/control directories are removed after child completion;
the [cleanup receipt](parity/context_corpus_source_cleanup_5_0_6.json.gz) records
their measured sizes. The permanent native preset, archived original evidence,
original source and all earlier uncommitted work remain intact. Shared Rust
targets are not built, removed or evicted by this reconciliation.
