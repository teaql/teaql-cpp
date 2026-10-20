# Complete expression source responsibility review

Baseline: Rust v5.0.5, commit
`62940ae2b88e7acc32763853b8b4685a01c7e90a`.
This review covers the complete production bodies of `teaql-core/src/expr.rs`
and `teaql-core/src/safe_expression.rs`. This complete source group is accepted on the source tables and current
integrated evidence below. Source inventory and passing SQL queries alone do
not establish these contracts or complete later stages.

## Expression state, factories and traits

| Source responsibility | Preserved C++ contract and evidence |
| --- | --- |
| BinaryOp and ExprFunction | All twelve operators and fourteen functions retain distinct unit values, Copy/Clone, PartialEq/Eq and derived Debug. There is no source Default, Hash or serde implementation. |
| Eleven Expr variants | Column name; Value payload; Function kind/ordered arguments; Binary left/operator/right; SubQuery left/operator/entity/query; Between operand/lower/upper; IsNull, IsNotNull and Not operand; And/Or ordered operands. C++ stores ordered children in args and complete subquery state in a scoped box. Equality compares active source payloads rather than inactive representation fields. |
| All fifty public methods and the threshold | Every constructor/helper is mapped in the factory review. LARGE_IN_THRESHOLD is exactly 20; automatic selection is strictly greater than 20, while explicit large operators stay large even when empty. |
| String helpers and comparison order | Wildcards are concatenated without invented escaping. Column/left/right/lower/upper order is retained; generic BETWEEN accepts source value-convertible inputs and converts lower before upper. |
| Logical construction and composition | Deduplicate each yielded operand in first-occurrence order, retaining source NaN non-equality. Existing left And/Or nodes extend without flattening a nested right operand. Owned ranges consume rejected duplicates; borrowed input copies only retained operands into the destination scope. |
| Subquery preparation | Copy complete metadata/query into the chosen owner; replace ordinary projection with the supplied field. Only NotIn and NotInLarge add IsNotNull to the existing filter. Other query configuration stays separate. |
| Expr derived traits | Scoped Clone, structural PartialEq and complete raw Debug; no source Eq, Default, Hash or serde. NaN-containing expressions remain non-reflexive. Raw diagnostics preserve source shapes and formatter flags and are separate from privacy-filtered logs. |

