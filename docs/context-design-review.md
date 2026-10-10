# Context API design and memory-safety review

Status: historical API and safety review before the memory redesign.
Implementation reviewed at C++ commit
`b90face53b11ac546b00f615560e0333febf8699`.
Rust baseline: v5.0.5, commit
`62940ae2b88e7acc32763853b8b4685a01c7e90a` in `upstream.lock.json`.

This note records the Context design discussion and the limits of the current
memory implementation. It does not advance migration stages or establish new
parity claims. The [lifetime contract](context-lifetime.md) describes implemented
mechanisms; the [parity contract](design-parity.md) and
[migration plan](migration-plan.md) remain the migration references.

The later [Context memory and string storage design](context-memory-design.md)
records bounded object IDs, separate Global/User bitmaps, payload storage, and
reuse-based string sharing. The
[implementation contract](context-memory-implementation.md) now records checked
factories, budgets, borrowing, and shutdown mechanisms. The baseline findings
below remain historical evidence; they are not the current implementation status.

## Agreed design direction

### Select parameters by the capabilities a function needs

`Memory` is a `std::pmr::memory_resource*`. It supplies allocation and
deallocation, but does not express user identity, logging policy, metadata access,
or execution coordination.

A function with a narrow resource requirement may continue to accept `Memory`.
Examples include allocator-aware construction, value copying, encoding, and pure
SQL compilation. There is no requirement to give every function a UserContext
solely because it participates in SQL processing.

A function that needs Context capabilities should explicitly receive UserContext.
Examples include selecting customized log behavior, consulting contextual
metadata, or selecting a lock or coordination policy for the current execution.
The relevant Context must reach the SQL execution boundary when that boundary
uses those capabilities.

An internal mutex protecting a particular object's state can remain owned by that
object. Context is needed when the lock's selection, scope, or policy depends on
the current user, transaction, or execution.

Ordinary Context-dependent execution should derive its allocator from its
UserContext. It should not accept independently selectable Context and Memory
arguments that can accidentally associate execution with one scope and results
with another. Explicit cross-domain copies still need an explicit destination.

### Application code sees UserContext only

UserContext is the application-facing entry point for both User allocation and
application-lifetime allocation. Business code should not obtain GlobalContext
or its raw allocator through a public `global()` accessor.

GlobalContext remains an explicit ownership domain inside the runtime/hosting
layer. That layer initializes and destroys it as part of the application
lifecycle, after dependent Users have ended. This does not introduce an implicit
singleton, thread-local owner, or a third ownership domain.

Proposed spellings illustrate the allocation intent; these methods and their
return types have not been implemented or finalized:

```cpp
user_context.make<T>();                 // User lifetime.
user_context.make_resource<T>();        // User-owned native resource.
user_context.make_global<T>();          // Application lifetime.
user_context.make_global_resource<T>(); // Application-owned native resource.
```

Delegation changes the public allocation entry point, not the actual owner.
Global objects still belong to the Global arena, and User objects still belong to
the User arena. Lifetime selection must remain visible at each allocation site.

### Preserve ownership and layer boundaries

Global objects must not retain User-owned buffers, objects, views, or callback
captures. Data retained beyond User close requires an explicit deep copy into the
Global domain, including nested containers and strings.

The discussed access model separates a Global resource's lifetime from the
lifetime of a User-facing access handle. The resource can survive User close;
access obtained through that UserContext should expire with it. A subsequent
UserContext can obtain its own access to the resource. The exact handle type and
how expiration is checked remain open; the current raw-reference APIs do not
enforce this model.

A Global-owned executor or service may receive a UserContext for each invocation.
It must not retain that invocation's UserContext or User data in Global state.
Pending operations borrow the invoking Context and must stop accessing its memory
before reclamation.

Per-operation trace, provenance, and diagnostic state remain attached to that
operation. A mutable "current SQL" field on UserContext must not overwrite the
state of other pending operations.

Data-service, SQL executor, transport, provider, and runtime responsibilities
remain distinct. Passing Context to an execution boundary does not move entity
behavior, mutation governance, or graph-save orchestration into the SQL layer.
Compiler provenance and operation-specific log inputs must still be preserved.

