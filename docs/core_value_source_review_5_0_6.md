# Core Value source review: Rust 5.0.6

Current source disposition: **accepted** at the [installed cohort checkpoint](source_cohort_acceptance_5_0_6.md). Earlier staged/pending paragraphs retain their historical scope.

Status: complete-body pre-acceptance review. `teaql-core/src/value.rs` remains
`pending_review`; permanent module/reproduction and complete native integration
remain required. Broader Decimal reexport, JSON/runtime boundaries and complete
G13/P0-P7 acceptance retain their own requirements.

The reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The entire
463-line source, including all thirteen tests, is byte-identical to the retained
exact 5.0.5 reference. The C++ Value declarations, constructors/extractors/JSON
implementation, integer helpers, standard-clock bridge and complete source test
port have been read. Historical [API acceptance](value_api_acceptance.md) and
[owned-input acceptance](value_owned_api_acceptance.md) remain separate records.

## Complete responsibility mapping

| ID | Original responsibility | C++ mapping and required observation |
| --- | --- | --- |
| VL01 | Ten DataTypes | Preserve Bool/I64/U64/F64/Decimal/Text/LargeText/Json/Date/Timestamp and their Debug/Clone/Copy/Eq responsibilities. Physical enum layout is not a declared wire protocol. |
| VL02 | Thirteen Value variants | Preserve Null, Bool, I64, U64, F64, Decimal, Text, Json, Date, Timestamp, Object, List and TypedNull as distinct alternatives. C++ variant order is not the Rust discriminant. |
| VL03 | Traits and absence of invented protocols | Value has Debug/Clone/PartialEq, without source Default/serde/Hash/Ord derives. An explicit-owner C++ null construction is not an implicit allocator or a new source serde contract. |
| VL04 | Borrowed/owned string conversions | Borrowed text is copied. Same-owner owning text transfers its heap storage; foreign retention prepares an explicit destination copy. No universal short-string pointer identity is promised. |
| VL05 | Six integer From implementations | Exercise actual i16/i32/i64/u16/u32/u64 argument types and preserve signed versus unsigned variants. Native integral convenience overloads do not imply additional Rust From implementations. |
| VL06 | Float32/float64 From | Preserve widening and finite IEEE results, signed zero, infinity and NaN observations under the existing width-changing-NaN disposition. Do not turn a present nonfinite floating result into a missing extractor. |
| VL07 | Other scalar/container From | Preserve Bool, Decimal, serde JSON, date and timestamp construction. JSON remains a distinct Value alternative; Object/List ownership remains explicit. |
| VL08 | DateTime<Utc> From | Map actual Unix UTC instants through value_from_utc. Native supported integral duration units floor negative sub-milliseconds and reject seconds multiplication overflow. Native carrier range extensions do not redefine Chrono's valid date range. |
| VL09 | Object construction | Preserve the complete supplied ordered string/Value map and nested payloads; do not reinterpret it as an authoritative typed entity. |
| VL10 | try_i64 | Accept I64, representable U64 and the Decimal conversion; reject float/text/Boolean/NULL and unsigned overflow. |
| VL11 | try_u64 | Accept U64, nonnegative I64 and source-compatible Decimal conversion; reject negatives and unrelated variants rather than using a generic numeric cast. |
| VL12 | try_decimal | Preserve decimal/integer/text conversion and finite floating human-decimal conversion. The 129.95 case must not use the retained binary expansion; invalid text/nonfinite/unrelated inputs remain absent. |
| VL13 | try_f64 | Preserve supported integer/decimal conversions and the original F64, including nonfinite presence. Exact finite bit patterns remain observable. |
| VL14 | try_text/try_bool | Accept only their actual matching variants; do not add string/number/Boolean coercion. Text results remain borrows. |
| VL15 | try_date | Preserve date/text/calendar parsing and checked integer epoch conversion with negative-day flooring. A Timestamp Value or Decimal does not become a date merely because it has a numeric representation. |
| VL16 | try_timestamp | Preserve timestamp, RFC3339/naive/date-text fallbacks and checked integer conversion. U64 overflow remains absent, separately from Timestamp's wrapping unsigned constructor. |
| VL17 | to_json_value | Preserve primitive projections, nonfinite-to-NULL, exact Decimal/date text, timestamp integer, recursive Object/List and cloned JSON. All TypedNull types project NULL without losing their original variant identity. |
| VL18 | Clone and PartialEq | Deep destination copies retain complete recursive state. Comparison preserves variant identity and NaN non-reflexivity; it is not SQL numeric comparison or a total ordering. |
| VL19 | Owning storage paths | Same-domain list/object moves retain buffers/nodes and nested strings. Foreign moves copy without consuming the existing source; failed preparation preserves source/destination and returns blocks on teardown. This is the explicit C++ ownership adaptation. |
| VL20 | Decimal reexport | Preserve the existing public Decimal type and its separate complete API/default/protocol responsibility. This Value conversion replay does not replace the full Decimal reexport audit. |
| VL21 | Thirteen original tests | Preserve all original successful/rejected scalar, Decimal, date and timestamp assertions, including integer bounds, 136.25/129.95, leap day, UTC offsets and malformed input. The complete C++ port additionally covers all ten TypedNull types. |

