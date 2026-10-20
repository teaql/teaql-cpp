# Scalar, optional and temporal JSON query execution

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This supplements G10/G09/G13/G17 qualification. It does not complete their
capability or phase requirements.

## Actual generated and SQLite boundaries

The unchanged complete query-field-branches Rust domain and its generated C++
counterpart apply `filter_with_json` / `filterWithJson`, then execute through
their original runtime and actual SQLite provider. The existing SQL fixture
contains ordinary/nullable text, password, Boolean, Decimal, double, float,
integer, date and timestamp fields, two tenants and a soft-deleted row. Its hash
is recorded independently of successful generated schema/bootstrap acceptance.

The **5,088/5,088** exact matrix crosses twenty self/foreign-key/path names,
fourteen operand shapes, ordinary/minimal projections, rows/count/exists and
ordinary/descending/ascending-page variants. Extra inputs cover malformed and
non-object JSON, duplicate keys, unknown paths and paging-control precedence.
NULL, both null sentinels, mixed/empty arrays, numeric pairs, structured values,
Unicode with embedded zero and temporal text retain original behavior.

Results contain the complete constructed query, all eighteen scalar row slots,
scalar results, complete errors and ordered safe SQL records with bindings and
intent. Omitted row columns are distinguished from selected SQL NULL. This is
compact-row decoding evidence; it does not claim generated E, presentation JSON
or fully loaded typed entities for omitted fields.

The original observer records **4,722** successes and **366** condition panics.
The latter do not reach database execution. Successful empty results remain
successful results rather than being confused with those rejected conditions.
No result, error, parameter or query normalization is applied to this matrix.

Default PMR allocation is disabled. Native input/query/result values leave scope
before actual UserContext deletion; SQLite resources use registered cleanup and
no User bytes may remain. No runtime, SQL, provider or generator algorithm changes.

## Original dependency and retained-policy replay

Two independent observer manifests match the reference registry versions and
checksums and runtime package versions. The ordinary-domain observer is copied
byte-for-byte, with a checked hash, from the retained historical test source.
Historical original code, locks and corpora remain unchanged.

Replaying the existing policy matrix initially found **448** differences in the
stream-reuse observation. Rust passes `whose_id_is(2_u64)`; the C++ fixture passed
an `int` literal. The earlier source-aligned generic comparison adapter now
preserves that argument as I64 instead of performing the previous implicit
unsigned conversion. The fixture's explicit `std::uint64_t{2}` restores equivalent
stimuli. The old C++ body and every initial difference are archived; no production
conversion is changed and the original expected corpus is not rewritten.

| Actual original/native replay | Outcomes |
| --- | ---: |
| New scalar/optional/temporal SQLite matrix | 5,088 |
| Retained JSON translation/schema boundary | 1,181 |
| Retained one/stream execution and early close | 2,240 |
| Retained request-policy/intent/logging matrix | 3,072 |
| **Total** | **11,581** |

The three historical matrices retain their existing exact quoted-Line descriptor
Debug spelling adaptation. It changes no SQL, field identity, value or error.
The new scalar matrix needs no adaptation. The original schema failure remains
part of the retained boundary and is separately replayed.

## Reproduction and remaining scope

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
  teaql_generated_dynamic_json_scalar_probe teaql_generated_dynamic_json_probe
python3 scripts/verify_dynamic_json_protocols_5_0_6.py
ctest --preset native --output-on-failure \
  -R '^(generated_dynamic_json_scalar_rust_parity|dynamic_json_policy_rust_parity)$'
```

The helper waits for successful original builds, verifies both dependency locks
and exact copied observer bytes, then replays the immutable corpora. Profile and
ownership outcomes are in the
[terminal record](parity/dynamic_json_scalar_5_0_6_terminal.json).
Both the new scalar and retained policy checks pass in native, imported modules
and ASan/UBSan: **2/2** per profile, with sanitizer leak scanning disabled.
All **648** canonical consumer bodies and **1,485** module exports verify.
The source-only observer change adds no C++ runtime dependency and requires no
generator emission/Java refresh. Pinned formatting is **1,440** C++ files.

Constant/candidate/Business-ID combinations, additional inherited physical
execution, complete protected storage/policy/privacy and integration requirements
remain open. No full-suite, MIPS or physical-device result is inferred.
