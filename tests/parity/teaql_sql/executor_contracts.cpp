// Original executor/cache/probe tests with controlled transport and explicit scopes.
#include <teaql/sql/executor.hpp>
#include <teaql/runtime/context.hpp>
#include <charconv>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class TestDialect : public SqlDialect {
public:
    bool probes = false;
    mutable unsigned compilations = 0;

    DatabaseKind kind() const override {
        return DatabaseKind::Sqlite;
    }

    String quote_ident(Memory owner, std::string_view name) const override {
        String result("\"", owner);
        result += name;
        result += '"';
        return result;
    }

    String placeholder(Memory owner, std::size_t index) const override {
        String result("?", owner);
        if (!probes) {
            char buffer[32];
            auto end = std::to_chars(buffer, buffer + sizeof(buffer), index).ptr;
            result.append(buffer, end);
        }
        return result;
    }

    bool prefers_small_parent_relation_probes() const override {
        return probes;
    }

    CompiledQuery compile_select(Memory owner,
                                 const EntityDescriptor& entity,
                                 const SelectQuery& query) const override {
        ++compilations;
        return SqlDialect::compile_select(owner, entity, query);
    }
};

// Synthetic extension-hook fixture only: no PostgreSQL or SQLite array backend.
class ArrayTestDialect final : public TestDialect {
public:
    bool large_in_uses_array_param() const override {
        return true;
    }

    String compile_in(Memory owner,
                      const EntityDescriptor& entity,
                      const Expr& left,
                      BinaryOp op,
                      const Expr& right,
                      SqlBindings& bindings) const override {
        if ((op == BinaryOp::InLarge || op == BinaryOp::NotInLarge) &&
            right.kind == Expr::Kind::Value && right.literal.as_list()) {
            auto output = compile_expr(owner, entity, left, bindings);
            bindings.push(right.literal);
            String result("(", owner);
            result += output;
            result += op == BinaryOp::InLarge ? " = ANY(" : " <> ALL(";
            result += placeholder(owner, bindings.size());
            result += "))";
            return result;
        }
        throw SqlCompileError(std::allocator_arg,
                              Allocator(owner),
                              SqlCompileError::Code::InvalidFunctionArguments,
                              "array test dialect only supports large IN");
    }
};

template <class T>
struct Ready {
    using allocator_type = Allocator;
    T value;

    Ready(std::allocator_arg_t, Allocator a, T value)
        : value(detail::move_in(a.resource(), std::move(value))) {}

    std::optional<T> poll(WakeSignal&) {
        return std::move(value);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

class EmptyTransport final : public SqlTransport<String> {
public:
    unsigned repeated_calls = 0, single_calls = 0;

    DataServiceOperationResult<Rows, String>& fetch_all_compact_sql(Memory owner,
                                                                    const CompiledQuery&) override {
        ++single_calls;
        using Output = Result<Rows, String>;
        return make_operation<Output, Ready<Output>>(owner, Output::ok(owner, Rows(owner)));
    }

    DataServiceOperationResult<Rows, String>&
    fetch_repeated_compact_sql(Memory owner,
                               const CompiledQuery& query,
                               std::size_t parameter,
                               std::span<const Value> values) override {
        ++repeated_calls;
        check(values.size() == 2 && values[0] == Value(owner, std::uint64_t(7)) &&
                  values[1] == Value(owner, std::uint64_t(9)) &&
                  query.params.at(parameter) == values[0],
              "original repeated boundary values and template binding");
        using Output = Result<Rows, String>;
        return make_operation<Output, Ready<Output>>(owner, Output::ok(owner, Rows(owner)));
    }

    DataServiceOperationResult<std::uint64_t, String>& execute_sql(Memory owner,
                                                                   const CompiledQuery&) override {
        using Output = Result<std::uint64_t, String>;
        return make_operation<Output, Ready<Output>>(owner, Output::ok(owner, 0));
    }
};

static EntityDescriptor test_entity(Memory owner) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "Order");
    result.property("id", DataType::U64).id().not_null();
    result.property("name", DataType::Text);
    result.mask_fields({});
    return result;
}

class CountingSchema final : public SchemaProvider {
public:
    const EntityDescriptor& entity;
    mutable unsigned lookups = 0;

    explicit CountingSchema(const EntityDescriptor& entity) : entity(entity) {}

    const EntityDescriptor* get_entity(std::string_view name) const override {
        ++lookups;
        return name == "Order" ? &entity : nullptr;
    }
};

using Executor = SqlDataServiceExecutor<String>;

struct Fixture {
    GlobalContext global;
    TestDialect dialect;
    ArrayTestDialect array_dialect;
    EntityDescriptor entity;
    CountingSchema schema;
    EmptyTransport transport;
    SqlExecutorState cache;
    Executor executor;
    std::unique_ptr<UserContext> user;
    Memory owner;

