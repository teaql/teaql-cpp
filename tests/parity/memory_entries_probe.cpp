#include "memory_data_service.hpp"
#include "teaql_runtime/root_fixture.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
using namespace teaql;
using Service = original_memory::MemoryDataService;

static CompactRow row(Memory r, unsigned dataset = 0) {
    Record fields(r);
    fields.emplace("id", Value(r, std::uint64_t(1)));
    fields.emplace("version", Value(r, std::int64_t(3)));
    if (dataset != 2) {
        fields.emplace("name", dataset == 3 ? Value(r, std::int64_t(9)) : Value(r, "one"));
    }
    return CompactRow(std::allocator_arg, Allocator(r), std::move(fields));
}

static Service& service(UserContext& user_context, bool child = true) {
    auto r = user_context.memory_resource();
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    EntityDescriptor parent(std::allocator_arg, Allocator(r), "Order");
    parent.relation("lines", "OrderLine").local("id").foreign("order_id").to_many();
    parent.relation("ghost", "MissingChild").local("id").foreign("order_id").to_many();
    metadata.register_entity(std::move(parent));
    if (child) {
        metadata.register_entity(EntityDescriptor(std::allocator_arg, Allocator(r), "OrderLine"));
    }
    auto& result = user_context.make<Service>(metadata);
    Vector<CompactRow> parents(r);
    parents.emplace_back(row(r));
    result.seed("Order", parents);
    Vector<CompactRow> children(r);
    for (unsigned i = 0; i < 2; ++i) {
        Record fields(r);
        fields.emplace("order_id", Value(r, std::uint64_t(1)));
        children.emplace_back(std::move(fields));
    }
    result.seed("OrderLine", children);
    return result;
}

static Json rows(Memory r, std::span<const CompactRow> values) {
    Json::Array result(r);
    for (const auto& value : values) {
        auto fields = detail::model_json(r);
        for (const auto& [key, item] : value.into_map(r)) {
            detail::model_put(fields, key, detail::debug_value(r, item));
        }
        result.emplace_back(std::move(fields));
    }
    return Json(r, std::move(result));
}

static Json error(Memory r, const Service::Error& value) {
    auto result = detail::model_json(r);
    detail::model_put(result, "error", value.to_string(r));
    const auto kind = value.kind();
    detail::model_put(result,
                      "error_kind",
                      kind == Service::Error::Kind::Runtime  ? "runtime"
                      : kind == Service::Error::Kind::Entity ? "entity"
                                                             : "executor");
    return result;
}

template <class T>
static Json list(Memory r, Result<SmartList<T>, Service::Error> result) {
    if (!result) {
        return error(r, result.error());
    }
    auto& value = result.value();
    Vector<CompactRow> mapped(r);
    if constexpr (std::same_as<T, CompactRow>) {
        for (const auto& item : value.data) {
            mapped.emplace_back(item);
        }
    } else {
        for (const auto& item : value.data) {
            Record fields(r);
            fields.emplace("id", Value(r, item.id));
            fields.emplace("version", Value(r, item.revision));
            fields.emplace("name", Value(r, item.name));
            mapped.emplace_back(std::move(fields));
        }
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "rows", rows(r, mapped));
    detail::model_put(out, "loaded", value.is_loaded);
    detail::model_put(out, "total", value.total_count ? Json(r, *value.total_count) : Json(r));
    detail::model_put(out, "aggregations", std::uint64_t(value.aggregations.size()));
    detail::model_put(out, "summary", std::uint64_t(value.summary.size()));
    detail::model_put(out, "facets", std::uint64_t(value.facets.size()));
    return out;
}