## Current implementation and design gaps

| Area | Current behavior | Gap relative to the discussed design |
| --- | --- | --- |
| Allocation API | `GlobalContext::make`, `UserContext::make`, and their resource factories select the owner directly. | UserContext does not yet delegate Global allocation; application code can see GlobalContext. |
| Public access | `UserContext::global()` exposes the Global owner; `memory_resource()` exposes the User allocator. | Global access must be hidden from business code. Whether business code may obtain the User allocator is a separate visibility decision; pure low-level helpers may still use Memory. |
| Execution interfaces | Runtime ContextDataService retains a Context metadata view. Query/mutation executor and SQL transport interfaces mainly accept Memory, requests, and observers. | Context capabilities are not explicit at those lower execution entry points. Log metadata and callbacks already carry some information, so Context is not completely absent from the overall path. |
| Allocator-to-runtime lookup | `detail::application_context(Memory)` downcasts to ContextArena and obtains GlobalContext for runtime logging. | The allocator interface hides a runtime capability dependency. Functions that need such capabilities should declare their Context dependency. |
| Borrow representation | `make()` returns `T&`; ResourceBorrow stores object and owner pointers. | Neither a raw reference nor `ResourceBorrow::get()` checks expiration on every access. |

Rust 5.0.5 QueryExecutor and SqlTransport also do not directly receive UserContext.
The C++ Memory arguments adapt allocation lifetimes. Introducing explicit Context
capabilities at selected boundaries is therefore a deliberate C++ design
adaptation, not automatically a missing Rust parameter. Its placement must
preserve the source responsibilities and execution semantics.

## What the current memory implementation protects

These protections apply to correctly used managed entry points. They do not
describe arbitrary C++ allocations or arbitrary objects reachable from a managed
object. Third-party library internals are outside this assessment.

| Problem | Existing protection and limits |
| --- | --- |
| Forgetting per-object destruction or individual arena blocks | Successfully constructed nontrivial objects are registered for destruction. Close returns every arena block to the configured upstream resource. |
| Reclaiming blocks before destructors run | Registered objects are destroyed in reverse registration order before blocks are reclaimed. Destructors must be `noexcept`. |
| Repeated close | Close is terminal and idempotent; the Context cannot be reset and reused for another request. |
| Failed construction | An unconstructed object is not registered for destruction. Unused arena space remains until close. C++ construction unwinding remains responsible for constructed members. |
| Allocating after close | Allocation through a still-existing closed Context or its live allocator object is rejected. This cannot make a pointer to an already-destroyed Context safe to use. |
| Common off-domain container allocation | `make()` supplies the selected allocator and rejects ordinary non-allocator-aware `std::string` and `std::vector`. Nested standard allocator-aware containers propagate that allocator. |
| Short-lived resource registration into a longer or unrelated domain | ResourceBorrow construction and resource registration check declared ownership-domain relationships. These checks do not inspect every internal pointer or prove the allocation origin of every custom object. |
| Destroying GlobalContext before live UserContext objects | Global destruction fails fast instead of proceeding with dependent Users still alive. |
| Reclaiming memory before registered asynchronous cleanup hooks run | Close first invokes registered stop/wait hooks while ordinary objects remain alive. Correct hooks must actually stop and join all relevant host-memory access. |

Implemented types also have explicit domain-copy constructors and selected
cross-domain callback checks. These are protections for those types and paths,
not a general C++ borrow checker.

## What the current implementation does not protect

