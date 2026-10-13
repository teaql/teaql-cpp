# Native relation dispatch and enhancement replay

This checkpoint replays two complete historical algorithm families using original
locked 5.0.6 method blocks and recording child-fetch SPI fixtures. It does not
qualify the full physical request/provider/SQL pipeline or dynamic loader/facet
hydration paths. The remaining full capabilities stay partial.

## Source methods and fixture boundaries

The original native-fetch branch, relation helpers and enhancement methods are
compiled verbatim. A post-build verifier checks complete emitted fragments and
every selected original block. The
[source provenance](parity/relation_dispatch_enhancement_5_0_6_source_provenance.json)
records a completed 25-observer build, 610 intact original archive prefixes and
original registry versions/checksums. The
[driver review](parity/relation_dispatch_enhancement_5_0_6_driver_review.json)
records exact observer hashes and scope.

The native fixture explicitly asserts no dynamic selection and calls the actual
native branch. The enhancement fixture records child-fetch inputs, suspension,
failure and Drop. Its request intent delegates to the complete original runtime
through an appended bridge; dynamic-loader fixture calls panic. Neither fixture
invents valid intent for historical inputs.

## Initial differences and shared runtime corrections

The initial native comparison found 336/432 differences. Fixed fixture expression
labels replace shifted C++ enum ordinals. Two actual execution-route differences
remain after that observer correction: one parent with capture disabled used a
semantic partition batch in C++, while Rust uses a scalar bounded probe. The
initial and post-label outputs are retained.

Provider batch eligibility now requires at least two distinct parent identities.
The real SQLite regression checks singleton scalar equality, no partition and the
same offset/limit row in both capture modes.

Enhancement initially differs on 231/266 inputs. The original child-query helper
validates comment/purpose before child-fetch even without dynamic selection.
C++ now performs that same validation and preserves callback absence on rejection.
Aggregate lineage uses the actual relation path, not the output alias. Canonical
relation-child scoping retains independent parent trace and request ownership.

Original aggregate key snapshots capture only declared local relation fields
before any aggregate runs. C++ now validates all sibling relation names and
captures the same fields once before fetch. Both subsequent queries and result
attachment use the snapshot, so an aggregate alias can replace a parent's ID
without corrupting later sibling lookup. The existing attachment API remains;
a key-aware overload serves the shared operation.

Native fixtures supply explicit child intent before invoking the helper and
assert the original relation-path frame. Their physical callback and lifetime
assertions remain. SQLite additionally checks sibling aggregates whose first
alias overwrites the ID.

## Additional source variants

All 75 historical child stimuli are separately expanded with explicit nonblank
comment/purpose. Another 36 sibling aggregate stimuli vary pending budgets,
polls, failure position and missing second relation; the first alias shadows the
local ID. Source assertions require missing sibling relations to reject before
any fetch. These 111 variants preserve actual method bodies and SPI boundaries.

Historical inputs and corpora remain intact. The
[count record](parity/relation_dispatch_enhancement_5_0_6_counts.json) separates
child-fetch calls, cancellation and errors from successful outcomes; these calls
are not counted as actual SQLite/provider executions.

## Reproduction and remaining acceptance

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family relation_execution --family relation_enhancement \
  --driver-dir tests/parity/rust_private_save_5_0_6/src/bin \
  --rust-dir /tmp/teaql_cpp_private_save_rust_506/debug \
  --cpp-dir build_native_current --check
python3 tests/parity/replay_relation_enhancement_variants_5_0_6.py \
  --rust /tmp/teaql_cpp_private_save_rust_506/debug/relation_enhancement_variants_5_0_6 \
  --cpp build_native_current/teaql_relation_enhancement_probe --check
```

All **698/698** historical outputs and **111/111** new variants match actual
original/native calls and reproduce. The broader native selection passes **11/11**.
After the fixture ownership correction, the changed native contract passes
**1/1**, targeted ASan/UBSan **2/2** (leak scan disabled) and canonical module
contracts **2/2**. All **110** canonical consumer/fixture bodies verify; pinned
format/check covers **994** handwritten files. All compiler/linker/test processes
terminate before success is recorded. See the
[terminal record](parity/relation_dispatch_enhancement_replay_5_0_6.json).

The [initial compiler error](parity/relation_dispatch_enhancement_5_0_6_initial_build_error.json)
required checked ResourceBorrow access through `get()`. The
[first verification](parity/relation_dispatch_enhancement_5_0_6_initial_verification.json)
retains sanitizer 1/2: adding explicit intent caused a local SelectQuery to own
allocated trace/string data while surviving explicit Context close. It now
belongs to the Context, preserving close/worker-join assertions and destruction
before arena reclamation.

Two historical comparators migrate and one explicit-variant comparator is added.
The [binding inventory](parity/active_corpus_baseline_after_dispatch_enhancement_5_0_6.json)
counts 136 active, 64 historical and 21 custom selections. Static bindings are
not behavioral acceptance. Initial [native](parity/relation_execution_5_0_6_initial_gaps.json),
[post-label](parity/relation_execution_5_0_6_after_label_gaps.json) and
[enhancement](parity/relation_enhancement_5_0_6_initial_gaps.json) differences remain
available independently of the final source corpus.

Broader dynamic loader, facet hydration, metadata/privacy, generated and model
configuration acceptance remains open, including the historical hydration family.
