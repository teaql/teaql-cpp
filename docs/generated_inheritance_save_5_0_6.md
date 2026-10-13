# Inherited model saves: Rust 5.0.6

Status: partial G01/G03/G04/G13 evidence. Normal inheritance generation remains
guarded; this checkpoint does not qualify every inherited graph configuration.

Runtime reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Original generator: `45e1572c4956ca963410d8ea436ead3c8857087a`.
The oracle compiles the complete, unchanged original inheritance crate and calls
its actual query runtime and public `save_audited_ledger_entity` API against
SQLite. The paired C++ probe uses generated models and the existing audited
ledger service, executor and provider.

## Observed contracts

School, Academy and ResearchAcademy queries vary source and decoded result types
independently. The returned model determines the save descriptor and physical
table; source projection and original snapshot still determine available values
and optimistic versions. The fixture deliberately gives the three rows with the
same ID different versions, exposing cross-model optimistic conflicts.

Normal, minimal and with-children queries exercise common name/rank updates,
descendant campus/budget updates, required/length diagnostics and sparse rows.
The comparison retains original snapshots, native values before and after save,
complete wire diagnostics, pending ledger fields and all four tables, including
Platform. Every failed save preserves the database state immediately before the
save, including any separately injected version increment. Every successful
save clears the pending ledger.

The original runtime bootstraps Platform ID 1. Both fixtures then explicitly seed
the same root name/version and rows; this does not qualify generated automatic
bootstrap equivalence. Decimal database text is observed as stored, without
normalization. Check failures retain their complete diagnostics; optimistic
conflicts and the injected trigger failure have explicit classification markers.
Other failures retain their complete error strings.

## Relation availability and attachment

The generator now emits definitions and metadata for inherited forward relation
accessors and E helpers using the full inherited field set. Own identity-field
selection remains separate, preserving the original query constructor defaults.

The comparison also exposed a runtime discrepancy. Original ordinary flat
hydration publishes related graph nodes and edges without marking the parent
relation available; compact flat hydration marks it available. The C++ ordinary
flat installer now follows that distinction. Thus graph presence alone does not
make a generated relation getter return a value.

Related cases clone an available Platform and attach its runtime state to the
root ledger before updating it. Rust uses the original generated
`attach_runtime_state_recursive`; C++ now uses generated
`attachRuntimeStateRecursive`. Its shared runtime operation adopts mutations and
preserves the entity's loaded view through `with_loaded_view_from`, matching the
original derive macro's state replacement. Both states must belong to the same
Context; cross-domain attachment rejects before target mutation. Only stored
forward objects are recursively attached, as in the original template; relation
lookups and reverse collections are not traversed or loaded. The original attachment
can cause a duplicate Platform insert and transaction rollback in these cases.
There are no successful saves with an available updated related Platform in this
matrix; matching those failures is not evidence of successful multi-node saving.

## Evidence and remaining scope

`tests/parity/generated_inheritance_save_5_0_6_cases.json.gz` now retains **1,350**
matching original/C++ observations: **169 successful saves**, **472 optimistic
conflicts**, **373 Graph failures** and **336 Check failures**. The original
producer files retain their provenance and are not patched to fit the C++ result.

The initial save checkpoint's native regression passes 11/11, including query/result/save
corpora and relation contracts. Targeted ASan/UBSan passes 3/3 with leak scanning
disabled; module integration passes 3/3. Selected Java generator tests pass 14/14,
and all seven generated artifact sets reproduce (77 files). These are targeted
checks; this checkpoint does not include MIPS or physical-device execution.

Broader multi-node graphs and the remaining model configurations need separate source
qualification before removing the normal generation guard. This checkpoint does
not complete P2 or the full generator alignment goal.

## Creation and ledger composition extension

The additional source matrix creates each inherited model through its original
generated `new_entity`, using a configured deterministic ID generator after
bootstrap. It varies a second ledger participant: absent, a new School, or an
existing School update. The existing participant's snapshot/version survives
attachment, and successful saves create the inherited row and update the second
row in one transaction. These are ledger composition cases, not proof of every
nested generated graph shape.

All 81 additional observations match: 24 successful saves (including nine with
an existing secondary-row update), 30 Graph failures and 27 Check failures.
Every failed operation preserves all four tables. The original 1,107 update
observations also pass after replacing the harness attachment with the generated
public adapter.

Root INSERT and participant INSERT/UPDATE triggers test rollback across both
participants. The original new participant has no Context-root field populated
before attachment and fails required-field validation. Its failure remains part
of the matrix; the C++ fixture does not prefill fields through private setters.
The automatically assigned Platform root is preserved: a second assignment
panics in the original Context and is not a supported fixture operation.

Native tests additionally exercise recursive stored-forward attachment, shared
subsequent mutations, snapshot/version retention, rejection across Contexts and
actual source Context deletion after an explicit Global copy. The default PMR
resource is disabled in these tests. Original producer artifacts remain unchanged.

The extension's final native checks pass 4/4 (runtime state, generated API,
inherited storage and the full 1,188-case save corpus). Targeted ASan/UBSan
passes 3/3 with leak scanning disabled, including recursive attachment and actual
Context deletion. Inherited module integration passes 1/1; selected Java tests
pass 14/14 and all 77 generated files reproduce. Formatting checks 830 C++ files.
Source/path integrity checks pass; all 17 generator capabilities remain
unqualified as complete responsibilities.

## Inherited deletion extension

The generated application method `markForDeletion()` follows the original
`teaql_entity` derive macro's public `mark_for_deletion`: record the entity key
in its deletion ledger and return the same model for chaining. It delegates to
the existing runtime deletion method; marking alone performs no SQL. Assist
documents the audited save boundary and SQLite soft-delete behavior.

The additional 162 original/C++ observations vary source/result models, normal/
minimal/with-children entries, optimistic conflicts, trigger failures, available
related Platform mutations and the sparse parent-only row. They include 23
successful deletions, 114 optimistic conflicts and 25 Graph failures. Successful
deletion changes version 5 to -6, for example, returns the complete stored row
even for the minimal query, and clears the deletion marker. All failures retain
the marker and preserve all four observed tables. Cross-model decoding does not
delete other inherited physical tables implicitly.

The native inherited application test verifies chaining, visibility before save,
negative-version readback, marker clearing and exclusion from normal queries
after commit. Native checks pass 3/3, module integration 1/1, selected Java tests
14/14, and all 77 generated files reproduce. This extension changes a thin
generated API and qualification harnesses, without changing memory-management
or asynchronous-cleanup implementations; sanitizer and MIPS were not repeated.
Full inherited configuration and nested graph qualification remain open.
