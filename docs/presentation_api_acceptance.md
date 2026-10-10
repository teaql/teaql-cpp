# Core Web/Xls model traits and JSON acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
web.rs and xls.rs. These are core frontend/spreadsheet data models, not a web
framework, cloud adapter or spreadsheet runtime dependency.

## Production capabilities

presentation_debug.hpp supplies all seven actual source derived Debug types
through core.hpp. Complete fields and declaration order remain visible, including
private allocation-domain adaptations, optional strings/boxed styles, recursive
JSON maps and duplicate ordered pages/blocks. Allocation-domain pointers are not
source fields and do not appear in diagnostics or equality.

| Source type | Preserved model/trait contract |
| --- | --- |
| WebStyle | Three optional strings; Default/Clone/PartialEq/Eq, raw Debug and camelCase JSON with None fields omitted. |
| WebAction | All nine optional strings; Default/Clone/PartialEq/Eq and raw Debug. request_url retains the special requestURL wire spelling, independently of its diagnostic label. |
| WebResponse | All seven fields; Clone/PartialEq, raw Debug and complete JSON. Success/failure/empty/count/facet responsibilities remain distinct. No source Default or Eq is invented. |
| XlsBlock | Eight fields in original page/top/bottom/left/right/style/value/properties order; recursive boxed style, Clone/PartialEq and raw Debug. Null value and empty properties are skipped only where the source serde derive specifies. |
| XlsBlockBuildContext | page/start_x/x/y; Clone/PartialEq/Eq, raw Debug and camelCase wire labels. Constructor coordinate clamping is separate from raw deserialization, which preserves negative coordinates. |
| XlsPage | name/ordered blocks; Default/Clone/PartialEq, raw Debug and JSON. Overlapping blocks retain first-match lookup. |
| XlsWorkbook | ordered pages; Default/Clone/PartialEq, raw Debug and JSON. Duplicate page names retain first-match lookup. |

Nine formatter modes propagate through nested models/JSON: compact/pretty,
lower/upper hex, pretty hex, width/precision, Unicode fill/alignment, explicit
sign and zero padding. Raw strings/properties are not safe audit/log projections.
The wire decoder keeps its existing structured ModelDecodeError code/static field
adaptation. This audit observes protocol acceptance and outcomes, not generic
serde's external diagnostic wording or duplicate object-key behavior after a JSON
value parser has already collapsed keys.

## Generic projection repair

Original WebResponse::from_records/from_entities accept arbitrary owning input
iterators. The C++ API previously exposed only span/Vector entry points. New
input-range overloads project non-contiguous records and owning arrays of
non-copyable entities. Borrowed ranges explicitly copy when possible and reject
non-copyable elements before consumption. Existing owning Vector and SmartList
paths remain available. Source from_entity requires Clone; its C++ declaration
now reflects the allocator-aware copy capability rather than advertising an
unusable non-copyable overload.

Projection writes every recursive JSON value into the explicit destination.
No reference retains the source arena and no process/default resource owns
Context data. This adds no runtime dependency, generator change or new lifetime.

## Original-call evidence

The actual original oracle records 524 inputs and 8,127 exact formatted outputs.
It observes every source type's derived Debug/Clone/equality, full serialized
values and serialization-then-deserialization acceptance/equality. Missing,
explicit-null, wrong-type and integer-range fields, unknown fields, struct
sequences, defaulted properties and recursive styles/pages are tested. Serialized
null Xls values may not deserialize when the source requires the omitted field;
the oracle preserves actual source roundtrip behavior instead of inventing a
universal roundtrip guarantee.

The earlier 452-input corpus matches the original source. All 524 expanded inputs
match the repaired native probe, including the 72 new binding sequences. Twelve additional
range inputs call original owning arrays/linked records and non-Clone entity
SmartLists, comparing complete responses. Constructor/navigation snapshots cover
all seven model shapes, repeated page/block additions and first/missing lookups.
The older 785-input presentation corpus continues to cover action factories and
JSON protocol behavior; it is not replaced by this new matrix.

