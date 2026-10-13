# Base and Context query intent replay

This checkpoint qualifies the separate base and Context query responsibilities
with actual locked 5.0.6 methods. It does not merge service/executor/provider
layers or qualify full generated Q, relation, SQL/audit or model configurations.

## Original methods and observer scope

The test crate build script extracts byte-identical original method blocks from
`data_service/base.rs` and `data_service/context.rs`. A post-build verifier checks
complete emitted files, enclosing impl declarations and every extracted method
block against the locked archive. The
[source provenance](parity/layered_query_5_0_6_source_provenance.json) records
markers, method/file hashes, the build-script hash, completed 20-observer build,
610 intact original file prefixes and original registry versions/checksums.

The existing field wrappers use original metadata, executor and telemetry SPI.
The Context wrapper delegates to the actual UserContext for flags, identifiers
and telemetry; its metadata-log method records call counts. It does not implement
or qualify the final SQL/audit projection pipeline. Existing provider type-name
projections are retained; no additional normalization is introduced.

The [driver review](parity/layered_query_5_0_6_driver_review.json) retains exact
diffs and hashes: validated request comment accessors, RuntimeError imports,
original mutation-intent/privacy fields initialized to None, and replacement
of an old fixture constructor by the included original constructor. Additional
purpose observers change only the query's input purpose assignment. Historical
drivers and observations remain intact.

## Historical missing-purpose stimuli

All **724/724** complete historical outputs match actual original/native calls.
The base family has 256 missing-purpose errors and no provider calls. Context
has 356 missing-purpose errors, 64 missing-comment errors and 48 cancellations
before provider work. Consequently this historical replay proves no positive
provider behavior, despite previously successful 5.0.5 fixtures.

## Explicit purpose and execution paths

Every original stimulus is separately expanded with valid, blank and padded
purpose strings. All **2,172/2,172** complete actual original/native outputs match
and reproduce.

| Family | Stimuli | Provider calls | Successful outcomes | Cancelled outcomes | Metadata-log callbacks |
| --- | ---: | ---: | ---: | ---: | ---: |
| Base query explicit purpose | 768 | 512 | 192 | 0 | 320 |
| Context query explicit purpose | 1,404 | 712 | 416 | 344 | 856 |

Other outcomes preserve missing-comment/purpose, executor failure and typed model
decoding errors. Counts distinguish provider calls from successful outcomes and
from telemetry-controlled fallback/cancellation. These are recording executor
fixtures, not SQLite executions. Complete outcomes, notes, telemetry operations,
request flags/traces/comments and metadata call sequences are compared.

The stimuli preserve original list/entity/enhanced adapters, capture/debug/event
flags, pending budgets, provider failure, decode failure, telemetry no-op,
activation and finish failures, callback activation variants and cancellation.
Neither a comment nor a purpose is invented for historical negative fixtures.
No C++ production implementation or generated artifact changes were required.

## Reproduction and verification

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
cmake --build build_native_current --target \
  teaql_runtime_base_probe teaql_context_query_probe teaql_context_query_tests
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family runtime_base --family context_query \
  --driver-dir tests/parity/rust_private_save_5_0_6/src/bin \
  --rust-dir /tmp/teaql_cpp_private_save_rust_506/debug \
  --cpp-dir build_native_current --check
python3 tests/parity/replay_layered_query_intent_5_0_6.py \
  --rust-dir /tmp/teaql_cpp_private_save_rust_506/debug \
  --cpp-dir build_native_current --check
```

Native contracts and all four comparisons pass **5/5** after terminal builds.
Pinned formatting/check covers **993** handwritten files. The change affects
observer inputs and source evidence; sanitizer/MIPS profiles are not rerun.
See the [terminal record](parity/layered_query_replay_5_0_6.json).

Two active historical comparators migrate and two explicit-purpose comparators
are added. The [binding inventory](parity/active_corpus_baseline_after_layered_query_5_0_6.json)
now has 131 active, 67 historical and 21 custom selections; static bindings do
not prove semantic acceptance. Remaining relation, log projection, generated and
model/configuration matrices stay open. All 17 full capabilities remain partial.
