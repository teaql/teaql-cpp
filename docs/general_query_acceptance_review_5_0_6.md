# G06 general-query acceptance review

G06 is qualified for the complete locked source-selected general-query interface,
with the explicit C++ ownership, naming and native-extension dispositions below.
All twelve requirements have body/guard review and semantic evidence. The complete
[pre-acceptance entry](parity/general_query_pre_acceptance_5_0_6.json.gz) preserves
its earlier questions. This decision does not complete P4/P7 or the adjacent
capabilities.

The reference is Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. The original responsibility is
preserved in the [twelve-row matrix](general_query_requirements_5_0_6.md).
The [current source review](parity/general_query_source_review_after_business_id_lookup_5_0_6.json)
retains complete generic request bodies, template branches, runtime helpers and
model guards. Each disposition below combines those bodies with executed
behavior. The source inventory alone is not acceptance.

## Requirement dispositions

| Requirement | Reviewed behavior and evidence |
| --- | --- |
| GQ01: every Q constructor and Business-ID entry | The [1,400-case guard matrix](general_query_guards_5_0_6.md) compares bare Default, ordinary, minimal and with-children construction through their actual source branches. The [528-case typed lookup matrix](generated_business_id_lookups_5_0_6.md) restores own-field factories before acceptance, including typed constructor errors, duplicate matches and first-or-empty/count behavior. Java checks own versus inherited fields, multiple IDs, human naming and keywords. |
| GQ02: source/result separation and public guards | [Return mapping](generated_return_type_5_0_6.md) retains source repository/creation identity while decoding the selected result. The [marker migration](generated_query_result_marker_5_0_6.md) compares complete construction state and tests arbitrary markers without constructing them. Actual original and native compiler refusals retain Entity on every public execution entry and creation, including compact rows/count/exists. |
| GQ03: clone, inspection, collections/options and conversions | [Request state](generated_request_state_5_0_6.md) has 432 original/native observations and 3,024 complete views/conversions; [core inspection](generated_core_query_view_5_0_6.md) adds 816 cases and 6,528 views. They distinguish independent collections, option snapshots, direct consuming core conversion and metadata-applied selection conversion. Native ownership tests cover consumed/stale access, independent mutation and explicit Global retention. |
| GQ04: search and Boolean/property criteria | [General Q](general_query_5_0_6.md), [field criteria](generated_field_criteria_5_0_6.md) and the accepted [G07 interface](predicate_projection_acceptance_5_0_6.md) compare replacement versus AND/OR/append composition, canonical fields, Value variants, operator failures and physical SQLite results. The complete source helper admits six column comparisons and six column LIKE variants; unsupported property operators retain their errors rather than becoming value comparisons. |
| GQ05: exact live-filter removal | General-Q and [selection-scope](generated_selection_scope_5_0_6.md) execution retain live/deleted rows. Complete helper review and core controls distinguish I64 zero from other Value variants, recurse through AND only, collapse empty/singleton conjunctions and retain OR. Bare Default has no live filter; deleted-only inserts its separate nonpositive-version condition. |
| GQ06: type matching and grouping | [Input ranges](generated_type_match_ranges_5_0_6.md) qualify Value conversion, lazy/single-pass ranges, thresholds and original missing-discriminator errors. [Inheritance acceptance](inheritance_acceptance_5_0_6.md) and result mapping cover descendant membership and type-group metadata. Neither helper invents a discriminator column for an ordinary model. |
| GQ07: matching, enhancements and complete input conversion | [Matching](generated_matching_any_5_0_6.md), [enhancement inputs](generated_enhancement_inputs_5_0_6.md), [relation inputs](relation_selection_inputs_5_0_6.md) and marker composition retain original Into conversions. Matching embeds a bare query with its name-only descriptor; enhancement retains the complete selection and options. Actual source failures remain failures. Independent child snapshots, another generated domain, explicit core/selection values, foreign Context rejection and destination copies have execution/compiler/lifetime controls. |
| GQ08: trusted raw surfaces | General-Q/state/inspection observations preserve raw statement, raw filter and computed-property storage independently. [Composed execution](composed_general_queries_5_0_6.md) compares actual raw/readonly/grouped/facet requests, all eight execution entries, application policies and protected diagnostics. Literal SQL stays trusted application input; adding a query predicate is not a sanitizer or a permission guarantee for a complete literal statement. |
| GQ09: self/all/children/any and guards | Selection-scope and constructor-guard matrices cover ordinary, no-forward, reverse-one, reverse-many and inherited branches. HasSelectAll is forward-object or reverse-one availability; HasSelectAny is reverse-many availability. Aliases preserve delegate behavior. Named relation selection does not import a child Q live filter; explicit child selections retain their own state. |
| GQ10: projection removal and field guards | The complete [G07 matrix](predicate_projection_requirements_5_0_6.md) reviews every scalar/object branch. Selection-scope execution checks raw/plain removal against independent aliases/aggregates, FK versus relation removal, reselection and typed availability. Keyword aliases and source metadata failures retain their separate declared identities. |
| GQ11: intent, binding, Policy and diagnostic integration | [Composed execution](composed_general_queries_5_0_6.md) has 4,864 actual original/native outcomes with all eight entries, ordinary/inherited private models, trusted raw surfaces, readonly/facet/grouped shapes and ordinary/ID-set/continuous configurations. It independently verifies 768 blank-intent failures before Policy/I/O, 1,536 denials before SELECT, sixteen precise privacy repairs and logging-disabled execution. Existing binding/deferred cleanup controls remain enforced by the shared runtime. |
| GQ12: naming, guidance, ownership and current gates | Central naming and canonical metadata remain separate from C++ member escaping. Model-specific Assist describes source Default/conversion/selection, marker and lookup boundaries. The bounded 77-SDK proofs preserve existing bodies. Strict ordinary/contract/identity/root generation, Java, positive/negative compiler checks and Context/module/lifetime evidence are recorded by the marker and lookup migrations. A current integrated native selection is required below; older counts are not relabeled. |

