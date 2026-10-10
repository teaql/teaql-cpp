# Core query API acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
This checkpoint restores public iterator/factory capabilities in core Expr,
SelectQuery and AggregationCacheOptions. It does not substitute SQL execution
tests for their model contracts or establish complete query/runtime acceptance.

## Full query-builder and helper comparisons, 2026-10-08

The complete query.rs production body is reviewed independently of expressions.
Fourteen direct builder inputs, fourteen direct helper inputs and thirty-nine
actual original validation calls expand this corpus from 1,022 to 1,089,
preserving all old observations unchanged. Each builder input records 57 direct
mutations plus construction/prepared state, for 826 complete query states with
exact Debug/self/scoped-Clone comparisons. Each helper input calls all thirty-six
listed constructors/factories and observes default constants. Validation cases
preserve exact original message and precedence for Unicode-blank names, embedded
NUL, zero TTL/maximum and zero hard limit. List preparation compares complete
recursive output and exact outer/nested ceiling failures.

All 1,089 inputs match live original Rust on native and MIPS. The row corpus also
matches all 401 inputs, including the final actual key/value iterator and JSON
projection observations; all previous 356 sequences are retained. Final ten-check
native, ASan/UBSan and MIPS selections pass 10/10, including public query/row
Debug. Native/MIPS also match all 169 row/list and 120 query/Trace Debug inputs.
The complete [query source review](query-source-acceptance.md) is accepted
after the final full native suite passes 281/281 in 72.92 seconds and the build
is clean. All stages 2–6 stay open.

## Current expression verdict, 2026-10-08

The complete expr.rs/safe_expression.rs source group is accepted on its
[full responsibility review](expression-source-acceptance.md). The final native
consumer refresh is clean and the complete suite passes 281/281 in 62.11 seconds.
Native, ASan/UBSan and MIPS selections each pass 12/12; current leak scanning
is unavailable under tracing. Named native/MIPS probes match all 1,022 query/Expr,
448 SafeExpression and 120 query/Trace Debug inputs against live Rust.

This verdict accepts expression responsibilities only. SelectQuery, all helpers,
CompactRow/layouts and JSON projection in the separate query.rs group remain
partial. Its complete body review must still close every helper/default/trait/
composition/ownership gate. Earlier checkpoints below describe their own state.

## Incremental logical construction repair, 2026-10-08

Reading the entire pinned expr.rs production body confirms that `and` and `or`
compare and retain each yielded operand before advancing the iterator. The C++
span overload already follows that order, but the generic range overload first
collected every operand into the destination Context and only then deduplicated.
That temporary collection imposed an avoidable linear payload/capacity cost.

A production native contract reproduces the failure: a single-pass transformed
range yields 1,000 references to the same Global-owned equality operand containing
an 8,192-byte string. Both logical builders need one retained operand, but the
old implementation exhausts a 64 KiB User byte budget. The registered
`query_api_contracts` check fails with `Context byte budget exhausted` before
the repair.

The range implementation now compares each yielded operand immediately and
copies only retained borrowed operands into the explicit destination owner.
Owned ranges move each yielded operand into a temporary, retain unique operands
by movement, and destroy rejected duplicates before advancing. This preserves
consumption of duplicates instead of leaving their payloads in the input. The
algorithm neither reserves capacity from total input length nor flattens nested
logical expressions; first-occurrence order and Value's non-reflexive NaN
comparison still follow the original implementation.

The native contract covers both AND and OR under the same budget, all 1,000
iterator yields, the surviving nested allocator/content, actual User deletion
and unchanged borrowed Global input. A separate same-owner owned range checks
node-address transfer and consumption of both retained and rejected operands.
Default PMR remains disabled. This repair introduces no dependency, source
concept reduction or generator change.

Fresh full native and selected sanitizer/MIPS consumer rebuilds are running.
The earlier 281/281 full native checkpoint predates these header changes and
must not be used as their integration result. Complete Expr/SafeExpression
source-group acceptance remains open pending all API/default/trait contracts.

## Complete Expr factory observations and BETWEEN repair, 2026-10-08

The complete pinned Expr body has 50 public methods, twelve BinaryOp variants,
fourteen ExprFunction variants, eleven Expr variants and LARGE_IN_THRESHOLD=20.
The complete source-method review adds 42 inputs without changing any of the
previous 980 observations, for 1,022 total. Each added input calls actual original
factories and records 82 complete expression states plus the threshold. Their
3,444 states supply exact Debug, self PartialEq and explicit Clone PartialEq:
10,374 observations including the per-input threshold. NaN remains non-reflexive.

