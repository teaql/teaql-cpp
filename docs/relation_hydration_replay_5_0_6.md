# Complete historical relation hydration replay

All **356** historical hydration inputs now execute through complete original
Rust 5.0.6 methods, the actual EntityDataService/Context query path, recording
QueryExecutor and graph decoder registry. The previous source observer compiled
private methods against a shortcut child-fetch SPI. That historical observer and
its corpus remain intact; they are not relabeled as evidence for the new boundary.

## Original execution and controls

The direct append-only bridge calls the original attach, compact installation,
embedded enhancement, flat and compact-flat methods. Attachment and embedded
enhancement use the original key snapshot helper. Subqueries flow through the
actual prepared compact query implementation into the recording executor.
No Rust hydration implementation or query execution algorithm is reconstructed.
The [completed source provenance](parity/relation_hydration_replay_5_0_6_source_provenance.json)
records 31 observers, preserved original file prefixes and locked registry
versions/checksums. The [driver review](parity/relation_hydration_replay_5_0_6_driver_review.json)
states each corpus and its observer limits.

Every original input remains in its original order. Outcomes are 87 successes,
231 missing-comment errors, 8 entity/decoder errors and 30 cancellations. These
inputs produce no executor calls: their absent intent remains absent rather than
being silently supplied by the observer. This matrix alone cannot qualify
successful protected query execution.

A separate matrix gives each historical relation plan an explicit child query
comment and purpose, leaving all other existing fields and controls intact.
Six additional cases distinguish reusable non-ID membership, inverse cardinality
and consumed ID buckets. These **362** observations produce 246 successes,
54 cancellations, 42 executor failures and 20 entity/decoder errors, with
**348** actual executor calls. This is a separate stimulus matrix, not a rewrite
of historical inputs.

Complete outputs include parents, identity tables, optional/list views, ledger
alias groups, graph counts, prepared request filters/comment/purpose/trace,
capability/drop counts, telemetry enter/leave/success/failure/drop events and
scope depth. Telemetry success projects cardinality only when present; this does
not qualify every telemetry attribute. The recording executor does not establish
physical original SQLite or final SQL/audit privacy acceptance.

## Shared-target correction

The first raw 356 observations already matched. The first explicit matrix had
one difference: two owners referenced the same non-ID target. C++ consumed the
target bucket for the first owner, then created an unfetched identity edge for
the second. The inverse view also retained the first owner rather than the
original's final owner projection. The [initial complete outputs](parity/relation_hydration_intent_5_0_6_initial_gaps.json)
remain recorded.

Shared flat and compact-flat installation now copies non-ID bucket membership
for each owner. ID-key buckets retain single-consumption behavior, matching the
original duplicate-owner contract. Existing signatures, Context allocation and
independent decoder/provider/executor/runtime responsibilities are preserved.
The SQLite regression now asserts that both owners of a fetched shared target
have no manufactured unfetched edge. Existing Global retention, native pending
work, Context cleanup, facet and loaded-row reconstruction gates remain active.

Source fixture result-count typing and C++ filter-debug include failures are
retained separately. They required fixture corrections and did not change
original source implementations.

## Terminal verification

The complete raw and explicit corpora match and reproduce **356/356** and
**362/362** actual original/native outputs. Relevant native checks pass **7/7**;
targeted ASan/UBSan **3/3** (leak scan disabled) and import-only module checks
**3/3** after terminal builds. The imported replay executes the same two source
corpora and the same SQLite/Context lifetime assertions. All **118** canonical
consumer/fixture bodies, **1,477** exports and module paths verify; pinned
format/check covers **1,003** handwritten files. See the
[terminal record](parity/relation_hydration_replay_5_0_6.json).

The [static binding inventory](parity/active_corpus_baseline_after_hydration_replay_5_0_6.json)
counts 144 active, 63 historical and 21 custom selections. This is corpus binding
evidence, not complete semantic acceptance.

## Reproduction and remaining qualification

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
cmake --build build_native_current --target teaql_relation_hydration_replay_probe
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family relation_hydration \
  --rust-dir /tmp/teaql_cpp_private_save_rust_506/debug \
  --driver-dir tests/parity/rust_private_save_5_0_6/src/bin \
  --cpp-dir build_native_current --cpp-observer-family relation_hydration_replay --check
python3 tests/parity/replay_relation_hydration_intent_5_0_6.py \
  --rust /tmp/teaql_cpp_private_save_rust_506/debug/relation_hydration_5_0_6 \
  --cpp build_native_current/teaql_relation_hydration_replay_probe --check
```

The active historical comparator now binds to the versioned source corpus and
actual-query C++ replay probe; the original corpus, comparator and probe remain
available for historical review. Complete dynamic/JSON view, facet/decoder/model
configurations, final telemetry/privacy and original physical SQL matrices remain
open. These observations qualify the retained input matrix, not every relation
capability. All 17 full capabilities and P7 remain partial.
