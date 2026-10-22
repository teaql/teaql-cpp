#include "memory_data_service.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <array>
#include <iostream>
using namespace teaql;

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    for (unsigned mode = 0; mode < 20; ++mode) {
        for (bool single : {true, false}) {
            for (bool projected : {false, true}) {
                for (bool registered : {true, false}) {
                    auto user_context = global.make_user_context();
                    auto r = user_context->memory_resource();
                    auto& metadata = user_context->make<InMemoryMetadataStore>();
                    EntityDescriptor parent(std::allocator_arg, Allocator(r), "Parent");
                    parent.relation("children", "Child").local("id").foreign("pid").to_many();
                    metadata.register_entity(std::move(parent));
                    if (registered) {
                        EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
                        metadata.register_entity(std::move(child));
                    }
                    auto& service =
                        user_context->make<original_memory::MemoryDataService>(metadata);
                    Vector<CompactRow> parents(r);
                    for (std::uint64_t id = 1; id <= 3; ++id) {
                        Record fields(r);
                        fields.emplace("id", Value(r, id));
                        fields.emplace("name", Value(r, "parent"));
                        parents.emplace_back(std::move(fields));
                    }
                    service.with_rows("Parent", parents);
                    Vector<CompactRow> children(r);
                    constexpr std::array ids{std::uint64_t(1), std::uint64_t(1), std::uint64_t(2)};
                    constexpr std::array kinds{"b", "a", "a"};
                    constexpr std::array values{std::int64_t(2), std::int64_t(3), std::int64_t(5)};
                    for (unsigned i = 0; i < 3; ++i) {
                        Record fields(r);
                        fields.emplace("pid", Value(r, ids[i]));
                        fields.emplace("kind", Value(r, kinds[i]));
                        fields.emplace("v", Value(r, values[i]));
                        children.emplace_back(std::move(fields));
                    }
                    service.with_rows("Child", children);
                    SelectQuery child(std::allocator_arg, Allocator(r), "Ignored");
                    auto unsupported =
                        Expr::function(r, ExprFunction::Count, std::span<const Expr>{});
                    switch (mode) {
                    case 1:
                        child.count("n");
                        break;
                    case 2:
                        child.sum("v", "sum");
                        break;
                    case 3:
                        child.count("n").sum("v", "sum");
                        break;
                    case 4:
                        child.group_by("kind").count("n");
                        break;
                    case 5:
                        child.group_by("kind");
                        break;
                    case 6:
                        child.group_by("pid").count("n");
                        break;
                    case 7:
                        child.count("n").having(Expr::gt(r, "n", std::int64_t(1)));
                        break;
                    case 8:
                        child.filter(Expr::gt(r, "v", std::int64_t(4)));
                        break;
                    case 9:
                        child.project("bad")
                            .project_expr("bad", unsupported)
                            .order_expr_desc(unsupported)
                            .offset(UINT64_MAX)
                            .limit(0)
                            .relation("missing");
                        break;
                    case 10:
                        child.filter(Expr::value(r, std::int64_t(1)));
                        break;
                    case 11:
                        child.having(Expr::value(r, std::int64_t(1)));
                        break;
                    case 12:
                        child.sum("kind", "sum");
                        break;
                    case 13:
                        child.count("pid");
                        break;
                    case 14:
                        child.group_by("kind").count("kind");
                        break;
                    case 15:
                        child.count("n").having(Expr::gt(r, "n", std::int64_t(99)));
                        break;
                    case 19:
                        child.filter(Expr::value(r, std::int64_t(1))).sum("kind", "sum");
                        break;
                    default:
                        break;
                    }
                    SelectQuery query(
                        std::allocator_arg, Allocator(r), mode == 16 ? "Missing" : "Parent");
                    if (projected) {
                        query.project("name");
                    }
                    Vector<RelationAggregate> aggregates(r);
                    aggregates.emplace_back(mode == 17 ? "missing" : "children",
                                            mode == 18 ? "id" : "result",
                                            child,
                                            single);
                    if (mode == 18) {
                        SelectQuery next(std::allocator_arg, Allocator(r), "Ignored");
                        aggregates.emplace_back("children", "again", next, single);
                    }
                    auto result = service.fetch_all_with_relation_aggregates(query, aggregates);
                    auto out = detail::model_json(r);
                    auto name = String("relation_", r) + debug_string(r, mode) + "_" +
                                (single ? "1" : "0") + "_" + (projected ? "1" : "0") + "_" +
                                (registered ? "1" : "0");
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
            }
        }
    }
}
