# SafeExpression ownership and generic API audit

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Original SafeExpression.new does not require Clone for its root or evaluated
result. apply/apply_optional and ordinary fallbacks likewise impose no Clone
requirement on outputs. Source Clone shares root/evaluator Arc handles instead
of copying their payloads, but its derived implementation still imposes Clone
bounds on both R and T. This was confirmed by compiling an original non-Clone
fixture; construction and mapping remain available while clone is rejected.
Value/root and entity/list-item adapters carry their documented stronger bounds.
C++ same-domain handle copying additionally works for non-copyable payloads;
that is an explicit C++ extension, not a claimed original Clone capability.

The earlier C++ implementation always cloned evaluator/mapper/fallback results
and accepted roots only through a const reference. Its allocator-aware callable
clone table also instantiated copying for every allocator-aware capture, even
when only same-domain sharing was requested. These were extra C++ restrictions,
not source responsibilities.

## Application-defined emptiness and current verification, 2026-10-08

The public TeaqlEmpty trait is reviewed as an application extension point, not
only a collection of built-in overloads. Four new actual original-call cases
cover an application-defined non-Clone result whose predicate counts calls.
All earlier 444 inputs are preserved unchanged, for 448 total. Missing values
skip the predicate; present values call it once for each of the four emptiness
entry points. Both callback branches and evaluation counts match unchanged
original methods. C++ supplies the same extension through ADL, with no runtime
registry or dependency; its observation fixture holds native counter borrows
only during its synchronous invocation.

Fresh standalone native contracts pass with default PMR disabled. Native and
MIPS/QEMU match all 448 inputs against live Rust. The final twelve-check
ASan/UBSan selection passes 12/12 in 14.24 seconds and MIPS/QEMU passes 12/12
in 11.53 seconds. These include the corrected logical-budget test teardown:
local owning expressions are destroyed before manual User deletion. The earlier
fixture destructor use-after-free has fresh passing sanitizer evidence after
its correction. Current leak scanning remains unavailable under tracing; runs
use detect_leaks=0 and retain explicit reclamation assertions.

The [complete two-source review](expression_source_acceptance.md) is recorded.
The final native consumer refresh is clean and full regression passes 281/281
in 62.11 seconds. The complete Expr/SafeExpression source group is accepted;
query/row and downstream runtime/privacy and stage acceptance remain open.

## Source Clone capability boundaries, 2026-10-08

The complete original production body places `value` and `root` in the
`R: Clone` identity block, entity adapters in the `E: BaseEntity + Clone` block,
and all three SmartList adapters in the `T: Clone` item block. Ordinary `new`,
evaluation, mapping and fallbacks do not require Clone for their root/results.

Two independent negative C++ compile assertions reproduce false capabilities:
`value` advertises support for an allocator-aware owning root with no scoped
copy constructor, and `is_constructible` advertises borrowed construction from
that same uncopyable root. Both assertions fail before the repair, although the
underlying bodies cannot instantiate the copies they declare. This is separate
from the supported owned-root constructor and same-domain handle sharing.

The borrowed-root constructor now requires an actual destination copy. Identity
factory/root access require matching R/T and that same scoped copy capability.
Entity and SmartList adapters declare their complete source Clone boundaries
using destination-copy capability, including `size` in the source's shared
list-item block. Positive/negative compile contracts retain scalar identity and
copyable-list adapters, reject uncopyable identity roots and list items, and
retain the existing noncopyable owned-root/result/capture runtime contract.
The four unchanged original SafeExpression module tests freshly pass 4/4.

C++ maps source thread-safe Arc ownership to explicit thread-confined Context
borrows; copying a handle cannot prolong its owner's lifetime. Explicit foreign
retention still requires complete destination copies of root and callback state.
No source evaluator/mapper callback branch, output ownership, model capability
or stronger Clone block is removed by the declaration repair.

Fresh native full and selected ASan/UBSan/MIPS consumer rebuilds are running.
Current leak scanning cannot execute under sandbox tracing, so new sanitizer
runs use `detect_leaks=0`; earlier leak-enabled checkpoints below remain
historical evidence. Full source-group and stage acceptance remain open.

## Implemented repair