| Remaining problem | Why the current mechanism does not prevent it |
| --- | --- |
| Use after Context close or destruction | Returned references, pointers, `string_view`, `span`, callback captures, and borrow wrappers can escape. Access is not universally guarded by a live owner check. |
| User data retained inside a Global object | Allocating the outer object in Global says nothing about the lifetime of its pointers or views. Arbitrary object graphs are not inspected or automatically deep-copied. |
| Incorrect allocator propagation in custom types | Advertising allocator support does not prove that each member uses it. Trivially destructible types can still contain borrowed pointers. Resource factories intentionally accept wrappers whose internal allocations are managed separately. |
| Bypassing managed construction | Direct `new`, raw allocator allocation, or default-resource containers are not automatically registered for Context cleanup. Raw bytes and destructor registration are separate responsibilities. |
| Manual destruction or conflicting ownership | A caller can still delete a managed object, explicitly destroy it, or put its address into an unrelated owning smart pointer. Registered cleanup can then destroy it again. |
| Stack/container cleanup after arena reclamation | A stack PMR container or scoped owner using Context memory must be destroyed before the Context closes. The Context cannot discover every external owner using its allocator. |
| Concurrent allocation, mutation, polling, and close | Arena and Context access are owner-thread contracts, not generally thread-safe implementations. There is no comprehensive thread-identity enforcement. WakeSignal synchronization does not make Context allocation thread-safe. |
| Unregistered background work or incorrect stop hooks | The Context cannot stop work it does not know about, or prove that a hook has finished all memory access. A cancellation request alone is insufficient. |
| Out-of-bounds access and arbitrary corruption | The ownership model adds no universal bounds checks or protection from arbitrary C++ pointer operations. |
| Unbounded memory growth | There is no Context allocation budget. Monotonic arenas generally retain unused bytes, old container buffers, and abandoned construction space until close. This is particularly significant for long-lived Global state. |

The following pattern is accepted by the current allocation constraints, but the
Global view becomes dangling when the User closes:

```cpp
auto& text = user.make<teaql::String>("hello");
auto& view = global.make<std::string_view>(text.data(), text.size());
user.close();
// view is Global-owned, but its character storage has been reclaimed.
```

Returning all blocks means returning allocation ownership to the upstream
resource. It does not necessarily reduce OS resident memory immediately; an
injected upstream resource can retain returned storage according to its own
policy.

## Integration work after memory implementation

1. Continue adopting the checked UserContext factories at application boundaries,
   while retaining explicit temporary native borrows for kernel operations.
2. Identify which execution functions actually need Context capabilities and how
   lower layers reference that contract without introducing a runtime dependency
   cycle or a second execution-context lifetime.
3. Preserve the implemented integer-reference encoding and User access expiry
   when integrating additional interfaces. Separate checked access from caller
   obligations for mutable views and temporary native borrows.
4. Define supported custom-type copy/capture contracts. Do not claim that
   allocator-awareness alone guarantees recursive ownership safety.
5. Specify any Global service coordination beyond the implemented owner-thread
   and asynchronous shutdown contracts.
6. Tune implemented allocation and byte limits for measured edge workloads.
   Native library/device heap accounting needs its own contract and must not be
   inferred from the managed-arena budget.

Future implementation should add acceptance evidence for application API
visibility, correct allocation domains, Context-capability propagation into SQL,
Global resource survival with expired User access, cross-domain retention, and
shutdown ordering. Lifetime changes require sanitizer validation. See the memory
implementation contract for current memory acceptance evidence; this historical
review does not claim completion of SQL capability propagation or save behavior.

## Implementation evidence

- [Memory and allocator aliases](../include/teaql/memory/types.hpp)
- [Arena construction and ownership](../include/teaql/memory/arena.hpp) and
  [close sequence](../src/memory/arena.cpp)
- [GlobalContext and UserContext](../include/teaql/runtime/context.hpp)
- [Resource borrowing and registration](../include/teaql/runtime/resources.hpp)
- [Domain-copy helpers](../include/teaql/memory/clone.hpp) and
  [scoped ownership helpers](../include/teaql/memory/scoped.hpp)
- [Context callback state and copy checks](../include/teaql/memory/function.hpp)
- [Early payload destruction](../include/teaql/memory/context_box.hpp)
- [Registered asynchronous operations](../include/teaql/async/operation.hpp)
- [Runtime Context metadata adapter](../include/teaql/runtime/detail/data_service.hpp)
- [Data-service executor interfaces](../include/teaql/data_service/executor.hpp),
  [SQL executor](../include/teaql/sql/executor.hpp), and
  [SQL transport](../include/teaql/sql/transport.hpp)
- [Existing Context memory tests](../tests/context_memory_tests.cpp)