## Explicit C++ and source-failure dispositions

Bare Rust `Default` has ID/version projections and no live predicate. Its mapping
is the explicit Context constructor with `(true, false)` flags. Existing native
ordinary full/live defaults remain an additional convenience. With-children
construction removes slicing, selects self fields and enhances subtypes without
adding the ordinary default live filter.

Rust's borrowed bare query view maps to an allocator-aware `coreQuery()` snapshot
in C++. The existing integrated `query()` view remains separate. Returned raw
PMR snapshots must be destroyed before their allocator or explicitly copied to
a destination scope. Ordinary wrappers are checked borrows; they do not keep a
UserContext alive. Dynamic request/expression payloads use the bound Context;
there is no request-private ownership region or implicit default allocator.

Native fluent operations after purpose and preexisting typed convenience
overloads remain documented extensions. They do not prove that Rust exposes the
same method at that stage. Public execution still requires an Entity result and
the matching UserContext. The approved execution-time C++ intent validation
preserves failure before Policy and provider I/O; it does not claim Rust panic
and C++ exception staging are identical.

Original missing-discriminator/name-only-descriptor/keyword-metadata SQL errors
and unsupported SQLite functions are retained. The inherited Business-ID lookup
uses the existing explicit model-binding bridge, with original `q.rs` unchanged;
that is not untouched whole-crate compilation. G04/G05/G07's accepted model,
primitive configuration, constant and language-binding dispositions remain
applicable rather than being silently replaced by a smaller general-query model.

## Adjacent obligations remain active

Execution/optimization protocols remain G09; relationship aggregates, facets and
cache behavior remain G08; dynamic JSON remains G10; complete runtime/sample/
Assist workflows remain G12. Shared model combinations, metadata/governance,
loaded state, persistent dynamic storage and protected trace/audit integration
remain G13-G17 and P4/P5/P7. Their requirements are unchanged. This G06 decision
qualifies the complete source-selected general-query interface; those whole
capabilities and the entire migration retain their independent exits. The
acceptance record preserves the previous questions and their dispositions.

## Current integrated gate

The selected native gate rebuilt its exact executables before running query,
state/conversion, markers, constructor guards, Business-ID lookup, type matching,
selection, enhancement, relation input, matching, return-type, Policy/raw/privacy
composition and execution-lifetime contracts. It passes **23/23**. A separate
current core selection passes **3/3**, including the original request-helper
comparison and exact conjunction/operator controls. All build/test/compiler
processes have exited successfully.

The fifteen comparison runs replay **14,850** retained source observations;
checkpoint-specific representation and C++ ownership dispositions remain explicit.
This total is a regression count, not a distinct-model count or the proof by
itself. The row-by-row bodies, branch evidence, real results, failures and Context
controls above support the decision.

The completed lookup migration adds **2/2 modules**, **2/2 ASan/UBSan** with leak
scanning disabled, **2 positive / 5 negative compiler controls**, **71/71 Java**,
strict **24-workspace/288-file**, **50-contract** and identity/root reproduction.
Its bounded SDK proof covers all **77** SDKs and preserves all prior bodies;
earlier marker/typing/ownership checkpoints retain their separate evidence.
Formatting/check covers **1,633** files; **784** canonical module consumer bodies
and **1,495** catalog exports verify.

The [acceptance record](parity/general_query_acceptance_5_0_6.json.gz) binds the
requirements, source identities, complete current test output and terminal
handles. No full-suite, MIPS, external-provider, GPU or physical-router result is
inferred from these gates.
