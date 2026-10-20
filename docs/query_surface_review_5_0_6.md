# General query and scalar surface review: Rust 5.0.6

The [complete G06 requirements](general_query_requirements_5_0_6.md) and
[97-function source review](parity/general_query_source_review_5_0_6.json)
now assign every generic request responsibility explicitly. The
[constructor/selection guard matrix](general_query_guards_5_0_6.md) supplies
1,400 exact outcomes including source Default and no-forward/one/many branches.
The review identified a missing Q Business-ID lookup family. The subsequent
[typed factory migration](generated_business_id_lookups_5_0_6.md) restores those
adapters and their model-specific guidance. G06 is now qualified by the
[complete acceptance review](general_query_acceptance_review_5_0_6.md), including
all twelve requirements and the final integrated gates.

Current status: G06 and G07 are qualified by their complete general-query and
[field-interface acceptance](predicate_projection_acceptance_5_0_6.md).
Earlier checkpoint counts and open-scope statements below remain historical;
other capabilities and P4/P7 retain their independent requirements.

This review turns remaining G06/G07 scope into named source responsibilities.
It is an audit index, not semantic acceptance or a reduced phase exit criterion.
The locked source is generator/src/main/resources/generator/rust-lib-core/request/index.stg at
45e1572c4956ca963410d8ea436ead3c8857087a; its full source hash/anchors remain
in the capability ledger. Related G04/G05/G08/G09/G10/G14/G17 responsibilities
retain their own required integration gates.

| Requirement | Source selection / current evidence | Remaining review |
| --- | --- | --- |
| Default/minimal/descendant request and return mapping | qMethod and request return_type; inherited/return-type/constructor comparison families. | Confirm complete generic Q mappings and all admitted configured types under the shared acceptance review. |
| Query and independent state reads/conversions | request query, relation collections, options and direct/From conversions; request-state and core-query matrices. | Preserve the explicit coreQuery snapshot ownership/name adaptation and integrated query() caller boundary. |
| General Boolean composition, text search and property operators | request search/filter/and/or/append/filter_property; general/field-criteria corpora and actual query baseline replay. | Complete combined source body/guard review, including type-group/raw/option interaction. |
| Deleted-row modes | request exact version conjunct removal and deleted-only insertion; general corpus. | Complete supported model/configuration integration, with policy/intent and physical execution boundaries. |
| Matching and enhancement | request matching_any_of/match_any_of/enhance_child; matching/independent-domain/inheritance matrices. | Keep source request/core/selection conversion differences and complete policy/provenance integration. |
| Trusted raw SQL, properties and projections | request raw/unsafe variants plus fieldMethods raw projection and removal. | Complete grouped/derived/optional/inherited/configured privacy and execution review; trust helpers are not authorization. |
| Self/all/children selection and removal | select_self/all/children/any and fieldMethods/objectStorage removal. | Complete source guard mapping with scalar/object aliases and interactions; previous shortcuts remain distinct from subtype enhancement. |
| Scalar predicate branch guards | fieldMethods excludes version; separates Business-ID, Boolean, ordinary scalar and ID comparison/range omission. | Review every emitted source-selected declaration, typing disposition and legacy native extension without calling method counts parity. |
| Generic/operator/independent criteria and temporal range | fieldMethods generic constructor, create criteria, comparison, DateRange, before/after; 8160 old criteria cases. | Complete source field/type/configuration and combined physical execution review. |
| Scalar IN/NOT IN conversions | fieldMethods ordinary scalar IntoIterator; restored constrained range adapters and 1188 actual original/native cases. | Retain separate typed Business-ID and object-reference contracts; do not infer all predicate typing from membership. |
| Text predicates and sound matching | string-selected contain/not contain/start/end/sound methods. | Complete field naming/type guard, prepared SQLite behavior and privacy/provenance combinations. |
| Projection functions, aliases and ordering | numeric function-selection branch, all scalar ascending/descending/GBK methods. | Close each source-selected function and alias/removal/order combination; keep explicit SQLite unsupported operations. |

Acceptance must compose actual source bodies, positive/negative application
contracts, source execution and appropriate Context/module checks. A method
inventory, successful generation or a passing older corpus does not satisfy the
complete P4 exit criteria. Pages/streams/count protocols remain G09; aggregates,
facets and dynamic/storage execution remain G08/G10/G16. Their interaction with
these general Q surfaces remains required for P7.

The [result-marker guard review](query_result_marker_5_0_6.md) found a concrete
G06 discrepancy: original ordinary requests admit non-Entity markers,
while the complete public purposed implementation requires Entity for all eight
execution entries and creation. Shared runtime and the
[generated facade migration](generated_query_result_marker_5_0_6.md) now preserve
this boundary, with exact SDK body preservation and positive/negative compiler
checks. Internal scalar helper bounds must not be mistaken for public API
availability. This closes the named guard discrepancy, not full G06 acceptance.

The [field-branch repair](query_field_branches_5_0_6.md) adds a complete rooted
original domain, 8,316 exact scalar-branch constructions and 432 restored float/
reference constructions. It closes the eleven demonstrated declaration omissions
and preserves method-specific original compiler refusals. The final 755-name
inventory includes asynchronous execution names but does not qualify their bodies.
Legacy native convenience extensions, broader field/configuration combinations
and physical execution of restored numeric methods retain explicit review.

The [actual numeric/reference SQLite checkpoint](query_field_sql_5_0_6.md)
subsequently qualifies 432 complete execution outcomes, including 108 successes
and exact unsupported-function, invalid-alias and binding failures. It does not
waive the remaining combined configuration, policy, privacy or typing review.

