# Complete core Expr source review

Status: EX01-EX12 are accepted for the complete named source, with explicit
C++ Context/range/interface adaptations. Wider capabilities and P0-P7 remain
open. The reference
is Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The finite source scope is all 420 lines of `teaql-core/src/expr.rs`, including
its fifty public methods and private typed-LIKE construction. This file has no
local unit tests. The actual unchanged core crate tests and original-call
observers supply executable evidence; an inventory alone does not.

The [historical expression review](expression_source_acceptance.md) records
5.0.5 evidence. Its eleven-variant model predates 5.0.6's `LikePattern`; it is
not relabeled as acceptance of the current source. SafeExpression evaluation,
SelectQuery's complete source, SQL compilation/execution and governed logging
retain their own responsibilities.

## Whole-source responsibility matrix

| ID | Original responsibility | C++ mapping and evidence |
| --- | --- | --- |
| EX01 | Twelve BinaryOp and fourteen ExprFunction variants | Distinct operators/functions preserve Copy/Clone/PartialEq/Eq and Debug. These source enums have no Default, Hash or serde contract. All generic functions and operators are exercised by actual original calls and diagnostic modes. |
| EX02 | All twelve expression variants | Preserve Column, Value, LikePattern, Function, Binary, SubQuery, Between, IsNull, IsNotNull, And, Or and Not. Recursive ordered children, active variant payloads and complete subquery descriptor/query state remain distinct. Native inactive representation fields do not enter source equality. |
| EX03 | Column and value construction | Preserve complete text bytes, typed Value variants and explicit destination allocation. Existing Value conversion contracts remain dependencies; constructing an expression does not convert a presentation value into database authority. |
| EX04 | Generic and specialized functions | Generic function construction preserves every argument in iteration order, including duplicates and an empty sequence. Soundex/GBK and twelve aggregate wrappers retain their exact function kind and unary/zero-argument shape. Count-all is not count of a fabricated literal. |
| EX05 | Comparisons and operand order | Six field/value comparisons, sound-like, arbitrary BinaryOp construction and column/column comparisons preserve left/right order and node types. Sound-like wraps both original operands in Soundex. |
| EX06 | Typed versus raw LIKE provenance | Six typed helpers concatenate exact prefix/original/suffix bytes and retain the unchanged original in LikePattern. Raw like/not_like use an ordinary Text Value. No wildcard escaping, trimming, Unicode normalization or inferred provenance is added. Both pattern and original participate in structural equality. |
| EX07 | Membership and threshold | Automatic IN/NOT IN select the large representation only when the complete collected input length is greater than twenty. Explicit large constructors stay large for zero/small lists. Collection order and duplicates remain; expression-list logical deduplication must not be applied to membership values. |
| EX08 | Complete subquery preparation | Preserve supplied left/op/descriptor and all query options; replace ordinary projection with exactly the supplied field. Only NotIn/NotInLarge append IsNotNull through the existing conjunction builder. Existing filters survive, including their own structural deduplication. Other operators do not gain the guard. |
| EX09 | Between and unary nodes | Lower then upper Value conversion retains order without sorting or coercing bounds. IsNull, IsNotNull and Not retain the actual operand without evaluating it. |
| EX10 | Ordered logical composition | And/Or retain the first structurally equal input, in order. NaN-containing nodes remain unequal and are not silently deduplicated. and_expr/or_expr return an equal left unchanged, extend only an existing same-kind left, avoid an already present right and preserve a nested right rather than flattening it. |
| EX11 | Full derived models | Expr has Clone/PartialEq/Debug, not Eq/Default/Hash/serde. Derived Debug is a raw structural model protocol, separate from safe SQL/log publication. LikePattern leaf and containing Binary are compared in all nine existing formatter modes; changing either leaf string changes equality. |
| EX12 | Explicit lifetime and frontend integration | Complete existing default-PMR-disabled contracts preserve nested descriptor/query/string/container allocators, independent destination copies, allocation failure recovery and actual User destruction. Same-domain borrows never extend Context lifetime. Imported consumers execute the complete canonical bodies and probes rather than reduced examples. |

C++ `all_of`/`any_of` adapt Rust's keyword-shaped `and`/`or` methods. Resource
arguments belong to these low-level constructors; application Q/E methods keep
their separate bound UserContext and lowerCamelCase contract. C++ generic ranges
copy borrowed inputs into the selected owner and consume owned values according
to the existing scoped range protocol. These are ownership/interface adaptations,
not a new query allocation region or an implicit shared Context owner.

## Actual-source observations