The unchanged original Xls navigation unit test passes 1/1 and its exact inputs
and assertions are ported in presentation_api_tests.cpp. Native contracts pass
with default PMR disabled, honest borrowed Clone constraints and consumed
non-copyable array entities. They actually delete User before rebinding retained
Global responses, recursive workbooks/boxed styles/JSON/map keys and raw output.
Formatting publishes no new object identities; a later User reproduces the full
pretty output. Foreign projection inputs are copied into the destination explicitly.

## Integrated acceptance

Five root lib.rs tests now have individually reviewed dispositions against
original_navigation_contract, original_web_contracts and
original_workbook_contract. The review retains exact source colors/actions,
Order values, response counts/messages, facet removal/take, coordinates, spans,
containment and complete nested workbook wire assertions.

The web fixture now keeps the original plain loaded list separately from the
facet list, sets the original loaded states, checks the mutable facet map, and
validates facets through the actual serialized response object. An internal
response.facets check alone would miss a serialization omission. Source empty
facet fixtures remain unloaded, as SmartList::empty specifies.

After these assertion/input repairs, the rebuilt presentation and new entity-
mapping selection passes 2/2 on native, ASan/UBSan with leak detection and MIPS/
QEMU. Unchanged Rust passes its 65 core tests. This subsequent scoped result
does not claim another full-platform regression or complete stage acceptance.

presentation_api_contracts/presentation_api_rust_parity are registered. The full
native build completed and all 236 CTest checks passed. Fresh ASan/UBSan with
leak detection and OpenWrt MIPS/QEMU builds each passed ten selected checks covering
new and existing presentation protocols, entity/collection/model ownership and
Context references. Fresh live original-source comparisons matched all 452 earlier inputs
on native and MIPS. The expanded binding acceptance is recorded below. The older presentation CTest also forwards the cross emulator.
The previous completed 234-check checkpoint predates these public header repairs.

Complete model API/default/source-test audits, generic serializer error messages,
all-stage runtime/privacy/provenance acceptance and physical-device execution
remain separate gates. Never equate core presentation models with a web integration.

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust_values/Cargo.toml \
  --bin presentation_api_oracle --locked --offline
python3 tests/parity/generate_presentation_api.py \
  --rust /home/philip/shared-cargo-target/debug/presentation_api_oracle
python3 tests/parity/compare_presentation_api.py --cpp <probe> \
  --rust /home/philip/shared-cargo-target/debug/presentation_api_oracle
