# Complete SafeExpression source review

Status: SE01-SE16 are accepted for the complete named source with explicit
C++ Context/interface adaptations. Whole capabilities and P0-P7 remain open. The reference is Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This covers all 308 lines of `teaql-core/src/safe_expression.rs`: twenty-four
methods, six TeaqlEmpty implementations, application-defined predicates, derived
Clone capability and all four local source tests.

The [historical ownership review](safe_expression_api_acceptance.md) and
[expression source group](expression_source_acceptance.md) retain their 5.0.5
checkpoints. They are not relabeled as current qualification. Current Expr
construction is accepted separately in the
[EX01-EX12 review](core_expression_source_review_5_0_6.md). Generated structured
E diagnostics and business query execution retain independent contracts.

## Whole-source responsibilities

| ID | Original contract | Native mapping and executable evidence |
| --- | --- | --- |
| SE01 | Separate shared root and evaluator | Root and callable state remain separate Context-owned nodes. Same-domain copies share these nodes as borrows; explicit foreign copies allocate complete roots and relocatable callback graphs in the destination. No shared ownership extends UserContext lifetime. |
| SE02 | new without Clone | Owned roots, results and captured state need no copy constructor for ordinary construction/evaluation. C++ allocator-aware movement preserves ownership; borrowed construction requires an actual scoped destination copy. The original non-Clone observer and positive compiler example execute these paths. |
| SE03 | eval and eval_with | Each invokes the evaluator exactly once. Supplied roots affect only that evaluation and do not replace the stored root. Native result handling moves owned values and explicitly copies borrowed callback results into the expression's owner. |
| SE04 | apply | Preserve evaluator-before-mapper order and skip the mapper for None. Owned mapper outputs do not acquire a Clone requirement. |
| SE05 | apply_optional | Preserve optional short circuit through every later mapper; an absent mapped result does not call the remaining chain. Repeated evaluations use the retained evaluator/root graph rather than caching a previous result. |
| SE06 | Eager fallback | or_else receives its already constructed fallback and returns it only for missing evaluation. Owned non-copyable fallback/result values remain supported. |
| SE07 | Lazy fallback and typed errors | or_else_with calls the fallback once only for None. or_else_throw returns typed Result, invokes its error supplier only for None and permits non-copyable error payloads. It is not a throwing business API; native callback exceptions retain normal C++ propagation. |
| SE08 | Outer Option nullness | is_null/is_not_null inspect the evaluator's outer Option. Some(Value::Null), Some(TypedNull) and Some(None) are present evaluations. Callback branches consume a present result or invoke the absent callback exactly once. |
| SE09 | Emptiness methods and callbacks | is_empty treats missing evaluation as empty; present values delegate to TeaqlEmpty. is_not_empty negates that result. when_is_empty/when_not_empty evaluate once and preserve matching-branch behavior. Missing values do not call a custom predicate. |
| SE10 | All six built-in TeaqlEmpty implementations | String and borrowed str check byte length; Vec and SmartList check length; Option checks only None, without recursively inspecting Some. Value's explicit Null/Text/List/Object/JSON branches are distinct from the nonempty scalar/TypedNull fallback. Whitespace, zero, false and nonempty containers containing empty values remain nonempty. |
| SE11 | Application predicate extension | Preserve the public TeaqlEmpty extension point through C++ ADL. The complete existing custom predicate body checks the exact evaluation/predicate/callback counts for absent, empty and nonempty values. Its local counters are synchronous borrows, not retained callback ownership. |
| SE12 | Identity methods | value and root require matching root/result types plus source Clone capability. Identity evaluation clones the root; root returns a borrow. Native scoped-copy capability replaces Rust Clone for destination allocation. Root/value are unavailable for the non-copyable fixture. |
| SE13 | Entity adapters | entity_id/entity_version/update_entity_id preserve the BaseEntity-plus-Clone block. ID updates modify a mapped copy, preserving original root identity and version. All retained identifier/version boundary cases call the actual original methods. |
| SE14 | SmartList adapters | size, first and get all retain the shared item-Clone bound, including size. First/get clone an existing item and preserve None for absent/out-of-range results. Native assertions check each method's availability independently, not just a combined concept. |
| SE15 | Derived Clone and absent traits | Rust's derived Clone imposes both R: Clone and T: Clone even though Arc handles share their nodes. Eight original negative compiler controls distinguish these bounds from ordinary construction and identity/list blocks. C++ same-domain handle copying additionally permits non-copyable payloads as an explicit borrow extension. The source has no Debug, Default, PartialEq/Eq, Hash or serialization implementation to invent. |
| SE16 | Lifetime, failures and current integration | Complete unchanged contracts check allocator-aware captures/results/errors, nested optional ownership, destination-copy failures, registered callback cleanup and Global retention after User close. Canonical imported consumers must execute these full bodies. Raw root/result/capture references remain borrows, invalid after their owner closes; this review does not promise arbitrary stale-pointer interception or cross-thread access. |

