# Generated C++ entity, query and expression APIs

Remaining alignment follows [cpp_generator_parity_plan.md](cpp_generator_parity_plan.md).
The Context-bound Q/E milestone is complete; full Rust generator parity remains active.


The C++ generator lives in the independent `teaql_code_gen_cpp` checkout on
branch `cpp_support`. It extends the existing Java `DomainGenerationService`
with scope `cpp-lib-core`, sharing the KSML parser, centralized pluralization
and Java/Rust naming model. No new runtime or generator dependency is added.
The generator baseline is locked in its `cpp_generator.lock.json`; the runtime
now targets the exact Rust v5.0.6 baseline. Earlier qualification remains
historical; see [the upgrade audit](baseline_5_0_6_upgrade.md).

Generated `AGENTS.md` starts with `ASSIST.md`, then links to the next entity
action in `assist/<entity>/<action>.md` or exact field guidance in
`assist/<entity>/fields/<field>.md`. Runtime support lives in `assist/runtime/`.
The [action examples](generated_application_workflows_5_0_6.md) and
[field/runtime guidance](generated_field_runtime_assist_5_0_6.md) use existing
SDK/runtime contracts; application helper functions in their code blocks are
examples rather than additional SDK methods. Missing external utility facades
remain explicitly unavailable. Full G12 acceptance still requires the separate
assembly, sample and integration matrix.

The three source-shaped startup entries now return
`Result<Ref<DataServiceExecutor>, ServiceRuntimeError>`, including
`serviceRuntimeFromConnection(user_context, connection)`. Check the Result before
schema or business execution. Business-ID domains install a managed environment
key before executor registration; ordinary domains do not read key variables.
The explicit filename `configureRuntime`/`setup` path retains manual typed-key
installation. See the [startup contract and qualification boundary](generated_business_environment_startup_5_0_6.md).

## Generate and reproduce

```sh
source scripts/toolchains/activate.sh
python3 scripts/generate_examples.py
python3 scripts/generate_examples.py --check
python3 scripts/format_cpp.py
python3 scripts/format_cpp.py --check
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

## Context-bound application API

The implementation follows [query_expression_change_plan.md](query_expression_change_plan.md).
Runtime Q/E and handwritten syntax gates passed against 5.0.5 before generator migration.
The generator emits thin Context-bound adapters; Phase E integration is qualified
and its historical 5.0.5 evidence is recorded in that plan. The new producer/runtime
reference is locked; complete 5.0.6 alignment remains open.

```cpp
cpp_examples::setup(user_context);
auto Q = user_context.queries<cpp_examples::query_factory>();
auto products = Q.products().selectName();
auto lines = Q.orderLines().selectProductWith(products);
auto request = Q.orders()
    .withNameIs("generated-order")
    .selectLinesWith(lines)
    .comment("Load the order and its products")
    .purpose("Display the saved graph");
const auto& rows = cpp_examples::require_value(request.executeForList(user_context).wait());
auto product_name = cpp_examples::E::order(rows[0])
    .getLines().first().getProduct().getName().unwrap();
