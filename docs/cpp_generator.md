# Generated C++ entity, query and expression APIs

The C++ generator lives in the independent `teaql_code_gen_cpp` checkout on
branch `cpp_support`. It extends the existing Java `DomainGenerationService`
with scope `cpp-lib-core`, sharing the KSML parser, centralized pluralization
and Java/Rust naming model. No new runtime or generator dependency is added.
The generator baseline is locked in its `cpp_generator.lock.json`; the runtime
continues to use the exact Rust v5.0.5 baseline.

## Generate and reproduce

```sh
python3 scripts/generate_examples.py
python3 scripts/generate_examples.py --check
python3 scripts/format_cpp.py
python3 scripts/format_cpp.py --check
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_generated_query_expression teaql_generated_api_tests
ctest --preset native -R '^(example_generated_query_expression|generated_api_contracts|generated_api_type_errors)$'
```

Use `--generator-dir PATH` for another copy of the generator branch. Generation
builds the existing Maven reactor, then calls the local Java entry point without
a service, API key, network request or generated application dependency on Java.
Maven may need to obtain its existing build dependencies on a clean machine.
Ordinary CMake builds consume the checked-in headers and do not run Maven.

The script generates both `examples/generator/model.xml` and
`tests/generator/types.xml`, formats all emitted C++ with clang-format18.1.8,
and records source/model/formatter fingerprints. `--check` renders into a new
temporary directory and compares every output byte and the complete file set.
It never edits checked-in generated files. Regenerate instead of patching headers.

## Application usage

After bootstrap creates UserContext, application code uses only that context:

```cpp
cpp_examples::setup(user);
auto& products = cpp_examples::Q::products(user).select_name();
auto& lines = cpp_examples::Q::order_lines(user).select_product_with(products);
auto& request = cpp_examples::Q::orders(user)
    .with_name_is("generated-order")
    .select_lines_with(lines)
    .comment("Load the order and its products")
    .purpose("Display the saved graph");
const auto& rows = cpp_examples::require_value(request.execute_for_list(user).wait());
auto product_name = cpp_examples::E::order(rows[0])
    .get_lines().first().get_product().get_name().unwrap();
```

The runnable [example](../examples/generated_query_expression.cpp) first saves
a product and then an audited order/child graph through the existing runtime.
It does not implement graph traversal, SQL execution or relationship hydration.
A generated `new_entity(user)` obtains its ID through UserContext, and saving
uses the existing `AuditedSaveExt` after `audit_as(reason)`.

Default Q entries select self fields and exclude nonpositive versions, following
Rust generation. `_minimal` entries select ID/version for explicit projection.
Relation filters use the target descriptor and its actual local/foreign keys.
Selecting a relation without a child request uses `select_relation()`; a nested
selection uses `select_relation_with(child_request)`.

`purpose()` returns a distinct executable request specialization. Filters and
comments may appear before or after purpose, without adjacency requirements.
Execution accepts only UserContext and checks nonblank purpose and comment.
A query cannot execute in a different context. The registered executor is
resolved through that context; missing setup fails closed.

`execute_for_list` / `execute_for_entity_list` return the runtime's pollable typed
SmartList result. `execute` / `execute_for_entity` return a pollable optional
entity; an absent row is None. `execute_for_rows` returns compact SmartList
results for aggregate projections. Cancellation forwards to the child operation; a test suspends the single-result query behind a live native cursor, cancels it and then reuses the connection.

## Ownership and expression behavior

Generated entity/request objects use UserContext allocation. Strings, JSON,
load state, query state, child models and relation collections carry the same
allocator. Scalar storage is private, and typed update methods record mutations.
An explicit destination constructor copies all nested data when retaining a
model in GlobalContext. Same-domain allocator-aware moves currently copy through
the qualified destination constructor; this is a performance limitation, not
an implicit shared owner.

Expressions borrow their source model/list. Evaluated owning scalars allocate
in that source domain. Closing the context invalidates all requests, returned
borrows and expression borrows. No shared_ptr or hidden cache extends them.
Global copies of complete models can be evaluated after the original User closes.

Expression access retains Value, Null and NotLoaded separately. An optional
loaded NULL or out-of-range list item is Null. An omitted field/relation is
NotLoaded and raises a C++ logic error when resolved; fallback methods do not
hide it. A loaded empty list has size zero. Evaluation never performs SQL I/O.
The Rust panic is adapted to an English C++ exception; its full structured
suggested-preload diagnostic is not yet reproduced.

## Feature scope against Rust generation

| Rust generator responsibility | C++ status |
| --- | --- |
| Entity/scalar metadata, ID/version, mapped columns, nullable fields, text/JSON/Decimal/date/time | Implemented; typed scalar runtime tests qualify the exercised shapes; not every schema option has a distinct semantic comparison. |
| Forward relations and reverse one/many descriptors; recursive row mapping; graph value injection | Implemented through generated mappings and existing runtime graph/query services. Nested forward/many behavior and explicit destination copies are exercised. |
| Mutation tracking and load state | Implemented through EntityRuntimeState and LoadState. Original snapshots and dirty fields remain separate from mutation values. |
| Q default/minimal projection and live-version predicate | Implemented and tested; this default is required for relation attachment keys. |
| Human/non-human predicates and centralized plurals | Implemented; paired cross-language naming tests and regular/irregular/uncountable fixtures pass. |
| Typed equality/order/range/IN predicates, boolean predicates, text predicates, projections/sort/group and scalar aggregates | Generated over the existing Expr/SelectQuery operations. Runtime range/text/boolean and relation composition are exercised; not every emitted aggregate has a separate semantic comparison. |
| Staged query intent; typed list/one/compact execution; context-only runtime lookup | Implemented. Real compiler negative tests verify unavailable/incorrect operations, and runtime tests cover intent failures and context mismatch. |
| E scalar, forward/reverse and list traversal; legitimate Null versus NotLoaded; null fallbacks | Implemented with existing EvalResult; a generated nested graph example and scalar/load-state/lifetime tests pass. |
| Inheritance, constant-object semantic shortcuts/seeding, generated custom checker/behavior scaffolds | Deferred. Inheritance and constant entities fail generation explicitly until their complete source contracts are qualified. |
| Dynamic JSON request decoding, candidate shortcuts, full relation aggregates/facets, child-type enhancements, generated stream/pagination-cache execution helpers | Deferred generator APIs; corresponding kernel concepts remain intact and callable through their existing APIs. |
| Rust Cargo/web/workspace integrations and external backends | Outside the agreed C++ edge scope. Only SQLite is generated. |
| Native named-module ownership for generated domain declarations | Deferred. Generated headers can consume the existing qualified runtime module frontend. |

This adds a usable C++ generation frontend; it does not claim complete Rust
generator parity. Existing handwritten examples remain unchanged reference
flows. Current validation is recorded in
[the generation evidence](../measurements/generator/2026_10_11_cpp_frontend/README.md).