static void emit(Memory r, std::string_view name, Json out) {
    detail::model_put(out, "case", name);
    std::cout << out.dump(r) << '\n';
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    if (argc == 2 && std::string_view(argv[1]) == "--lifetime-test") {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& instance = service(*user_context);
        ResourceBorrow<Service> shared(
            std::allocator_arg, Allocator(r), instance, detail::resource_arena(r));
        if (&shared.get() != &instance) {
            return 1;
        }
        user_context->close();
        try {
            (void)shared.get();
        } catch (const ReferenceError& e) {
            return e.kind() == ReferenceError::Kind::Closed ? 0 : 2;
        }
        return 3;
    }
    for (unsigned dataset = 0; dataset < 8; ++dataset) {
        for (unsigned method = 0; method < 4; ++method) {
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto& instance = service(*user_context);
            Vector<CompactRow> input(r);
            if (dataset != 1) {
                input.emplace_back(row(r, dataset));
            }
            instance.seed("Order", input);
            SelectQuery query(std::allocator_arg, Allocator(r), dataset == 4 ? "Missing" : "Order");
            if (dataset == 5) {
                query.filter(Expr::value(r, std::int64_t(1)));
            }
            if (dataset == 7) {
                query.project("id");
            }
            SelectQuery child(std::allocator_arg, Allocator(r), "Ignored");
            Vector<RelationAggregate> aggregates(r);
            aggregates.emplace_back(dataset == 6 ? "missing" : "lines", "total", child, true);
            using Order = original_runtime_root::Order;
            auto output =
                method == 0 ? list(r, instance.fetch_smart_list(query))
                : method == 1
                    ? list(r, instance.fetch_smart_list_with_relation_aggregates(query, aggregates))
                : method == 2 ? list(r, instance.fetch_entities<Order>(query))
                              : list(r,
                                     instance.fetch_entities_with_relation_aggregates<Order>(
                                         query, aggregates));
            emit(r,
                 String("fetch_", r) + debug_string(r, dataset) + "_" + debug_string(r, method),
                 std::move(output));
        }
    }
    for (unsigned mode = 0; mode < 10; ++mode) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& instance = service(*user_context, mode != 6);
        SelectQuery query(std::allocator_arg, Allocator(r), "Order");
        auto parents = instance.fetch_all(query).value();
        if (mode == 5 || mode == 6 || mode == 9) {
            parents.clear();
        }
        Vector<RelationAggregate> aggregates(r);
        SelectQuery child(std::allocator_arg, Allocator(r), "Ignored");
        if (mode != 0 && mode != 9) {
            aggregates.emplace_back(
                mode == 5 ? "missing" : "lines", mode == 7 ? "id" : "first", child, true);
            if (mode == 2 || mode == 3 || mode == 7 || mode == 8) {
                if (mode == 3) {
                    child.filter(Expr::value(r, std::int64_t(1)));
                }
                aggregates.emplace_back(mode == 2   ? "missing"
                                        : mode == 8 ? "ghost"
                                                    : "lines",
                                        "second",
                                        child,
                                        true);
            }
        }
        auto result = instance.enhance_relation_aggregates(
            mode == 0 || mode == 4 || mode == 9 ? "Missing" : "Order", parents, aggregates);
        auto status = detail::model_json(r);
        if (result) {
            detail::model_put(status, "ok", true);
        } else {
            status = error(r, result.error());
        }
        auto out = detail::model_json(r);
        detail::model_put(out, "result", std::move(status));
        detail::model_put(out, "rows", rows(r, parents));
        detail::model_put(out, "stored", rows(r, instance.fetch_all(query).value()));
        emit(r, String("enhance_", r) + debug_string(r, mode), std::move(out));
    }
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& instance = service(*user_context);
    ResourceBorrow<Service> shared(
        std::allocator_arg, Allocator(r), instance, detail::resource_arena(r));
    Vector<CompactRow> changed(r);
    auto replacement = row(r).into_map(r);
    replacement.insert_or_assign(String("name", r), Value(r, "replacement"));
    changed.emplace_back(std::move(replacement));
    shared.get().seed("Order", changed);
    SelectQuery query(std::allocator_arg, Allocator(r), "Order");
    auto out = detail::model_json(r);
    detail::model_put(out, "rows", rows(r, instance.fetch_all(query).value()));
    detail::model_put(out, "shared_rows", rows(r, shared.get().fetch_all(query).value()));
    emit(r, "shared_seed", std::move(out));
}
