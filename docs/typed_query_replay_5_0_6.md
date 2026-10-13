# Typed query, stream and entity decoding replay

This checkpoint replays five complete historical input families through actual
locked Rust 5.0.6 and native C++ calls. It qualifies those stimuli, not the full
generated Q surface, model/configuration matrix or P7.

| Family | Original observations |
| --- | ---: |
| Typed entity queries | 165 |
| Entity streams | 193 |
| Enhanced entities and relations | 154 |
| Entity graph decoders | 198 |
| ID-set pagination | 233 |
| Total | 943 |

All **943/943** complete outputs match and reproduce without new output
normalization. Historical inputs, order, drivers and corpora are retained.
Five active CMake comparators now select separate versioned 5.0.6 corpora.
The [driver review](parity/typed_query_5_0_6_driver_review.json) checks exact
adaptations: two request comment fields become validated intent accessors;
one fixture import changes location; two observers remain byte-identical.

## Source build and dependencies

The existing private-source archive observer gains an appended bridge to the
original ID-set page preparation helper. All 610 original archive files retain
their original prefixes; production methods are unchanged. The
[source provenance](parity/typed_query_5_0_6_source_provenance.json) records a
completed locked offline build and exact registry versions/checksums from the
original lock. The frozen reference checkout remains untouched.

The first source build exposed four missing test-crate imports needed by the
existing SQLite enhancement observer. The [initial source errors](parity/typed_query_5_0_6_source_build_errors.json)
are retained. Test-only dependencies now explicitly include the archived SQL
and SQLite packages and the original rusqlite version/features. These do not
add C++ production dependencies or broaden provider scope.

## Compact list execution responsibility

The first comparison found four differences, all in compact list queries with
a continuous-page hint. The other four complete families already matched.
See [initial differences](parity/typed_query_5_0_6_initial_gaps.json).

In the original runtime, `fetch_smart_list_internal` prepares selection and
uses `fetch_prepared_compact_owned`. This path does not enable ordinary entity
aggregation-cache or continuous-page execution. It moves the query trace into
the physical request when capture is enabled, otherwise clears it, while
preserving independent validated request intent.

C++ previously delegated this compact list branch to the general Context
query path, retaining physical query traces. The shared entity list operation
now delegates to the existing owned compact preparation operation and existing
row-list adapter. Typed entity branches, public APIs and ownership domains are
unchanged. No generator or generated artifact is modified.

The canonical native regression checks both SQL capture modes, independent
comment/purpose intent, empty physical query traces, capture-controlled request
traces, reusable caller trace preservation, one provider call and pending/cancel
forwarding. Context allocation remains explicit and nested; the existing test
body also checks application Q/E execution and lifetime contracts.

## Reproduction and verification

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family entity_query --family entity_stream --family enhanced_entities \
  --family entity_decoder --family id_set_page \
  --driver-dir tests/parity/rust_private_save_5_0_6/src/bin \
  --rust-dir /tmp/teaql_cpp_private_save_rust_506/debug \
  --cpp-dir build_native_current --check
```

The relevant native selection passes **10/10**. Targeted ASan/UBSan passes **2/2** (leak scan disabled); the canonical
import-only application/entity query contract passes **1/1**. All **107**
consumer/fixture bodies verify. See the [terminal record](parity/typed_query_replay_5_0_6.json). Completion is recorded
only after compiler/linker and test processes terminate. Pinned format/check covers **993** handwritten C++ files; active module paths verify.
Complete configurations,
separate build-profile cursor boundaries, historical replay and final integration
acceptance remain open; all 17 full capabilities remain partial.