All fifty methods are covered by actual original calls, not a copied AST builder:
the 1,022-input query/Expr corpus preserves all earlier 980 observations and adds
42 complete-factory inputs. The added cases observe 3,444 complete expressions
with exact Debug, self equality and Clone equality, plus the threshold. The
existing 120-input query/Trace Debug corpus supplies all formatter modes and
recursive state. Native query-model contracts cover nested metadata/query
allocator propagation, independent edits and copy failure recovery. See
[the full factory review](query_api_acceptance.md#complete-expr-factory-observations-and-between-repair-2026-10-08)
and [public diagnostic review](query_debug_acceptance.md).

## SafeExpression and TeaqlEmpty

| Source responsibility | Preserved behavior and explicit adaptation |
| --- | --- |
| Root and evaluator are separate shared state | Context-owned root and evaluator nodes remain separate. Same-owner handle copies share the graph as borrows. Explicit foreign copies clone root and allocator-aware callable graph into the destination; opaque/noncopyable foreign state is rejected without retaining its source implicitly. |
| new, eval and eval_with | Owned construction requires no Clone for R/T; borrowed construction requires an actual destination copy. Each evaluation calls the stored evaluator once; eval_with supplies the given root without replacing the stored root. |
| apply and apply_optional | Ordered mapper nodes preserve owned outputs without imposing Clone. None stops later mappers. Mapping results and captured owned state use the expression allocator. |
| or_else, or_else_with and or_else_throw | Eager fallback ownership, lazy fallback only for None, and lazy structured Result errors retain source branches. Owned noncopyable results/errors remain supported. Callback exceptions propagate. |
| is_null/is_not_null and matching callbacks | Test the outer Option rather than Value's SQL-null category. Each call evaluates once; callbacks run only for their matching branch and consume the present result. |
| is_empty/is_not_empty and matching callbacks | None is empty; Some delegates to TeaqlEmpty. Matching callbacks evaluate once and call the predicate once for a present value. Application-defined predicates use the public C++ ADL extension point. |
| value and root | Identity methods require R=T and source Clone capability, expressed as scoped destination copying. Identity evaluation copies R; root returns a borrow of the stored root. |
| entity_id, entity_version and update_entity_id | Require BaseEntity plus scoped Clone, matching the source block. ID updates map a copy, preserve the original root and retain other entity state. |
| SmartList size, first and get | Declare the source item Clone bound. Size observes length; first/get clone the selected item or return None. These entry points stay separate. |
| Derived Clone and absent traits | Source derived Clone has R/T Clone bounds although Arc payloads are shared; construction/mapping do not. C++ same-domain handle sharing additionally permits noncopyable payloads as an explicit borrow extension. Source has no Default, Debug, PartialEq/Eq, Hash or serde for SafeExpression. |

The six source TeaqlEmpty implementations retain their exact distinctions:
strings and vectors test length; SmartList tests its items; Option tests None
without recursively inspecting Some; Value treats Null, empty text/list/object,
and corresponding empty JSON categories as empty. TypedNull, false, zero,
nonempty containers and other scalar values are not empty.

The 448-input original-call corpus preserves all previous 444 observations and
adds four application-defined TeaqlEmpty cases. It covers all twenty-four
public SafeExpression methods, exact callback order, non-Clone roots/results,
entity ID/version updates, list navigation and missing/present/empty custom
predicates. The four unchanged source unit tests freshly pass 4/4. Native
compile checks enforce positive/negative identity, borrowed-root and list Clone
capability boundaries; existing default-PMR-disabled tests exercise noncopyable
allocator-aware captures, owned fallback/errors and foreign-copy rejection.
See [the ownership and capability review](safe_expression_api_acceptance.md).

## Scope, ownership and integration gate

C++ maps source Arc ownership to the agreed explicit Global/User Context
lifetimes. There is no implicit shared ownership keeping a Context alive.
Context arenas are thread confined; this review does not promise arbitrary
cross-thread access through raw borrows. Owning callback state must propagate
its allocator. Raw pointers/views in roots, results or captures remain native
borrows with an explicit lifetime obligation; copying the containing value does
not copy an arbitrary pointee or extend that pointee lifetime. Same-owner sharing
does not promote borrows into foreign ownership. Temporary owning objects
must be destroyed before manually deleting their arena.

The three mutation map roles, SelectQuery, entity/runtime behavior, SQL executor,
transport/provider, governance and safe logging retain their separate module
gates. This group constructs/evaluates expressions; it does not execute SQL or
validate runtime policy. No dependency or generator edit is introduced.

Current integrated acceptance, 2026-10-08: the initial 572-task native build and
final 327-task consumer refresh both complete successfully; Ninja has no pending
work. Native selected checks pass 12/12 in 2.61 seconds, ASan/UBSan selected
checks pass 12/12 in 14.24 seconds, and MIPS/QEMU passes 12/12 in 11.53 seconds.
The final complete native suite passes 281/281 in 62.11 seconds. Named native
and MIPS probes match all 1,022 query/Expr, 448 SafeExpression and 120 query/Trace
Debug inputs against live unchanged original Rust. All four original source
SafeExpression unit tests also pass 4/4.

The corrected bounded-memory fixture has current passing sanitizer evidence;
its pre-correction destructor use-after-free is not a positive result. Current
sandbox tracing prevents LeakSanitizer, so these ASan/UBSan runs disable leak
scanning and retain explicit arena reclamation assertions. Pinned upstream,
source inventory, independent layer, 451-test-disposition, formatting and
whitespace checks pass. This accepts the complete expression source group;
SelectQuery/row and downstream SQL/runtime/governance/privacy contracts,
physical-router execution and all stage 2–6 acceptance remain open.

## Separate current 5.0.6 source receipts

The preceding review remains historical 5.0.5 evidence. The complete current
[Expr EX01-EX12 review](core_expression_source_review_5_0_6.md) and
[SafeExpression SE01-SE16 review](safe_expression_source_review_5_0_6.md) now
have independent full-source acceptance receipts. Their actual-source outcomes,
current canonical imported bodies, compiler capabilities and explicit Context
ownership contracts are recorded separately. This does not accept complete
SelectQuery, generated E integration, protected callers or whole P0-P7.
