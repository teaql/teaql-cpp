#include "memory_data_service.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
using namespace teaql;

static Vector<Value> keys(Memory r) {
    Vector<Value> result(r);
    result.emplace_back();
    result.emplace_back(TypedNull{DataType::I64});
    result.emplace_back(true);
    result.emplace_back(false);
    result.emplace_back(std::int64_t(1));
    result.emplace_back(std::uint64_t(1));
    result.emplace_back(std::int64_t(-1));
    result.emplace_back(1.0);
    result.emplace_back(-0.0);
    result.emplace_back(0.0);
    result.emplace_back(Decimal::from_i64_with_scale(10, 1));
    result.emplace_back(Decimal::from_i64_with_scale(100, 2));
    result.emplace_back("a");
    result.emplace_back("a");
    Value::Object a(r);
    a.emplace("x", Value(r, std::int64_t(1)));
    result.emplace_back(std::move(a));
    Value::Object b(r);
    b.emplace("y", Value(r, std::int64_t(2)));
    result.emplace_back(std::move(b));
    Value::List x(r);
    x.emplace_back(std::int64_t(1));
    result.emplace_back(std::move(x));
    Value::List y(r);
    y.emplace_back(std::int64_t(2));
    result.emplace_back(std::move(y));
    result.emplace_back(*Json::parse(r, "{\"x\":1}"));
    result.emplace_back(*Json::parse(r, "{\"x\":2}"));
    result.emplace_back(*Date::from_ymd(1970, 1, 1));
    result.emplace_back(Timestamp(0));
    return result;
}

static void add_row(Vector<CompactRow>& rows, const Value& value) {
    auto r = rows.get_allocator().resource();
    Record fields(r);
    fields.emplace("k", Value(r, value));
    rows.emplace_back(std::move(fields));
}

static void emit(Memory r,
                 std::string_view name,
                 Result<Vector<CompactRow>, original_memory::MemoryDataService::Error> result) {
    auto out = detail::model_json(r);
    detail::model_put(out, "case", name);
    if (result) {
        Json::Array rows(r);
        for (const auto& row : result.value()) {
            auto fields = detail::model_json(r);
            for (const auto& [k, v] : row.into_map(r)) {
                detail::model_put(fields, k, detail::debug_value(r, v));
            }
            rows.emplace_back(std::move(fields));
        }
        detail::model_put(out, "rows", Json(r, std::move(rows)));
    } else {
        detail::model_put(out, "error", result.error().to_string(r));
    }
    std::cout << out.dump(r) << '\n';
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    auto& missing_metadata = user_context->make<InMemoryMetadataStore>();
    EntityDescriptor parent(std::allocator_arg, Allocator(r), "Parent");
    parent.relation("children", "Child").local("k").foreign("k").to_many();
    metadata.register_entity(detail::clone_in(r, parent));
    missing_metadata.register_entity(std::move(parent));
    EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
    metadata.register_entity(std::move(child));
    auto& service = user_context->make<original_memory::MemoryDataService>(metadata);
    auto& missing_service =
        user_context->make<original_memory::MemoryDataService>(missing_metadata);
    auto values = keys(r);
    auto query = [&] {
        return SelectQuery(std::allocator_arg, Allocator(r), "Parent");
    };
    auto relation = [&](const auto& target, bool single) {
        SelectQuery child_query(std::allocator_arg, Allocator(r), "Ignored");
        RelationAggregate aggregate(
            std::allocator_arg, Allocator(r), "children", "total", child_query, single);
        return target.fetch_all_with_relation_aggregates(
            query(), std::span<const RelationAggregate>(&aggregate, 1));
    };
    for (unsigned i = 0; i < values.size(); ++i) {
        for (unsigned j = 0; j < values.size(); ++j) {
            Vector<CompactRow> rows(r);
            add_row(rows, values[i]);
            add_row(rows, values[j]);
            service.with_rows("Parent", rows);
            auto q = query();
            q.group_by("k").count("count");
            auto name = String("group_", r) + debug_string(r, i) + "_" + debug_string(r, j);
            emit(r, name, service.fetch_all(q));
        }
    }
    Vector<CompactRow> all(r);
    for (const auto& value : values) {
        add_row(all, value);
    }
    all.emplace_back();
    service.with_rows("Parent", all);
    auto q = query();
    q.group_by("k").count("count");
    emit(r, "group_all", service.fetch_all(q));
    for (unsigned i = 0; i < values.size(); ++i) {
        for (unsigned j = 0; j < values.size(); ++j) {
            Vector<CompactRow> parents(r);
            add_row(parents, values[i]);
            service.with_rows("Parent", parents);
            Vector<CompactRow> children(r);
            add_row(children, values[j]);
            service.with_rows("Child", children);
            for (bool single : {true, false}) {
                auto name = String("relation_", r) + debug_string(r, i) + "_" + debug_string(r, j) +
                            "_" + (single ? "1" : "0");
                emit(r, name, relation(service, single));
            }
        }
    }
    for (bool single : {true, false}) {
        Vector<CompactRow> rows(r);
        add_row(rows, values[4]);
        add_row(rows, values[5]);
        service.with_rows("Parent", rows);
        service.with_rows("Child", rows);
        auto suffix = single ? "1" : "0";
        emit(r, String("relation_mixed_", r) + suffix, relation(service, single));
        Vector<CompactRow> empty(r);
        missing_service.with_rows("Parent", empty);
        emit(r, String("relation_empty_", r) + suffix, relation(missing_service, single));
        empty.emplace_back();
        missing_service.with_rows("Parent", empty);
        emit(r, String("relation_missing_", r) + suffix, relation(missing_service, single));
    }
}
