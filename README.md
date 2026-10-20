# TeaQL C++

An AI-native library and runtime for building reviewable business applications
in C++20, with SQLite and explicit memory lifetimes for edge deployment.

TeaQL brings model-driven business operations into the C++ ecosystem. It gives
coding agents typed business APIs, model-aware guidance and software checks.
People review the business operation while shared query construction,
relationship loading and persistence mechanics live in the runtime.

Applications can combine these APIs with existing C++ system code and native
resources. Context owns the lifetime of TeaQL-managed data and registered
resources; ordinary C++ pointers and references remain application-managed
borrows.

Automatic execution logs connect business intent to SQL, timing and outcomes
without adding logging statements to every business method.

[Run the local example](#run-the-local-example) ·
[Explore capabilities](#business-capabilities) ·
[Memory lifetimes](#explicit-memory-and-resource-lifetimes) ·
[Customize the runtime](#customize-the-runtime-for-your-application) ·
[Use with a coding agent](#try-it-with-your-coding-agent)

The reference design is **Rust 5.0.6**, pinned in
[upstream.lock.json](upstream.lock.json). C++ runtime and generator alignment is
in progress. This README describes implemented capabilities and links to their
examples and scoped verification. Complete cross-language parity remains an
active migration goal. The Rust reference version is separate from the C++
release version.

Start with the retained generated Order example below: it runs against SQLite
and demonstrates typed Q/E, audited graph Save and list, one and stream execution.

## Why this is an AI-native library

AI-native means the APIs, guidance and verification workflow are designed for
how coding agents work: limited context, incomplete project knowledge and a
tendency to guess unfamiliar operations. Ordinary queries execute through the
runtime and SQLite; they do not call an LLM.

A semantic model describes business objects, fields, relationships and states.
The domain generator turns those concepts into language-native **Q** queries,
**E** expressions and mutation APIs. This repository provides the reusable,
domain-independent runtime beneath them: metadata, Context, Checker, Policy,
data services and database execution.

Generated libraries include `AGENTS.md`, `ASSIST.md` and linked entity/action
and field guides. Agents discover the API needed next, then use compilation,
runtime checks and tests for feedback. Generation and guidance are development
tools; generated C++ code compiles into the application.

## Less code for a reviewer to reconstruct

| Application code to review | Shared work handled by TeaQL |
| --- | --- |
| A composed Q request | SQL construction, projections and selected relationship loading |
| An E access path | Loaded-field checks and explicit NULL handling |
| A mutation with an audit reason | Change tracking, Checker/Fix, configured Policy and persistence |
| Business intent | Automatic execution logs and trace propagation |

Reviewers still decide whether data access, business rules and authorization
are correct. Compilation verifies API use; application tests must verify
business behavior. **Token savings have not been measured in a controlled
comparison.**

## Run the local example

From this checkout in Bash:

```bash
set -e
python3 scripts/toolchains/setup.py
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_generated_query_expression
./build_native_current/teaql_generated_query_expression
ctest --preset native --output-on-failure --no-tests=error \
    -R '^example_generated_query_expression$'
```

The [locked development toolchain](docs/development_toolchain.md) targets Linux
x86_64. Setup requires Python 3, network access, tar, zstd and system libc
development headers. It downloads verified tools into `.tools`; check an
existing installation with `python3 scripts/toolchains/setup.py --check`.

The example creates a fresh in-memory SQLite database on every run and prints:

```text
generated-order: keyboard
```

The retained generated code is checked in. Running it requires no database
server, generator, Java or Maven. Diagnostic logs default to the executable
filename plus `.log`.

## See a business operation execute

The [generated Order example](examples/generated_query_expression.cpp)
initializes SQLite, saves a product and an order graph with audit reasons, then
loads the order, its lines and their products through generated APIs.

Bootstrap owns GlobalContext and creates UserContext. Business functions receive
UserContext and request global allocation through it. The model-specific names
below come from the retained Order example:

```cpp
#include "generated/include/cpp_examples.hpp"

teaql::GlobalContext global;
auto user_context_owner = global.make_user_context();
auto& user_context = *user_context_owner;
cpp_examples::setup(user_context);
```

After setup and seeding, a business function composes a query and reads the loaded
graph using only its UserContext:

```cpp
void displayOrder(teaql::UserContext& user_context) {
    auto Q = user_context.queries<cpp_examples::query_factory>();
    auto products = Q.products().selectName();
    auto lines = Q.orderLines().selectName().selectProductWith(products);
    auto request = Q.orders()
                       .withNameIs("generated-order")
                       .selectName()
                       .selectLinesWith(lines)
                       .comment("Load the order with its product")
                       .purpose("Display the generated order graph");

    const auto& orders = cpp_examples::require_value(request.executeForList(user_context).wait());
    if (orders.len() != 1) {
        throw std::runtime_error("Expected one generated order");
    }
    auto product_name =
        cpp_examples::E::order(orders[0]).getLines().first().getProduct().getName().unwrap();
    std::cout << product_name << '\n';
}
```

This function uses the generated header above and the standard `<iostream>` and
`<stdexcept>` headers. The complete runnable example includes initialization,
seeding and error handling.

`Q.orderLines()` takes no Context argument: the factory carries its binding.
Queries and expressions can be composed and passed through functions before
execution. Their dynamic data belongs to that Context, even if they are never
executed. Execution receives the same `user_context`.

E reads loaded data without implicit database queries. **Value**, **NULL** and
**NotLoaded** remain distinct, so an omitted projection cannot silently become
a business conclusion.

Before querying the graph, the same example creates and saves a product:

```cpp
auto Q = user_context.queries<cpp_examples::query_factory>();
auto& product = Q.products()
                    .comment("Construct the example product")
                    .purpose("Prepare an audited product")
                    .newEntity(user_context);
product.updateName("keyboard");
cpp_examples::require_value(
    cpp_examples::AuditedSaveExt<cpp_examples::Product>::save(
        std::move(product).audit_as("Seed the generated example product"), user_context)
        .wait());
```

The complete example also saves the order and its child, and exercises
`executeForOne` and `executeForStream`. `executeForOne` returns an optional first
match; it does not assert uniqueness. Application-facing Q/E/model methods use
lowerCamelCase. Runtime protocols such as `audit_as` keep snake_case; the local
query entry stays `Q`.

## Business capabilities

The following capabilities have implemented C++ paths and scoped verification.
Each link identifies the supported models, failure cases and qualification
boundaries; a verified path does not establish complete Rust feature parity.

| Capability | Implemented behavior | Details and examples |
| --- | --- | --- |
| **Q: queries and analytics** | Typed filters, projections, sorting, pagination, subqueries, selected relations and scalar grouped aggregates. | [Query acceptance](docs/general_query_acceptance_review_5_0_6.md), [aggregate execution](docs/generated_execution_shapes_5_0_6.md) |
| **E: explicit business reads** | Checked scalar, object and list traversal with Value, NULL, NotLoaded and missing-load diagnostics. | [Expression acceptance](docs/expression_acceptance_5_0_6.md) |
| **DDD and audited graph Save** | Stage additions, updates and deletions within an application-defined aggregate; run Checker/Fix and configured Mutation Policy before transactional persistence. | [Order graph](examples/sqlite_relations_graph.cpp), [mutation composition](docs/generated_ledger_composition_5_0_6.md) |
| **Schema and bootstrap** | Create missing modeled tables, add missing columns and initialize modeled roots and seed data through governed operations. | [Schema example](examples/sqlite_schema_crud.cpp), [bootstrap acceptance](docs/constants_acceptance_5_0_6.md) |
| **Business IDs** | Generated identifier types, Context key/allocation services and SQLite-backed allocation with recovery and retry checks. | [Generated IDs](docs/generated_business_ids_5_0_6.md), [recovery](docs/sqlite_business_id_recovery_5_0_6.md) |
| **Dynamic fields** | Trusted field definitions, SQLite extension storage, selected-field hydration and governed Set, NULL and Delete mutations. | [Storage acceptance](docs/dynamic_storage_acceptance_5_0_6.md) |
| **Typed JSON and inheritance** | Model-bound decoding preserves omitted fields, NULL and nested views; inherited fields support typed queries and audited graph Save. | [JSON boundaries](docs/generated_json_boundaries_5_0_6.md), [inheritance acceptance](docs/inheritance_acceptance_5_0_6.md) |
| **Constants and candidates** | Typed constant APIs, candidate predicates and governed seed initialization. | [Acceptance](docs/constants_acceptance_5_0_6.md) |
| **List, one, page and stream execution** | Typed lists, optional first matches, count-bearing pages and incremental scalar cursor reads. | [Execution example](examples/generated_query_expression.cpp), [pages](docs/deferred_plain_page_5_0_6.md), [streams](docs/dynamic_json_execution_5_0_6.md) |

Graph Save atomicity follows the provider and route; modified persisted objects
require complete scalar loading. Schema reconciliation adds tables and columns.
Streaming covers scalar entity rows; relation and aggregate enhancement are
outside that streaming scope. Available SQL functions depend on the SQLite
build.

## Operability and security

Queries require nonblank `comment` (what) and `purpose` (why); mutations declare
`audit_as`. Intent validation remains active when logging is disabled.

| Capability | Behavior and boundary | Details |
| --- | --- | --- |
| **Automatic diagnostics** | Connect intent, trace, parameterized SQL, timing and outcome through configured logging. | [Logging evidence](docs/logging_baselines_5_0_6.md) |
| **Application-owned authorization** | Request Policy restricts queries; configured Mutation Policy reviews the immutable change plan and can deny writes. Applications supply tenant and authorization rules. | [Query policy](docs/dynamic_json_policy_5_0_6.md), [Mutation Policy](docs/mutation_policy_api_contract.md) |
| **Audit and concurrency** | Optimistic versions reject stale writes. Application mutation audits are delivered after commit and discarded on rollback. | [Save and audit](docs/dynamic_save_audit_5_0_6.md) |
| **Sensitive-data protection** | Diagnostic projections mask sensitive bindings, exclude credentials and restrict value-bearing output. Masked SQL is labeled `MASKED; NOT REPLAYABLE`. | [Audit privacy](docs/audit_event_provenance_5_0_6.md), [query privacy](docs/dynamic_json_privacy_5_0_6.md) |
| **Trusted input boundaries** | Model-bound JSON rejects unknown keys and duplicate aliases. Decoded presentation data does not authorize persistence. | [JSON hydration](docs/json_hydration_5_0_6.md), [alias validation](docs/generated_json_column_alias_5_0_6.md) |
| **Natural-language findings** | Context language and replaceable I18n catalogs render validation findings using installed model vocabulary. | [Generated catalog](docs/generated_i18n_5_0_6.md) |

Audit delivery is not a durable outbox: a delivery failure after commit does not
mean the transaction rolled back. Missing customer Mutation Policy or exact
approval produces a governance warning; explicit denial blocks persistence.
Raw SQL entry points accept trusted application code; query filters do not
sanitize a literal SQL statement or establish authorization for it.

## Explicit memory and resource lifetimes

TeaQL uses two explicit ownership scopes:

- **GlobalContext** owns application-lifetime state. Bootstrap owns it; business
  operations request global allocation through UserContext.
- **UserContext** owns its objects, query state and results. Closing it destroys
  registered objects and resources, then returns every block of its growing arena.

Managed objects use the scope's `make<T>()`; native RAII resources use
`make_resource<T>()`. Business code requests global objects through
`user_context.make_global<T>()`. Nested strings and containers use their owner's
allocator. Retention across scopes requires an explicit destination copy.

Each scope has allocation identities, an exact live bitmap and configurable
object/byte limits. Strings and byte buffers use owner-local storage without
consuming an object identity for each backing allocation. String interning is
explicit.

Checked references validate identity and liveness. Raw pointers, references and
ordinary Q/E wrappers remain borrows; they do not extend Context lifetime.
Allocator-aware stack values must be destroyed before their storage owner.
Asynchronous work must finish before referenced storage is reclaimed. Streamed
rows remain in the arena until Context close, so bound result volume and Context
lifetime on edge devices.

See the [memory implementation](docs/context_memory_implementation.md),
[lifetime contract](docs/context_lifetime.md) and
[ownership example](examples/context_lifetime.cpp).

## Customize the runtime for your application

Install application-owned capabilities through trusted UserContext assembly:

| Extension point | Application customization | Reference |
| --- | --- | --- |
| **Checker / Fix and Behavior** | Domain validation, repair and entity-service hooks alongside generated checks. | [Customization example](examples/custom_checker_behavior.cpp) |
| **Request and Mutation Policies** | Query restrictions, whole-plan decisions and approval providers. | [Mutation Policy contract](docs/mutation_policy_api_contract.md) |
| **Data services and schema** | Executor registration, routing and schema-provider contracts. | [Runtime module map](docs/parity/cpp_module_map.md) |
| **IDs and business time** | Internal identity generation, business-ID allocation and business clock. | [Context API](teaql_runtime/include/teaql/runtime/context.hpp) |
| **Logging and audit sinks** | Diagnostic delivery and entity-event handling subject to privacy controls. | [Logging contract](docs/runtime_log_api_contract.md) |
| **Language and messages** | Context language and validation catalogs. | [I18n contract](docs/i18n_api_contract.md) |
| **Dynamic-field storage** | Trusted metadata and provider-backed storage/loading. | [Storage SPI](docs/dynamic_storage_spi_5_0_6.md) |

Configure the generated runtime first, apply customization, then initialize
schema and run operations. Reconfiguration installs generated defaults again.
The custom Checker and Behavior example exercises this order.

## Discover the API needed next

Start with generated [AGENTS.md](examples/generated/AGENTS.md) and
[ASSIST.md](examples/generated/ASSIST.md). Follow entity/action links, then field
guides for exact C++ methods. Guidance uses canonical model names to locate the
language-native API. Report `MISSING_ASSIST` when guidance is absent rather than
inventing methods.

The generator lives in the independent `teaql_code_gen_cpp` checkout on branch
`cpp_support`, using Java/Maven tooling. Regenerate domain code rather than
editing generated headers; see the [generator guide](docs/cpp_generator.md).

## Try it with your coding agent

Paste this prompt into your coding agent from this checkout:

```text
Read AGENTS.md, examples/generated/AGENTS.md and examples/generated/ASSIST.md.
Start from examples/generated_query_expression.cpp and implement a small C++
order-management operation using SQLite and the documented Q/E and audited
Save APIs. Pass UserContext into business functions and respect its lifetime.
Follow docs/development_toolchain.md, build and run the example, and verify
business results, intent and audit behavior. Report commands, outcomes and
missing API guidance. Do not claim token savings without a measured comparison.
```

## Packages and supported environment

The runtime targets edge applications and uses C++20 standard facilities,
platform threading and SQLite. SQLite is the only database provider.
Application-owned RAII resources provide a boundary for integrating existing
C++ libraries while preserving Context cleanup order.

| Package | Responsibility |
| --- | --- |
| [teaql_core](teaql_core/) | Values, metadata, queries, mutations and traces |
| [teaql_data_service](teaql_data_service/) | Data-service and executor contracts |
| [teaql_sql](teaql_sql/) | SQL construction, execution and transport contracts |
| [teaql_runtime](teaql_runtime/) | Context, graph execution, Checker, Policy, logging and audit |
| [teaql_provider_sqlite](teaql_provider_sqlite/) | SQLite schema, queries and persistence |
| [teaql_memory](teaql_memory/) | Arenas, limits and checked references |
| [teaql_async](teaql_async/) | Context-bound asynchronous execution and cleanup |

The `native` preset uses headers. All seven packages also expose C++20 named
modules through the `modules_native` frontend; see the
[module compiler matrix](docs/module_compiler_matrix.md) for its qualified
Clang configuration.

OpenWrt MIPS uses the header frontend. Recorded
[WNDR4300 measurements](measurements/mips-wndr4300/2026-10-05-linkage/README.md)
cover artifact sizes and QEMU evidence at their stated checkpoints.
Physical-device execution remains unverified.

## Use the runtime in your application

The CMake installation exports all seven runtime packages. A header-based
application using SQLite links the provider target, which carries its runtime
dependencies:

```cmake
find_package(teaql_cpp CONFIG REQUIRED)

add_executable(order_app main.cpp)
target_link_libraries(order_app PRIVATE teaql::provider_sqlite)
```

Set `CMAKE_PREFIX_PATH` to the installed prefix when configuring the application.
Generate the application-specific domain library separately and add its include
directory to the application target.

See [installed SDK usage](docs/module_packaging.md) for installation and header
and named-module consumers, and [domain generation](docs/cpp_generator.md) for
the independent development workflow.

## More local examples

| Example | Build target | What it exercises |
| --- | --- | --- |
| [Generated Q/E](examples/generated_query_expression.cpp) | `teaql_generated_query_expression` | Audited graph Save, nested reads and list/one/stream execution |
| [Custom Checker and Behavior](examples/custom_checker_behavior.cpp) | `teaql_custom_checker_behavior` | Application-owned checks and query behavior |
| [Schema and CRUD](examples/sqlite_schema_crud.cpp) | `teaql_sqlite_schema_crud` | Schema setup, audited changes and soft deletion |
| [Order graph](examples/sqlite_relations_graph.cpp) | `teaql_sqlite_relations_graph` | Order/child Save and selected relationship loading |
| [Context lifetime](examples/context_lifetime.cpp) | `teaql_context_lifetime` | Allocation scopes, checked references, copies and cleanup |

After configuring the native preset, build and test all five examples with:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_query_expression \
    teaql_custom_checker_behavior teaql_sqlite_schema_crud \
    teaql_sqlite_relations_graph teaql_context_lifetime
ctest --preset native --output-on-failure --no-tests=error \
    -R '^example_(generated_query_expression|custom_checker_behavior|sqlite_schema_crud|sqlite_relations_graph|context_lifetime)$'
```

## Verification and current status

Software checks complement the generated guidance:

| Stage | Checks and feedback |
| --- | --- |
| Generate | Model metadata validation and reproducible generated artifacts |
| Compile | Typed Q/E/model APIs, result types and supported operation contracts |
| Execute | Context binding, required intent, loaded state, Checker and configured Policy |
| Test | Business results, database readback, rollback, audit delivery and lifetime boundaries |

Successful generation or compilation alone does not establish that an
application's business decisions are correct.

The [generator parity plan](docs/cpp_generator_parity_plan.md),
[capability ledger](docs/cpp_generator_parity_ledger.json) and
[integration record](docs/native_integration_5_0_6.md) distinguish focused
acceptance from complete baseline qualification. Historical suite totals apply
to their recorded checkpoints. Check the pinned reference before making a
runtime or generator change:

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_upstream.py
```

Routine development uses relevant native tests. Memory/lifetime changes use
selected ASan/UBSan checks; architecture changes use selected MIPS/QEMU checks.
See [development instructions](AGENTS.md) and the
[baseline upgrade audit](docs/baseline_5_0_6_upgrade.md).

Before finishing handwritten C++ changes, apply and check the pinned format:

```bash
python3 scripts/format_cpp.py
python3 scripts/format_cpp.py --check
```

The formatter uses four-space indentation and braces around control-flow bodies.
It is a development tool and adds no runtime dependency.

## Logging and troubleshooting

Configure the built-in logging backend before initialization. Logging settings
are cached in application-owned state.

| Variable | Default / accepted values | Purpose |
| --- | --- | --- |
| `TEAQL_LOG_ENDPOINT` | Executable filename plus `.log`; accepts `stdout`, a file path or `off` | Select the ordinary diagnostic destination. `off` leaves intent validation and Policy active. |
| `TEAQL_DOMAIN` | Unset | Use `<domain>.log` when no endpoint is configured. |
| `TEAQL_LOG_FORMAT` | Human-readable; `debug` or `json` selects the Debug formatter | Select diagnostic rendering. The `json` spelling selects Debug text, not a JSON serialization contract. |
| `TEAQL_SQL_LOG` | `_summary` | Set the SQL diagnostic level. |
| `TEAQL_AUDIT_LOG` | `_full` | Set the audit diagnostic level. |
| `TEAQL_SQL_LOG_TABLES` | Unset; comma-separated names | Filter diagnostic SQL by table-name text; this is not an authorization rule. |
| `TEAQL_AUDIT_LOG_ENTITIES` | Unset; comma-separated names | Filter audit output by entity name. |

Levels accept `_silent`, `_summary`, `_full` and `_full_with_payload`; unknown
values use the listed defaults. See the
[logging contract](docs/runtime_log_api_contract.md) for additional filters,
endpoints and privacy controls.

Start with masked diagnostics and keep secrets out of comments, purposes and
audit reasons. A payload level alone does not authorize sensitive plaintext.
Controlled plaintext requires the exact acknowledgement and separate destination
specified in the logging contract. Credentials remain protected; custom sinks
have their own configuration.
