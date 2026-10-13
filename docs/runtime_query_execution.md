# Runtime query construction and execution

This document records the transitional implementation checkpoint. The active
target is [query_expression_change_plan.md](query_expression_change_plan.md):
Context-created Q factories, Context-owned dynamic state and checked Q/E bindings.
The private RAII query region and cross-Context execution described below are
historical prototype behavior to replace, not the final ownership contract.

## Context-bound Q implementation update

The current Q path has moved beyond this historical prototype: UserContext now
provides `queries<Factory>()`, the example uses a local Q instance, and request
state is registered through UserContext rather than a private PMR region.
ContextBinding checks application-owned Context metadata before dereferencing
User memory. Copies allocate independent Context roots, nested requests require
compatible bindings, and execution checks construction/argument affinity.

`copyTo(destination)` is an explicit destination deep copy. `copyToGlobal(access)`
delegates an owning Global copy through UserContext; its wrapper still has that
User access binding. `rebindGlobal(destination)` deliberately rebinds Global data
after original User deletion, validates the application/type/live object, and
rejects ordinary User data. Rebinding is an explicit borrow of the retained state;
ordinary mutable-query copies still create independent state.

Contexts and bound wrappers remain thread-confined. The raw kernel Expr/SelectQuery
values and allocator pointers exposed by lower-level APIs are still borrows/scoped
values, not safe standalone wrappers. Their owning C++ members must be destroyed
before the allocator Context is deleted. This does not weaken the Q wrapper gate:
Q values themselves carry only checked references and may be destroyed safely
after User deletion while the application GlobalContext remains alive.

The Context-bound Q checkpoint passes native 3/3 and ASan/UBSan 3/3 with leak
scanning disabled, plus module example/import 2/2. The final helper-return/temporary
text assertions also pass in native and sanitizer refreshes. The module catalog
verifies 1,293 exports and all seven facade archives contain no duplicated
out-of-line implementations. All 720 C++ files pass the pinned formatter.

Runtime E binding is qualified in [runtime_expression_binding.md](runtime_expression_binding.md);
generator now emits equivalent Context-bound adapters; final Phase E evidence is
recorded in the change plan. See the active change
plan for implementation gates and current evidence rather than treating the
historical private-storage tests below as the target contract.

## Implementation order

1. Put first-result composition and typed stream decoding in runtime, preserving
   the existing entity data-service, executor and transport boundaries.
2. Add a caller-owned, context-independent query description. Resolve runtime
   dependencies only at execution and deep-copy all descriptions into UserContext.
3. Exercise no-argument Q entry points and list/one/stream execution through a
   handwritten model syntax adapter. Qualify the runtime before changing generation.
4. In a later generator change, emit adapters over this runtime and retire the
   existing generator-local execution implementations. Generator changes are deferred
   by the user for this checkpoint.

## Public boundary

Application-facing C++ Q/E/model methods use lowerCamelCase. The current Q adapter
uses `orderLines`, `selectName`, `selectProductWith`, `withNameIs`, `executeForList`,
`executeForOne` and `executeForStream`. Runtime/kernel infrastructure APIs retain
snake_case, including `entity_data_service`, `fetch_enhanced_entities`,
`fetch_entity_stream` and `memory_resource`. This deliberate distinction makes
application expressions and infrastructure calls easier to identify.

The boundary follows interface responsibility rather than physical file location.
EntityQuery is an application-facing facade implemented in the runtime package,
so its fluent methods use lowerCamelCase and delegate to snake_case service APIs.
Adapters map names while runtime continues to own execution and cleanup. Do not
add duplicate runtime implementations just to provide another naming style.

Ordinary variables and parameters, descriptive factory types, file, package,
module and namespace names keep their underscore naming. Common types retain
PascalCase, while the local query entry variable remains `Q`. Authentic model
metadata, external identifiers and Rust reference APIs keep their original names.
Existing generated Q/E/model interfaces will adopt the application convention
when generator migration resumes; that output remains unchanged in this step.

The agreed next Context-bound factory syntax is:

```cpp
auto Q = user_context.queries<query_factory>();
auto request = Q.orderLines().withNameIs("first-line");
```

That ownership change is not implemented by this naming checkpoint. The current
runtime description still has private RAII construction storage as documented
below; its handwritten Q adapter uses static, no-argument entry points.

`EntityQuery<Model, Executor, Purposed>` describes a typed entity query. Executor
is a compile-time protocol; callers supply only `UserContext` at execution.
`purpose()` stages an executable specialization. Comment and purpose may occur
in either order, with filters and selections between them. Every execution checks
both values using the existing Unicode whitespace rules.

