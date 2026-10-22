#include "memory_data_service.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <array>
#include <iostream>
using namespace teaql;

static Expr bad(Memory r) {
    return Expr::soundex(Expr::value(r, std::int64_t(1)));
}

static Expr count(Memory r) {
    return Expr::function(r, ExprFunction::Count, std::span<const Expr>{});
}

static Expr leaf(Memory r, unsigned index) {
    switch (index) {
    case 0:
        return Expr::value(r, true);
    case 1:
        return Expr::value(r, false);
    case 2:
        return Expr::value(r, Value(r));
    case 3:
        return Expr::value(r, std::int64_t(1));
    case 4:
        return Expr::column(r, "missing");
    case 5:
        return bad(r);
    case 6:
        return Expr::gbk(Expr::value(r, true));
    default: {
        EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
        SelectQuery query(std::allocator_arg, Allocator(r), "Child");
        return Expr::in_subquery(r, "id", child, query, "id");
    }
    }
}

static Expr expression(Memory r, unsigned index) {
    if (index < 256) {
        std::array parts{leaf(r, index / 32), leaf(r, (index / 4) % 8)};
        auto mode = index % 4;
        auto result = mode % 2 == 0 ? Expr::all_of(r, parts) : Expr::any_of(r, parts);
        return mode >= 2 ? Expr::negate(result) : std::move(result);
    }
    switch (index - 256) {
    case 0:
        return Expr::all_of(r, std::span<const Expr>{});
    case 1:
        return Expr::any_of(r, std::span<const Expr>{});
    case 2:
        return Expr::negate(leaf(r, 4));
    case 3:
        return Expr::negate(Expr::negate(leaf(r, 0)));
    case 4:
        return Expr::binary(bad(r), BinaryOp::Eq, count(r));
    case 5:
        return Expr::binary(count(r), BinaryOp::Eq, bad(r));
    case 6:
    case 7:
    case 8: {
        Expr result(r, Expr::Kind::Between);
        result.args.emplace_back(index == 262 ? bad(r) : leaf(r, 2));
        result.args.emplace_back(index == 262 ? count(r) : index == 263 ? bad(r) : leaf(r, 2));
        result.args.emplace_back(index == 262
                                     ? Expr::function(r, ExprFunction::Gbk, std::span<const Expr>{})
                                 : index == 263 ? count(r)
                                                : bad(r));
        return result;
    }
    case 9: {
        std::array parts{leaf(r, 0), leaf(r, 1)};
        Expr result(r, Expr::Kind::IsNull);
        result.args.emplace_back(Expr::all_of(r, parts));
        return result;
    }
    case 10: {
        Expr result(r, Expr::Kind::IsNotNull);
        result.args.emplace_back(leaf(r, 2));
        return result;
    }
    case 11: {
        std::array parts{leaf(r, 0), leaf(r, 1)};
        return Expr::gbk(Expr::all_of(r, parts));
    }
    case 12: {
        std::array parts{leaf(r, 0), leaf(r, 1)};
        return Expr::soundex(Expr::any_of(r, parts));
    }
    case 13: {
        std::array parts{leaf(r, 0), bad(r)};
        return Expr::function(r, ExprFunction::Gbk, parts);
    }
    case 14:
        return Expr::soundex(leaf(r, 2));
    default: {
        EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
        SelectQuery query(std::allocator_arg, Allocator(r), "Child");
        return Expr::not_in_subquery(r, "id", child, query, "id");
    }
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
    for (unsigned index = 0; index < 272; ++index) {
        auto out = detail::model_json(r);
        detail::model_put(out, "case", String("recursive_", r) + debug_string(r, index));
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
