#include "memory_data_service.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <array>
#include <iostream>
using namespace teaql;

static Expr expression(Memory r, unsigned index) {
    switch (index) {
    case 0:
        return Expr::value(r, std::int64_t(0));
    case 1:
        return Expr::value(r, "teaql");
    case 2:
        return Expr::value(r, Value(r));
    case 3:
        return Expr::value(r, true);
    case 4:
        return Expr::soundex(Expr::value(r, std::int64_t(1)));
    case 5:
        return Expr::function(r, ExprFunction::Soundex, std::span<const Expr>{});
    case 6: {
        std::array<Expr, 2> args{Expr::value(r, "a"), Expr::value(r, "b")};
        return Expr::function(r, ExprFunction::Soundex, args);
    }
    case 7:
        return Expr::soundex(Expr::column(r, "name"));
    case 8:
        return Expr::function(r, ExprFunction::Gbk, std::span<const Expr>{});
    case 9:
        return Expr::gbk(Expr::value(r, true));
    case 10:
        return Expr::function(r, ExprFunction::Count, std::span<const Expr>{});
    case 11:
        return Expr::binary(
            Expr::value(r, std::int64_t(1)), BinaryOp::In, Expr::value(r, std::int64_t(1)));
    case 12:
        return Expr::binary(
            Expr::value(r, std::int64_t(1)), BinaryOp::NotIn, Expr::value(r, std::int64_t(1)));
    case 13:
        return Expr::between(r, "missing", std::int64_t(1), std::int64_t(2));
    case 14:
        return Expr::binary(
            Expr::value(r, std::int64_t(1)), BinaryOp::Eq, Expr::value(r, std::uint64_t(1)));
    case 15:
        return Expr::binary(Expr::value(r, Value(r)), BinaryOp::NotLike, Expr::value(r, "x"));
    default:
        throw std::runtime_error("unknown fixture");
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Item");
    descriptor.property("name", DataType::Text);
    metadata.register_entity(std::move(descriptor));
    auto& service = user_context->make<original_memory::MemoryDataService>(metadata);
    Record record(r);
    record.emplace("name", Value(r, "teaql"));
    Vector<CompactRow> rows(r);
    rows.emplace_back(std::move(record));
    service.with_rows("Item", rows);
    constexpr std::array names{"literal_zero",
                               "literal_text",
                               "literal_null",
                               "literal_true",
                               "soundex_number",
                               "soundex_zero_args",
                               "soundex_two_args",
                               "soundex_text",
                               "gbk_zero_args",
                               "gbk_boolean",
                               "count_scalar",
                               "in_non_list",
                               "not_in_non_list",
                               "null_between",
                               "mixed_integer_equal",
                               "null_not_like"};
    for (unsigned index = 0; index < names.size(); ++index) {
        auto out = detail::model_json(r);
        detail::model_put(out, "case", names[index]);
        try {
            SelectQuery query(std::allocator_arg, Allocator(r), "Item");
            query.filter(expression(r, index));
            auto result = service.fetch_all(query);
            if (result) {
                detail::model_put(out, "ok", !result.value().empty());
            } else if (result.error().kind() ==
                       original_memory::MemoryDataService::Error::Kind::Executor) {
                detail::model_put(out, "error", result.error().executor_error().to_string(r));
            } else {
                throw std::runtime_error("unexpected service error kind");
            }
        } catch (const std::exception& error) {
            detail::model_put(out, "exception", error.what());
        }
        std::cout << out.dump(r) << '\n';
    }
}