    explicit Fixture(bool probes = false, bool array = false)
        : entity(test_entity(global.memory_resource())), schema(entity),
          cache(std::allocator_arg, Allocator(global.memory_resource())),
          executor(std::allocator_arg,
                   Allocator(global.memory_resource()),
                   array ? static_cast<const SqlDialect&>(array_dialect)
                         : static_cast<const SqlDialect&>(dialect),
                   transport,
                   schema,
                   cache),
          user(global.make_user_context()), owner(user->memory_resource()) {
        dialect.probes = probes;
    }

    SelectQuery query() {
        return SelectQuery(std::allocator_arg, Allocator(owner), "Order");
    }

    QueryRequest request(const SelectQuery& query, bool debug = false, bool metadata = true) {
        auto intent = QueryIntent::create(
            owner, "exercise SQL executor fixture", "verify cache and partition contracts");
        return QueryRequest(
            std::allocator_arg, Allocator(owner), query, intent.value(), debug, metadata);
    }

    const QueryResult& run(const SelectQuery& query, bool debug = false, bool metadata = true) {
        const auto& result = executor.query(owner, request(query, debug, metadata)).wait();
        check(result.is_ok(), "original empty transport query succeeds");
        return result.value();
    }
};

static void caches_entity_descriptors_across_executor_clones() {
    Fixture f;
    auto query = f.query();
    auto& result = f.run(query);
    auto clone = detail::clone_in(f.global.memory_resource(), f.executor);
    check(clone.query(f.owner, f.request(query, true)).wait().is_ok(),
          "original cloned executor query succeeds");
    check(f.schema.lookups == 1 && !result.metadata.debug_query,
          "original shared descriptor cache and absent debug expansion");
}

static void cached_query_keeps_binding_policies_and_checks_descriptor_identity() {
    Fixture f;
    auto private_entity = test_entity(f.owner);
    private_entity.mask_fields({"name"});
    for (auto value : {"first", "second"}) {
        auto query = f.query();
        query.filter(Expr::eq(f.owner, "name", value));
        auto compiled = f.cache.compile_select_cached(f.owner, f.dialect, private_entity, query);
        check(compiled.params.size() == 1 && compiled.params[0] == Value(f.owner, value) &&
                  compiled.log_context.parameter_policies ==
                      Vector<SqlParameterLogPolicy>({SqlParameterLogPolicy::Masked}, f.owner) &&
                  compiled.log_context.generated_sql,
              "original cached private values and policies");
    }
    check(f.dialect.compilations == 1, "original second call really hits cache");
    auto query = f.query();
    query.filter(Expr::eq(f.owner, "name", "public"));
    auto compiled = f.cache.compile_select_cached(f.owner, f.dialect, f.entity, query);
    check(compiled.log_context.parameter_policies ==
                  Vector<SqlParameterLogPolicy>({SqlParameterLogPolicy::Plain}, f.owner) &&
              f.dialect.compilations == 2,
          "original descriptor privacy identity separates cached plans");
}

static void batch_metadata_keeps_each_statement_and_its_numbered_bindings() {
    Fixture f;
    MutationRequest::Batch batch(f.owner);
    for (auto value : {"first", "second"}) {
        InsertCommand command(std::allocator_arg, Allocator(f.owner), "Order");
        command.value("name", value);
        batch.emplace_back(std::move(command), "ported batch child");
    }
    auto request = MutationRequest(std::allocator_arg,
                                   Allocator(f.owner),
                                   std::move(batch),
                                   "ported mutation execution fixture");
    const auto& result = f.executor.mutate(f.owner, std::move(request)).wait();
    check(result && result.value().metadata.statements.size() == 2,
          "original two batch statements");
    for (std::size_t index = 0; index < 2; ++index) {
        const auto& child = result.value().metadata.statements[index];
        check(child.params.size() == 1 &&
                  child.params[0] == Value(f.owner, index == 0 ? "first" : "second") &&
                  child.parameterized_query &&
                  child.parameterized_query->find("?1") != String::npos &&
                  child.sql_log.parameter_policies ==
                      Vector<SqlParameterLogPolicy>({SqlParameterLogPolicy::Plain}, f.owner) &&
                  !child.debug_query,
              "original per-statement bindings restart at the first numbered parameter");
    }
}

static void skips_execution_metadata_when_caller_will_discard_it() {
    Fixture f;
    const auto& metadata = f.run(f.query(), false, false).metadata;
    check(metadata.backend.empty() && metadata.started_at == ExecutionMetadata::SystemTime{} &&
              !metadata.parameterized_query && metadata.params.empty() &&
              metadata.trace_chain.empty(),
          "all original unrecorded metadata fields");
}

