#include <teaql/runtime/context.hpp>
#include <teaql/runtime/inmemory_engine.hpp>
#include <teaql/core/list.hpp>
#include <teaql/core/request.hpp>
#include <teaql/core/dynamic_fields_debug.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <array>
#include <iostream>

static unsigned integer(const Json& input, std::string_view name) {
    return unsigned(std::get<std::uint64_t>(field(input, name).data));
}

static const DynamicPropertyDefinitions&
schema(UserContext& user_context, std::string_view name, bool empty = false) {
    auto owner = user_context.memory_resource();
    Vector<DynamicPropertyDefinitions::Field> fields(owner);
    if (!empty) {
        fields.emplace_back(name, DataType::I64);
    }
    auto result = DynamicPropertyDefinitions::from_fields(owner, fields);
    return user_context.make<DynamicPropertyDefinitions>(std::move(result).value());
}

static CompactRow row(UserContext& user_context, unsigned profile, unsigned index) {
    auto owner = user_context.memory_resource();
    auto& layout = user_context.make<ColumnLayout>(
        std::initializer_list<std::string_view>{"id", "group", "label", "_rank"});
    Vector<Value> values(owner);
    values.emplace_back(std::uint64_t(index));
    values.emplace_back(std::uint64_t(index % 2));
    String label("label" + std::to_string(index), owner);
    label.append(std::string_view("\0雪", 4));
    values.emplace_back(label);
    values.emplace_back(std::int64_t(index));
    CompactRow result(std::allocator_arg, Allocator(owner), layout, std::move(values));
    if (profile == 1 || profile == 2 || profile == 8) {
        SmartList<CompactRow> list(std::allocator_arg, Allocator(owner));
        list.loaded();
        if (profile == 8) {
            auto& child_layout =
                user_context.make<ColumnLayout>(std::initializer_list<std::string_view>{"child"});
            String text(owner);
            for (unsigned i = 0; i < 32; ++i) {
                text.append(std::string_view("child\0雪", 9));
            }
            Vector<Value> child_values(owner);
            child_values.emplace_back(text);
            list.data.emplace_back(child_layout, std::move(child_values));
        }
        result.set_loaded_relation("lines", list);
        if (profile == 2) {
            (void)result.take_loaded_relation("lines");
        }
        RelationShapeCache shapes(std::allocator_arg, Allocator(owner));
        result.mark_relation_loaded("lines", shapes);
    }
    if (profile == 3 || profile == 4 || profile == 5 || profile == 8 || profile == 9) {
        const std::array<DynamicFieldDefinitions::Field, 1> fields{{{"memo", DataType::Text}}};
        auto definitions = DynamicFieldDefinitions::from_fields(owner, "Item", "v1", fields);
        auto& metadata = user_context.make<DynamicFieldDefinitions>(std::move(definitions).value());
        Value::Object contents(owner);
        if (profile == 4) {
            contents.emplace("memo", Value(owner, nullptr));
        }
        if (profile == 5 || profile == 8) {
            String text(owner);
            for (unsigned i = 0; i < 32; ++i) {
                text.append(std::string_view("dynamic\0雪", 11));
            }
            contents.emplace("memo", Value(owner, text));
        }
        auto payload = DynamicFieldValues::from_values(owner, metadata, contents);
        result.set_loaded_dynamic_fields(payload.value());
        if (profile == 9) {
            (void)result.take_loaded_dynamic_fields();
        }
    }
    if (profile == 5 || profile == 6 || profile == 7 || profile == 8 || profile == 9) {
        CompactRow::attach_dynamic_property_definitions(
            std::span<CompactRow>(&result, 1),
            schema(user_context, "_rank", profile == 6 || profile == 9));
    }
    return result;
}

