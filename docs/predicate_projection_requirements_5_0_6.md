# G07 predicate and projection requirements

This matrix preserves the complete G07 responsibility: negative text predicates,
temporal/range helpers, function projections, aliases and source-defined ordering.
It also reviews the adjacent field-template branches so their guards, operands
and ownership are not inferred from method-name coverage. The locked template is
`request/index.stg` at generator `45e1572c4956ca963410d8ea436ead3c8857087a`;
the runtime is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

The [source review](parity/predicate_projection_source_review_after_alias_fix_5_0_6.json) retains
all 63 original method bodies, their source spans and enclosing conditions. The
count is an inventory. The dispositions and actual execution evidence below
establish behavior; neither a count nor a hash establishes semantic agreement.

| ID | Complete requirement | Source/body disposition and behavioral evidence |
| --- | --- | --- |
| PP01 | Plain, raw and explicit unsafe projections; removal | Non-ID/non-version source branches delegate to projection/raw-projection storage. Scalar removal clears ordinary/raw field entries while retaining independent aliases and aggregates. The selection-scope matrix compares 288 successful executions, complete query/options/relation state, typed availability and safe SQL records. General-query ownership controls exercise transient inputs, explicit copies and retained Global requests. |
| PP02 | Numeric function selection | Both source methods construct an Aggregate, with the canonical field as the default alias and caller text as the explicit alias. They do not construct an expression projection. The source adaptor's nonempty attribute precedence and Boolean/non-Boolean truth rules determine numeric eligibility; float and configured flags are covered by Java/source branch controls. The 4,376 projection/text executions cross all twelve AggregateFunction variants; restored float adapters have a separate 432-case actual SQLite matrix. Unsupported SQLite functions remain errors. |
| PP03 | Grouping aliases and function projections | Grouping appends the canonical group field. An alias additionally appends a column expression; the function form additionally appends its aggregate. Repetition/order remain visible. Scalar aggregation, projection/text and selection-scope matrices compare those complete states and actual compact results. Object grouping/detail loading remains independently governed by G08. |
| PP04 | Every field aggregate/default alias | The eighteen source scalar aggregate conveniences preserve count/sum/avg/min/max and statistical function variants, literal defaults and caller aliases. Field eligibility follows the original guards. The alias matrix includes C++ keyword names, Rust keyword escapes, an ordinary control and an object FK count. All 660 unchanged original/native construction/execution outcomes agree after the literal-alias and metadata/projection repairs. Default result keys remain source data; C++ member escaping and camelCase do not rename them. |
| PP05 | Known/unknown availability | The six source definitions preserve SQL NULL tests across Business-ID, Boolean and ordinary-scalar branches. Ordinary ID/version omissions remain distinct. The 8,316 field constructions and 5,936 typed-schema executions include ordinary/optional availability and selected NULL versus omitted values. Existing C++ Null/NotNull aliases have their explicit native-extension disposition below. |
| PP06 | Boolean predicate vocabulary | Source positive/negative Boolean helpers append equality with actual true/false Values. Human classification and plural predicate naming use the shared naming service. Paired human/non-human generator controls and actual Boolean/optional-Boolean SQLite execution qualify the branch; there is no name-based humanity inference. |
| PP07 | Generic operators and independent criteria | Generic operators consume Value-convertible ranges and delegate to the original FieldOperator mechanics. Version, Boolean, Business-ID and object-reference branches omit these generic methods. Independent factories are Context-bound const methods returning checked Ref<Expr> in C++; their shared operator semantics, exact errors, source panic observations and explicit Global rebinding are covered by the 8,160-case criteria matrix and lifetime controls. |
| PP08 | Scalar/ID/Business-ID comparisons and membership | Source Value variants and conjunction/deduplication remain intact. Ordinary ID has equality/membership while non-ID scalars add ordering/between. Business-ID equality remains strongly typed. The 2,016 scalar comparisons, 1,188 membership observations, 8,316 branch constructions and typed execution matrix cover NULL, mixed values, thresholds and source guard refusals. Independently typed between operands and single-pass input ranges are compiled and exercised. |
| PP09 | Complete text and sound family | All seven string-selected predicates retain ordinary/human naming, text escaping and negative semantics. Large-text/password/source Business-ID branches do not gain the new generic sound operand. Sound accepts source Value variants, while the other text methods accept text. All 4,376 projection/text SQLite outcomes agree; actual Rust E0277 and C++ refusals preserve the absent Option-to-Value conversion. |
| PP10 | Before/after and inclusive DateRange | Source non-ID ordinary scalar methods append less-than/greater-than/between criteria; they do not replace the live-row predicate or silently coerce values to dates. Actual date/timestamp, optional and non-temporal SQLite cases, repeated operations and the historical 144-mismatch repair verify that conjunction boundary. The initial TEXT-schema failure corpus remains separate from successful typed DATE/TIMESTAMP storage. |
| PP11 | All four ordering variants | Ascending/descending and both GBK-marked routes append canonical field order entries for source-selected scalar, ID and version branches. The typed execution matrix covers all four routes, Boolean/optional/temporal carriers and repeated ordering. The locked default GBK SQL compiler passes its argument through; this does not promise GBK collation. |
| PP12 | Naming, binding, trust, guidance and integration | Application methods retain camelCase and the existing C++ escaped-member API. Every dynamic operand/alias belongs to the bound Context; ordinary wrappers expire, and retention uses an explicit destination copy. No field adapter owns an executor or performs implicit SQL. Execution continues through the existing purpose/comment/Context/policy pipeline. Actual sensitive predicate bindings and canonical SQL traces agree in the typed execution matrix. Assist now quotes literal result keys and states the source keyword metadata boundary. Positive/negative compiler, native/module/lifetime, Java and strict generation gates close this interface responsibility; wider execution/policy/storage interactions remain required by their own capabilities and P4/P5/P7. |