static void sql_executor_never_eagerly_expands_private_values_even_when_diagnostics_requested() {
    Fixture f;
    auto query = f.query();
    query.filter(Expr::eq(f.owner, "name", "PRIVATE-BINDING-CANARY"));
    const auto& result = f.run(query, true);
    check(!result.metadata.debug_query && result.metadata.parameterized_query &&
              result.metadata.parameterized_query->find("PRIVATE-BINDING-CANARY") == String::npos &&
              result.metadata.params.size() == 1 &&
              result.metadata.params[0] == Value(f.owner, "PRIVATE-BINDING-CANARY"),
          "original requested diagnostics never expand private query bindings");
    InsertCommand command(std::allocator_arg, Allocator(f.owner), "Order");
    command.value("name", "PRIVATE-BINDING-CANARY");
    const auto& inserted = f.executor
                               .mutate(f.owner,
                                       MutationRequest(std::allocator_arg,
                                                       Allocator(f.owner),
                                                       std::move(command),
                                                       "ported mutation execution fixture"))
                               .wait();
    check(inserted && !inserted.value().metadata.debug_query &&
              inserted.value().metadata.params.size() == 1 &&
              inserted.value().metadata.params[0] == Value(f.owner, "PRIVATE-BINDING-CANARY"),
          "original insert retains private binding without eager debug rendering");
}

static SelectQuery partition_query(Fixture& f) {
    Vector<Value> values(f.owner);
    values.emplace_back(std::uint64_t(7));
    values.emplace_back(std::uint64_t(9));
    auto query = f.query();
    query.filter(Expr::in_list(f.owner, "id", values)).order_desc("id").limit(1).partition_by("id");
    return query;
}

static void topn_004_011_sqlite_reuses_one_repeated_probe_boundary() {
    Fixture f(true);
    const auto& result = f.run(partition_query(f), false, false);
    check(result.rows.empty() && f.transport.repeated_calls == 1 && f.transport.single_calls == 0 &&
              result.metadata.started_at == ExecutionMetadata::SystemTime{},
          "original repeated probe boundary and no metadata");
}

static void keeps_partition_query_observable_when_metadata_is_enabled() {
    Fixture f(true);
    (void)f.run(partition_query(f));
    check(f.transport.repeated_calls == 0 && f.transport.single_calls == 1,
          "original observed partition query uses normal transport");
}

static void topn_001_explicit_zero_threshold_forces_window_for_probe_provider() {
    Fixture f(true);
    auto query = partition_query(f);
    query.top_n_probe_parent_threshold(0);
    (void)f.run(query, false, false);
    check(f.transport.repeated_calls == 0 && f.transport.single_calls == 1,
          "original explicit zero disables probe optimization");
}

static void topn_007_probe_rewrites_only_partition_membership_filter(Memory owner) {
    Vector<Value> values(owner);
    values.emplace_back(std::uint64_t(11));
    values.emplace_back(std::uint64_t(12));
    auto policy = Expr::eq(owner, "tenant_id", std::uint64_t(7));
    auto visibility = Expr::gt(owner, "version", std::int64_t(0));
    auto business = Expr::eq(owner, "status", "ACTIVE");
    Vector<Expr> filters(owner);
    filters.emplace_back(Expr::in_list(owner, "owner_id", values));
    filters.emplace_back(policy);
    filters.emplace_back(visibility);
    filters.emplace_back(business);
    SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
    query.filter(Expr::all_of(owner, filters)).order_desc("id").limit(3).partition_by("owner_id");
    auto probe =
        detail::scalar_partition_probe_query(owner, query, Value(owner, std::uint64_t(11)));
    filters[0] = Expr::eq(owner, "owner_id", std::uint64_t(11));
    auto expected = Expr::all_of(owner, filters);
    check(probe && probe->predicate && *probe->predicate == expected && !probe->partition_field &&
              probe->order_by == query.order_by && probe->slice == query.slice,
          "original probe preserves tenant, visibility, business, sort and slice");
}

