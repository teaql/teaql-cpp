# Context-bound query and expression change plan

Historical milestone: the acceptance below used Rust 5.0.5. The active reference
is now 5.0.6; [cpp_generator_parity_plan.md](cpp_generator_parity_plan.md) and
[baseline_5_0_6_upgrade.md](baseline_5_0_6_upgrade.md) govern further alignment.
These ownership/naming contracts remain prerequisites; their old tests do not
qualify the additional 5.0.6 semantics.


Status: agreed implementation contract, recorded 2026-10-11.
Phases A-E are qualified within the scope documented below. Target API examples
describe the generated application surface. The original six Rust core migration
stages remain accepted; full Rust generator parity is not claimed.

## Purpose and precedence

Make generated C++ query and expression code composable while preserving the
explicit GlobalContext/UserContext memory model. Construction, function returns,
subquery embedding and execution must have predictable allocation and lifetime
rules. Application-facing APIs and runtime infrastructure use two deliberate
naming styles.

This document governs the next Q/E implementation. It supersedes the private
query-storage ownership adaptation in runtime_query_execution.md and the old
Context argument on each generated Q entry. That document remains evidence of
the transitional runtime checkpoint. The six-stage Rust v5.0.5 acceptance remains
historical; the active lock is Rust v5.0.6. Independent
runtime/data-service/SQL/executor/provider layers remain unchanged.
SQLite is the only database dependency; generation remains a
development step. No new runtime dependency is required.

The implementation order is runtime first, handwritten syntax qualification next,
generator migration last. Do not start another generator change while the
runtime ownership and composition contracts are still unqualified. Preserve the
existing original examples and previous uncommitted work throughout this change.

## Target application syntax

### Q factory and execution

```cpp
auto Q = user_context.queries<sales::query_factory>();

auto products = Q.products().selectName();
auto lines = Q.orderLines().selectProductWith(products);
auto request = Q.orders()
    .selectLinesWith(lines)
    .comment("Read orders and products")
    .purpose("Display saved orders");

const auto& orders = require_value(request.executeForList(user_context).wait());
const auto& first = require_value(request.executeForOne(user_context).wait());
```

`Q` is a local factory variable, not a process-global object. `query_factory` is
a descriptive generated type; its namespace identifies the model domain.
`UserContext::queries<Factory>()` binds the factory to that context. Individual
entry methods such as `Q.orderLines()` take no Context argument. The factory
propagates its Context allocation domain into requests, strings, constants,
expressions and nested queries.

The three execute methods retain exactly one caller-supplied runtime argument:
UserContext. This makes the execution site explicit and preserves the established
query execution contract. The supplied Context must match the construction
Context. Resolve metadata, policies, custom logging, locking and the registered
executor through that Context; missing initialization fails closed. Do not accept
an executor, connection or service as an extra application argument.

`purpose()` produces an executable request type. Filters, comment, relation
selections and other modifiers may occur before or after purpose. Both purpose
and comment must be nonblank at actual execution; preserve their original values
and existing trace/privacy behavior. Saving retains its separate audited contract.

### Function processing and embedding

```cpp
auto namedOrders(sales::OrderRequest request, std::string_view name) {
    return std::move(request).withNameIs(name);
}

auto request = namedOrders(Q.orders(), "example")
    .comment("Read matching orders")
    .purpose("Display orders");
```

Requests and expressions must work as ordinary values in function parameters,
returns, members and nested expressions. A function returning a wrapper must
not leave a reference to its local wrapper, local string or temporary subquery.
Returning the value does not extend its Context lifetime.

### Streaming

```cpp
auto request = Q.orderLines()
    .stream(32)
    .comment("Read lines incrementally")
    .purpose("Process order lines");
const auto& handle = require_value(request.executeForStream(user_context).wait());
while (const auto* item = handle.get().wait_next()) {
    const auto& line = require_value(*item);
    // Process this Context-owned entity.
}
```

The stream's application method uses camelCase. The underlying runtime async
protocol keeps names such as `wait_next`, `poll_next` and `cancel_and_wait`.

### E construction

The existing entity-bound entry is the initial target to preserve and rename:

```cpp
auto name_expression = sales::E::order(order).getName();
auto name = name_expression.unwrap();
```

E derives the allocation domain from its bound entity. Its wrapper can be returned
from a function or embedded in another expression while that binding remains
valid. An owned scalar expression and an entity/list borrow must remain distinct.

A Context-created factory for unbound E descriptions may be needed later. Its
exact spelling and supported operators are not settled by this contract; do not
advertise `user_context.expressions<expression_factory>()` as an implemented or
approved public API. Qualify the existing E roles before expanding that surface.

