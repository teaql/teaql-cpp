# Entity-save responsibility reconciliation (Rust 5.0.6)

Status: **accepted** after the [complete source closure](entity_save_source_acceptance_5_0_6.md).
This matrix organizes the evidence in
[the chronological source review](runtime_entity_save_source_review_5_0_6.md).
Historical focused gates retain their checkpoint scope; current acceptance also
includes fresh original17+5576 and the joined native25/module25 consumers.
The source is `teaql-runtime/src/entity_save.rs` at the exact runtime commit in
`upstream.lock.json`. Its seventeen original tests have direct ports; production
responsibilities, caller coverage and integration remain separate obligations.

| ID | Original responsibility | Candidate implementation | Evidence and remaining boundary |
| --- | --- | --- | --- |
| ES01 | Fix clock, recording and publication | `detail/graph_fix_scope.hpp` | Original graph-fix observations and generic dynamic completion/cancellation gates. [Nested public-call correction](nested_save_scope_5_0_6.md) reproduces three collector-sharing differences with a clean sanitizer counterfactual; fresh original/native4, native6/module2/sanitizer2 pass. Scope storage belongs to UserContext; explicit close and cancellation do not publish incomplete evidence. Recheck consuming entry changes at their recorded checkpoints. |
| ES02 | `DynGraphSaver`, executor erasure and registration | `detail/data_service/graph_saver.hpp` | Distinct graph and ledger methods, trusted resource lookup and registered executor adapter remain present. Saver and automatic-entry contracts exercise missing capabilities; eight original/native controls reproduced three ordinary-path precedence differences; intent-before-lookup correction passes native4/module4 with the complete expected registry and full saver comparison. No provider/runtime layers are merged. |
| ES03 | Ordinary graph transaction | `graph_saver.hpp` | Seventeen-source-test port and saver contracts cover pre-begin policy denial, execution, rollback and post-commit delivery. Configured faults that stop before their target branch are not counted as downstream evidence. |
| ES04 | Ledger transaction and readback | `graph_saver.hpp`, ledger execution outcome | Direct ports reach readback race, rollback, tombstone, persisted-update commit failure and post-commit audit failure. Failed commit retains pending changes; delivery failure is committed, not retryable rollback. |
| ES05 | Saved typed entity and dynamic overlay | `entity_save.hpp`, audited operation completion | Six original owner/installer/success controls, missing indexed snapshot, generic plain-row repair and four executor-fallback controls distinguish the decoder branches. Preserve loaded relations only when eligible. |
| ES06 | Cancelled new root | `entity_save_preparation.cpp` | Direct original test and preparation observations retain the create-then-delete error. The predicate requires membership in both sets. |
| ES07 | Relation graph preservation | `entity_save_preparation.cpp` | Source predicate and direct relation-state port reject new/deleted keys, other-entity scalar/dynamic edits and changed local relation keys. Empty or root-only scalar changes remain eligible. |
| ES08 | Typed entity extraction | `entity_save.hpp` | Preparation corpus covers extraction/typed loading. Source and candidate capture loaded scalars, dirty fields, original snapshot, new/deleted flags, comment and dynamic fields before consuming values; deletion clears relations. Entry descriptor/audit ordering has a separate eight-case correction gate. |
| ES09 | Mutation values to graph | `entity_save_preparation.cpp` | Source comparison preserves reserved marker consumption, ordinary NULL versus typed NULL, object/list traversal, metadata resolution and exact malformed-relation errors. Preparation observations are relevant; they do not prove persistence. |
| ES10 | Relation mutations into ledger | `entity_save_preparation.cpp` | Both implementations require child identity, stage Create values, restrict Upsert to explicit dirty fields, mark Remove, preserve Reference, capture original versions and recurse. Preparation/source ports exercise these boundaries. |
| ES11 | Request trace ancestry and retries | `entity_save_preparation.cpp` | Six source trace tests, allocated-ID ownership controls and explicit-executor empty-ledger capture correction preserve deterministic first route, standalone pending scopes, retry replacement and NULL/zero ownership. Typed NULL is not an unallocated ID. |
| ES12 | Hydrating pending relations | `entity_save_preparation.cpp` | Source reconciliation preserves scalar/dynamic/new/deleted pending union, existing-child exclusion, normalized foreign identity, visited keys, original snapshots, state-derived operation and no recursion into deleted children. Source ports cover signed/unsigned IDs and composed deleted children. |
| ES13 | Graph and standalone dynamic preflight | `entity_save_preparation.cpp` | Status/dirty markers are removed before propagating Checker errors; fixes update the ledger and locations remain model-relative. Original/native callbacks exposed two cached-state differences. Per-entity membership queries now pass native4, including three callback controls and 1,719 preparation comparisons; module4/sanitizer2 and canonical/reference gates also pass. |
| ES14 | Ledger diagnostic locations | `entity_save_preparation.cpp` | Direct original location test and preparation corpus preserve first deterministic model/JSON path, root empty path and entity-qualified identity. |
| ES15 | Generic audited save | `audited_auto_save.hpp` | Generic-only descriptor sampling, audit application, dynamic carrier validation, scoped continuation and ordinary row decode are separately observed. Eight-case entry gate passes native4/module2/sanitizer2. |
| ES16 | Typed/generated ledger save | `audited_auto_save.hpp` | Typed carrier is optional. Recent correction removes the generic pre-audit descriptor read and mandatory-carrier guard from this entry. Eight original sequence/route controls and the native4/module2/sanitizer2 gate pass. |
| ES17 | Executor-bound save | `audited_save.hpp` | Existing exact comparisons and valid-intent scope controls preserve caller transaction ownership and returned retryable ledger. Four original/native fallback controls confirm ordinary decode has no dynamic installation and returns no pending ledger. |
| ES18 | Error conversion and commitment boundary | Audited/saver operations | Provider failures become graph errors; existing runtime errors remain typed; audit delivery errors retain `AuditAfterCommit` and its diagnostic code. Original commit/audit direct ports establish reached branches, not merely configured faults. |

Candidate header paths are relative to `teaql_runtime/include/teaql/runtime`.
Preparation implementation is `teaql_runtime/src/entity_save_preparation.cpp`.
See the source review for receipts, prior failures and scope limits. This matrix
is a review map rather than an additional execution result. The confirmed callback and entry discrepancies have focused correction gates.
The current complete-file closure reconciles every responsibility above with
the original source and current consumers. It accepts this source only: totals
are 117 accepted / 5 pending / 2 excluded_external. Capability counts and P0-P7
remain unchanged, and the independent graph/resolved/provider/root reviews remain open.
