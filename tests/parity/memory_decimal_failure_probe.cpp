#include "memory_data_service.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
#include <limits>
using namespace teaql;
using Service = original_memory::MemoryDataService;

static Json output(Memory r, Result<Vector<CompactRow>, Service::Error> result) {
    auto out = detail::model_json(r);
    if (!result) {
        detail::model_put(out, "error", result.error().to_string(r));
        return out;
    }
    Json::Array rows(r);
    for (const auto& row : result.value()) {
        auto fields = detail::model_json(r);
        for (const auto& [key, value] : row.into_map(r)) {
            detail::model_put(fields, key, detail::debug_value(r, value));
        }
        rows.emplace_back(std::move(fields));
    }
    detail::model_put(out, "rows", Json(r, std::move(rows)));
    return out;
}

static Vector<CompactRow> rows(Memory r, const Vector<Value>& values) {
    Vector<CompactRow> result(r);
    for (const auto& value : values) {
        Record fields(r);
        fields.emplace("v", value);
        result.emplace_back(std::move(fields));
    }
    return result;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    for (unsigned dataset = 0; dataset < 6; ++dataset) {
        for (unsigned op = 0; op < 2; ++op) {
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto& metadata = user_context->make<InMemoryMetadataStore>();
            metadata.register_entity(EntityDescriptor(std::allocator_arg, Allocator(r), "Item"));
            auto& service = user_context->make<Service>(metadata);
            Vector<Value> values(r);
            if (dataset == 5) {
                values.emplace_back(std::numeric_limits<double>::max());
                values.emplace_back(std::numeric_limits<double>::infinity());
            } else {
                values.emplace_back(dataset == 1 ? Decimal::MIN : Decimal::MAX);
                if (dataset == 0 || dataset == 1 || dataset == 3) {
                    values.emplace_back(dataset == 0 ? Decimal::MAX : Decimal::MIN);
                } else {
                    values.emplace_back(std::int64_t(1));
                }
                if (dataset == 4) {
                    values.emplace_back(std::int64_t(-1));
                }
            }
            service.seed("Item", rows(r, values));
            SelectQuery query(std::allocator_arg, Allocator(r), "Item");
            if (op == 0) {
                query.sum("v", "result");
            } else {
                query.avg("v", "result");
            }
            auto result = detail::model_json(r);
            try {
                result = output(r, service.fetch_all(query));
            } catch (const std::overflow_error& error) {
                // Only the established Decimal addition panic/exception boundary
                // is normalized. Other exceptions fail the probe.
                if (std::string_view(error.what()) != "Addition overflowed") {
                    throw;
                }
                detail::model_put(result, "panic", error.what());
            }
            SelectQuery all(std::allocator_arg, Allocator(r), "Item");
            auto after = output(r, service.fetch_all(all));
            Vector<Value> replacement(r);
            replacement.emplace_back(std::int64_t(2));
            service.seed("Item", rows(r, replacement));
            auto replaced = output(r, service.fetch_all(query));
            InsertCommand insert(std::allocator_arg, Allocator(r), "Item");
            insert.value("v", std::int64_t(4));
            auto inserted = service.insert(insert);
            if (!inserted) {
                throw std::runtime_error("insert after query failed");
            }
            auto out = detail::model_json(r);
            detail::model_put(out,
                              "case",
                              String("decimal_failure_", r) + debug_string(r, dataset) + "_" +
                                  debug_string(r, op));
            detail::model_put(out, "result", std::move(result));
            detail::model_put(out, "after", std::move(after));
            detail::model_put(out, "replacement", std::move(replaced));
            detail::model_put(out, "inserted", inserted.value());
            detail::model_put(out, "after_insert", output(r, service.fetch_all(query)));
            std::cout << out.dump(r) << '\n';
        }
    }
}
