# Complete generated-support source review

GS01-GS19 qualify the complete named production source under the established
C++ Context, error-protocol and owner-thread adaptations. Current native 3/3,
imported 4/4, 822 new exact observations per frontend and both original source
observer/interface tests pass. The prior exact-reference gate supplies 2,196
unchanged outcomes per frontend and nineteen original SQLite tests. Complete
capability and P0-P7 exits remain separate.

The reference is all 478 lines of Rust 5.0.6
`teaql-runtime/src/generated_support.rs`, SHA-256
`f21089ff92ae378fa74174f1eb42c028728034e3cb92841fd0795afd14119a2c`.
Runtime commit: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The C++ implementation remains in its independent runtime, service, executor and
provider layers; no generator change is necessary for these shared mechanics.

## Complete responsibility mapping

| ID | Original responsibility | C++ mapping and qualification |
| --- | --- | --- |
| GS01 | Four query-service operations and associated Error | `TeaqlQueryDataService` checks exact result/operation signatures for rows, SmartList, aggregate SmartList and stream. New declaration-only compiler controls reject omission of each operation independently. Original generic compiler controls exercise every trait entry and the Error bounds. |
| GS02 | Two typed entity operations | `TeaqlEntityDataService<R,T>` retains both enhanced-entity entries and requires Entity. The unchanged complete native contract instantiates an actual streaming EntityDataService with a handwritten model; a non-Entity result is rejected. |
| GS03 | Six service implementations | The original six trait bodies delegate to EntityDataService; C++ concepts describe that existing service rather than create another implementation. Original generic calls compile through all six traits. Real retained SQLite raw/list/aggregate/stream/typed observations exercise the underlying service independently. |
| GS04 | Error alias | `TeaqlDataServiceError` retains the associated DataServiceError carrier. Both language compiler controls retain the alias and distinguish it from RuntimeError. Native errors follow the established C++ scoped result protocol. |
| GS05 | Runtime interface | TeaqlRuntime independently exposes UserContext and facet SmartList execution with entity, purpose wrapper, runtime aggregates and separate ancestry. Recording and real SQLite adapters implement this SPI; they do not copy execution algorithms. |
| GS06 | Hidden consuming save | AuditedSave remains separate from AuditedSaveExt, consumes its receiver and returns a GraphNode result with its own error. The source supplies no implementation. Source generic and native positive/lvalue/non-runtime compiler controls verify shape without adding a saver. |
| GS07 | General purpose wrapper | Arbitrary purpose strings, public inner/purpose and move-only inputs remain supported. No Rust Clone/Debug/Default/Eq/hash/serialization derive is invented. Native allocator-aware destination copies/moves remain the established ownership adaptation. |
| GS08 | Validated select wrapper and inspection | Unicode-trimmed nonempty purpose is validated while original text is retained. Construction sets purpose and appends exactly one Purpose frame; copies do not append another. Query access is readonly and consuming extraction deliberately discards private diagnostic state. The retained 46 formatting/validation observations and new complete-state outcomes cover these responsibilities. |
| GS09 | Select wrapper Debug | The executable query and diagnostic-presence flag are visible. Private diagnostic trees are not printed as wrapper state. New observations inspect the private tree separately through existing infrastructure access, without changing the public Debug contract. |
| GS10 | Derived exact COUNT | Snapshot once; clear only projection, expression projection, order, slice and relations; append COUNT. All other fields, including existing aggregates, remain. New inputs independently mutate all 31 query fields over four profiles and compose repeated COUNT/facet/clone operations. |
| GS11 | Facet detection | Visit direct facets, queried relations, child enhancements and object groups. Plain relations are skipped. Expression subqueries and core relation aggregates are not additional detection branches. Nine independent/mixed shapes compare actual helper outcomes. |
| GS12 | Selection diagnostic conversion | Apply runtime metadata through selection conversion, add builder relation-aggregate sources and expand nested facets. New fixtures include option comments, trusted raw metadata, criteria, enhancements, aggregates and nested facets. |
| GS13 | Diagnostic expansion | Consume direct facets into diagnostic children, recurse through queried relations/children/object groups and preserve traversal boundaries. New complete tree observations distinguish detected/ignored nodes and repeated expansion; original public SQL privacy observations remain a separate integration proof. |
| GS14 | Facet registration and snapshots | Empty options without executable facets are a no-op. Otherwise snapshot once, append new option sources and expand that private tree. Repeated registration and COUNT ordering preserve the initial source; executable state remains independent. |
| GS15 | Restriction | Resolve outer metadata first, with Graph for direct missing entity and MissingRelation for a missing edge. Clone and selectively clear seven field groups before applying the foreign-key IN-subquery on the local key. Other query/selection state and the caller's outer query remain unchanged. Current public facets and native restriction contracts exercise success and error branches; complete query Clone has separate accepted source evidence. |
| GS16 | Root intent and metadata order | Empty facet lists still validate comment and purpose. Execution resolves entity/relation even with include_all_facets, using MissingEntity rather than direct restriction's Graph error. Merge outer filters into builder aggregates before optional restriction. Original and native public observations retain exact request-intent, metadata and policy failures. |
| GS17 | Facet lineage and private source | Copy root comment/purpose, append the named relation frame with the root edge, and pass no duplicate separate trace. The private source is invocation-local and is never an executed enhancement. The 36 public facet logs plus 576 derived and 360 composition outcomes retain SQL, privacy, failures and unrelated follow-up behavior. |
| GS18 | Sequential recursive results | Await each fetch before the next, stop on failure, recursively compute nested facets and replace returned nested maps. Duplicate names execute separately and last result wins in the ordered map. Preserve SmartList rows, totals, aggregation/summary and loaded state. Recording, pending/drop, real SQLite and nested public observations exercise this behavior. |
| GS19 | Ownership and cleanup | C++ operations snapshot their Context inputs, check runtime liveness and foreign-future ownership, forward wakeups and join/cancel child work before reclamation. Existing contracts remain unchanged. A new private-tree copy test destroys the source User, reads its complete Global copy and rebinds the complete tree into a new User. No shared owner extends the old User. |