## Allocation and lifetime contract

| Object or data | Owner | Release/invalidation |
| --- | --- | --- |
| Local Q factory and Q/E wrappers | Ordinary C++ wrapper lifetime | Wrapper destruction releases the wrapper; it does not close Context or reclaim arena blocks. |
| Query roots and independently published expression state | Bound Context, through its scoped factory | Registered managed objects are destroyed during Context close. |
| Strings, constants, field paths, nested AST/query state and containers | Same Context as the owning request/expression | Nested allocators propagate; blocks are reclaimed with their Context. |
| Bound entity/list references | Borrow of an existing source object in an allowed domain | Cannot outlive the source object or its Context; binding must be checked before dereference. |
| Operations, result models and stream handles/items | Executing UserContext | Stop/join/cancel native work before destruction and arena reclamation. |
| Explicit retained copies | Explicit destination Context | Independent data lifetime after a complete owning deep copy; source borrows are not promoted implicitly. |

There are two managed allocation domains: application-owned GlobalContext and
UserContext. Application business code receives only UserContext and delegates
explicit Global allocations through it. The initial Q factory binds to UserContext;
no implicit Global query factory or extra query-owned heap region is introduced.

The current prototype's private monotonic buffer and `make_unique<QueryDescription>`
are transitional and must be removed from the Context-bound Q path. Wrapper
objects can live on the stack without giving their dynamic data a separate
allocation domain. Do not introduce hidden singleton/default PMR/thread-local
owners or shared ownership that keeps Context data alive after close.

Existing Context arenas grow by adding blocks. A query does not need to execute
in order to be reclaimed: closing its Context destroys its registered objects
and returns all blocks. Intermediate or unused query/expression payloads normally
remain charged until close, even after their local wrappers are destroyed. This
is the accepted cost of the unified lifetime. Preserve allocation limits, failure
behavior, explicit object identities and payload accounting; character bytes
must not become separately counted object identities.

GlobalContext's explicit upstream memory_resource injection remains the supported
backend customization mechanism. Pooling, statistics or alternative upstreams can
be implemented there. Replacing process-wide operator new/delete is not needed
for this target and does not establish Context ownership.

## Copy, move and composition rules

| Operation | Required behavior |
| --- | --- |
| Copy a mutable Q value | Independent query state in the same bound Context; later modifications must not alter the source. |
| Move a Q value | Transfer the wrapper/state binding; never extend Context lifetime. Diagnose unsupported moved-from use before touching state. |
| Process/return a Q value in a helper | Preserve its Context and own all incoming string/constant payloads. Ref-qualified fluent returns must not leak references to temporary wrappers. |
| Embed a child query | Validate compatible Contexts and capture a deep snapshot of its description. Later child changes cannot mutate the parent implicitly. |
| Combine two E descriptions | Preserve typed payloads, Value/Null/NotLoaded behavior and every borrowed source's validity. Sharing a safe binding is different from sharing mutable owned query state. |
| Compose across UserContexts | Reject by default before retaining any source pointer. Use a deliberate destination-copy operation while the source is still valid. |
| Retain into Global | Deep-copy owned data through UserContext's explicit Global delegation. A copied pointer/path is not an owning copy of a bound entity. |

Cross-domain Q copies preserve intent, projections, predicates, relations and
all nested allocator domains. The exact destination-copy method name is an
implementation decision; it must be explicit in the application API.

An E wrapper must not silently turn a borrowed entity into a shared owner.
Copying expression metadata cannot make the underlying entity survive destruction.
To retain entity-bound evaluation across Contexts, copy the entity itself into the
destination and rebuild the binding, or use another separately qualified explicit
operation that provides the same ownership guarantee.

## Binding safety and E semantics

Every Q factory/request carries a checked Context identity and a way to resolve
its allocation scope. Keeping only a raw UserContext pointer is insufficient:
access after actual UserContext deletion must be rejected without reading freed
memory. Existing Context identity/diagnostic infrastructure should be reused.

E needs both Context validity and source-object validity. An open Context does
not prove that a stack entity still exists, that a vector element has not moved,
or that an explicitly destroyed object is still live. Use checked object IDs or
stable-owner bindings with validated element identities/generations where needed.
Choose and document the representation during the E phase. Reject unsafe temporary
bindings; do not claim arbitrary entity borrows are safe based only on the Context
close check. Nested/list traversal must propagate the binding and validate it.