static SelectQuery query(UserContext& user_context, unsigned mode, bool rich) {
    auto owner = user_context.memory_resource();
    SelectQuery q(std::allocator_arg, Allocator(owner), "Item");
    if (rich) {
        SelectQuery ignored(std::allocator_arg, Allocator(owner), "Ignored");
        ignored.limit(20001);
        q.hard_limit_value = 1;
        q.project_expr("computed", Expr::value(owner, std::uint64_t{7}));
        q.search_with_text("ignored").having(Expr::value(owner, false));
        q.partition_by("ignored").top_n_probe_parent_threshold(0).group_by("ignored");
        q.relation_query("lines", ignored);
        q.facets.emplace_back("facet", "lines", ignored, true);
        q.relation_aggregates.emplace_back("lines", "related", ignored, false);
        q.enable_aggregation_cache_for(7).propagate_aggregation_cache(8);
        q.comment("ignored comment").purpose("ignored purpose").raw_sql("not SQL");
        q.add_raw_sql_search_criteria("ignored").dynamic_property_raw("_request", "7");
        q.with_dynamic_property_definitions(schema(user_context, "_request"));
        q.select_dynamic_fields(DynamicFieldSelection::all(owner));
        q.project_raw("raw", "8").object_group_by("object", "owner", ignored);
        q.child_enhancement(ignored)
            .stream(0)
            .optimize_for_continuous_page_fetch()
            .optimize_pagination_with_id_set();
    }
    if (mode == 1) {
        q.order_desc("id").page(1, 1);
    } else if (mode == 2) {
        q.project("id");
    } else if (mode == 3 || mode == 4) {
        q.group_by_fields.clear();
        if (mode == 4) {
            q.group_by("group");
        }
        q.count("count").page(999, 0).project("absent");
    }
    return q;
}

static Json pattern(UserContext& user_context, const Json& input, Ref<Value>& retained) {
    auto owner = user_context.memory_resource();
    auto p =
        Expr::like_pattern(owner, text(field(input, "pattern")), text(field(input, "original")));
    Record data(owner);
    if (!std::holds_alternative<std::monostate>(field(input, "text").data)) {
        data.emplace("x", Value(owner, text(field(input, "text"))));
    }
    CompactRow row(std::allocator_arg, Allocator(owner), data);
    auto wrapper = integer(input, "wrapper");
    auto expr = std::move(p);
    if (wrapper >= 1 && wrapper <= 3) {
        Expr wrapped(owner,
                     wrapper == 1   ? Expr::Kind::IsNull
                     : wrapper == 2 ? Expr::Kind::IsNotNull
                                    : Expr::Kind::Not);
        wrapped.args.emplace_back(std::move(expr));
        expr = std::move(wrapped);
    } else if (wrapper == 4) {
        expr = Expr::binary(
            Expr::column(owner, "x"), BinaryOp(integer(input, "operator")), std::move(expr));
    }
    auto resolved = ExprEvaluator::resolve(expr, row);
    retained = user_context.make_global<Value>(resolved);
    Json::Object result(owner);
    result.emplace("eval", Json(owner, ExprEvaluator::eval(expr, row)));
    result.emplace("resolved", formatted_debug(owner, resolved));
    return Json(owner, std::move(result));
}