The general match_types input bound had a concrete omission despite complete
method-name inventory: only Value spans were callable in C++. Constrained
Context-bound input ranges now preserve original IntoIterator conversion through
thin generated delegates. All198 original/native outcomes, explicit source
internal_type metadata failures, positive/negative compiler and Context/Global
lifetime controls qualify that scoped responsibility. See
[type-matching input ranges](generated_type_match_ranges_5_0_6.md). Other rows and
complete G06/G07/P4/P7 integration remain required.

The complete ordinary selection family now has fourteen original body reviews
and288 actual successful original/native SQLite outcomes covering self aliases,
all/children/any, scalar versus object removal, raw/readonly/group aliases,
explicit child options, search and deleted-row composition. Full query/options,
330 typed JSON/load-state snapshots and468 safe SQL records agree. The unchanged
892-case general observer requalifies with reference registry identities. See
[selection scope](generated_selection_scope_5_0_6.md). The named relationship
version boundary is now explicit in generated Assist. Broader model/guard and
complete G06/G07/P4/P7 integration remain required.

The enhance_child Into<QuerySelection> bound had a concrete cross-domain/value
conversion gap. Shared runtime now snapshots complete explicit selections/core
queries and accepts checked unpurposed children with another executor; thin
adapters use public runtimeQuery conversion. All786 source/native outcomes match,
including18 real successful compact queries. The controlled134 public-access
replacements and268 exact value delegates leave every other SDK body unchanged.
See [enhancement input conversions](generated_enhancement_inputs_5_0_6.md).
Other typed relation-selection/matching/analysis conversion families retain their
own original Into bounds and require review; they are not waived by this repair.

The [relation selector input checkpoint](relation_selection_inputs_5_0_6.md)
restores original Into<QuerySelection> inputs through the shared runtime and
thin forward/reverse generated adapters. All1,152 original/generated SQLite
executions agree; separate original/compiler proof accepts unrelated entities
and reverse construction. Native4/module2/SAN2/compiler3-positive3-negative/
Java59,24-workspace288-file/44-SDK strict reproduction and669 canonical consumers
pass. Controlled456 additions and76 target-specific delegation replacements
preserve every unrelated body. The full capability/phase requirements remain;
matching, grouping, facet and relation aggregate conversion bounds are next.

The [relation analysis runtime checkpoint](relation_analysis_inputs_5_0_6.md)
restores the kernel input conversions of facets, object grouping and relation
count/detail aggregates. All 1,944 original/native outcomes agree, including
360 original intent failures. It repairs 24 nested graph omissions through
existing scoped read views while preserving forward NULL/NotLoaded behavior.
Native 9/module 3/SAN 4, canonical 671/exports 1,485/format 1,455 pass.
This is runtime-first evidence: generated delegates, Assist and controlled
regeneration/compiler/Java integration remain required. Matching, roll-up and
field-statistic conversion families and full capability/phase exits remain open.

The [generated relation-analysis checkpoint](relation_analysis_adapters_5_0_6.md)
adds original request/selection/core inputs for facets, object grouping and
relation count/details through thin delegates. All 1,944 original/generated
outcomes agree; 360 source intent failures remain failures. It fixes both
reproduced NULL-facet loss and repeated-builder view reuse. Native 9/SAN 6/
module 5/compiler 3-positive-3-negative/Java 60, strict 24-workspace288-file/
44-SDK reproduction and canonical 673/exports 1,485/format 1,457 pass.
Exactly 1,824 delegates are added and every old SDK body is preserved.
Matching, roll-up and field-statistic conversions and full phase exits remain.

The [matching/roll-up/field-statistic checkpoint](matching_statistics_inputs_5_0_6.md)
closes the demonstrated input-bound omissions using shared runtime mechanics
and thin generated delegates. All 2,808 complete original/generated outcomes
agree, including 816 source failures; full filter sidecars and expected target
descriptors are retained. Native 7/SAN 3/module 3/compiler 1-positive-3-negative/
Java 61, strict 24-workspace288-file/44-SDK reproduction and canonical 676/
exports 1,485/format 1,460 pass. Exactly 1,500 new delegates preserve all old
SDK bodies. Other rows, complete model/configuration/governance/privacy and
phase integration remain required; no full G06/G08 or P4/P5/P7 claim follows.

The [scalar predicate SQLite execution matrix](query_predicate_execution_5_0_6.md)
now supplies actual compact-row and typed-list results for all seventeen scalar
branches in the rooted field fixture, including optional date/timestamp fields,
Boolean availability, DateRange, repeated methods and all four order variants.
The typed schema is a controlled DATE/TIMESTAMP variant of the retained TEXT
fixture; both original behaviors are preserved. It closes the named lack of
physical optional/temporal/Boolean branch evidence in this fixture. It does not
close arbitrary configured types, source/native extension dispositions or the
remaining composed policy/raw/projection/page/stream requirements.

The [composed general-query checkpoint](composed_general_queries_5_0_6.md) now
combines actual trusted raw statement/filter/projection and readonly/facet/grouped
shapes with application Policy, inherited private fields, all eight execution
entries and ordinary/ID-set/continuous configuration. Its 4,864 source/native/module/
instrumented outcomes preserve all declared business/Policy/SQL fields and repair
sixteen raw-statement intent exposures in shared runtime scopes. The independent
denied-I/O and intent checks do not make trusted SQL an authorization rule. Full
source/guard/model and capability/phase closure remains separate.