## Explicit language and source-failure dispositions

C++ member names must escape C++ keywords. Established methods such as
`sumModelClass()` remain available, while their result key is the exact source
`sum_class`. A Rust keyword can independently produce the literal
`sum_type_field`; that spelling is not converted to camelCase. Explicit aliases
are copied unchanged, including aliases that the selected SQLite backend rejects.

Rust's derived descriptor records `type_field` while its generated query uses
`type`. These are two source responsibilities. The C++ runtime can now construct
a query from generated projection names independently of descriptor identities.
The generated metadata preserves the original member name, and the resulting
unknown-field error agrees with actual Rust. This is a retained source failure,
not successful support for querying that keyword field. Ordinary models keep
using the existing descriptor-driven constructor path.

The earlier C++ surface contains typed convenience methods beyond the source's
selected branch: identity/version selections, typed Boolean/version/ID
comparison/range or membership conveniences, some positive text conveniences and Null/NotNull
aliases. They are native extensions, not proof that Rust admits those calls.
The newly restored generic adapters keep source omissions, typed Business-ID
bounds and genuine negative compiler controls. This disposition preserves existing
public C++ APIs without claiming those extensions as Rust API equivalence.

Inheritance and constant/candidate generation retain G04/G05's separate accepted
model and source-failure dispositions. Their scalar adapters use these same
canonical-name/operator/alias mechanics, including the existing constant-selection
mutation wrapper. Configured primitive mappings retain the reviewed six accepted
storage shapes and early rejection of source-invalid or opaque declarations.
There is no new scalar storage type or provider introduced by G07.

## Capability and phase boundaries

This matrix does not remove remaining G06 general-query/trust/return-mapping,
G08 relationship/aggregate execution, G09 page/stream optimization, G10 dynamic
JSON, G12 complete Assist/assembly or G13-G17 integration requirements. Those
requirements retain their locked source contracts and phase gates. Qualifying
this source-selected field interface does not complete P4, P5 or P7, and no
full-suite, external-provider or physical-device evidence is inferred.