This mapping combines the actual production bodies, compiler contracts, private
state and public execution evidence. A source name, inventory count or empty
diagnostic flag alone does not qualify a responsibility.

## New private observation boundary

The original observer runs inside a disposable complete runtime view. All 67
original source/test/fixture files retain their complete original bytes; only
one test module is appended. The existing full-query fixture is included
byte-for-byte in that module, followed by the new observation code. Actual
original private methods perform every transformation. Only disabled external
exporter development dependencies are omitted; registry versions and checksums
match the locked reference. The frozen checkout stays clean.

The 822 cases contain 768 complete-state sequences and 54 traversal sequences.
Each observes the full initial query, actual detection/expansion/selection source,
all intermediate executable and private trees, wrapper Debug, independent copy,
consuming extraction and source immutability. Embedded NUL/Unicode, long strings,
zero/large limits, schema/selection state and every query field retain their
separate fixture values. Output equality performs no normalization.

The original service-interface test is compiler evidence: its generic bodies
type-check all six actual trait entries, error/alias bounds and consuming hidden
save. It does not claim to execute all six delegates at runtime. Real SQLite
execution is supplied by the previous exact-reference checkpoint. C++ structural
concepts use existing allocator-aware errors, operation handles and Entity
contracts. Rust Send/Sync and borrowed futures map to the established Context
owner-thread and joined-operation adaptation, not an arbitrary C++ Send/Sync
guarantee.

Three existing diagnostic helpers needed matching module aliases, catalog entries
and isolated-import lookup. These export changes expose existing declarations;
no reviewed production function/header body changes. The first export check correctly
rejected the incomplete lookup registration; its
[failure record](parity/generated_support_source_initial_export_failure_5_0_6.json.gz)
is retained. The first controller run passed native822 and all three native
contracts, then failed to locate its relative JUnit path. Its
[execution](parity/generated_support_source_initial_driver_failure_5_0_6.json.gz)
and [controller snapshot](parity/generated_support_source_initial_driver_failure_details_5_0_6.json.gz)
remain separate. The corrected driver resolves evidence/JUnit paths absolutely.

## Evidence and remaining scope

The [prior exact-reference replay](generated_support_reference_5_0_6.md) supplies
2,196 unchanged actual-source observations per frontend and nineteen unchanged
original SQLite tests. Its runtime/observer/test bytes and corpora remain
preserved; new private observations supplement that gate rather than replace it.
The [current private-state gate](parity/generated_support_source_execution_5_0_6.json.gz)
records its own source view, terminal commands, complete outputs, compiler
contracts, Context retention and frontend checks. Whole-source acceptance remains
a separate decision from the helper's `complete` field. The
[qualification decision](parity/generated_support_source_qualification_5_0_6.json.gz)
joins GS01-GS19, the current and explicitly reused unchanged source evidence,
and the [fresh import/lookup gate](parity/generated_support_source_frontends_5_0_6.json.gz).
All final selected builds report no work. Catalog verification covers 1,516
exports and 960 canonical bodies. Native discovery is 765 checks and imported
discovery is 521; neither discovery count is a complete-suite pass.

The [preservation record](parity/generated_support_source_preservation_5_0_6.json.gz)
verifies all 3,457 files in 78 SDKs, 2,201 protected artifacts and fifteen existing
runtime/observer files. New tests and the three existing-helper export aliases
are the changes; Java/generation reproduction is not claimed. Pinned formatting
and its check cover 1,695 C++ files. Source dispositions advance only this file,
to 53 accepted / 69 pending / 2 external exclusions. The seventeen capability
verdicts remain eleven qualified and six partial.

Generated aggregate/facet model configurations and quoted-FK behavior, whole
service/provider execution, protected publication, complete assembly/sample/
Assist and current whole integration remain their own capabilities and phase
exits. Historical failed whole-suite epochs remain failed. No external-provider,
GPU, MIPS or physical-router acceptance is inferred. This review does not reduce
any of those requirements.

## Reproduce

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_support_source_5_0_6.py \
    --module-build-dir build_modules_native \
    --record /tmp/teaql_generated_support_source_fresh_5_0_6.json.gz
```