```

Canonical missing-load suggestions for scalar C++ keywords have callable Q/E
aliases: for example, `selectClass()`, `unselectClass()` and `getClass()` delegate
to the existing escaped `selectModelClass()`, `unselectModelClass()` and
`getModelClass()` methods. The model-specific Assist lists affected fields;
see [the repair contract](scalar_diagnostic_repairs_5_0_6.md).

Generated E factories also supply their own static entry name to runtime
provenance, so keyword entities display `E::modelClass(...)` while retaining
canonical identity `Class`. Business calls keep the same one-argument signature;
see [expression entry identity](expression_entry_identity_5_0_6.md).

Application Q/E/model methods use lowerCamelCase. Runtime protocols such as
`memory_resource`, `audit_as` and `wait_next`, canonical metadata and SQL identifiers
retain their spelling. Entries take no Context argument. Execution takes exactly
one UserContext and must match construction. Default entries project self fields
and positive versions; `ordersMinimal()` selects ID/version.

`purpose()` produces an executable specialization. Comment and purpose need not
be adjacent; both must be nonblank at execution. `executeForList` returns an enhanced
typed SmartList, `executeForOne` an optional first match and `executeForStream` a
pollable typed stream. `execute`/`executeForEntity` and `executeForEntityList` retain
their existing one/list roles. `executeForRows` provides compact aggregate rows.
Generated methods delegate execution, cancellation and decoding to runtime.
Stream enhancement restrictions remain enforced.

`returnType<T>()` also accepts a non-Entity marker during ordinary request
construction, inspection and composition. No marker object is allocated.
All public execution entries, including compact rows/count/exists, and
`newEntity` require an Entity result; restore that result before execution.
The [generated marker contract](generated_query_result_marker_5_0_6.md) records
the original enclosing guards, compiler checks and Context ownership evidence.

Models provide `Model::withId(user_context, id)`, `isLoaded(field)` and
`setLoadState(state)` over existing Value/indexed-state mechanics. The Context
argument selects the ID Value's allocation domain. State replacement copies
into the model's domain and rejects incompatible layouts before replacement;
it changes availability without changing values or staging mutations. See
[entity helper entries](entity_helper_entries_5_0_6.md).

The runnable [example](../examples/generated_query_expression.cpp) saves an audited
order/child graph, loads nested relations and exercises all three execution forms.
`newEntity(user_context)` delegates ID allocation; graph save uses the existing
`AuditedSaveExt` and `audit_as(reason)` protocol.

## Ownership and expressions

Q wrappers are values with Context-owned registered roots. Copies create independent
state, moves transfer bindings, fluent methods preserve value categories, and child
queries are checked and captured as snapshots. `copyTo` explicitly copies into a
UserContext; `copyToGlobal` delegates Global retention and `rebindGlobal` explicitly
changes its User access binding. Unexecuted queries are reclaimed at Context close.

E adapters use `RuntimeExpression`, checked `EntityBorrow`/`EntityListBorrow` and
Context-owned immutable scalar state. Scalar getters preserve eager snapshots.
Entity destruction, moves, list relocation and actual User deletion are checked
before dereference. Temporary entity bindings are deleted overloads. Wrappers can
outlive a closed/deleted User safely while Global diagnostics remain alive, but
access then fails. No hidden/shared owner extends the source lifetime.

Value, Null and NotLoaded remain distinct. Missing fields never trigger SQL and
fallbacks never hide NotLoaded. Loaded NULL/out-of-range elements are legitimate
Null; loaded empty lists have size zero. Explicit scalar `copyTo` or complete model
copies into a destination scope permit deliberate retention. Runtime allocation
protocols propagate into nested strings, JSON, query state and model relations.
See [Q execution](runtime_query_execution.md) and
[E binding](runtime_expression_binding.md) for the qualified contracts and limits.

## Feature scope against Rust generation

| Rust generator responsibility | C++ status |
| --- | --- |
| Entity/scalar metadata, ID/version, mapped columns, nullable fields, text/JSON/Decimal/date/time | Implemented; typed scalar runtime tests qualify the exercised shapes; not every schema option has a distinct semantic comparison. |
| Forward relations and reverse one/many descriptors; recursive row mapping; graph value injection | Implemented through generated mappings and existing runtime graph/query services. Nested forward/many behavior and explicit destination copies are exercised. |
| Mutation tracking and load state | Implemented through EntityRuntimeState and LoadState. Original snapshots and dirty fields remain separate from mutation values. |
| Q default/minimal projection and live-version predicate | Implemented and tested; this default is required for relation attachment keys. |
| Human/non-human predicates and centralized plurals | Implemented; paired cross-language naming tests and regular/irregular/uncountable fixtures pass. |
| Typed equality/order/range/IN predicates, boolean predicates, text predicates, projections/sort/group and scalar aggregates | Generated over the existing Expr/SelectQuery operations. Runtime range/text/boolean and relation composition are exercised; not every emitted aggregate has a separate semantic comparison. |
| Staged query intent; typed list/one/stream/compact execution; context-only runtime lookup | Implemented. Real compiler negative tests verify unavailable/incorrect operations, and runtime tests cover intent failures and context mismatch. |
| E scalar, forward/reverse and list traversal; legitimate Null versus NotLoaded; null fallbacks | Implemented with RuntimeExpression over EvalResult and checked entity/list borrows; a generated nested graph example and scalar/load-state/lifetime tests pass. |
| Inheritance | Supported in normal generation. Parent/child identity, projections/enhancement, typed decoding, graph save and lifecycle checks have source comparisons. Inherited Business IDs use the documented source binding adaptation; the untouched broken Rust artifact is preserved. See [inheritance qualification](inheritance_qualification_matrix_5_0_6.md). |
| Constant initialization/seeding | Supported in normal generation. Root/constant recipes, candidates, typed fields, topology, governance, overrides and cleanup are qualified with explicit source binding/Context adaptations. Invalid models still reject before output. See [constant acceptance](constants_acceptance_5_0_6.md). |
| Typed checker/behavior scaffolds and candidate shortcuts | Emitted over runtime protocols. Candidate and bootstrap evidence compose in [P3 acceptance](constants_acceptance_5_0_6.md); full general Checker/behavior configuration retains its own G02/G03 obligations. |
| Dynamic JSON request decoding, relation aggregates/facets and pagination optimization helpers | Generated adapters use the shared runtime. The capability ledger records scoped source/execution evidence and remaining complete model/configuration qualification. |
| Rust Cargo/web/workspace integrations and external backends | Outside the agreed C++ edge scope. Only SQLite is generated. |
| Native named-module ownership for generated domain declarations | Deferred. Generated headers can consume the existing qualified runtime module frontend. |

This adds a usable C++ generation frontend; it does not claim complete Rust
generator parity. Existing handwritten examples remain unchanged reference
flows. Current Q/E validation is recorded in
[the change plan](query_expression_change_plan.md). The
[initial generation evidence](../measurements/generator/2026_10_11_cpp_frontend/README.md)
is historical and does not describe the new naming/binding surface.


Generated dynamic-carrier checkpoint adds the shared GeneratedDynamicFields
helper, thin indexed Model install/mutation adapters, camelCase application
methods and ref-qualified Q selectDynamicFields. All five fixtures regenerate
and reproduce (55 files), selected Java tests pass 11/11, and 80 direct frozen
entity-macro observations match. Native 5/5, targeted sanitizer 5/5 (leak scanning
disabled), modules 3/3, formatting 804 files, 1,406 exports and seven facade
archives pass. See [generated_dynamic_fields_5_0_6.md](generated_dynamic_fields_5_0_6.md).
The original generator checkout remains untouched. Full readonly, generated
configuration and remaining G01-G17/P0-P7 acceptance stay open.