Preserve Value, legitimate Null and NotLoaded separately. Fallbacks must not hide
NotLoaded, and evaluation must not silently perform SQL or preload a relation.
Preserve Rust source evaluation/snapshot behavior when choosing representations;
changing eager scalar snapshots to lazy reads requires an explicit semantic review.

Current E operations allocate in the entity's domain but use raw pointers and
may copy string scalars both during field access and during unwrap/eval. Move
reusable expression mechanics into runtime and remove redundant copies only when
ownership and source semantics remain intact. Primitive values returned by value
are independent values; strings, JSON, lists and entity borrows keep their
explicit allocation/binding rules.

All bound wrappers become invalid at Context close. Ordinary wrappers that live
outside the arena must not later run destructors that dereference the deleted
Context resource. Keep owned payload destruction inside Context-managed state,
and test wrapper destruction after actual Context deletion as well as access
rejection. Do not treat invalidation as guaranteed safe diagnostics after the
application-owned GlobalContext itself has been destroyed.

## Execution responsibilities

| API | Semantics and layer |
| --- | --- |
| executeForList(UserContext&) | Pollable typed SmartList through the existing enhanced entity data service, including qualified relationship hydration. |
| executeForOne(UserContext&) | Pollable optional first match; limit a private execution snapshot to one, preserve ordering and leave the reusable request unchanged. Multiple matches are permitted. |
| executeForStream(UserContext&) | Pollable Context-owned typed stream; incrementally decode existing executor chunks, forward pending wakes and preserve typed errors. |

Metadata, access policies, telemetry, log intent/privacy and database resources
are resolved through UserContext at execution. Application/generated adapters
translate model names and types; they do not reimplement graph save, SQL or
runtime orchestration. Preserve the independent data-service/executor/transport/
provider layers.

EOF, explicit cancellation, errors and Context close must stop underlying work
and release cursors before arena blocks are reclaimed. Delivered stream items
remain valid until Context close. Streaming retains the current root-query
restriction: unsupported relation/child/object-group enhancement fails closed.
Do not replace streaming with eager list execution or claim bounded total arena
usage; delivered objects accumulate under the existing Context policy.

## Naming contract

| Category | Convention | Examples |
| --- | --- | --- |
| Generated application types | PascalCase | OrderRequest, OrderExpression, Product |
| Generated application Q/E/model methods | lowerCamelCase | orderLines, selectName, withNameIs, getName, updateName, executeForList, newEntity |
| Local entry variables | Explicit short exceptions | Q; E when an expression entry variable is provided |
| Descriptive factory type | Approved snake_case exception | query_factory |
| Ordinary variables and parameters | snake_case | user_context, product_name |
| Runtime/kernel infrastructure methods | snake_case | entity_data_service, fetch_entity_stream, memory_resource |
| File/directory/package/module/namespace names | snake_case | entity_query.hpp, teaql_runtime, sales |
| Schema/model metadata and external protocols | Preserve authentic spelling | order_id; existing Rust and standard-library identifiers |

Classify methods by interface responsibility, not file location. The application-
facing EntityQuery facade keeps camelCase even when its implementation lives in
teaql_runtime. Internal helpers and service protocols retain snake_case. Generated
implementation code calls those runtime APIs using their original spellings.

Keep centralized pluralization and the established Java predicate meanings. Change
casing after resolving the semantic name; do not invent plurals or keep known-
wrong predicate aliases. All domain Q methods and generated E/model application
methods follow this contract, not only the example's handful of methods.

## Current implementation versus target

| Area | Current checkpoint | Qualification boundary |
| --- | --- | --- |
| Runtime list/one/typed stream | Runtime implementations and real SQLite example qualified. | Native 6/6 and sanitizer 5/5 integration gate. |
| Q allocation | Context-owned registered roots; no private query region. | Preserve object/byte limits and allocator propagation. |
| Q entry syntax | Local Q from UserContext.queries<domain::query_factory>(); no Context on entries. | Both ten-file fixtures reproduce byte for byte. |
| Copy/embedding | Independent mutable copies, child snapshots, explicit copyTo/copyToGlobal/rebindGlobal. | Generated adapters run the existing binding tests. |
| Context affinity | Checked construction/execution and foreign composition rejection. | Preserve deletion diagnostics while Global lives. |
| E | RuntimeExpression and checked entity/list borrows; eager snapshots and three evaluation states. | Generated accessors run the E lifetime/composition tests. |
| Generated naming | Application Q/E/model methods use lowerCamelCase; schema and runtime protocols unchanged. | One positive/nine negative compiler cases; 17 Java regressions. |
| Generator workspace | Independent cpp_support checkout; generated fixtures rebuilt; original checkout untouched. | Deterministic output and centralized naming gates passed. |