- The owned-root constructor moves a root into its explicit Context; the existing
  borrowed constructor still copies into that Context.
- Evaluator, mapper and lazy fallback outputs move when owned. Explicitly
  borrowed C++ callback outputs still copy into the expression's domain.
- Evaluator/mapper state has allocator-aware movement, admitting non-copyable
  captured state without silently allocating it outside the Context.
- Same-domain expression/callable copies share registered state as borrows.
  Explicit cross-domain retention requires an allocator-aware copy; unavailable
  root/callback copying is rejected instead of imposing compile-time Clone
  requirements on all same-domain use.
- Eager fallback moves its owned argument when needed. Structured Result errors
  may also be owned and non-copyable. Lazy callbacks retain their source branch
  and evaluation behavior.

No shared ownership extends C++ Context lifetime. Root/evaluator/mapper state
remain registered in their Context, and all result strings/containers use that
owner. Context close invalidates expression borrows. Arbitrary captured raw
pointers and native references retain the existing lifetime contract.

## Current evidence and open acceptance

The native C++20 contract in `teaql_core/tests/safe_expression_api_tests.cpp` runs with
usable default PMR disabled. It uses non-copyable, non-assignable owning roots,
results and errors, plus non-copyable allocator-aware captures. It checks owned
construction, same-domain sharing, apply/apply_optional, eager and lazy fallback,
structured errors and foreign-root rejection. A separate copyable-root fixture
checks rejection of foreign non-copyable callback state. Rejected copying leaves
the original expression usable. This contract is registered as
`safe_expression_api_contracts`; its observation probe is registered as
`safe_expression_api_rust_parity`.

The 444-input corpus matches live original Rust on native C++. Its 396 general
inputs observe evaluation/mapping/fallback/error/branch methods, exact callback
order, identity roots and shared clones, Value/Option emptiness and list
size/first/get adapters. Twelve additional fixtures call unchanged original
methods with non-Clone roots/results/captures. Thirty-six entity fixtures cover
ID/version adapters and independent ID updates, including full i64 version
boundaries. The four unchanged original SafeExpression module tests pass 4/4.
The original non-Clone fixture does not call clone: its derived bounds remain
stronger than ordinary construction/mapping.

Acceptance on 2026-10-07, after all builds and tests completed:

| Configuration | Result |
| --- | --- |
| Native Debug, rebuilt consumers and full CTest | 204/204 checks passed. |
| ASan/UBSan with leak detection | Six selected checks passed 6/6. |
| OpenWrt MIPS cross-build and QEMU 74Kf | The same six checks passed 6/6. |
| Native and MIPS probes against live unchanged Rust | All 444 observations matched in each configuration. |

The six checks are `entity_expression_contracts`, `core_collection_contracts`,
`safe_expression_api_contracts`, `safe_expression_api_rust_parity`,
`context_memory_contracts` and `context_reference_contracts`. Existing
entity-expression contracts retain branching, root evaluation, domain-copy and
entity/list coverage. Every selected target was rebuilt before its checks ran.
Pinned upstream verification, public declaration verification, the 58-group
inventory check and clang-format checks also passed; declaration/inventory
checks alone do not prove behavioral parity.

```sh
cmake --build build --target teaql_safe_expression_api_tests \
  teaql_safe_expression_api_probe teaql_entity_expression_tests \
  teaql_core_collection_tests teaql_context_memory_tests teaql_context_reference_tests
ctest --test-dir build --output-on-failure \
  -R '^(safe_expression_api_contracts|safe_expression_api_rust_parity|entity_expression_contracts|core_collection_contracts|context_memory_contracts|context_reference_contracts)$'
python3 tests/parity/compare_safe_expression_api.py \
  --cpp build/teaql_safe_expression_api_probe \
  --rust build/rust-original-roundtrip/debug/safe_expression_api_oracle
```

Use the respective sanitizer/MIPS build directories for the same target/check
selection. Set `ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1` for
sanitizer CTest. For the MIPS live comparison, also pass
`--cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'`.

Complete SafeExpression/public model and all-stage acceptance is not established
by this checkpoint. No generator or production dependency changed. MIPS
validation uses QEMU rather than the physical router.
