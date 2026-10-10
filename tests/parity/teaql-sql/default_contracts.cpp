// Provider-independent defaults and schema contracts from the pinned sources.
#include <teaql/sql/dialect.hpp>
#include <teaql/data_service/executor.hpp>
#include <teaql/memory/arena.hpp>
#include <algorithm>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void test_mutation_request_trace_and_comment_accessors(Memory owner) {
    TraceChain traces(owner);
    traces.emplace_back(TraceKind::Entity, "User", "Create User", std::uint64_t(1));
    traces.emplace_back(TraceKind::Entity, "Profile", "Create Profile");
    InsertCommand insert(std::allocator_arg, Allocator(owner), "User");
    insert.trace_chain = TraceChain(traces, owner);
    MutationRequest request_insert(std::allocator_arg, Allocator(owner), insert);
    check(request_insert.trace_chain().size() == 2 &&
              request_insert.trace_chain()[1] == traces[1] &&
              request_insert.comment() == "Create Profile",
          "original insert trace and final comment");
    UpdateCommand update(std::allocator_arg, Allocator(owner), "User", std::int64_t(1));
    update.trace_chain = TraceChain(traces, owner);
    MutationRequest request_update(std::allocator_arg, Allocator(owner), update);
    DeleteCommand deletion(std::allocator_arg, Allocator(owner), "User", std::int64_t(1));
    deletion.trace_chain = TraceChain(traces, owner);
    MutationRequest request_delete(std::allocator_arg, Allocator(owner), deletion);
    RecoverCommand recovery(std::allocator_arg, Allocator(owner), "User", std::int64_t(1), 1);
    recovery.trace_chain = TraceChain(traces, owner);
    MutationRequest request_recover(std::allocator_arg, Allocator(owner), recovery);
    for (auto* request : {&request_update, &request_delete, &request_recover}) {
        check(request->trace_chain().size() == 2 && request->comment() == "Create Profile",
              "original update/delete/recover trace accessors");
    }
    MutationRequest::Batch batch(owner);
    batch.emplace_back(std::move(request_insert));
    batch.emplace_back(std::move(request_update));
    MutationRequest request_batch(std::allocator_arg, Allocator(owner), std::move(batch));
    check(request_batch.trace_chain().empty() && !request_batch.comment(),
          "original batch has no aggregate trace");
    InsertCommand empty(std::allocator_arg, Allocator(owner), "User");
    MutationRequest request_empty(std::allocator_arg, Allocator(owner), std::move(empty));
    check(request_empty.trace_chain().empty() && !request_empty.comment(),
          "original empty trace has no comment");
}

class NoIo final : public QueryExecutor<String> {
public:
    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory, QueryRequest) override {
        throw std::logic_error("Intent discovery must not execute a query");
    }
};

static void
default_provider_intent_is_unknown_in_debug_and_never_visits_relation_siblings(Memory owner) {
    EntityDescriptor nested(std::allocator_arg, Allocator(owner), "Child");
    nested.property("id", DataType::U64).id();
    SelectQuery child(std::allocator_arg, Allocator(owner), "Child");
    child.filter(Expr::eq(owner, "name", "SUBQUERY-SECRET"));
    Record credential(owner);
    credential.emplace("password", Value(owner, "NESTED-CREDENTIAL"));
    Vector<Expr> filters(owner);
    filters.emplace_back(Expr::eq(owner, "name", "FILTER-SECRET"));
    filters.emplace_back(Expr::in_subquery(owner, "id", nested, child, "id"));
    filters.emplace_back(Expr::eq(owner, "payload", Value(owner, credential)));
    SelectQuery sibling(std::allocator_arg, Allocator(owner), "Child");
    sibling.filter(Expr::eq(owner, "name", "UNRELATED-SIBLING"));
    SelectQuery query(std::allocator_arg, Allocator(owner), "Root");
    query.filter(Expr::all_of(owner, std::move(filters)))
        .project_expr("value", Expr::value(owner, "PROJECTION-SECRET"))
        .having(Expr::eq(owner, "name", "HAVING-SECRET"))
        .search_with_text("SEARCH-SECRET")
        .relation_query("siblings", sibling)
        .limit(10)
        .comment("what: fallback fixture");
    auto intent = NoIo{}.query_log_intent(owner, query);
    for (auto debug : {false, true}) {
        Vector<String> secrets(owner);
        intent.extend_secrets(debug, secrets);
        for (auto expected : {"FILTER-SECRET",
                              "SUBQUERY-SECRET",
                              "NESTED-CREDENTIAL",
                              "PROJECTION-SECRET",
                              "HAVING-SECRET",
                              "SEARCH-SECRET"}) {
            check(std::find(secrets.begin(), secrets.end(), expected) != secrets.end(),
                  "original fallback protects every query secret");
        }
        check(std::find(secrets.begin(), secrets.end(), "UNRELATED-SIBLING") == secrets.end(),
              "original fallback does not traverse unrelated relation siblings");
    }
    check(intent.debug(owner).find("SECRET") == String::npos,
          "original intent Debug hides secret content");
}

