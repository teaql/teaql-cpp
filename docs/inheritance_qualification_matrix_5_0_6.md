# Inheritance qualification matrix: Rust 5.0.6

Status: P2/G04 qualified with the language/ownership adaptations in
[the acceptance record](inheritance_acceptance_5_0_6.md). The default inheritance
guard is removed. This checklist retains every requirement; passing one row
does not waive another. Runtime and producer references remain those in the active locks.

| Source responsibility | Current evidence | Remaining gate |
| --- | --- | --- |
| Flatten ancestor fields while retaining distinct model/table identities | 48 field/layout/descriptor observations; complete original generated crates; compatible name redeclaration in variants; 36 exact original/C++ parent metadata observations | Preserve fixed-layout API validation; this does not qualify malformed KSML frontend failures |
| Direct-child filters include descendants; arbitrary type matching and type grouping | 1,800 initial query observations and SQLite unknown-column failures | Preserve the SQLite failures; do not manufacture discriminator columns absent from source |
| Recursive child enhancement and normal/minimal/with-children defaults | 144 initial SQLite observations; variants add 120 constructions and 120 reads with sibling descendants, a fieldless intermediate and a third level | Preserve these source gates when extending hierarchy handling |
| Inherited scalar/foreign projections and relation shortcuts | 1,944 query-construction observations and 216 additional SQLite shortcut observations; ordered duplicates, alias semantics and separate child enhancement | Preserve these source gates when expanding other query APIs |
| Returned type controls decoding/save identity; source controls query/creation | 702 list/one/stream source/result observations, 1,350 save observations | Preserve sparse decoding, conflict/readback and source-model creation gates |
| Forward/nested and reverse-member ledger graphs | 2,313 actual observations; member commits, soft deletion, failures, rollback, retry and exact edge views after identity reload | Preserve these gates; broader aggregate/constant/readonly configurations retain their separate capabilities |
| Context lifetime and allocator ownership | Explicit User/Global copies, actual Context deletion, expired Q/E/list-member borrows, default PMR disabled | Re-run targeted lifetime checks when runtime graph ownership/installation changes |
| Inherited typed Checker/fix responsibilities | Variants add 135 audited saves with inherited optional/boolean/clock fields; an explicitly adapted source fixture executes five-model helpers and matches 60 SQLite creations plus 20 update/retry scenarios; native/module each 3/3 and ownership ASan/UBSan 1/1 pass; default generation/Assist and reproduction pass | Preserve the documented producer binding defects and qualified adaptation; do not claim untouched Rust artifact compilation |
| Stream boundary for inherited models | 432 original/C++ plain stream observations, including no polling, early cancellation, whole-chunk decode failure, exhaustion and immediate executor reuse | Preserve these cases and the documented C++ startup/error-delivery timing adaptation |

The checklist distinguishes shared generator gaps from inheritance-specific
responsibilities. Dynamic readonly-property hooks, full query facets/aggregates,
automatic schema/bootstrap assembly, constants/candidates and complete Assist
remain their existing G05/G08/G12/G16 responsibilities. They do not disappear
when P2 advances, and full P0-P7 completion still requires every capability.

## Execution order

1. Completed: qualify the four relation/projection shortcuts against actual original
   query construction and SQLite views; repair shared runtime behavior first if
   those observations expose a discrepancy. The exact-edge hydration discrepancy
   is repaired and recorded in generated_projection_shortcuts_5_0_6.md.
2. Add the branched/empty-intermediate and inherited scalar/Business-ID/clock
   fixtures, preserving unmodified original producer output and exact metadata.
3. Completed within the recorded scope: compare fixed-layout parent-input
   validation and plain inherited stream cleanup outcomes. See
   [generated_inherited_streams_5_0_6.md](generated_inherited_streams_5_0_6.md).
   Full malformed KSML frontend failures are not implied by the metadata API probe.
4. Review every row and its native/compiler/generator/reproduction/module/lifetime
   evidence. Remove the guard only when the evidence satisfies these conditions;
   update G04/P2 status separately from full generator completion.

The earlier blanket wording "remaining model configurations" is superseded by
these named gates. Additional concrete gaps found while executing them must be
recorded and addressed without narrowing the original objective.

The [inherited Business-ID binding experiment](inherited_business_binding_bridge_5_0_6.md)
retains the original failure and makes its source adaptations inspectable. Its
passing source execution alone does not remove the default inheritance guard.
