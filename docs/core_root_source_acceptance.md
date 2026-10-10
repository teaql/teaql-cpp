# Core root facade and source-test review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete 984-line core/src/lib.rs, all 26 embedded tests and their helper
fixtures were compared directly with the C++ root facade and ports on2026-10-09.
This source group remains partial pending current complete consumer validation.

## Production facade

All six public namespaces and 69 named root reexports were checked against
core.hpp. The final eval declaration after the test module is retained; the
export verifier scans the entire source. Core input EntityGraph/Builder/Node/
Operation remain distinct from the runtime identity graph through the core
namespace. The facade owns no data and adds no independent Default/Clone/Eq/
Hash/Debug/serde protocol; imported models retain their accepted source contracts.
Export-name checks are locators, not proof of member behavior.

## Complete original test responsibilities

Ten handwritten mapping tests preserve Order name/table/id/display_name/max100;
checked i32/u32 boundaries and Decimal precision19/scale7; missing/Null defaults
versus present invalid types; Product relation keys and attach/delete_missing
flags; boxed optional Product#7 Object roundtrip; ordered Order/OrderLine
registration; all five table-name checks; JSON/Date/UTC Timestamp variants;
dynamic I64/F64/Object JSON fields and BaseEntityData U64/I64/Json/F64 roundtrip.
Their handwritten model mappings express the locked derive output semantics.
The generator itself remains unchanged and outside the authorized scope.

Six expression tests retain property-to-property comparison, Soundex equality,
all six Java-style string match patterns, explicit large-IN operators and U64
lists, both 20/21 ordinary IN/NOT IN thresholds and complete subquery variant/
left/op/entity/projection assertions.

Five composition tests retain the complete ordered query builder chain, outer
Or, expression projection, descending/GBK sorting, grouping, three aggregates,
cache1000/propagation2000, HAVING, separate relation loads and page(offset20,
limit10). SmartList retains exact U64 IDs/signed versions and consuming records;
metadata, JSON, optional lookups, list/set/map/group helpers, replacement/appended
merge, retained IDs and borrowed iteration remain complete. SafeExpression
retains source entity7/version3/name, line11, missing index4, size1, null versus
present-empty fallback and branch-specific callback behavior.

Five presentation tests preserve original style/action order and requestURL;
single/list responses, result/status/version/count and complete failure wire;
all facet getters/mutators/removal/take and nested status/count payloads; Xls
navigation geometry/containment and properties; workbook page/block lookup,
ordered JSON and nested style reference. The response helper uses the original
Order7/version2/name, and facet record statusPENDING/count5.

The original exact Value variant assertions are retained before conversion;
whole AST/query/JSON comparisons retain source order and optional presence.
Supplementary ownership tests disable default PMR, deep-copy long handwritten
models/response/workbook payloads into Global and actually delete their source
User. Test expectations and handwritten mappings do not implement SQL, runtime
saving or the deferred generator.

## Validation boundary

The unchanged original core library and current sanitizer/MIPS selections are
being validated. Root-export verification passes six namespaces and69 names.
Whole native integration remains active. Source-group acceptance requires these
current results; broader runtime-root composition, privacy and final Stage6
remain separate. Physical-router execution is deferred by the user.

Fresh unchanged core library tests pass65/65, including all26 root cases.
Current ASan/UBSan and MIPS/QEMU source selections each pass4/4 after eight-task
builds finish, with post-test no-work and sanitizer leak scanning disabled.
Evidence is retained in measurements/parity/2026-10-09-core-root-source-review.
Whole native integration remains active; no complete source verdict is added.

## Complete current source acceptance, 2026-10-09

Final native integration passes 371/371 in 210.67 seconds after its 656-task
consumer rebuild, with 418 nonempty compiler dependency records and no-work
before/after testing. All corresponding fresh sanitizer/MIPS consumer selections
pass; complete source/body/helper/input/assertion or API/default/trait/protocol/
ownership review is recorded above. The reviewed source groups in this document
are accepted within their explicit C++ ownership and SQLite scope. This does
not accept broader Context/services, graph/event/log privacy, runtime-root or
complete stages 4–6. Current full output is retained in
measurements/parity/2026-10-09-checker-direct-callback-review.