Historical focused native, sanitizer and module checks in runtime_query_execution.md
prove only that earlier checkpoint. They do not by themselves prove the Context factory,
checked E bindings or generated surface; those have separate phase evidence. The six original core migration stages
remain accepted; this change does not reopen them or claim complete Rust generator
parity, additional backends, GPU adapters or physical-router execution.

## Implementation phases and exit criteria

### Phase A: Record and align the contract

Deliver this document, link it from AGENTS.md, README and generator/runtime plans,
and mark private query storage as transitional. This documentation phase is
complete when the active instructions no longer recommend that prototype as
the target ownership model. No runtime test is needed for documentation alone.

### Phase B: Bind Q construction to UserContext in runtime

Phase B is accepted for the documented Q scope. The final native and sanitizer
checks pass 3/3 each; relevant module example/import checks pass 2/2. ContextBinding
checks Global-owned ContextState before touching User storage, and query wrappers
contain no allocator-owning payloads. The new query_binding_tests exercise custom
upstream cleanup, unused queries, object/byte limits, deleted-Context access,
safe wrapper destruction and delegated Global retention/rebind. Runtime query tests
cover independent copies, temporary text, helper-returned requests and all three
execution variants. The helper-returned request test also executes a real SQLite query after its
factory/function wrappers die and its input string changes. No generator change
is part of this phase.

Evidence commands: build native and sanitize targets teaql_query_binding_tests,
teaql_entity_query_tests and teaql_generated_query_expression, then select
runtime_query_binding_contracts, runtime_entity_query_contracts and
example_generated_query_expression in CTest. Sanitizer leak scanning remains
disabled by the pinned preset. Build the module example/import contracts and
select module_generated_query_expression and module_import_contracts. The export
catalog verifies 1,293 symbols across seven modules; the seven facade archives
contain no duplicated out-of-line implementations. All 720 C++ files pass the
pinned formatter. These are scoped phase checks, not full suites or E acceptance.

The Global-retention fixture initially kept a raw kernel Expr beyond source
Context deletion. Native/GDB and ASan identified its destructor use-after-free;
the fixture now destroys that scoped kernel value before closing its allocator.
The retained Q/factory wrappers deliberately remain alive after actual User
deletion, reject accesses and are safely destroyed. This distinction is tested,
not implemented by suppressing cleanup or extending Context lifetime.

Implement the generic Context factory entry and checked binding, allocate query
state through UserContext and propagate its allocator throughout nested data.
Remove Q-private heap ownership, preserve independent copies/moves and snapshots,
and reject cross-context execution/composition unless copied explicitly.

Acceptance: no hidden/default-resource allocations; owned temporary string inputs;
function return/embedding; factory destruction before request use; allocation-limit
failures; clone independence; source/target copy ownership; unused-query cleanup;
close and actual deletion; safe destruction of surviving wrappers. Preserve list,
one, stream, audit/log intent and cancellation behavior. Run native relevant tests
and targeted ASan/UBSan for these lifetime changes.

### Phase C: Qualify E composition and checked entity/list bindings in runtime

Phase C is accepted for the documented runtime E scope. Native 6/6 includes
original Rust runtime/model comparisons, sanitizer passes 4/4 and module
example/import passes 2/2. Explicit-copy, list-owner destruction, actual deletion,
accessor Null/NotLoaded, callback suppression and allocator-aware JSON default
checks additionally pass native/sanitizer refreshes. See [runtime_expression_binding.md](runtime_expression_binding.md)
for representation, scope and evidence. Generator output is not part of this gate.

Move shared E evaluation/composition mechanics into runtime. Select stable binding
representations and preserve source evaluation semantics, Value/Null/NotLoaded and
explicit scalar ownership. Eliminate unsupported raw-pointer retention and
redundant allocations without erasing model/evaluation abstractions.

Acceptance: helper-returned expressions; nested/list composition; independent
owned snapshots and borrowed values; temporary-binding rejection; entity destroy,
container relocation/element replacement, Context close and actual deletion;
Global retention through explicit entity copies; no implicit SQL/preload. Run
native relevant checks and targeted sanitizers.

### Phase D: Qualify handwritten application syntax and examples

Phase D is accepted. One positive and eight negative handwritten compiler
contracts pass; native original CRUD/graph examples and their Rust comparison
pass 3/3. The Phase B/C helper, execution and module gates also cover this
application syntax. Generator migration starts after these completed gates.

Adapt the example factory to the new runtime contract and demonstrate exactly
`auto Q = user_context.queries<query_factory>()` with camelCase Q/E/model entry
methods. Keep adapters limited to model-specific names/types. Preserve original
handwritten examples as semantic baselines and use real runtime graph/query paths.