## Ownership and interface adaptations

Rust's thread-safe Arc and static callback bounds become explicit Context
allocation, owner-thread access and destination copying. Root, evaluator and
mapper wrappers do not own a Context or keep it alive. Managed state is destroyed
before its arena blocks; native resources retain their actual cleanup contract.
A caller must destroy allocator-aware value wrappers before manually deleting
their storage owner. A captured native pointer remains a borrow; copying its
containing wrapper does not copy or prolong an arbitrary pointee.

C++ apply/apply_optional retain the source wrapper as a borrow rather than
consuming it. Same-domain copies may support non-copyable payloads, while foreign
retention still requires complete root and callback copies. Opaque or non-copyable
foreign state is rejected; failure leaves the original expression usable. These
are explicit C++ interface/ownership extensions, not source Clone capabilities.
The kernel methods retain snake_case; application-facing generated E adapters
keep their separate lowerCamelCase and bound-Context contracts.

## Actual-source and compiler evidence

The isolated `rust_safe_expression_source_5_0_6` project calls the unchanged
retained original observer plus a new direct TeaqlEmpty observer. All dependencies
must match the reference versions/checksums. The retained 448 cases cover general
callbacks, shared root clones, identity, Value/Option emptiness, list navigation,
non-Clone owned state, entity adapters and custom predicate counts. No observer
implements the SafeExpression mapping or fallback algorithms locally.

The 132 new cases separately cover String 16, borrowed str 16, Vec 12,
SmartList 16, Option 18 and Value 54 observations. Each records direct emptiness,
all four null/empty queries, four callback branches and eight evaluations. Static
borrowed-string fixtures remain static data; dynamic strings and containers
belong to their selected Context. Nested Option presence, loaded/unloaded lists,
containers holding empty values, TypedNull, scalar/JSON zero and Unicode/NUL text
retain separate branches. The new native observer disables default PMR through
Global teardown, destroys local values before actual User deletion and checks
that no User bytes remain.

One original positive example executes owned non-Clone construction/mapping,
evaluation/fallback/errors, ordinary Clone/identity and all three copyable-list
adapters. Eight independently compiled examples must fail for their exact source
bound: non-Clone root Clone, non-Clone result Clone, non-Clone identity value/root,
mismatched root/result root access, and each of size/first/get with non-Clone list
items. Native positive/negative declarations are checked when compiling the full
new observer on both header and imported frontends. Expected compiler rejection
is recorded explicitly and is not an unfinished or failed runtime test.

## Acceptance boundary

The [terminal acceptance](parity/safe_expression_source_acceptance_5_0_6.json.gz)
joins the [actual original/native/imported source replay](parity/safe_expression_source_final_5_0_6.json.gz)
and [installed contracts](parity/safe_expression_source_installed_5_0_6.json.gz).
All required build/test processes are terminal and successful. Expected source
compiler rejections are recorded separately with their required bound diagnostics.

