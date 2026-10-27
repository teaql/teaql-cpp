# Resolved-operation corpus source provenance

Status: seven more baseline-only corpora have verified original-source execution
bindings and matching current native outputs. Together with the
[Context cohort](context_corpus_source_provenance_5_0_6.md), this covers 11 of the
27 identified baseline-only direct corpora. This does not complete SQ02/GP13,
G13, G14, G17 or the P0-P7 qualification plan.

The locked references remain runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Complete observations in this cohort

The [contract registry](../tests/parity/resolved_corpus_source_contracts_5_0_6.json)
identifies each actual original-execution receipt, producer, observer version,
input generator, appended source file, Rust test and native comparison.

| Corpus | Observations | Observer template |
| --- | ---: | --- |
| Aggregate membership | 10,560 | Current |
| Enhanced aggregate | 320 | Explicitly pinned archive |
| Query entry cursor | 8,000 | Explicitly pinned archive |
| Query entry protocol | 6,080 | Explicitly pinned archive |
| Resolved dynamic rows | 2,724 | Current |
| Resolved select preparation | 1,036 | Current |
| Resolved mutation preparation | 1,690 | Current |
| Total | 30,410 | Seven complete original callers |

The [source verifier](../scripts/verify_corpus_source_contracts_5_0_6.py) checks
all 67 original runtime source/test/vector files for each binding, the actual
append target, source snapshots and their original input hashes. It reproduces
every ordered input with the current input generator. The original private
manifest must match captured Cargo metadata; only the two disabled external
exporter development dependencies are omitted. All 119 registry package
version/source/checksum identities must belong to the locked reference.

The original Cargo test must have selected the named observer, used offline and
locked resolution, recompiled `teaql-runtime v5.0.6`, exited successfully and
emitted the complete observations. Actual marker-framed stdout must match both
the receipt summary and every current corpus expected value. Expected values
and original receipts are not rewritten.

The [current native replay](parity/resolved_corpus_source_current_native_5_0_6.json.gz)
successfully builds the four distinct native probes and matches all **30,410**
outputs. The original Rust executions are reused from their identified receipts;
this replay does not claim a new Rust execution. The separate
[source-only replay](parity/resolved_corpus_source_initial_5_0_6.json.gz) verifies
the same archived chains without executing C++.

## Observer versions remain distinct

The shared aggregate observer evolved after the enhanced, protocol and cursor
runs. Those three contracts pin the exact archived template SHA-256 separately
from the current file. The original receipt input hash must identify those
archived bytes, and the actual appended observer must match that template with
only its absolute `inputs.json` path substituted. The verifier also records the
current observer hash. An archived contract requires a different current file;
it cannot silently become a claim about current observer execution.

Current-version contracts still require exact current template bytes. Producer,
input generator and runtime-view helper identities remain checked for both
versions. No changed current file is relabeled as an older executed observer.

## Permanent gates and rejection controls

The [corruption controls](parity/resolved_corpus_source_controls_5_0_6.json.gz)
reject **20** effective changes: the existing 12 source/dependency/output
corruptions plus missing or wrong archive pins, a false current-version claim,
an unsupported version, a wrong append target, an extra appended observer,
self-consistently rehashed replacement archive bytes and current bytes
relabeled as an archive. All three untouched archived versions pass first.

`parity_resolved_corpus_source_contracts` verifies the seven source chains;
`parity_resolved_corpus_source_controls` runs the corruption controls. Each of
the seven native comparisons requires the source-contract check through a
CTest fixture. The source-control dispatcher registration is classified as an
existing provenance invariant, rather than as a C++ output comparator.

The [registered native gate](parity/resolved_corpus_source_registered_gates_5_0_6.json.gz)
passes **15/15** affected checks: both new source checks, seven resolved-operation
comparisons, both preceding Context source checks and four Context comparisons.
JUnit names match the expected set, with no failures, errors or skips. A second
focused selection of only `resolved_select_preparation_rust_parity` automatically
adds and successfully runs its source fixture first (**2/2**). The configured
native registry at this checkpoint contains 883 tests; this is a targeted gate,
not a new full-suite result.

## Remaining qualification and artifact lifecycle

At this checkpoint, the cumulative 11 bindings covered **36,266** observations,
with 16 baseline-only direct corpora remaining. The subsequent
[composite-observer cohort](composite_corpus_source_provenance_5_0_6.md) adds nine
bindings without relabeling this checkpoint. Other direct corpora contain
embedded metadata that also needs
verification; custom dispatcher provenance and normalization review remain open.
Metadata presence, a source hash or an earlier full-suite result is insufficient
to close these obligations.

There are no production C++, generated SDK, dependency, module export or memory
algorithm changes in this cohort. No Java, sanitizer, MIPS or physical-device
run is added. The [milestone receipt](parity/resolved_corpus_source_milestone_5_0_6.json.gz)
binds this scope without promoting any partial capability.

Every recorded owned temporary directory is removed after its child processes
finish. The [cleanup receipt](parity/resolved_corpus_source_cleanup_5_0_6.json.gz)
lists exact paths and measured bytes. Stable preset build directories, source
fixtures, original execution evidence and earlier uncommitted work are retained.
Shared Rust targets are neither built nor evicted by this reconciliation.
