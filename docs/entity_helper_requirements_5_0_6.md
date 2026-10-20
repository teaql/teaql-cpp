# G01 entity helper requirements: Rust 5.0.6

Status: G01 qualified; all eight helper obligations are accepted with the
documented C++ adaptations. See [the acceptance review](entity_helper_acceptance_5_0_6.md)
and terminal gates. This matrix does not replace the wider P1/P0-P7 requirements
or retire existing source/runtime evidence.

The producer reference is `45e1572c4956ca963410d8ea436ead3c8857087a`. Template
anchors below refer to its `generator/rust-lib-core/entity/struct.rs.stg` under
`generator/src/main/resources`. Runtime reference is the exact 5.0.6 lock.

| Source obligation | C++ implementation / evidence | Acceptance |
| --- | --- | --- |
| Scalar getter/update/changed/eval; required versus optional/JSON storage | CppCoreGenerator.models delegates to EntityRuntimeAccess and indexed LoadState. [Entity helpers](generated_entity_helpers_5_0_6.md) and [optional fields](generated_optional_fields_5_0_6.md) observe raw ledger versus trimmed storage, NULL presence, conversion fallback, independent clone availability and original snapshots. | Current reference-lock replay passes all 3172 retained observations through the unchanged observer and original entity/Checker bodies; see [source closure](parity/entity_helper_source_closure_5_0_6.json). The acceptance review separately reconciles the source naming/template branches and emitted adapters. |
| `with_id(id) -> Value::U64(id)` | Value already implements the runtime representation; generated models now provide `withId(user_context, id)`. Source index template lines 53-55. | Implemented as a thin Value adapter. [Entry qualification](entity_helper_entries_5_0_6.md) covers zero/full-width IDs, the explicit UserContext argument and native domain checks. |
| Default runtime construction and recursive state attachment | Context-managed generated Model constructors provide defaults and indexed NotLoaded; attachRuntimeStateRecursive delegates ledger adoption and forward child attachment. | Actual private runtime_new and row/root adoption have 512 exact source observations; required timestamp defaults are repaired. See [constructor/child helpers](entity_constructor_helpers_5_0_6.md). Another [192 recursive attachment observations](recursive_attachment_helpers_5_0_6.md) preserve forward-child adoption, local values, shared reads and loaded snapshots. Runtime/provider orchestration stays separate. |
| `is_loaded` and `set_load_state`, including compatible indexed layouts | Framework `is_loaded` exists. The generated `setLoadState` and `isLoaded` entries now delegate existing indexed conversion. Source index template lines 76-85. | Implemented; 1008 source observations and twelve ownership controls cover variants, incompatible layouts, failure atomicity and explicit same-Global cross-User destination copies. See [entry qualification](entity_helper_entries_5_0_6.md). |
| Forward relation getter/eval, local value precedence, graph lookup and NULL versus NotLoaded | Generated relation methods delegate root lookup and GeneratedRelationAccess. Existing same-model forward/reverse-to-one matrix contains actual absent/NULL/zero/wrong-type/cache/selected outcomes. | Source objectRelationMethods and C++ accessors preserve local/view/cache precedence and known optional NULL. Required/optional embedded children add actual local reads before/after attachment. No implicit database reads are introduced. |
| Reverse-to-one handle/eval | Generated handle and object evaluation preserve Loaded/Empty/NotLoaded. Existing entity-helper and graph source comparisons exercise the actual generated getter. | Retain exact diagnostics and checked-borrow ownership in closure. |
| Reverse-to-many handle/eval, including Empty as a Value | [Reverse-many checkpoint](generated_reverse_many_helpers_5_0_6.md) adds 864 exact source/native outcomes over required/optional child types, initialized graph views, independent list/selection flags, wrong type/key and sparse members. | Source Empty/Value, unavailable views and exact mismatch behavior are preserved; checked C++ list borrows and mutable staging containers retain explicit ownership. |
| Business-ID and constant helper specializations | Distinct value/slot types, candidate updates and scalar changed/eval are exercised by the Business-ID and accepted G04/G05 matrices. The [inherited Checker/helper matrix](inherited_checker_helpers_5_0_6.md) composes five actual models. | Explicit producer-failure/language binding dispositions remain documented. Child-only amount/status/reference and inherited name have direct getter/update/changed/eval/snapshot/clone comparisons. The source specialized helper templates are reconciled separately from service/profile/governance responsibilities. |

The two application-entry gaps are now implemented and qualified separately.
Existing equivalent low-level operations were not treated as proof that the
generated abstractions existed. Shared runtime mechanics must precede thin model adapters; current Value
construction and LoadState indexed conversion already provide those mechanisms.

Checker/Fix/Behavior assembly is G02/G03, privacy metadata is G14, indexed loaded
snapshots are G15, dynamic/readonly authority is G16 and trace provenance is G17.
Their integration with entity helpers remains required by P1/P7. G01 acceptance
uses source/template review plus the terminal artifact/compiler/native/module/
lifetime gates; this inventory alone is not acceptance. The other capabilities
retain their own unresolved obligations.
