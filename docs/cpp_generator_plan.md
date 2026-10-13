# C++ generator and example integration

Remaining alignment follows [cpp_generator_parity_plan.md](cpp_generator_parity_plan.md).
The Context-bound Q/E milestone is complete; full Rust generator parity remains active.


The next implementation follows
[query_expression_change_plan.md](query_expression_change_plan.md). Qualify the
Context-bound Q/E runtime and handwritten application syntax first, then migrate
the independent cpp_support generator to thin adapters and camelCase application
APIs. The initial frontend sequence/checkpoint below describes earlier work;
it does not establish acceptance of the new factory or binding contract.

## Authorized scope and source checkpoints

The current source reference is producer commit `45e1572c4956ca963410d8ea436ead3c8857087a`
from the Rust 5.0.6 retained fixture. The workspace base stays `77b301e9`; the
source reference and implementation ancestry are separate.

The user authorized generator development on 2026-10-11, superseding the earlier
instruction to defer it. The original generator checkout stays unchanged.
An independent clone lives at `/home/philip/githome/teaql_code_gen_cpp` on
`cpp_support`, based on `77b301e9` from `teaql-code-gen`.
The C++ runtime checkpoint is `0.0.3` (`42e688ee`), with the historical Rust v5.0.5 qualification. The active alignment reference is
now Rust v5.0.6, pinned by `upstream.lock.json`. Existing Rust templates are the feature and
naming reference; the runtime source baseline does not change.

## Implementation sequence

1. Add a C++ core generation scope to the existing Java generator discovery and
   model parser. Use centralized pluralization and Java predicate semantics.
   Keep existing language generators unchanged and avoid new dependencies.
2. Generate allocator-aware entity models, descriptors, mappings, relation
   storage, mutation tracking and application setup for the SQLite runtime.
   Preserve UserContext allocation and explicit destination copies.
3. Generate typed Q requests and E accessors, covering scalar predicates,
   projections, ordering, relations and staged purpose/execution. Execution
   takes only UserContext, resolves the registered executor there, and rejects
   blank purpose/comment. Preserve NotLoaded versus legitimate Null in accessors.
4. Generate a reproducible example fixture and add a runnable example using
   these APIs for real SQLite query/graph-save behavior. Test generated output,
   naming, positive/negative type checks, intent, result ownership and module
   consumption. Retain original handwritten examples as semantic baselines.

Generation is a development step. Checked-in artifacts let ordinary CMake
builds run without Java/Maven, a generator service, network or new runtime
libraries. Generated C++ follows the pinned formatter and underscore file names.

## Acceptance boundaries

Compare actual generated APIs and behavior, not just text inventories. Record
which Rust generator capabilities are implemented and which remain deferred;
do not call the complete Rust generator surface supported without evidence.
Web/cloud adapters, other databases and app frameworks are outside this scope.
Run native relevant checks by default; use targeted sanitizers for new allocator,
borrow and lifecycle paths. Do not repeat MIPS/QEMU without target changes.

## Initial frontend checkpoint

Steps 1–4 are implemented and qualified within the explicit feature scope in
[cpp_generator.md](cpp_generator.md). The independent generator branch adds the
C++ scope without changing existing Rust templates or introducing dependencies.
Both model fixtures reproduce byte for byte; generated APIs drive a real SQLite
graph/query/expression example and pass positive/negative compiler, ownership,
intent, cancellation and module-consumer checks. Full Rust generator parity is
not claimed: unsupported inheritance/constant shapes fail generation, and the
feature table records deferred APIs. See [the evidence](../measurements/generator/2026_10_11_cpp_frontend/README.md).

## Context-bound Q/E migration checkpoint

Runtime and handwritten phases B-D completed before generator changes. The
independent cpp_support generator now emits Context-created query_factory,
ref-qualified value requests, lowerCamelCase application methods and thin
RuntimeExpression/EntityBorrow/EntityListBorrow adapters. Runtime owns execution
and allocation; canonical metadata and kernel protocols retain their names.
Both fixtures are regenerated, and the temporary handwritten Q/E adapters are
removed. The active [change plan](query_expression_change_plan.md) records final
Phase E qualification; the initial frontend measurements remain historical.
