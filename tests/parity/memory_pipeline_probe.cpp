#include "memory_data_service.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <array>
#include <iostream>
using namespace teaql;

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Item");
    metadata.register_entity(std::move(descriptor));
    auto& service = user_context->make<original_memory::MemoryDataService>(metadata);
    Vector<CompactRow> rows(r);
    constexpr std::array names{"zed", "alpha", "beta"};
    for (unsigned i = 0; i < names.size(); ++i) {
        Record fields(r);
        fields.emplace("id", Value(r, std::uint64_t(i + 1)));
        fields.emplace("name", Value(r, names[i]));
        if (i != 2) {
            fields.emplace("n", Value(r, std::int64_t(i == 0 ? 3 : 1)));
        }
        rows.emplace_back(std::move(fields));
    }
    service.with_rows("Item", rows);
    constexpr std::array cases{"project_soundex",
                               "project_gbk",
                               "project_missing",
                               "project_binary_error",
                               "project_function_error",
                               "project_source_snapshot",
                               "sort_gbk_asc",
                               "sort_gbk_desc",
                               "sort_missing_field",
                               "sort_error_equal",
                               "slice_before_error",
                               "slice_huge_offset",
                               "slice_order",
                               "having_signed_count",
                               "having_non_boolean",
                               "aggregate_gbk",
                               "aggregate_sequential",
                               "aggregate_ignores_slice"};
    auto count_function = [&] {
        return Expr::function(r, ExprFunction::Count, std::span<const Expr>{});
    };
    for (unsigned i = 0; i < cases.size(); ++i) {
        SelectQuery query(std::allocator_arg, Allocator(r), "Item");
        switch (i) {
        case 0:
            query.project_expr("code", Expr::soundex(Expr::column(r, "name")));
            break;
        case 1:
            query.project_expr("copy", Expr::gbk(Expr::column(r, "name")));
            break;
        case 2:
            query.project("absent").project_expr("copy", Expr::column(r, "absent"));
            break;
        case 3:
            query.project_expr("flag", Expr::eq(r, "n", std::int64_t(3)));
            break;
        case 4:
            query.project_expr("count", count_function());
            break;
        case 5:
            query.project_expr("name", Expr::value(r, "changed"))
                .project_expr("copy", Expr::column(r, "name"));
            break;
        case 6:
            query.order_gbk_asc("name").project("id");
            break;
        case 7:
            query.order_gbk_desc("name").project("id");
            break;
        case 8:
            query.order_asc("n").project("id");
            break;
        case 9:
            query.order_expr_asc(count_function()).project("id");
            break;
        case 10:
            query.offset(3).project_expr("bad", count_function());
            break;
        case 11:
            query.offset(UINT64_MAX).project("id");
            break;
        case 12:
            query.order_asc("name").offset(1).limit(1).project("id");
            break;
        case 13:
            query.count("total").having(Expr::eq(r, "total", std::int64_t(3)));
            break;
        case 14:
            query.count("total").having(Expr::column(r, "total"));
            break;
        case 15:
            query.count("total").project_expr("copy", Expr::gbk(Expr::column(r, "total")));
            break;
        case 16:
            query.count("total")
                .project_expr("next", Expr::column(r, "total"))
                .project_expr("copy", Expr::column(r, "next"));
            break;
        case 17:
            query.count("total").offset(99).limit(0);
            break;
        }
        auto out = detail::model_json(r);
        detail::model_put(out, "case", cases[i]);
        auto result = service.fetch_all(query);
        if (result) {
            Json::Array output_rows(r);
            for (const auto& row : result.value()) {
                auto output_row = detail::model_json(r);
                auto fields = row.into_map(r);
                for (const auto& [name, value] : fields) {
                    detail::model_put(output_row, name, detail::debug_value(r, value));
                }
                output_rows.emplace_back(std::move(output_row));
            }
            detail::model_put(out, "rows", Json(r, std::move(output_rows)));
        } else {
            detail::model_put(out, "error", result.error().executor_error().to_string(r));
        }
        std::cout << out.dump(r) << '\n';
    }
}