static void cached_select_plan_rebinds_values_and_separates_in_list_lengths() {
    Fixture f;
    auto first_query = f.query();
    first_query.filter(Expr::eq(f.owner, "id", std::uint64_t(7)));
    auto second_query = f.query();
    second_query.filter(Expr::eq(f.owner, "id", std::uint64_t(9)));
    const auto& first = f.run(first_query);
    const auto& second = f.run(second_query);
    check(first.metadata.parameterized_query == second.metadata.parameterized_query &&
              first.metadata.params.size() == 1 &&
              first.metadata.params[0] == Value(f.owner, std::uint64_t(7)) &&
              second.metadata.params.size() == 1 &&
              second.metadata.params[0] == Value(f.owner, std::uint64_t(9)),
          "original same-shape plan rebinds current values");
    Vector<Value> values(f.owner);
    values.emplace_back(std::uint64_t(1));
    values.emplace_back(std::uint64_t(2));
    auto short_query = f.query();
    short_query.filter(Expr::in_list(f.owner, "id", values));
    const auto& short_result = f.run(short_query);
    values.emplace_back(std::uint64_t(3));
    auto long_query = f.query();
    long_query.filter(Expr::in_list(f.owner, "id", values));
    const auto& long_result = f.run(long_query);
    check(short_result.metadata.parameterized_query != long_result.metadata.parameterized_query &&
              short_result.metadata.params.size() == 2 && long_result.metadata.params.size() == 3,
          "original IN cardinality separates plan shapes");
}

static void cached_select_plan_rebinds_large_in_as_one_array_parameter() {
    Fixture f(false, true);
    Vector<Value> first_values(f.owner), second_values(f.owner);
    for (std::uint64_t value = 1; value <= 21; ++value) {
        first_values.emplace_back(value);
        second_values.emplace_back(value + 100);
    }
    auto first_query = f.query();
    first_query.filter(Expr::in_list(f.owner, "id", first_values));
    auto second_query = f.query();
    second_query.filter(Expr::in_list(f.owner, "id", second_values));
    const auto& first = f.run(first_query);
    const auto& second = f.run(second_query);
    check(
        first.metadata.parameterized_query == second.metadata.parameterized_query &&
            first.metadata.params.size() == 1 &&
            first.metadata.params[0] == Value(f.owner, first_values) &&
            second.metadata.params.size() == 1 &&
            second.metadata.params[0] == Value(f.owner, second_values),
        "original abstract array-dialect hook retains one complete List parameter across rebinds");
}

static void cached_select_plan_preserves_parameter_order_for_supported_query_shapes() {
    Fixture f;
    auto rebound = [&](const SelectQuery& warm, const SelectQuery& current) {
        (void)f.run(warm);
        const auto& actual = f.run(current).metadata;
        auto expected = f.dialect.compile_select(f.owner, f.entity, current);
        check(actual.parameterized_query && *actual.parameterized_query == expected.sql &&
                  actual.params == expected.params,
              "original cached binding order equals direct uncached production compilation");
    };
    auto first = f.query(), second = f.query();
    first.search_with_text("first");
    second.search_with_text("second");
    rebound(first, second);
    auto projections = [&](std::int64_t value) {
        auto query = f.query();
        query.project_expr("marker", Expr::value(f.owner, value))
            .filter(Expr::eq(f.owner, "id", std::uint64_t(value + 1)))
            .having(Expr::gt(f.owner, "id", std::uint64_t(value + 2)))
            .add_order(OrderBy::asc_expr(Expr::value(f.owner, value + 3)));
        return query;
    };
    rebound(projections(1), projections(11));
    auto partition = [&](std::uint64_t value) {
        auto query = f.query();
        query.filter(Expr::eq(f.owner, "id", value))
            .add_order(OrderBy::asc_expr(Expr::value(f.owner, std::int64_t(value + 1))))
            .page(0, 10)
            .partition_by("name");
        return query;
    };
    rebound(partition(1), partition(3));
    auto subquery = [&](std::uint64_t value) {
        auto child = f.query();
        child.filter(Expr::gt(f.owner, "id", value));
        auto query = f.query();
        query.filter(Expr::in_subquery(f.owner, "id", f.entity, child, "id"));
        return query;
    };
    rebound(subquery(20), subquery(30));
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        caches_entity_descriptors_across_executor_clones();
        cached_query_keeps_binding_policies_and_checks_descriptor_identity();
        batch_metadata_keeps_each_statement_and_its_numbered_bindings();
        skips_execution_metadata_when_caller_will_discard_it();
        sql_executor_never_eagerly_expands_private_values_even_when_diagnostics_requested();
        topn_004_011_sqlite_reuses_one_repeated_probe_boundary();
        keeps_partition_query_observable_when_metadata_is_enabled();
        topn_001_explicit_zero_threshold_forces_window_for_probe_provider();
        {
            GlobalContext global;
            auto user = global.make_user_context();
            topn_007_probe_rewrites_only_partition_membership_filter(user->memory_resource());
        }
        cached_select_plan_rebinds_values_and_separates_in_list_lengths();
        cached_select_plan_rebinds_large_in_as_one_array_parameter();
        cached_select_plan_preserves_parameter_order_for_supported_query_shapes();
        std::pmr::set_default_resource(previous);
        std::cout << "Original twelve SQL executor contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
