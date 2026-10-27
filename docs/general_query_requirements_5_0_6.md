# Complete G06 general-query requirements

Current correction: the [creation boundary](generated_creation_identity_5_0_6.md)
was reopened after90 source/native differences and is now requalified:90/90
complete results, runtime first gates,80 SDK byte preservation, current Java86,
native10/module1/ASan-UBSan1 and196 compiler contracts. G06 returns to qualified;
G13/G14/G17 and P0-P7 remain open. The remaining acceptance text preserves its
original checkpoint scope.

Status: G06 qualified through the [complete acceptance review](general_query_acceptance_review_5_0_6.md).
The original capability scope and P4/P7 exits remain intact. The runtime is Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; the generator is
`45e1572c4956ca963410d8ea436ead3c8857087a`.

The [current complete source review](parity/general_query_source_review_after_business_id_lookup_5_0_6.json)
retains 97 full generic request functions, seven complete template helpers,
three runtime helper bodies and the original model guards. Every generic
function has an explicit responsibility, including adjacent execution,
aggregation, dynamic-field/JSON and Save functions. Assigning an adjacent
capability does not waive its shared integration or phase obligations.

| ID | Complete responsibility | Evidence and current disposition |
| --- | --- | --- |
| GQ01 | Q ordinary/minimal/with-children entries, bare Default, identity projection and Business-ID lookup entries | The [constructor/selection guard matrix](general_query_guards_5_0_6.md) exercises the four constructor states and actual source branches. Bare Rust Default maps to `Request(user_context, true, false)`; the existing native constructor defaults remain a separate convenience. The [typed Business-ID factories](generated_business_id_lookups_5_0_6.md) restore original own-field selection, distinct identifier inputs and normal-entry/predicate/limit composition; the inherited source bridge remains an explicit binding adaptation. |
| GQ02 | Result adaptation and the complete public typing boundary | [Result marker qualification](generated_query_result_marker_5_0_6.md) restores unconstrained construction/composition while preserving Entity on every public execution entry and creation, including compact rows/count/exists. Source identity, lvalue independence, rvalue transfer, explicit copies and creation model remain distinct. |
| GQ03 | Clone, bare inspection, independent collections/options and both consuming conversions | [Request state](generated_request_state_5_0_6.md), [core views](generated_core_query_view_5_0_6.md), marker and guard matrices compare complete state. `coreQuery()` is the explicit-domain snapshot adaptation of the source borrow; native integrated `query()` remains separate. Direct `intoQuery` excludes independent options; selection conversion applies them. |
| GQ04 | Search, replacement/AND/OR composition, appended criteria and property-to-property operators | [General Q](general_query_5_0_6.md), [field criteria](generated_field_criteria_5_0_6.md), [typed execution](query_predicate_execution_5_0_6.md) and existing core baseline comparisons retain complete predicates and source errors. Source property operators allow six comparisons and six LIKE variants; Between/In/availability/sound operators refuse rather than becoming value predicates. |
| GQ05 | Exact default live-filter removal and deleted-only insertion | General and selection-scope execution compare live/deleted rows. The full original remove_filter_expr body removes matching `version > I64(0)` only through AND nodes, retaining OR and other Value variants; deleted-only then ANDs `version <= I64(0)`. The new bare-default matrix independently verifies absence of the live predicate at construction. |
| GQ06 | Type matching, direct child subtype filters and type-group projection | [Input ranges](generated_type_match_ranges_5_0_6.md), [inheritance acceptance](inheritance_acceptance_5_0_6.md) and [return mapping](generated_return_type_5_0_6.md) retain Value conversions, descendant match lists and source internal_type/type_group metadata errors. Source helpers do not manufacture discriminator columns. |
| GQ07 | Matching aliases, complete selection conversion and child enhancement | [Membership](generated_matching_any_5_0_6.md), [enhancement inputs](generated_enhancement_inputs_5_0_6.md) and [relation input contracts](relation_selection_inputs_5_0_6.md) distinguish bare-query membership from metadata-applied enhancement. Preserve the original name-only descriptor's SQL failure, independent child snapshots, unpurposed/foreign-domain input bounds and explicit destination copies. |
| GQ08 | Trusted raw statement/filter/property storage and explicit unsafe segments | General/request-state/core-view matrices and [composed protected execution](composed_general_queries_5_0_6.md) preserve separate fields, option application, readonly values, Policy rejection, intent and diagnostic privacy. Source string conveniences explicitly create trusted segments; they are not sanitization or authorization. |
| GQ09 | Self aliases, all/children/any and exact model guards | [Selection scope](generated_selection_scope_5_0_6.md), inheritance and the new guard matrix cover forward/to-one/to-many and no-forward branches. HasSelectAll means object fields or reverse to-one; HasSelectAny means reverse to-many. Named relations keep their source version boundary and do not import a child request's live filter. |
| GQ10 | Scalar/object projection removal and criteria guard interaction | The [complete G07 field interface](predicate_projection_requirements_5_0_6.md) and selection matrix qualify scalar raw/ordinary removal versus independent aliases, object FK/relation removal, source omissions and supported field/configuration branches. G07 acceptance is reused for shared source bodies; it does not establish full G06 or execution acceptance. |
| GQ11 | Nonblank intent, Context binding, Policy and protected execution integration | Composed source/native/module/instrumented execution spans ordinary, inherited, raw, readonly, grouped, facet, page/stream and optimization shapes. Blank intent and Policy denial precede I/O. Preserve source root/trace identities and privacy; complete G09/G14/G17/P7 obligations remain active under their own requirements. |
| GQ12 | Naming, own/inherited model selection, accurate Assist and current compiler/reproduction/lifetime gates | Preserve canonical metadata and lowerCamelCase application methods, bound Q, exactly one execution Context and explicit two-scope ownership. Marker and Business-ID migrations supply 77-SDK body preservation and strict ordinary/contract/root/identity reproduction. Model-specific lookup Assist records the actual typed delegate and first-or-empty boundary. The complete acceptance review records the final integrated gate selection and every row disposition; method totals are not acceptance. |

## Business-ID omission and repair

The unchanged original business_ids Q contains
`receipt_by_receipt_number(ReceiptNumber)` and
`ticket_by_ticket_number(TicketNumber)`; inheritance_business_ids Q contains
`document_by_document_number(DocumentNumber)`. Each composes the normal Q entry,
the existing strongly typed equality predicate and `limit(2)`.

The initial C++ factories lacked all three shortcuts. The
[repair](generated_business_id_lookups_5_0_6.md) qualifies existing typed predicates
and shared limit mechanics against the actual original functions before emitting
thin lowerCamelCase factories. It retains own-field selection, omits descendant-
owned aliases, publishes model-specific Assist and regenerates the artifacts.
The complete SDK proof permits only those delegates and their guidance; existing
implementation bodies remain unchanged.

This finding replaces an unspecified constructor/configuration concern with a
named source responsibility. It does not remove any other requirement or
justify accepting G06 before the full row-by-row review and integrated gates are
complete. The original [source review](parity/general_query_source_review_5_0_6.json)
and constructor checkpoint retain their historical hashes and observations.
