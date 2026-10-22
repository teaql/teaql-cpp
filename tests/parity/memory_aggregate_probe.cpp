#include "memory_data_service.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <array>
#include <iostream>
#include <limits>
using namespace teaql;

static Vector<Value> values(Memory r, unsigned index) {
    Vector<Value> result(r);
    switch (index) {
    case 0:
        break;
    case 1:
    case 14:
        result.emplace_back();
        break;
    case 2:
        result.emplace_back(std::int64_t(-3));
        result.emplace_back(std::int64_t(1));
        break;
    case 3:
        result.emplace_back(std::uint64_t(4));
        result.emplace_back(std::uint64_t(7));
        break;
    case 4:
        result.emplace_back(std::int64_t(1));
        result.emplace_back(std::uint64_t(2));
        break;
    case 5:
        result.emplace_back(Decimal::from_i64_with_scale(125, 2));
        result.emplace_back(Decimal::from_i64_with_scale(275, 2));
        break;
    case 6:
        result.emplace_back(1.5);
        result.emplace_back(2.5);
        break;
    case 7:
        result.emplace_back("b");
        result.emplace_back("a");
        break;
    case 8:
        result.emplace_back(true);
        break;
    case 9:
        result.emplace_back(true);
        result.emplace_back(false);
        break;
    case 10:
        result.emplace_back(TypedNull{DataType::I64});
        break;
    case 11:
        result.emplace_back(UINT64_MAX);
        result.emplace_back(std::uint64_t(1));
        break;
    case 12:
        result.emplace_back(INT64_MIN);
        result.emplace_back(std::int64_t(-1));
        break;
    case 13:
        result.emplace_back(std::numeric_limits<double>::quiet_NaN());
        result.emplace_back(std::numeric_limits<double>::infinity());
        break;
    case 15:
        result.emplace_back("a");
        break;
    }
    return result;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Item");
    metadata.register_entity(std::move(descriptor));
    auto& service = user_context->make<original_memory::MemoryDataService>(metadata);
    constexpr std::array names{"empty",
                               "null",
                               "signed",
                               "unsigned",
                               "mixed",
                               "decimal",
                               "float",
                               "text",
                               "single_bool",
                               "bool_pair",
                               "typed_null",
                               "u64_overflow",
                               "i64_underflow",
                               "nonfinite",
                               "missing",
                               "single_text"};
    constexpr std::array functions{AggregateFunction::Count,
                                   AggregateFunction::Sum,
                                   AggregateFunction::Avg,
                                   AggregateFunction::Min,
                                   AggregateFunction::Max,
                                   AggregateFunction::Stddev,
                                   AggregateFunction::StddevPop,
                                   AggregateFunction::VarSamp,
                                   AggregateFunction::VarPop,
                                   AggregateFunction::BitAnd,
                                   AggregateFunction::BitOr,
                                   AggregateFunction::BitXor,
                                   AggregateFunction::Count};
    for (unsigned i = 0; i < names.size(); ++i) {
        Vector<CompactRow> rows(r);
        for (auto& value : values(r, i)) {
            Record fields(r);
            if (i != 14) {
                fields.emplace("v", std::move(value));
            }
            rows.emplace_back(std::move(fields));
        }
        service.with_rows("Item", rows);
        for (unsigned j = 0; j < functions.size(); ++j) {
            auto out = detail::model_json(r);
            String name(names[i], r);
            name += '_';
            name += debug_string(r, j);
            detail::model_put(out, "case", name);
            SelectQuery query(std::allocator_arg, Allocator(r), "Item");
            Aggregate aggregate(
                std::allocator_arg, Allocator(r), functions[j], j == 12 ? "*" : "v", "result");
            query.aggregate(aggregate);
            try {
                auto result = service.fetch_all(query);
                if (result) {
                    detail::model_put(
                        out, "value", detail::debug_value(r, *result.value().at(0).get("result")));
                } else {
                    detail::model_put(out, "error", result.error().executor_error().to_string(r));
                }
            } catch (const std::exception& error) {
                detail::model_put(out, "exception", error.what());
            }
            std::cout << out.dump(r) << '\n';
        }
    }
}
