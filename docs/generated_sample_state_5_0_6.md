# Sample-data runtime foundation against Rust 5.0.6

Status: partial G12/P6 implementation. This checkpoint provides sample-data
state and report contracts; it does not implement or advertise complete generated
dataset execution. The later shared async execution and generated recipe checkpoint
is documented in [generated_sample_execution_5_0_6.md](generated_sample_execution_5_0_6.md).

Runtime reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
The source is the frozen `rust-lib-core/sample_data/index.stg` and the unchanged
retained checkers fixture's generated `sample_data.rs`.

## Implemented shared runtime contracts

`generated_sample_data.hpp` preserves distinct SampleDataScale, SampleDataPlan,
SampleDataState, SampleDataReport, SampleDataSkipped and SampleDataError types.
The small plan selects Small scale and seed zero. Plan/scale are ordinary values;
managed states/reports/errors are allocated explicitly in GlobalContext or
UserContext. Nested reference vectors, map keys and skipped/error strings use the
owner's allocator. Cross-domain retention requires an explicit destination copy.

Reference selection follows the source exactly: retain insertion order and
duplicates; select `salt % pool.size()`; if that ID is already used, scan from the
start for an unused ID; if every ID is used, reuse the preferred ID. An absent or
empty pool returns no ID. Zero is a valid stored ID. Keys are case-sensitive and
are not silently normalized. Seed does not affect these reference helpers.

Generated counts and skipped records are distinct from references. Turning state
into a report transfers counts/skips and consumes the reference pool. Error
messages are owned in their explicit Context and render by copying to a requested
destination. Count overflow rejects rather than silently wrapping.

The source IntoU64 responsibility is represented by `sample_reference_id`
overloads for typed IDs and optional Value pointers. Missing/unconvertible values
produce zero through the existing core Value conversion contract.

## Source evidence and ownership checks

The Rust probe executes actual original SampleDataState methods. **144** original
state/report observations include **2,304** reference choices, all three scales,
zero/ordinary/maximal seeds, empty/single/duplicate pools, maximum unsigned IDs,
case-sensitive/missing keys, partial/exhausted used sets, repeated generated
counts, skipped records and eight optional/typed Value conversions. No outputs
are normalized. The original model and every retained generated file hash are
verified before comparison.

The native ownership test sets the default PMR resource to null, inspects nested
allocators, deep-copies state/report/error into Global, mutates the originals,
actually deletes UserContext and checks independent retained values. The old
managed reference is rejected. A synthetic maximum count verifies overflow
rejection. These tests qualify the C++ ownership adaptation, not SQL execution.

Completed gates: native **2/2**, selected ASan/UBSan **2/2** with leak scanning
disabled, module isolated import **1/1**, **144/144** live original/C++ state
observations, and pinned formatting of **933 handwritten C++ files**. Module
exports/path ownership, source lock and whitespace checks pass. No generator
artifacts or third-party dependencies are added by this checkpoint.

## Remaining generated execution contract

At this foundation checkpoint, the Source sample generator was missing from C++
output. The later execution checkpoint implements shared async orchestration and
thin recipes; full configuration qualification remains open. The source contracts
that guided that implementation are:

1. Sort non-view/non-value-object models by depth then name, preserving source
   root/constant/business classification. Load root references first, then
   constants; source load failures default to an empty list.
2. Check required object-field reference pools before generating a model. Record
   the exact source skip reason and stop that model when a dependency is absent.
3. Preserve fanout: `max(1, object_field_count) * 20`, multiplied by 1/5/50 for
   Tiny/Small/Medium. Choose reference IDs with the already qualified helper.
4. Fill model fields using source sample defaults, numeric seed arithmetic and
   temporal rules. Business IDs remain assigned by the existing Checker/Fix.
   Source date/time generation uses the wall clock; seed alone does not make
   temporal output deterministic. Qualify that behavior explicitly.
5. Construct through Context-bound generated Q/newEntity and save through the
   existing audited path with `Init Sample Data`. Record counts only after a
   successful save, and add generated references only under the source rule.
   Preserve pending/wake/cancel and error ordering without duplicating governance.
6. Emit the supported generated sample API and accurate Assist, regenerate and
   compare actual SQLite rows/report/errors against unchanged producer calls.

The application API must retain UserContext as its execution dependency; Global
allocation remains accessed through that context. Required internal root-field
assignment must preserve the existing application visibility boundary. This
checkpoint does not remove model configuration guards or qualify G12/P6 overall.