| Source responsibility | Original-call observation |
| --- | --- |
| column/value and six scalar comparisons | Empty and Unicode/NUL column names; all fixture Value variants, signed/unsigned limits, NaN, Decimal and recursive containers. |
| function and all fourteen named unary helpers, count_all, sound_like | Existing arbitrary-function/argument corpus remains; new cases call each named helper directly and record complete nested operands. |
| like/not_like and six contain/begin/end helpers | Empty, Unicode/NUL, percent, underscore, backslash and long input patterns; wildcard concatenation is preserved without inventing escaping. |
| binary and compare_columns | All twelve BinaryOp values, independently retained operand order and complete resulting states. |
| in_list/not_in_list and explicit large variants | Previous original cases retain sizes 0, 1, 2, 20 and 21, all Value kinds, duplicates and strictly-greater-than-20 automatic selection. |
| subquery/in_subquery/not_in_subquery | Every BinaryOp, absent/present initial filter, old projection replaced by the supplied field, preserved comments and complete metadata/query. Only NotIn/NotInLarge append an IsNotNull constraint. |
| between/is_null/is_not_null/negate | Mixed Value bounds and ordinary scalar/text bounds; complete three-operand ordering and distinct unary variants. |
| and/or and_expr/or_expr | Existing full collection comparisons and new equal/self, existing left logical nodes and nested right logical operands. Preserve first occurrence, left-only extension and no right-side flattening. |

A minimal public C++ fixture reproduces a missing source capability:
`Expr::between(owner, "age", 18, 25)` fails to compile because only the
const-Value overload exists. The generic overload now constructs both bounds
through the same explicit-owner Value conversion as scalar comparisons. It
builds the column, lower bound and upper bound in that sequence. The const-Value
overload remains available; owned same-domain string bounds transfer their
buffers instead of adding a Clone requirement.

Native default-PMR-disabled contracts check scalar BETWEEN, two 8,192-byte
string buffer transfers, each conversion once, lower-before-upper conversion,
and no upper evaluation after an injected lower conversion failure. This is
source generic-factory coverage, separate from SQL BETWEEN execution.

The fresh standalone native contract passes both incremental logical and BETWEEN
checks. The fresh standalone native probe matches all 1,022 inputs against live
unchanged original Rust, including every added state. Current named native,
sanitizer and MIPS rebuilds remain in progress. The final consumer refresh must
include the latest BETWEEN declaration and probe edits before full regression
or source-group acceptance can be claimed.

### Integrated MIPS and fixture-cleanup follow-up

The final named MIPS/QEMU build passes all twelve selected expression/core/
lifetime checks in 11.56 seconds. Its fresh probes match all 1,022 query/Expr,
444 SafeExpression and 120 query/Trace Debug inputs against live unchanged Rust.
Native full consumer and final selected sanitizer integration remain running.

A subsequent fixture-lifetime review finds that the new budget test manually
deletes User while local owning Expr/vector values are still in lexical scope.
Running the already-built sanitizer binary independently reproduces a
heap-use-after-free in their later destructors. This is a test-fixture teardown
error, not evidence of successful sanitizer acceptance. The corrected fixture
ends the expression scope before deleting User. The rebuilt named MIPS
query API contract now passes after that fixture correction (0.39 seconds),
and the fresh standalone native contract also passes. Final sanitizer and full
native integration remain running; complete group/stage acceptance stays open.

### Final selected sanitizer and custom-predicate integration

The final ASan/UBSan selection passes all twelve checks in 14.24 seconds after
the fixture correction. MIPS/QEMU also passes 12/12 in 11.53 seconds after
adding the application-defined TeaqlEmpty fixture. All 448 SafeExpression
inputs match live original Rust on native/MIPS, preserving the earlier 444;
query/Expr and Debug corpora stay at 1,022 and 120 respectively. The earlier
fixture destructor use-after-free is closed by the fresh passing sanitizer
contract; current runs do not include leak scanning under sandbox tracing.
Full native consumer refresh/regression and complete group acceptance remain open.

## Public input and cache responsibilities

| Original entry | C++ entry and ownership |
| --- | --- |
| SelectQuery.projects accepts any IntoIterator of string-convertible names | projects accepts input ranges as well as the existing initializer-list overload. Snapshot names into the query owner before growing projection. |
| Expr.function accepts IntoIterator<Expr> | Owned ranges move elements; borrowed ranges copy into the explicit owner. Preserve order and duplicates. Existing span overload remains. |
| Expr.and / or accept IntoIterator<Expr> | all_of / any_of retain C++ keyword renaming, arbitrary input ranges, first-occurrence deduplication and source NaN non-equality. |
| in_list / not_in_list / in_large / not_in_large accept IntoIterator<Value> | Both borrowed and consuming ranges are supported. The automatic threshold remains strictly greater than 20; explicit large variants preserve their selected operation even when empty. |
| AggregationCacheOptions.enabled | for_duration constructs the standalone options. Rename is required because C++ cannot give a field and static method the same name. |
| AggregationCacheOptions.propagate | with_propagation sets the independent propagation flag and duration without changing enabled or the original duration. SelectQuery delegates to these same operations. |

