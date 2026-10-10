#include <teaql/runtime/context.hpp>
#include <teaql/runtime/inmemory_engine.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void lifetime_and_allocator_contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    Ref<QueryResult> retained;
    Ref<QueryResult> direct_retained;
    Ref<Value> resolved_retained;
    {
        auto& columns = user->make<ColumnLayout>(
            std::initializer_list<std::string_view>{"id", "name", "payload"});
        Vector<CompactRow> rows(r);
        for (int i = 0; i < 64; ++i) {
            Value::List payload(r);
            payload.emplace_back(String(128, 'x', r));
            Vector<Value> values(r);
            values.emplace_back(i % 4);
            values.emplace_back(String(128, static_cast<char>('a' + i % 4), r));
            values.emplace_back(payload);
            rows.emplace_back(columns, std::move(values));
        }
        SelectQuery query(std::allocator_arg, Allocator(r), "Item");
        query.order_by.emplace_back(OrderBy::desc(r, "id"));
        query.slice = Slice{5, 3};
        query.projection.emplace_back("payload");
        query.projection.emplace_back("name");
        query.comment_text = String("memory query purpose", r);
        auto result = InMemoryQueryEngine::execute(r, query, std::span<const CompactRow>(rows));
        check(result.rows.size() == 5 && rows.size() == 64,
              "borrowed execution keeps inputs and applies ordered pagination");
        auto& metadata = result.metadata;
        check(metadata.backend == "memory" && metadata.operation == DataServiceOperation::Query &&
                  metadata.result_count == 5 && metadata.started_at <= metadata.ended_at &&
                  metadata.statements.empty() && metadata.sql_log == SqlLogContext(r) &&
                  !metadata.debug_query && !metadata.affected_rows &&
                  metadata.trace_chain.empty() && !metadata.comment &&
                  !metadata.backend_request_id && !metadata.parameterized_query &&
                  metadata.params.empty(),
              "memory metadata matches source defaults without inventing query provenance");
        retained = user->make_global<QueryResult>(result);
        auto& copy = user->get(retained);
        check(copy.memory_resource() == global.memory_resource() &&
                  copy.rows[0].keys()[0] == "name" && copy.rows[0].keys()[1] == "payload" &&
                  std::get<String>(copy.rows[0].get("name")->data).get_allocator().resource() ==
                      global.memory_resource() &&
                  std::get<String>(copy.rows[0].get("payload")->as_list()->front().data)
                          .get_allocator()
                          .resource() == global.memory_resource(),
              "retained rows, projected layout and nested characters are copied into Global");
        auto literal = Expr::value(r, *rows[0].get("payload"));
        auto resolved = ExprEvaluator::resolve(global.memory_resource(), literal, rows[0]);
        check(resolved.memory_resource() == global.memory_resource() &&
                  resolved.as_list()->get_allocator().resource() == global.memory_resource(),
              "expression resolution propagates an explicit destination allocator");
        resolved_retained = user->make_global<Value>(std::move(resolved));
        auto direct =
            InMemoryQueryEngine::execute(global.memory_resource(), query, std::move(rows));
        check(direct.rows[0].memory_resource() == global.memory_resource() &&
                  direct.rows[0].shared_columns()->memory_resource() == global.memory_resource(),
              "foreign-domain consuming execution never retains User layout pointers");
        direct_retained = user->make_global<QueryResult>(std::move(direct));
    }
    user.reset();
    auto next = global.make_user_context();
    auto& copy = next->get(next->rebind_global(retained));
    check(
        copy.rows.size() == 5 && copy.rows[0].get("name")->try_text()->size() == 128 &&
            copy.rows[0].get("payload")->as_list()->front().try_text()->size() == 128,
        "nested results remain usable after actual User destruction only through explicit Global copying");
    auto& direct = next->get(next->rebind_global(direct_retained));
    auto& resolved = next->get(next->rebind_global(resolved_retained));
    check(direct.rows.size() == 5 &&
              direct.rows[0].get("payload")->as_list()->front().try_text()->size() == 128 &&
              resolved.as_list()->front().try_text()->size() == 128,
          "direct Global execution and resolution retain no User payload after its destruction");
}

static void stable_order_and_unicode() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    Vector<CompactRow> rows(r);
    for (int i = 0; i < 128; ++i) {
        Record record(r);
        record.emplace(String("group", r), Value(r, i % 4));
        record.emplace(String("position", r), Value(r, i));
        rows.emplace_back(record);
    }
    SelectQuery query(std::allocator_arg, Allocator(r), "Item");
    query.order_by.emplace_back(OrderBy::asc(r, "group"));
    auto result = InMemoryQueryEngine::execute(r, query, std::move(rows));
    for (int group = 0; group < 4; ++group) {
        for (int i = 0; i < 32; ++i) {
            check(result.rows[group * 32 + i].get("position")->try_i64() == group + 4 * i,
                  "PMR merge sorting preserves encounter order for equal keys");
        }
    }
    Record record(r);
    record.emplace(String("text", r), Value(r, "\xe4\xb8\xad\xf0\x9f\x98\x80"));
    CompactRow row(std::allocator_arg, Allocator(r), record);
    check(ExprEvaluator::eval(Expr::like(r, "text", "__"), row) &&
              !ExprEvaluator::eval(Expr::like(r, "text", "________"), row),
          "LIKE underscore consumes Unicode scalars, not bytes");
    query.slice = Slice{1, std::uint64_t{1} << 32};
    auto paged = InMemoryQueryEngine::execute(r, query, std::span<const CompactRow>(result.rows));
    check(paged.rows.size() == (sizeof(std::size_t) == 4 ? 1 : 0),
          "pagination preserves the source's executing-target usize conversion");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        lifetime_and_allocator_contracts();
        stable_order_and_unicode();
        std::cout << "In-memory query ownership contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