The isolated `rust_core_expression_source_5_0_6` project invokes the unchanged
retained original API and Debug observers, with exact registry versions/checksums
from the reference Cargo lock. The new typed-LIKE observer calls each actual
original helper, inspects its resulting variant and uses derived formatting and
equality. It does not reimplement wildcard construction or trace privacy logic.

The corresponding C++ observer disables the default PMR resource, uses actual
runtime constructors and clones, destroys allocator-aware values before actually
deleting each UserContext and checks that no User bytes remain. Its private
source observation never authorizes raw model diagnostics for publication.

The retained 1,089 query/API inputs include all fifty Expr methods; their complete
expected observations and 120 recursive Debug inputs stay byte-identical.
Additional 162 factory/threshold cases exercise embedded NUL, empty and long
strings, wildcard/control/Unicode bytes, both existing-filter states, NaN and
nested values, all fourteen generic functions and list lengths 0/1/19/20/21/22/41.
Thirty-two typed-LIKE cases cross four column names and eight original strings.
Each checks all six typed helpers, actual pattern/original state, clone equality,
raw-helper distinction, independent original/pattern edits and nine Debug modes
for the complete expression, leaf and raw comparison.

## Acceptance boundary

The [terminal acceptance](parity/core_expression_source_acceptance_5_0_6.json.gz)
joins the complete source review, exact
[original/native/imported replay](parity/core_expression_source_final_5_0_6.json.gz)
and [installed gate](parity/core_expression_source_installed_5_0_6.json.gz).

| Gate | Final evidence |
| --- | --- |
| Actual original tests | All 69 unchanged core crate unit tests pass. This source file has no local tests; the crate total is not presented as 69 Expr-specific tests. |
| Retained observations | All 1,089 API and 120 Debug outcomes reproduce against their immutable source corpora on both C++ frontends. |
| New boundaries | All 162 constructor/threshold and 32 typed-LIKE outcomes agree, including all nine formatter modes and independent provenance edits. |
| Exact corpus replay | All 1,403 outcomes per original/native/imported frontend pass again, reproducing exact new and retained corpus bytes. No result or error normalization is used. |
| Installed contracts | Native 6/6 and imported 6/6, exact required case sets, no skips and stable builds before and after execution. |
| Canonical modules | 934 complete consumer/fixture bodies verify, including four newly installed consumers. Existing bodies are unchanged. |
| Formatting | Pinned format apply/check passes for all 1,682 handwritten C++ files. |
| Preservation | Existing C++ production/test/observer bodies, six native archives, 78 SDKs/3,457 files and 2,201 protected artifacts retain their hashes. |

The source disposition for `teaql-core/src/expr.rs` advances to accepted. Current
source status is **44 accepted / 78 pending / 2 excluded_external**; all seventeen
capability statuses remain unchanged, with eleven qualified and six partial.
Two new native comparisons bring discovery to 746. That count is not a whole-suite
pass. Earlier full native outcomes 735/736 and 718/721 remain failed checkpoints.

No production implementation, public API, generator or generated SDK body changed.
Full SQL execution, protected caller publication, SafeExpression evaluation and
G13/G14/G17/P0-P7 remain separate obligations, including the unanswered source
behavior decisions. No new Java/generation, sanitizer, MIPS, external-provider,
GPU or physical-router result is claimed.

## Reproduce

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --parallel 2 --target \
    teaql_query_model_tests teaql_query_api_tests teaql_query_api_probe \
    teaql_query_debug_probe teaql_core_like_expression_source_probe
cmake --preset modules_native
cmake --build --preset modules_native --parallel 2 --target \
    teaql_module_query_model_tests teaql_module_query_api_tests \
    teaql_module_query_api_probe teaql_module_query_debug_probe \
    teaql_module_core_like_expression_source_probe
replay_directory=$(mktemp -d)
python3 scripts/verify_core_expression_source_5_0_6.py \
    --module-build-dir build_modules_native --record "$replay_directory/source.json.gz"
ctest --preset native --no-tests=error --output-on-failure \
    -R '^(query_model_contracts|query_api_contracts|query_api_rust_parity|query_debug_rust_parity|core_expression_boundaries_rust_parity|core_like_expression_source_rust_parity)$'
ctest --preset modules_native --no-tests=error --output-on-failure \
    -R '^module_(query_model_contracts|query_api_contracts|query_api_rust_parity|query_debug_rust_parity|core_expression_boundaries_rust_parity|core_like_expression_source_rust_parity)$'
```

The reproducer verifies the original checkout and dependency lock before
compiling its observers, waits for successful process completion and compares
every result and corpus byte. The installed receipt retains exact build/test
commands, terminal output, JUnit results and post-test no-work builds.
