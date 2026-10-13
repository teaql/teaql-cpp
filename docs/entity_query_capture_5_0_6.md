# Independent SELECT and MUTATION query capture options

This extends [typed query replay](typed_query_replay_5_0_6.md) with explicit
original public `SqlLogOptions` configuration. The earlier observer already
captured SELECT metadata with its enabled default; this matrix adds independent
SELECT/MUTATION combinations rather than claiming previously absent coverage.

The Rust observer differs only by the original public option setter before
provider/service construction. All actual query calls remain unchanged. C++
adds optional fixture input fields, preserving historical stimuli. There are
no new runtime or generator implementations in this checkpoint.

## Actual source evidence

**768/768** complete actual Rust/native outputs match and reproduce across four
entry points (`fetch_smart_list`, `fetch_all_owned`, `fetch_all`, and list with
relation aggregates), both independent flags, continuous-page hints, execution
success/failure, absent/nonblank/blank comments, scalar/aggregate projections,
and aggregation cache with its backend. Each stimulus requests two executions.

The matrix records **488** provider requests, **244** with capture enabled.
**512** stimuli are rejected before provider work, preserving original missing
or blank request-comment errors. Every entry point has 192 stimuli, 64 successful
execution outcomes and 320 error outcomes. Errors and repeated calls are not
counted as successful SQLite execution; the fixture provider is a recording SPI.
See the [source review](parity/entity_query_capture_5_0_6_source_review.json).

Source assertions independently verify capture/debug flags, MUTATION-option
independence, physical compact-query trace removal, capture-controlled request
traces, and two direct compact list calls even when caching/continuous hints are
present. Other entry points retain their distinct cache/pagination behavior.
Full outputs are compared without new normalization.

The [source provenance](parity/entity_query_capture_5_0_6_source_provenance.json)
records the completed 16-observer build, intact original file prefixes and
original registry lock. The historical observers and corpora remain unchanged.

## Reproduction and gates

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
cmake --build build_native_current --target teaql_entity_query_probe
python3 tests/parity/replay_entity_query_capture_5_0_6.py \
  --rust /tmp/teaql_cpp_private_save_rust_506/debug/entity_query_capture_5_0_6 \
  --cpp build_native_current/teaql_entity_query_probe --check
```

Native query/capture/application contracts pass **3/3**; pinned formatting/check
covers 993 handwritten files. The preceding production correction passed
ASan/UBSan 2/2 and canonical module 1/1, with 107 unchanged consumer bodies; this
observer-only extension does not rerun those profiles. See the
[terminal record](parity/entity_query_capture_5_0_6.json).

A new direct comparator selects the separately versioned corpus. The
[binding inventory](parity/active_corpus_baseline_after_query_capture_5_0_6.json)
now counts 127 active, 69 historical and 21 unresolved custom selections. This
static inventory is not behavioral acceptance. Complete generated/model/privacy
configurations, profile-specific cursor boundaries and P7 remain open.
