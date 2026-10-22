# Relation source acceptance review (Rust 5.0.6)

Status: accepted for the complete named source; native26/module26 and original3 pass.

## Scope and source identity

This review concerns the complete named source
`teaql-runtime/src/data_service/relation.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`fb28e3d7a93a7eba937154db25c899cd768d3a0b2d1a1353e821573543e874ad`.
The [responsibility review](relation_complete_responsibility_review_5_0_6.md)
and [inventory](parity/relation_responsibility_inventory_5_0_6.json) account for
all 39 production function definitions (including nested decoder eligibility),
three source tests and their fixture. R01-R11 and T01 retain their distinct
Context, service, executor, provider, graph and diagnostic responsibilities.

The C++ counterparts distribute those responsibilities across shared planning,
execution, enhancement, hydration, facet and installation code. Finite source
observations support the function/body review; neither inventory counts nor a
single passing query establish complete source acceptance.

## Evidence and resolved findings

- Repository/explicit plans, query construction and grouped top-N ordering retain
  the 112 planning and 1,152 extension observations. Current model diagnostics
  and recursive ownership contracts run in the final native union.
- All three original planner tests pass in a fresh complete source view. The
  [supplemental 32-case dispatch contract](relation_probe_boundaries_5_0_6.md)
  covers every singleton-provider stimulus through actual C++ execution, including
  parent count 32 and explicit threshold 32. It is separate from the unchanged
  432-case source differential corpus.
- [Complete enhancement replay](relation_enhancement_complete_5_0_6.md) matches
  754 actual-runtime outcomes in native and modules. It preserves both unchanged
  inputs and a separate explicit-intent matrix. The older child-fetch-SPI results
  remain scoped historical evidence; they no longer substitute for the complete
  Context/request/executor boundary.
- [Hydration replay](relation_hydration_replay_5_0_6.md),
  [flat owner facets](hydration_flat_facets_5_0_6.md), and
  [owner facet cancellation](hydration_owner_facets_async_5_0_6.md) retain actual
  original graph/query execution and partial-publication observations. Current
  tests replay their corpora and Context-close contracts.
- [Dynamic storage acceptance](dynamic_storage_acceptance_5_0_6.md) and
  [SQLite relation membership](relation_membership_source_audit_5_0_6.md) supply
  the selected dynamic-value, checked overlay, aliased foreign-key, explicit
  inverse projection and real provider integration evidence. Their independent
  accepted scopes are preserved.
- The final review found an outer relation SmartList left unloaded in C++.
  Shared attachment now marks it loaded without changing nested sidecars or
  Context ownership. [24 original attachment states](relation_loaded_state_5_0_6.md)
  match native/modules, including empty facets, to-one truncation and identity
  fallback. The pre-fix failure and deliberately failing controls remain intact.

## C++ ownership adaptations

Growing Context arenas own recursive plans, query snapshots, row sidecars and
poll operations. Operations hold checked Context borrows rather than extending
Context lifetime. Explicit destination copying is required for Global retention.
Allocator propagation and foreign-domain rejection are C++ ownership guarantees,
not changes to business relation semantics.

C++ cancellation forwards to native, descendant, dynamic and facet operations
before releasing referenced host memory. Original future-drop semantics are
represented by this explicit stop/cleanup protocol. Native facet and graph
contracts include Context shutdown; previously qualified focused sanitizer
receipts remain scoped lifetime evidence. No new sanitizer/MIPS run is claimed
for the loaded-Boolean repair or test-only additions. Threaded scheduler and
physical router execution are not inferred from these tests.

## Final gates and boundary

The [native union](parity/relation_native_closure_5_0_6.json.gz) builds 18 affected
targets and passes the exact 26 selected tests, binding current service source
hashes before/after execution. The [module union](parity/relation_modules_closure_v2_5_0_6.json.gz) builds
canonical imports for the same 18 targets and passes all 26 corresponding commands,
including the same comparison inputs and Context contracts. The controller derives
commands from the exact native CTest selection and changes only the executable
path. Canonical verification checks 1,126 consumer bodies. Both gates bind the
same current service headers/implementations; module configuration hashes also
remain unchanged during its gate.

The initial module gate failed during CMake configuration because two new test
registrations omitted the required empty environment arrays. That failed receipt
is retained; v2 follows the registration repair, with no implementation or assertion
changes. Pinned C++ format/check remains the successful 1,791-file gate, with no
subsequent handwritten C++ changes. Stable build directories are retained for
reuse; no disposable source checkout remains from the original observations.

The source ledger is promoted from pending_review to accepted only for this row.
Active source totals are 112 accepted / 10 pending / 2 excluded_external. The
[acceptance receipt](parity/relation_source_acceptance_5_0_6.json.gz) binds this
review, responsibility inventory, terminal receipts and the ledger at promotion.

This source review does not accept adjacent pending `graph.rs`, `resolved.rs`,
`entity_save.rs`, runtime root or SQLite sources by implication. It does not
claim generator P0-P7 completion, additional database providers, release artifact
qualification or general cross-language parity. Full feature and source ledgers
remain independently authoritative.