JSON projection is presentation data. It does not install trusted metadata,
authorize persistence or replace model-bound hydration. Raw Value/DataType
diagnostics likewise do not establish a safe logging projection. Custom raw
pointers and invalid use of expired C++ borrows remain outside the ownership
guarantees of these constructors.

## Current exact-reference replay

The [terminal source/import receipt](parity/value_source_import_5_0_6.json.gz)
is complete. All **thirteen original source tests pass** in the actual locked
core crate. The independent observer checks all **87 registry identities**
against that crate's exact Cargo.lock before executing unchanged drivers.

| Actual original/imported family | Inputs |
| --- | ---: |
| Complete Value API, constructors and pairwise comparison results | 115 |
| Owning text/list/object construction | 171 |
| Scalar extraction/projection and retained surrounding outputs | 722 |
| UTC standard-clock conversion | 64 |
| DataType raw diagnostics | 10 |
| **Exact paired observations** | **1,082** |

Complete inputs, outputs and active corpora are preserved. No error or result
field is removed to isolate successful cases. The API family retains its
13,225 pairwise equality results within those observations. Existing floating
fixtures classify NaNs only across float32-to-float64 width changes; finite
values, infinities and unchanged-width observations keep their existing exact
checks. This checkpoint is native-64-bit evidence, not new MIPS qualification.

Four unchanged C++ unit programs pass through imports: Value/model lifetime,
owning constructors, the complete original source port and UTC conversion.
Their null-default-PMR controls cover arena growth, recursive destination
allocators, same-domain storage transfer, failed foreign copy/assignment without
source loss, actual User destruction, retained Global payloads and all-block
return. Supplementary JSON/trimmed-string/Timestamp checks in the same bodies
remain guards, not acceptance of their entire independent source modules.

All nine probe/unit programs compile and link before execution. **Eleven
canonical entry/helper bodies** retain the handwritten sources and assertions,
differing only in include prologues/paths. Module archive/BMI fingerprints and
all **10,653 frozen native inputs** remain unchanged after the gate. Production
functions, producer, generated SDKs and both references are untouched.

## Remaining integration boundary

Temporary manifests, canonical mirrors and compiled consumers live under
`/tmp/teaql_value_source_review_5_0_6`. The compressed receipt preserves the
controller, sources/fixtures, manifest/lock and successful terminal commands.
No running native compiler/linker output is executed for this supplemental gate.

After the frozen native epoch ends, install the missing permanent Value/API/
owning/UTC/DataType module consumers and comparison entries and provide
repository-owned reproduction. Full native/source acceptance and remaining
reexport/dependency obligations must pass before promoting this file. This
review changes no source disposition or capability verdict and claims no new
sanitizer, MIPS, physical-device or external integration result.