```

For MIPS add --cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'.

## Entity binding and original source tests

Source WebStyle::bind_entity calls the entity's public put_dynamic method. C++
previously bypassed an override and wrote directly to BaseEntityData. The new
72 original-call binding sequences include ordinary, redirected and suppressed
style writes, absent/existing values, null/scalar/array/object JSON and repeated
style/action binding. Before the production repair, only 452/524 inputs matched.
After repair the native probe matches 524/524. Models inheriting BaseEntityDefaults
dispatch their override; handwritten models without that helper use the source
base default. WebAction intentionally retains direct base mutation, as in Rust.

Native ownership contracts bind an 8192-byte User style to a Global entity,
actually delete User and validate Global JSON and override metadata through a
new User. Suppressed styles do not acquire a dynamic field; actions do not invoke
the style override. No source data is retained outside its declared owner.

The unchanged pinned source passes four web tests and two XLS tests using
build/rust-original-roundtrip as an isolated Cargo target. A shared target run
failed due to a stale derive artifact with a different Entity method contract;
the isolated target passes without changing source or generator. The C++ public
contract includes handwritten entity mappings and original constructor/binding/
response/facet/navigation/workbook inputs. The final native source-port contract passes. Fresh rebuilt MIPS acceptance
passes all ten selected checks and all 524 live original comparisons. The rebuilt ASan/UBSan selection with leak detection also passes 10/10.
The complete native Debug rebuild and regression pass 236/236; the final
native probe again matches all 524 live original comparisons.


## Complete source-surface review, 2026-10-07

The full production bodies of serde_utils.rs, web.rs and xls.rs have now been
reviewed against their concrete C++ implementations, independently of source-test
names. This review maps source serde to the project's native JSON-value codecs
and source ownership to explicit Context domains; it does not introduce a web
server or spreadsheet engine. Native structured decode errors remain an explicit
adaptation, not identical external-serde error strings or non-JSON formats.

| Source responsibility | Complete mapped surface and evidence |
| --- | --- |
| trimmed_string / trimmed_opt_string | All four serialize/deserialize functions. The actual source matrix covers every one of the 25 Unicode White_Space codepoints, nine nearby non-whitespace/control boundaries, both ends and interior positions, unchanged input, long Unicode-wrapped strings, required/optional null distinctions and wrong JSON kinds. Some whitespace-only becomes Some(empty), never None. |
| Constants and WebStyle | style/actionList/1.001 constants; three optional fields, new/default and all three static factories/setters; complete JSON and base/entity/record binding. Field/method collisions use explicit C++ set_ names. Native binding plus 72 original sequences prove override dispatch for Style. |
| WebAction | Nine fields, new/default, fifteen static action factories including common actions, nine setters and optional warning setter, JSON and three bindings. Exact values/order/requestURL spelling and absent versus Some(empty) warnings match source. Action binding intentionally mutates the base rather than calling the style override. |
| WebResponse | Seven fields; success/fail/empty_list; from_records/from_entity/from_entities/from_smart_list; data/count/facet/optional-facet/push builders; complete JSON. Single entity requires Clone; owning entity iteration does not. Source totals and taken facet projection remain distinct from data length and explicit count overrides. |
| XlsBlock | Eight fields; new/from_context, region/span/value/property/style builders, set_property, width/height/contains and JSON. Span uses source saturating_sub(1), while remaining checked coordinate overflow maps a source arithmetic panic to a C++ exception. Recursive styles are explicit owner-local boxes. |
| XlsBlockBuildContext | Four fields; new/page, next/new_line/next_line/to_block. Constructor clamps negatives; raw serde preserves them. new_line resets x to zero; next_line restores start_x. The C++ page factory is for_page because the field retains its name. |
| XlsPage / XlsWorkbook | All fields; new/default, add/push block or page, first matching block/page lookup, workbook JSON. Ordered duplicates/overlap, absence and complete nested wire values are checked. |
| Derived traits | Source Clone/PartialEq/Eq/Default/Debug capabilities map individually to scoped copies, structural equality, explicit-owner constructors and public Debug writers. WebResponse and JSON-bearing XLS types do not acquire an invented Rust Eq or Default contract. Optional strings, boxed style, JSON/maps, sequence order, raw deserialization and required/default/omitted fields retain the actual source protocol. |
| Ownership | Default PMR is disabled in native ownership fixtures. Explicit Global copies retain models, nested JSON/map keys, styles, responses and Debug output after actual source User deletion. Raw pointer/getter borrows remain temporary; serialization/projection does not extend source lifetimes. |

### Owning iterator repair

The additional review found an actual generic omission: a named move-iterator
subrange was classified as borrowed merely because the range was an lvalue.
The minimal noncopyable entity reproduction returned `Borrowed entity projection
requires an allocator-aware copy`, despite the iterator yielding owned E&&.
A named transform view yielding E by value had the same problem. Source
WebResponse::from_entities accepts both corresponding owning iterators without
Clone. C++ now determines consumption from the iterator's reference category
as well as ownership of the container. Borrowed E& ranges retain their explicit
copy requirement; genuine owning references/values are consumed.

Six actual original-call inputs exercise both named modes with null, recursive
JSON and 4KiB payloads. The expanded corpus has **675** inputs: 524 earlier
model/binding/range cases, 145 trimming cases and six owning-iterator cases.
Native and MIPS each match all 675 live Rust outputs. The independent older
785-input protocol corpus remains matching on both. Unchanged original core
passes 65/65. Rebuilt five-check selections pass ASan/UBSan with leak detection
5/5 (4.62 seconds) and MIPS/QEMU 5/5 (3.38 seconds).

The current CMake native probe also matches all 675 live observations; an isolated
native probe linked against the current archives supplied earlier evidence while
the consumer rebuild was active. The full native integration and subsequent
header-refresh build completed; the current Debug regression passes 279/279 in
67.99 seconds. Ninja reports no pending work. The five native presentation checks
pass again after integration. The three-file responsibility group is accepted
on the complete source review and the scoped evidence above. Existing historical
totals describe their own checkpoints. Other module and stage acceptance remains
open.