Acceptance: positive and meaningful negative compiler contracts, composed helper
functions, nested queries, all three execution variants and source comparison.
Update named-module exports/lookup for new public declarations and test relevant
module consumers. Directory/module moves stay separate from function changes.

### Phase E: Migrate the C++ generator to the qualified runtime

Phase E is accepted. The independent cpp_support generator emits the full
previously supported typed field/relation surface over EntityQuery and
RuntimeExpression, with camelCase application methods and authentic schema/runtime
identifiers. Both ten-file fixtures reproduce byte for byte with
`python3 scripts/generate_examples.py --check`. Temporary handwritten Q/E adapters
have been removed; examples and binding tests now consume generated equivalents.
No Rust/other-language template or original generator checkout is changed.

Final native checks pass 6/6, including generated API, example, Q/E binding tests
and original example/Rust comparison. Targeted ASan/UBSan passes 5/5 with leak
scanning disabled; named-module example/import checks pass 2/2. The compiler gate
passes one positive and nine negative cases, including purpose, audit, scalar/
relation types, extra execution arguments, Global factory, Context on entry and
temporary E binding. Java regressions pass 17/17: CppGenerationTest (5),
PredicateNamingParityTest (1, comparing eight language scopes), RustNamingTest (9)
and PluralizationUtilsTest (2). Additional assertions keep ASSIST namespace,
standard-library types and Context factory spelling authentic.

Commands: native/sanitize builds of teaql_generated_api_tests,
teaql_generated_query_expression, teaql_entity_query_tests,
teaql_query_binding_tests and teaql_runtime_expression_tests; CTest selects their
five contracts, plus runtime_examples_rust_parity for native. Module builds use
teaql_generated_query_expression and teaql_module_contracts. Generator reactor
checks use `mvn -q -pl generator -am
-Dtest=CppGenerationTest,PredicateNamingParityTest,PluralizationUtilsTest,RustNamingTest
-Dsurefire.failIfNoSpecifiedTests=false test`. Compiler checks use
`python3 scripts/verify_generated_type_errors.py --build-dir build_native_current`.
The pinned formatter verifies all 722 handwritten C++ files; the export catalog
checks 1,301 declarations across seven modules, and all seven facade archives
contain no duplicated out-of-line implementations. This historical checkpoint
used the exact Rust v5.0.5 commit; it does not qualify the active v5.0.6 baseline.
No MIPS/physical-hardware run was needed for this scope.

Typed compact aggregate-row execution and foreign generated child-query rejection
are exercised against SQLite. List/one/stream and graph save continue through the
independent runtime/data-service/executor/provider layers. Stream enhancement
restrictions and the generator's deferred inheritance, constants and other feature
boundaries remain as documented in cpp_generator.md.

Only after phases B-D pass, resume the independent cpp_support workspace. Emit
Context-bound query_factory, camelCase Q/E/model methods, thin runtime adapters
and accurate model-aware ASSIST/AGENTS material. Preserve centralized naming,
metadata and complete existing responsibilities; document unsupported features.

Regenerate both checked-in model fixtures, remove temporary duplicate application
adapters when their generated equivalents are qualified, and keep Java/generation
out of ordinary CMake builds. Acceptance includes deterministic byte reproduction,
Java generator/naming regressions, native generated API/runtime examples, meaningful
compile rejections and relevant module/lifetime checks. Do not hand-edit generated
headers or introduce duplicate SQL/graph implementations.

## Work and verification policy

Complete each phase against its exit criteria before dependent work begins. Keep
ongoing changes intact, update status with actual evidence and continue under this
target contract. Generator work remains sequenced after runtime qualification;
no additional approval checkpoint is introduced by this plan.

Use the locked toolchains and pinned formatter. Prefer relevant native tests;
use targeted sanitizers for binding, allocator and async cleanup changes. Use
module tests for public module-interface changes. MIPS/QEMU is required only for
architecture/toolchain changes or an explicit request. Reserve full-suite runs
for appropriate broad integration/release milestones. Physical hardware tests
remain deferred. Wait for every build/link process to finish successfully before
using its output. Never label prototype checks as target acceptance.

## References

- [Runtime query checkpoint](runtime_query_execution.md)
- [Generator plan](cpp_generator_plan.md)
- [Generated frontend scope](cpp_generator.md)
- [Context memory implementation](context_memory_implementation.md)
- [Design parity](design_parity.md)
- [Original six-stage migration plan](migration_plan.md)