// The original dummy reports PostgreSql. Only its generic compiler assertions
// are under test; adapt the helper's kind/placeholders to the SQLite edge scope.
class TestDialect final : public SqlDialect {
public:
    DatabaseKind kind() const override {
        return DatabaseKind::Sqlite;
    }

    String quote_ident(Memory owner, std::string_view name) const override {
        return String(name, owner);
    }

    String placeholder(Memory owner, std::size_t) const override {
        return String("?", owner);
    }

    String schema_type_sql(Memory owner, DataType, const PropertyDescriptor&) const override {
        return String("TEST", owner);
    }
};

static void test_regression_issue_56_typed_null_conversion(Memory owner) {
    TestDialect dialect;
    EntityDescriptor entity(std::allocator_arg, Allocator(owner), "User");
    entity.property("paid_at", DataType::Timestamp);
    InsertCommand command(std::allocator_arg, Allocator(owner), "User");
    command.value("paid_at", Value(owner));
    auto query = dialect.compile_insert(owner, entity, command);
    check(query.params.size() == 1 &&
              query.params[0] == Value(owner, TypedNull{DataType::Timestamp}),
          "original compiler replaces untyped Null with field-specific TypedNull");
}

static void rejects_invalid_schema_shape_before_emitting_ddl(Memory owner) {
    TestDialect dialect;
    EntityDescriptor entity(std::allocator_arg, Allocator(owner), "Order");
    entity.table_name = "orders";
    Vector<PropertyDescriptor> properties(owner);
    properties.emplace_back("empty_text", DataType::Text);
    properties.back().with_max_length(0);
    properties.emplace_back("partial_decimal", DataType::Decimal);
    properties.back().with_numeric_precision(19);
    properties.emplace_back("invalid_decimal", DataType::Decimal);
    properties.back().with_numeric_precision(5).with_numeric_scale(7);
    properties.emplace_back("numeric_text", DataType::Text);
    properties.back().with_numeric_precision(19).with_numeric_scale(7);
    for (const auto& property : properties) {
        entity.properties.clear();
        entity.properties.emplace_back(property);
        bool rejected = false;
        try {
            (void)dialect.compile_create_table(owner, entity);
        } catch (const SqlCompileError& error) {
            check(error.code == SqlCompileError::Code::InvalidSchemaShape &&
                      error.to_string(owner).find(property.name) != String::npos,
                  "original structured schema error includes field name");
            rejected = true;
        }
        check(rejected, "all original invalid metadata fail before DDL emission");
    }
}

static void existing_storage_shape_must_cover_the_model_value_domain() {
    check(storage_length_covers(100, 100) && storage_length_covers(100, 255) &&
              storage_length_covers(100, {}) && !storage_length_covers(100, 32),
          "all original storage length cases");
    check(storage_numeric_covers(19, 7, 19, 7) && storage_numeric_covers(19, 7, 38, 10) &&
              storage_numeric_covers(19, 7, {}, {}) && !storage_numeric_covers(19, 7, 18, 2) &&
              !storage_numeric_covers(19, 7, 19, 8) && !storage_numeric_covers(19, 7, 20, 6) &&
              !storage_numeric_covers(19, 7, 38, {}),
          "all original numeric-domain coverage cases");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        {
            Arena arena(4096, std::pmr::new_delete_resource());
            test_mutation_request_trace_and_comment_accessors(&arena);
            default_provider_intent_is_unknown_in_debug_and_never_visits_relation_siblings(&arena);
            test_regression_issue_56_typed_null_conversion(&arena);
            rejects_invalid_schema_shape_before_emitting_ddl(&arena);
            existing_storage_shape_must_cover_the_model_value_domain();
        }
        std::pmr::set_default_resource(previous);
        std::cout << "Original five data-service/schema contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
