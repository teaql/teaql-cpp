# Complete trimming-helper source review: Rust 5.0.6

Status: complete `teaql-core/src/serde_utils.rs` accepted under SU01-SU06 and
the existing C++ JSON/destination-allocator adaptation. Native **3/3** and
imported modules **1/1** pass. Both frontends match all **675** retained complete
presentation observations, including **145** trimming cases. All **four**
original trimming unit tests pass against the locked source and dependencies.
Source dispositions advance to **22 accepted / 100 pending / 2 excluded_external**.
This accepts the 118-line helper file, not the adjacent Web/Xls sources or P0-P7.

## Complete source responsibilities

| ID | Original responsibility | C++ mapping and evidence |
| --- | --- | --- |
| SU01 | Required serialization trims a borrowed string without modifying it and delegates string serialization | `trimmed_string::serialize(destination, value)` trims a local view and creates destination-owned Json. The retained corpus compares the original value and serialized result, including every Unicode whitespace class and adjacent non-whitespace controls. |
| SU02 | Required deserialization requires a string and returns owned trimmed text | `trimmed_string::deserialize(destination, value)` accepts only the Json string alternative and creates a new destination String. Every other JSON kind rejects; NULL does not become an empty string. |
| SU03 | Optional serialization distinguishes None from Some, including Some(empty) | `trimmed_opt_string::serialize` maps absence to JSON null and delegates present text to the required helper. An all-whitespace present value remains a present empty string. |
| SU04 | Optional deserialization preserves None, trims Some and rejects wrong kinds | `trimmed_opt_string::deserialize` returns an empty optional only for JSON null. Whitespace-only input produces Some(empty); wrong non-string alternatives return a typed failure. |
| SU05 | Owned output and non-mutating input across the four helper roles | Every C++ result/string/Json uses the explicit destination allocator. The native value-model contract disables default PMR and verifies an explicit Global copy of optional decoded text after closing its source User arena. The immutable input is retained in complete comparisons. |
| SU06 | All four original source tests, including their complete two-field objects and None/empty distinctions | The actual original unit-test selection passes4/4. The existing unchanged C++ functional contract calls all four direct ports and checks both object fields. The imported presentation observer calls all four public helpers and matches the complete retained corpus. |

The required/optional namespaces, serialization/deserialization roles and four
distinct entry points remain intact. No trimming behavior is implemented in the
observer or application examples.

## Source, dependency and comparison provenance

The complete helper file is byte-identical between the previous and current
locked runtime commits. This is continuity evidence, not the sole acceptance
basis. The current gates execute the actual four original tests and the unchanged
original presentation observer against 5.0.6. The observer's registry versions
and checksums are a subset of the exact reference lock.

All 675 inputs and expected results are reused unchanged. Both gates retain full
original/C++ stdout and stderr, compare complete outputs without normalization,
and use LF framing to preserve U+0085/U+2028/U+2029 inside JSON strings. The 145
trimming inputs cover whitespace and non-whitespace boundaries, NULL and invalid
JSON kinds. Successful serialization retains the original input alongside both
helper results, so accepting the right output after mutating input cannot pass.

The other 530 observations provide unchanged presentation regressions. They do
not accept every responsibility of the other source files merely because their
outputs appear in this corpus.

## Existing language adaptation

Rust supplies generic Serde Serializer/Deserializer traits and their error type.
The existing C++ interface supplies destination-owned Json and distinct typed
ExpectedString/ExpectedOptionalString failures. This retains the helper roles
and JSON success/failure/value semantics without adding a serialization library.
Third-party Serde error text and non-JSON formats are outside this existing
adapter contract; this review does not claim byte-equal Serde diagnostics or a
new general-purpose serializer framework. The previous explicit three-file
adaptation and responsibility text is retained in
[the prior-disposition archive](parity/serde_source_previous_disposition_5_0_6.json.gz).

C++ Context ownership replaces Rust's ordinary owned String allocation. Nested
optional/result payloads propagate the destination allocator, and cross-domain
retention needs an explicit copy. The native ownership test reads its retained
Global string after the source arena closes; it does not dereference an expired
User borrow. No helper stores input pointers, extends Context lifetime or owns
data through a hidden default resource.

## Terminal gates and scope

The [native receipt](parity/serde_source_native_5_0_6.json.gz) includes the complete
presentation comparison, value-model ownership contracts and all seventeen
original core functional ports, including the four trimming tests. The
[module receipt](parity/serde_source_module_5_0_6.json.gz) compiles the canonical
imported presentation observer and replays its complete matrix. Both rebuild
their selected consumers before execution and finish with no-work builds.
The [acceptance receipt](parity/serde_source_acceptance_5_0_6.json.gz) binds their
current inputs and source review.

Reproduce with locked tools and fresh receipt paths:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_serde_source_5_0_6.py \
    --profile native --record /tmp/serde_source_native.json.gz
python3 scripts/verify_serde_source_5_0_6.py \
    --profile modules_native --record /tmp/serde_source_module.json.gz
```

No C++ body, generated SDK, dependency, CMake registration or compiler-contract
input changed. Existing source/ownership tests provide the relevant checks;
unchanged memory mechanics require no new sanitizer or MIPS run. Native registry
736, imported-body875 and export1506 counts remain structural observations.
The separate full generated compiler-contract process remains active. G13/G17,
all other pending files and complete P0-P7 acceptance remain open.