static Json execute(UserContext& user_context, const Json& input, Ref<QueryResult>& retained) {
    auto owner = user_context.memory_resource();
    auto q = query(user_context, integer(input, "mode"), std::get<bool>(field(input, "rich").data));
    Vector<CompactRow> rows(owner);
    for (unsigned i = 0; i < integer(input, "count"); ++i) {
        rows.emplace_back(row(user_context, integer(input, "profile"), i));
    }
    Json::Array before(owner);
    for (const auto& row : rows) {
        before.emplace_back(formatted_debug(owner, row));
    }
    bool borrowed = std::get<bool>(field(input, "borrowed").data);
    auto start = std::chrono::system_clock::now();
    auto result = borrowed
                      ? InMemoryQueryEngine::execute(owner, q, std::span<const CompactRow>(rows))
                      : InMemoryQueryEngine::execute(owner, q, std::move(rows));
    auto end = std::chrono::system_clock::now();
    retained = user_context.make_global<QueryResult>(result);
    Json::Array output_rows(owner);
    for (const auto& row : result.rows) {
        output_rows.emplace_back(formatted_debug(owner, row));
    }
    Json input_unchanged(owner);
    if (borrowed) {
        Json::Array after(owner);
        for (const auto& row : rows) {
            after.emplace_back(formatted_debug(owner, row));
        }
        input_unchanged = Json(owner, before == after);
    }
    const auto& m = result.metadata;
    Json::Object metadata(owner);
    metadata.emplace("backend", Json(owner, m.backend));
    metadata.emplace(
        "operation",
        Json(owner, m.operation == DataServiceOperation::Query ? "Query" : "Mutation"));
    metadata.emplace("result_count",
                     m.result_count ? Json(owner, std::uint64_t(*m.result_count)) : Json(owner));
    metadata.emplace("statements", Json(owner, std::uint64_t(m.statements.size())));
    metadata.emplace("sql_log_default", Json(owner, m.sql_log == SqlLogContext(owner)));
    metadata.emplace("debug_query", m.debug_query ? Json(owner, *m.debug_query) : Json(owner));
    metadata.emplace("affected_rows",
                     m.affected_rows ? Json(owner, *m.affected_rows) : Json(owner));
    metadata.emplace("trace", Json(owner, std::uint64_t(m.trace_chain.size())));
    metadata.emplace("comment", m.comment ? Json(owner, *m.comment) : Json(owner));
    metadata.emplace("backend_request_id",
                     m.backend_request_id ? Json(owner, *m.backend_request_id) : Json(owner));
    metadata.emplace("parameterized_query",
                     m.parameterized_query ? Json(owner, *m.parameterized_query) : Json(owner));
    metadata.emplace("params", Json(owner, std::uint64_t(m.params.size())));
    metadata.emplace("started_within_call",
                     Json(owner, start <= m.started_at && m.started_at <= end));
    metadata.emplace("ended_within_call", Json(owner, start <= m.ended_at && m.ended_at <= end));
    metadata.emplace("ordered", Json(owner, m.started_at <= m.ended_at));
    Json::Object output(owner);
    output.emplace("rows", Json(owner, std::move(output_rows)));
    output.emplace("input_unchanged", std::move(input_unchanged));
    output.emplace("metadata", Json(owner, std::move(metadata)));
    return Json(owner, std::move(output));
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        std::string line;
        while (std::getline(std::cin, line)) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            Ref<Value> value;
            Ref<QueryResult> result;
            Ref<Json> output;
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("Invalid in-memory source fixture");
                }
                auto observed = text(field(*input, "kind")) == "pattern"
                                    ? pattern(*user_context, *input, value)
                                    : execute(*user_context, *input, result);
                output = user_context->make_global<Json>(observed);
            }
            user_context.reset();
            auto next_user_context = global.make_user_context();
            auto owner = next_user_context->memory_resource();
            const auto& expected = next_user_context->get(next_user_context->rebind_global(output));
            if (value.id() != 0) {
                const auto& actual =
                    next_user_context->get(next_user_context->rebind_global(value));
                if (formatted_debug(owner, actual) != field(expected, "resolved")) {
                    throw std::runtime_error("Retained pattern value expired");
                }
            } else {
                const auto& actual =
                    next_user_context->get(next_user_context->rebind_global(result));
                if (actual.memory_resource() != global.memory_resource() ||
                    actual.metadata.backend != "memory" ||
                    actual.metadata.backend.get_allocator().resource() !=
                        global.memory_resource() ||
                    actual.metadata.result_count != actual.rows.size()) {
                    throw std::runtime_error(
                        "Retained engine execution metadata changed owner or content");
                }
                Json::Array actual_rows(owner);
                for (const auto& row : actual.rows) {
                    actual_rows.emplace_back(formatted_debug(owner, row));
                }
                if (Json(owner, std::move(actual_rows)) != field(expected, "rows")) {
                    throw std::runtime_error("Retained engine metadata or values expired");
                }
            }
            std::cout << expected.dump(owner) << '\n';
        }
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
