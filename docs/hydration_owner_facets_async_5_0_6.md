# Embedded owner facet asynchronous qualification

The actual locked Rust 5.0.6 enhancement, relation facet and shared facet
execution methods match C++ for **432/432** observations. The matrix covers
include-all/restricted facets, to-one/to-many relations, valid/typed-null/missing
owner keys, three pending delays, four poll budgets and failure at either owner.
It records complete parent updates, facet values, request filters, errors and
executor stop counts. Outcomes are **60 success, 132 failure and 240 cancellation**.

The Rust observer drops the actual future before inspecting partial updates.
Its recording executor retains a drop guard across pending facet fetches. The
C++ executor stops once on completion, error or cancellation. This qualifies
the runtime protocol at the recording executor boundary; it does not qualify
physical SQL, a threaded scheduler or full privacy/model configurations.
See the [driver review](parity/hydration_owner_facets_async_5_0_6_driver_review.json)
and [completed 28-observer source build provenance](parity/hydration_owner_facets_async_5_0_6_source_provenance.json).
Original archive prefixes and registry versions/checksums remain preserved.

## Context close and modules

A separate C++ contract suspends the public relation enhancement operation while
the facet executor owns a 32 KiB Context host buffer. Closing UserContext calls
the executor stop while that buffer remains readable, then reclaims every arena
block. An external counter verifies exactly one stop after close; no borrowed
query data is inspected after reclamation. Default PMR allocation is disabled.

The first import-only build failed because the cleanup fixture directly named a
private operation type. The [original failure](parity/hydration_owner_facets_async_5_0_6_initial_module_error.json)
is preserved. The fixture now enters through the existing exported enhancement
interface, matching ordinary callers. Production exports and implementations
did not change for this checkpoint. Canonical header and module fixtures retain
the same bodies and assertions.

Native, targeted ASan/UBSan and import-only module checks pass **2/2 each** after
terminal compiler/linker processes. Sanitizer leak scanning is disabled. All
**113** canonical consumer/fixture bodies, **1,477** catalog exports and module
paths verify; pinned formatting/check covers **999** handwritten files. See the
[terminal gates](parity/hydration_owner_facets_async_5_0_6.json).

## Reproduction and remaining work

```sh
source scripts/toolchains/activate.sh
python3 tests/parity/build_private_save_oracles_5_0_6.py
cmake --build build_native_current --target teaql_hydration_owner_facets_async_probe
python3 tests/parity/replay_hydration_owner_facets_async_5_0_6.py \
  --rust /tmp/teaql_cpp_private_save_rust_506/debug/hydration_owner_facets_async_5_0_6 \
  --cpp build_native_current/teaql_hydration_owner_facets_async_probe --check
ctest --test-dir build_native_current --output-on-failure \
  -R '^hydration_owner_facets_async_(rust_parity|context_close)$'
```

The [static binding inventory](parity/active_corpus_baseline_after_owner_facets_async_5_0_6.json)
counts 139 active, 64 historical and 21 custom selections. These counts do not
prove behavioral parity. Flat graph facet storage/readback, dynamic loading,
physical SQL/privacy and complete model configurations remain required. All 17
full capabilities and P7 remain partial.