The handwritten example adapter declares `Q::products()`, `Q::orders()` and
`Q::orderLines()` with no runtime arguments. It maps known model fields and
relations to the runtime builder while preserving typed arguments and relation
models. It does not implement SQL, graph saving, decoding or stream cleanup.

```cpp
auto request = example_query::Q::orderLines()
    .withNameIs("first-line")
    .comment("Read matching order lines")
    .purpose("Display order lines");
const auto& rows = cpp_examples::require_value(
    request.executeForList(user_context).wait());
const auto& one = cpp_examples::require_value(
    request.executeForOne(user_context).wait());
auto streaming = request;
streaming.stream(32);
const auto& handle = cpp_examples::require_value(
    streaming.executeForStream(user_context).wait());
while (const auto* item = handle.get().wait_next()) {
    const auto& entity = cpp_examples::require_value(*item);
    // Inspect this Context-owned entity.
}
```

List returns the existing pollable `Result<SmartList<Model>, DataServiceError>`
with graph/relation enhancement. One returns a pollable optional model and applies
limit one to a destination copy, leaving the reusable description unchanged.
As in the Rust generator, one means first match or none, not a uniqueness check.
Ordering determines the first match.

Stream returns a pollable creation result containing a Context borrow of
`AsyncStream<Result<Model, DataServiceError>>`. It decodes individual entities
from the existing streaming executor's compact-row chunks. Pending wake signals
and cancellation propagate through the existing asynchronous protocols. Empty
chunks are skipped; decoder and executor errors remain distinct and terminate
iteration after yielding their error. EOF, explicit cancellation and Context close
release the underlying cursor. Delivered items remain valid until Context close.

Streaming preserves the existing root-query restriction: relation selections,
child enhancements and object grouping fail closed. Use list for graph hydration.
The adapter does not replace streaming with eager list materialization. Context
arenas still retain delivered objects until close; streaming bounds current chunk
storage and cursor work, not total arena growth for an unlimited stream.

## Construction and managed lifetimes

The two managed lifetimes remain GlobalContext and UserContext. A query description
is an ordinary caller-owned C++ value, not a managed object or a Context borrow.
It privately owns a monotonic PMR buffer with an explicit new-delete upstream.
No default PMR resource, singleton, ambient Context, thread-local owner or shared
ownership is used. Description destruction releases this construction storage.

This is an explicit construction-time ownership adaptation needed for Q factories
without Context parameters. It must never hold entity results or Context resources.
Copying a description deep-copies filters, strings and nested queries into a new
private buffer. Moving transfers its sole owner. Lvalue purpose takes a snapshot;
rvalue purpose transfers ownership. Child selections deep-copy their descriptions.

Execution resolves metadata, policies and the registered executor through the
supplied UserContext. A full query/purpose copy enters that Context before an
operation is returned; the builder may immediately be destroyed. Queries can be
reused against independently initialized contexts. Missing initialization fails
closed. Runtime models, operations, stream handles and results remain exclusively
Context-owned and become invalid at close. Explicit destination model copies are
still required for retention.

## Generator transition

The current generated artifacts retain their old Context-bound Q constructors;
the generator is intentionally unchanged in this checkpoint. The runnable generated
model/expression example now uses `examples/query_syntax.hpp` for the corrected
query syntax and runtime execution. Generated entities and E accessors remain in
use. Existing generated API tests continue to qualify the older generator surface
until its separately authorized migration.

## Validation

`runtime_entity_query_contracts` covers description copies/moves, staged execution,
independent contexts, builder destruction before polling, nested result ownership,
missing setup/intent, first/absent results, pending one cancellation, native SQLite
stream EOF/cancellation/close, empty chunks, wake forwarding and decoder/executor
errors with the default PMR resource disabled. The runnable example exercises a
real graph save, enhanced list/one and incremental typed stream.

Run the native and targeted sanitizer checks for these ownership changes. Module
lookup and the module example also need verification because the public runtime
boundary is exported. MIPS and physical-device testing are not part of this change.

The initial runtime checkpoint passes six selected native tests, two selected
ASan/UBSan tests with leak scanning disabled, and two selected module tests. The
expanded nested-copy and wake-forwarding assertions also pass in native and
sanitizer reruns. All 718 C++ files pass the pinned formatting check; the module
catalog checks 1,292 exports, and all seven facade archives contain no duplicated
out-of-line implementation symbols. These are targeted checks, not full-profile
suite results.