| Gate | Final evidence |
| --- | --- |
| Actual source tests | All four unchanged local SafeExpression unit tests pass. |
| Behavior and builtins | All 448 retained and 132 new original outcomes agree on native and imported frontends, for 580 per frontend. |
| Exact corpus replay | Final replay reproduces both complete retained/new corpus bytes without result normalization. |
| Source compiler controls | One positive example compiles and executes; all eight negative examples fail for the exact missing Clone/type capability. |
| Native declarations | Both frontends compile independent positive/negative identity/root and size/first/get availability assertions, plus the disclosed non-copyable handle-sharing extension. |
| Installed contracts | Native 5/5 and imported 5/5, exact case sets, no skips and no-work builds before and after execution. |
| Canonical modules | 938 complete consumer/fixture bodies verify, including three new consumers and their existing shared fixture. Existing bodies remain unchanged. |
| Formatting | Apply/check of all 1,683 handwritten C++ files passes with the pinned formatter. |
| Preservation | Production/test/observer/helper bodies, six native archives, all 78 SDKs/3,457 files and 2,201 protected artifacts retain their hashes. |

The [initial observer failure](parity/safe_expression_source_initial_json_null_failure_5_0_6.json.gz)
retains all 132 mismatches: `serde_json::json!` interpreted the bare token `null`
as JSON null instead of the observer's boolean variable of that name. Every
other output field agreed. Renaming only that local observer variable to
`is_null` exposes the actual result; no original library or C++ algorithm changed.
The incomplete initial replay remains separate from final acceptance.

Exactly `teaql-core/src/safe_expression.rs` advances to accepted. Source status
is **45 accepted / 77 pending / 2 excluded_external**. All seventeen capability
statuses remain unchanged: eleven qualified and six partial. Native discovery is
747 after one new comparison; it is not a whole-suite pass. Historical full
735/736 and 718/721 failed epochs retain their original outcomes.

No production implementation, generator or generated SDK body changed. Whole
generated E, Context/query/save callers, protected publication and
G13/G14/G17/P0-P7 remain independent, with unanswered source-behavior decisions
preserved. No new production dependency, Java/generation, sanitizer, MIPS,
external-provider, GPU or physical-device result is claimed.

## Reproduce

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --parallel 2 --target \
    teaql_safe_expression_api_tests teaql_safe_expression_api_probe \
    teaql_safe_expression_empty_source_probe teaql_entity_expression_tests \
    teaql_core_collection_tests
cmake --preset modules_native
cmake --build --preset modules_native --parallel 2 --target \
    teaql_module_safe_expression_api_tests teaql_module_safe_expression_api_probe \
    teaql_module_safe_expression_empty_source_probe teaql_module_entity_expression_tests \
    teaql_module_core_collection_tests
replay_directory=$(mktemp -d)
python3 scripts/verify_safe_expression_source_5_0_6.py \
    --module-build-dir build_modules_native --record "$replay_directory/source.json.gz"
ctest --preset native --no-tests=error --output-on-failure \
    -R '^(safe_expression_api_contracts|safe_expression_api_rust_parity|safe_expression_empty_source_rust_parity|entity_expression_contracts|core_collection_contracts)$'
ctest --preset modules_native --no-tests=error --output-on-failure \
    -R '^module_(safe_expression_api_contracts|safe_expression_api_rust_parity|safe_expression_empty_source_rust_parity|entity_expression_contracts|core_collection_contracts)$'
```

The reproducer verifies source identity and exact registry versions/checksums,
waits for actual successful original builds before executing their outputs,
checks precise negative compiler diagnostics and compares every observation and
corpus byte. The installed receipt retains commands, complete output, JUnit
results and stable builds. Java/generated-package evidence remains unchanged
and is not presented as newly executed work.
