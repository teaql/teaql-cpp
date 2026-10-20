# Deferred typed pages and ID-store ownership

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This checkpoint addresses deferred ID-set pagination through actual generated
page execution and the existing Context store SPI. Broader pagination and final
integration qualification remain open.

## Actual source comparison

The complete unchanged dynamic_json Rust domain supplies the original
`execute_for_page` implementation. Both observers install the same existing
SQLite schema/data, original/generated domain executor and active Platform root.
The query uses ordered Order rows and ID-set optimization. Only the application
store SPI is controlled: it can miss, hit, return an empty set, fail a read or
fail a write, and suspend at the initial get, locked recheck or put.

The declared 252-case matrix crosses those valid store/gate combinations,
resume/cancel actions, offsets 0/1/4 and limits 0/1/2. Observations retain exact
callback keys, order, IDs, pending/wake behavior, typed rows/versions/totals,
errors, cancellation counts and a subsequent actual page request. Cancelling
at the locked recheck or put must release the build guard so the retry can
execute. No local SQL/query/pagination implementation supplies the results.

There are 104 initial successful outcomes, 81 controlled cancellations and 67
original empty-IN errors. An empty retained set or a slice beyond its retained IDs is not converted into
a fabricated successful empty page: the original SQL compiler's
`IN requires at least one value` failure is preserved. Retry results are recorded
separately, including when an error or cancellation changes later store calls.
No query-key, error or result normalization is used.

## C++ resource replacement and destruction

Rust borrows its Context immutably during the page future, so its safe setter
cannot replace that Context's store while the future is pending. C++ permits
explicit Context registration changes and has checked Context/object identities;
those ownership cases have a separate native contract rather than invented
original-Rust observations.

Three actual store wait positions each exercise four actions, for twelve cases:

- Replace registration while pending, resume the old request, then require a new
  request to select the replacement. The old request retains its checked store
  borrow; the replacement returns a distinguishable one-ID set.
- Replace registration and explicitly destroy the old store. Its pending access
  must reject the expired reference, cancel once and release its build lock.
- Explicitly cancel before destroying the old store, then reuse the replacement.
- Actually delete UserContext while the page is pending and require cancellation
  and lock cleanup exactly once.

The controls and captured observations have an explicit Global scope. Store and
operation state belong to UserContext; no shared owner retains a deleted store.
The default PMR resource is disabled. Existing checked-reference and cancellation
mechanics supply the rejection/cleanup, not a fixture-owned replacement algorithm.

An initial replacement assertion used an empty replacement set and incorrectly
expected successful zero-row pagination. The source comparison demonstrated the
actual empty-IN failure. The ownership fixture uses a valid first-page slice of a distinct set instead;
production behavior is unchanged.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_deferred_id_set_page_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --locked --bin deferred_id_set_page_5_0_6
TEAQL_LOG_ENDPOINT=off python3 tests/parity/generate_deferred_id_set_page_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/deferred_id_set_page_5_0_6 \
  --cpp build_native_current/teaql_deferred_id_set_page_probe --check
ctest --preset native --output-on-failure \
  -R '^deferred_id_set_page_(rust_parity|context_contracts)$'
```

Ordinary count-first deferred pages, continuous cursor replacement/interleaving,
other model/grouped/facet/dynamic shapes and protected-I/O/trace requirements
remain independent G09/G13/G17 obligations. This does not qualify every pagination
configuration or close P0-P7.

## Terminal verification

The 252-case corpus agrees and reproduces against live original/native observers.
All twelve native ownership cases pass. Native, targeted ASan/UBSan and imported
module selections each pass **2/2**, after successful terminal builds; sanitizer
leak scanning is disabled. Canonical consumer verification covers 159 bodies,
with 1,479 exports and seven acyclic module dependencies. Pinned format/check
covers 1,020 handwritten C++ inputs. See the
[terminal record](parity/deferred_id_set_page_5_0_6.json). No production function,
generator body or generated artifact changed; Java/generation reproduction and
MIPS/physical-device checks were not rerun.