The four cache fields remain public and separate; no cache service, TTL clock or
backend behavior is introduced here. SelectQuery's other fields/defaults and
method contracts retain their existing native and runtime evidence. These
factory tests do not establish complete audit of every query/Expr member.

Range collection uses the explicit destination resource, including nested Expr
and Value payloads. Single-pass input requires neither random access nor a
second traversal. Borrowed projection ranges may reverse or alias the target
projection; snapshots prevent growth invalidation. Failed snapshot preparation
does not publish a partial projection. Ordinary custom moving iterators retain
their C++ invalidation/exception contract; no universal transactional move or
raw-pointer lifetime guarantee is implied.

Owned IN-list input transfers the collected Value::List directly into the
literal rather than copying it again through a borrowed-list constructor.
Char buffers and vector capacity remain payload allocations, not new public
object identities. Global retention still requires explicit destination copying.

## Source and native evidence

`query_api_cases.json` records 980 unchanged original public-call inputs:
fourteen Expr functions, all Value variants selected in the generator and
list sizes 0, 1, 2, 20 and 21. Projection calls retain duplicates, empty/Unicode
names and ordering. Standalone cache construction/propagation covers zero,
ordinary and maximum u64 values. Every input compares complete source query
Debug at construction and after each projection/cache edit, full function and
logical Expr trees, and all four IN variants. Expectations come from original
public methods rather than copied builders or SQL algorithms.

Native checks disable default PMR and exercise non-contiguous reversed ranges,
single-pass streams, move-consuming Expr input, borrowed foreign Value input,
standalone cache operations and source User deletion before reading a retained
Global query. Existing query_model_contracts cover complete fields, nested
allocator propagation, independently edited copies, subquery metadata, list
preparation and allocation-failure recovery. Keep those tests alongside the
new range cases; export names alone do not establish the contracts.

The Rust Expr and SelectQuery models do not define derived serde. Test JSON is
an observation format, not an invented query wire protocol. Query Debug values
are raw diagnostics; they do not replace safe SQL/audit/log projections.

## Reproduction and remaining gates

```sh
CARGO_TARGET_DIR=/home/philip/shared-cargo-target cargo build \
  --manifest-path tests/parity/rust-values/Cargo.toml \
  --bin query_api_oracle --locked --offline
python3 tests/parity/generate_query_api.py \
  --rust /home/philip/shared-cargo-target/debug/query_api_oracle
cmake --build build --target teaql_query_api_tests teaql_query_api_probe \
  teaql_query_model_tests
ctest --test-dir build --output-on-failure \
  -R '^(query_api_contracts|query_api_rust_parity|query_model_contracts)$'
python3 tests/parity/compare_query_api.py --cpp build/teaql_query_api_probe \
  --rust /home/philip/shared-cargo-target/debug/query_api_oracle
```

Checkpoint on 2026-10-07: the three selected native checks passed 3/3, and all
980 observations matched live original Rust. The same three checks passed 3/3
under OpenWrt MIPS/QEMU, and all 980 MIPS observations also matched live Rust.
The same three checks passed 3/3 under ASan/UBSan with leak detection.
The complete native consumer rebuild and full CTest run completed successfully:
202/202 tests passed, including consumers of the SafeExpression callable repair.
After registering the two additional SafeExpression checks, full native CTest
also passed 204/204.
Complete core API/default/diagnostic and broader
query/save/cancellation/privacy/source-test acceptance remain required.
The generator and original reference remain unchanged; no production dependency
is added. MIPS validation uses QEMU, not the physical router.

## Production Debug acceptance gap identified on 2026-10-07

The original-call observations above use production private non-pretty cache-key
helpers and prove their compact shapes. They do not provide the public generic
Debug interface or inherited formatter options. A correct allocator-aware
SelectQuery construction followed by debug_string(owner, query), using core.hpp,
fails to compile because write_debug lacks the query overload. Original Expr and
all derived query model diagnostics still require a production implementation
with complete fields, declaration order and inherited formatter options. Keep
this gate open alongside the existing structural/ownership acceptance. Raw
model diagnostics do not replace privacy-filtered SQL logs.

The previously missing public formatter is now implemented by
[the query Debug kernel](query-debug-acceptance.md): twenty source-type overloads
and all nested formatter options, with default cache-key wrappers delegated to
the same implementation. Its initial native comparison matches 120 original
inputs and 27,441 outputs. Fresh integration passes native 226/226 and ten selected sanitizer/MIPS
checks 10/10 each. Old/new native query probes match all 980 inputs, and old/new
MIPS key probes match all 451 target-width observations, closing this public
diagnostic gate while surrounding query/API/default acceptance remains open.
