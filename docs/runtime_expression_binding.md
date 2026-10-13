# Runtime expression binding and snapshots

This is the Phase C implementation contract/evidence companion to
[query_expression_change_plan.md](query_expression_change_plan.md). The C++
generator now emits thin adapters over this runtime. The temporary handwritten
adapter qualified Phase C before generator migration and has been removed.

## Runtime responsibilities

RuntimeExpression<T> is a small, trivially destructible value wrapper. Its immutable
EvalResult<T> state is registered in the source Context arena. Copies may share that
immutable state within its lifetime; no shared owner extends the Context. The
wrapper itself owns no string, container or independent heap region. It remains
safely destructible after actual UserContext deletion while Global diagnostics live.

ExpressionScope checks User ContextState metadata before touching its arena. Global
and standalone Arena domains require their explicit owner to remain alive. Global
entity copies can therefore produce expressions independent of an earlier User
Context, with their dynamic state allocated in Global. There is no ambient Context,
default resource, thread-local owner or process-global expression cache.

EntityBorrow<Model> binds a const entity lvalue to its EntityRuntimeState lifetime
anchor. A lifetime stamp is created lazily on first binding and consumes a managed
object identity; scalar character payloads do not get separate identities. Reading
checks Context validity, stamp type/live slot and object generation before touching
the model pointer. Temporary model binding is rejected at compile time.

EntityRuntimeState retains its existing shared ledger/graph clone behavior and
identity comparison. Its new lifetime anchor tracks only the concrete C++ object.
Copies have independent anchors. Moves transfer a stamp and advance its generation
without allocation, invalidating earlier borrows. Destruction or replacement marks
the old stamp dead. During Context shutdown, the anchor consults metadata and avoids
stamps that may already have been destroyed under reverse cleanup ordering.

EntityListBorrow<Model> checks its owning entity anchor before accessing the list,
and rejects changes to its data address, length or loaded flag. Each selected
entity receives its own checked lifetime binding. Vector relocation and element
replacement therefore invalidate previously selected entity borrows. A list view
whose structural fingerprint is unchanged may select current elements again;
it does not freeze or copy the collection. Bound wrappers are thread-confined like
Context, and arbitrary raw references obtained from checked getters remain borrows.
The checks do not promise safety for user code bypassing the API/object lifetimes.

## Evaluation behavior

Field getters preserve the existing eager scalar snapshot behavior: changing or
destroying the source entity does not change a captured scalar while its Context
remains alive. Owning strings/JSON/etc remain allocator-aware Context values;
primitive results returned by value are ordinary independent values.

Value, legitimate Null and NotLoaded remain distinct. andThen preserves the Rust
missing-path concatenation rules; map transforms only Value. Neither invokes a
callback for Null/NotLoaded. Fallbacks apply only to legitimate Null and never hide
NotLoaded. Getters/evaluation do not execute SQL or preload relations.

isNull/isNotNull inspect state and binding without copying owned scalar payloads.
Returning an independent owning scalar through eval/unwrap still requires its
proper scoped copy. Such returned raw allocator-aware C++ values must be destroyed
before their allocator Context is deleted; this differs from the safe lightweight
expression wrapper. No destructor suppression is used to disguise this contract.

Composition rejects foreign expression domains. Raw pointer payloads are excluded
from RuntimeExpression; entity/list pointers use checked borrow types. copyTo on
owning scalar expressions performs an explicit destination deep copy. Borrowed
entity/list expressions cannot use that operation to promote a pointer: copy the
entity into the destination and rebuild its binding. Global retention is exercised
through UserContext's existing make_global/rebind_global delegation.

## Application adapter and current scope

Generated E adapters supply model-specific names/types and loaded-field checks.
They expose order(...).getLines().first().getProduct().getName(), product(...)
and orderLine(...) over the generic runtime, with all existing model fields.
The generated example uses these adapters alongside the Context-created Q factory
for real SQLite graph, list/one and stream execution. Production ownership and
execution algorithms remain in runtime. Phase E evidence is recorded in the plan.

## Acceptance scope

runtime_entity_expression_contracts covers helper returns, eager string snapshots,
no-copy null predicates, moves, stack/managed entity destruction, vector relocation,
element replacement, nested entity/list traversal, out-of-range Null, null fallback,
NotLoaded propagation, typed map, foreign composition rejection, explicit scalar
copy, Global entity retention and safe wrappers after actual Context deletion.
Tests run with the default PMR resource disabled.

Relevant native acceptance includes the new contracts and runnable example plus
entity_runtime_contracts, entity_runtime_model_contracts and both original Rust
runtime/model comparisons. The initial selection passes 6/6; targeted ASan/UBSan
passes 4/4 with leak scanning disabled, and module example/import checks pass 2/2.
The final scalar-copy/list-owner checks, real accessor Null/NotLoaded propagation,
callback suppression and allocator-aware JSON default pass in native and
sanitizer refreshes. The export catalog contains 1,301 declarations and formatting covers
724 C++ files. These are scoped phase results, not a full-suite or generator claim.
