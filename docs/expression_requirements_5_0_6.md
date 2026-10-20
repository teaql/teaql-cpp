# G11 complete E contract requirements: Rust 5.0.6

Status: all eleven requirements are qualified with documented C++ ownership and presentation adaptations; see [the acceptance review](expression_acceptance_5_0_6.md). These requirements follow
all eleven named templates in the locked expression/index.stg and their common
ValueExpression/diagnostic bodies. Acceptance requires every row; a fixture count
or successful source inventory alone does not qualify a row.

| Requirement | Source responsibility and acceptance evidence |
| --- | --- |
| Root entry, identity and immutable provenance | eMethod captures the entity type and ID once. Clones and traversal retain that description. Verify original generated entry calls, later ledger changes, runtime root capture and explicit copied snapshots. |
| Scalar resolution and NULL fallback | index defines ValueExpression new/resolve/eval/unwrap, eager/lazy/default fallback and the T: Default constraint. Verify Value/NULL/NotLoaded for all supported scalar carrier branches and typed IDs; missing-load fallback callbacks remain uncalled. |
| Scalar and object-key access | scalarAccessor/objectStorageAccessor delegate to entity eval methods over source-selected scalar/FK types. Verify mandatory/optional, JSON null, finite/non-finite numeric inputs, type errors, availability and strict field decoding with unchanged original E bodies. |
| Forward objects | objectRelationAccessor preserves entity eval and parent root description; no database query occurs. Verify omitted, known empty, present and loaded-child missing states. |
| Reverse to-one | oneToOneReverseRelationAccessor delegates selected relation availability. Verify Value/Empty/NotLoaded and missing scalar traversal after a selected relation. |
| Reverse collections | oneToManyReverseRelationAccessor/listExpression expose eval/unwrap/size/first/get. Verify selected empty lists, stored availability flags, indexed/out-of-range NULL, missing views and checked child binding. |
| Constant predicates | constantObjectExpressionAccessor/constantObjectExpressionMethod preserve typed FK predicates and loaded-field checks. Verify candidate, constant NULL/unknown and original invalid-binding dispositions; do not invent APIs for broken original models. |
| Inherited and configured model surfaces | The same entry/accessor branches apply to flattened inherited fields and mapped configured scalar declarations. Verify inherited root/field diagnostics, typed Q/E readback and preserved unmapped/opaque source-failure dispositions. |
| Complete structured diagnostics and text | trigger_logic_bug_panic supplies root, original expression/broken marks, path, breakpoint, missing preload, leaf/nested fixes, severity and human text. Verify empty/repeated/Unicode/control/quoted/delimiter-injection paths and full original raw frames, with explicit C++ naming/language adaptations and negative adapter controls. |
| C++ ownership and composition | Context-owned state, checked entity/list borrows, eager scalar copies, move/relocation rejection, actual User deletion, allocator propagation and explicit destination retention preserve the agreed two-scope adaptation. Verify no implicit SQL, no owner extending Context and safe static exception what(). |
| Usable model-aware guidance and final gates | Generated Assist must explain source leaf-only paths, diagnostic pseudocode, typed repair selection, borrow/retention rules and default constraints. Verify real positive/negative compiler use, current source replays, relevant native/module/lifetime gates and complete byte reproduction. |

## Preserved phase boundaries

The [pre-acceptance archive](parity/expression_pre_acceptance_5_0_6.json) retains the
complete earlier capability entry and its open question verbatim. G11 owns the
expression and diagnostic portions of that question. Runtime assembly, sample
data and general extension/Assist workflows belong to G12 and the full P6 exit
criterion; G13 owns complete model/provider integration and P7. Those obligations
remain required even after E acceptance. No automatic executable query repair or
implicit loading is added to the original renderer contract.
