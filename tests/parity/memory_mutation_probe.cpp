#include "memory_data_service.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <array>
#include <iostream>
using namespace teaql;
using Service = original_memory::MemoryDataService;

static Json outcome(Memory r, Result<std::uint64_t, Service::Error> result) {
    auto out = detail::model_json(r);
    if (result) {
        detail::model_put(out, "count", result.value());
    } else {
        detail::model_put(out, "error", result.error().to_string(r));
    }
    return out;
}

static Json snapshot(Service& service, std::string_view entity = "Item") {
    auto r = service.memory_resource();
    auto out = detail::model_json(r);
    SelectQuery query(std::allocator_arg, Allocator(r), entity);
    auto result = service.fetch_all(query);
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
    return out;
}

static Value version(Memory r, unsigned index) {
    switch (index) {
    case 0:
    case 1:
        return Value(r);
    case 2:
        return Value(r, std::int64_t(-3));
    case 3:
        return Value(r, std::int64_t(0));
    case 4:
        return Value(r, std::int64_t(3));
    case 5:
        return Value(r, std::uint64_t(3));
    case 6:
        return Value(r, "3");
    case 7:
        return Value(r, INT64_MAX);
    case 8:
        return Value(r, INT64_MIN);
    default:
        return Value(r, INT64_MAX - 1);
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    for (unsigned named = 0; named < 2; ++named) {
        auto id_field = named == 0 ? "id" : "pk";
        auto version_field = named == 0 ? "version" : "revision";
        for (unsigned v = 0; v < 10; ++v) {
            for (unsigned identity = 0; identity < 3; ++identity) {
                for (unsigned op = 0; op < 19; ++op) {
                    auto user_context = global.make_user_context();
                    auto r = user_context->memory_resource();
                    auto& metadata = user_context->make<InMemoryMetadataStore>();
                    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Item");
                    if (named == 1) {
                        descriptor.property(id_field, DataType::I64).id();
                        descriptor.property(version_field, DataType::I64).version();
                    }
                    metadata.register_entity(std::move(descriptor));
                    auto& service = user_context->make<Service>(metadata);
                    Record fields(r);
                    fields.emplace(id_field, Value(r, std::int64_t(1)));
                    fields.emplace("name", Value(r, "before"));
                    if (v != 0) {
                        fields.emplace(version_field, version(r, v));
                    }
                    Vector<CompactRow> rows(r);
                    rows.emplace_back(std::move(fields));
                    service.with_rows("Item", rows);
                    auto id = identity == 1 ? Value(r, std::uint64_t(1))
                                            : Value(r, std::int64_t(identity == 0 ? 1 : 2));
                    auto invoke = [&]() {
                        if (op < 6 || op == 18) {
                            const std::array<std::optional<std::int64_t>, 6> expected{
                                std::nullopt, 3, -3, INT64_MAX, INT64_MIN, 0};
                            UpdateCommand command(std::allocator_arg, Allocator(r), "Item", id);
                            command.value("name", "after");
                            command.expected_version_value =
                                op == 18 ? std::optional<std::int64_t>(3) : expected[op];
                            if (op == 18) {
                                command.value(version_field, std::int64_t(42));
                            }
                            return service.update(command);
                        }
                        if (op < 13) {
                            const std::array<std::optional<std::int64_t>, 5> expected{
                                std::nullopt, 3, -3, INT64_MAX, INT64_MIN};
                            DeleteCommand command(std::allocator_arg, Allocator(r), "Item", id);
                            command.expected_version_value =
                                op < 11 ? expected[op - 6] : expected[op - 11];
                            command.soft_delete = op < 11;
                            return service.delete_command(command);
                        }
                        const std::array<std::int64_t, 5> expected{3, -3, INT64_MAX, INT64_MIN, 0};
                        RecoverCommand command(
                            std::allocator_arg, Allocator(r), "Item", id, expected[op - 13]);
                        return service.recover(command);
                    };
                    auto result = detail::model_json(r);
                    bool panicked = false;
                    try {
                        result = outcome(r, invoke());
                    } catch (const original_memory::MemoryArithmeticPanic& error) {
                        panicked = true;
                        detail::model_put(result, "panic", error.what());
                    }
                    auto out = detail::model_json(r);
                    auto name = String("mutation_", r) + debug_string(r, named) + "_" +
                                debug_string(r, v) + "_" + debug_string(r, identity) + "_" +
                                debug_string(r, op);
                    detail::model_put(out, "case", name);
                    detail::model_put(out, "result", std::move(result));
                    detail::model_put(out, "after", snapshot(service));
                    if (panicked) {
                        Record replacement(r);
                        replacement.emplace("name", Value(r, "replacement"));
                        Vector<CompactRow> seed(r);
                        seed.emplace_back(std::move(replacement));
                        service.with_rows("Item", seed);
                        detail::model_put(out, "after_seed", snapshot(service));
                        Json::Array actions(r);
                        InsertCommand insert(std::allocator_arg, Allocator(r), "Item");
                        actions.emplace_back(outcome(r, service.insert(insert)));
                        UpdateCommand update(
                            std::allocator_arg, Allocator(r), "Item", std::int64_t(1));
                        actions.emplace_back(outcome(r, service.update(update)));
                        DeleteCommand remove(
                            std::allocator_arg, Allocator(r), "Item", std::int64_t(1));
                        actions.emplace_back(outcome(r, service.delete_command(remove)));
                        RecoverCommand recover(
                            std::allocator_arg, Allocator(r), "Item", std::int64_t(1), 3);
                        actions.emplace_back(outcome(r, service.recover(recover)));
                        actions.emplace_back(snapshot(service, "Missing"));
                        detail::model_put(out, "poison_actions", Json(r, std::move(actions)));
                    }
                    std::cout << out.dump(r) << '\n';
                }
            }
        }
    }
    for (unsigned mode = 0; mode < 4; ++mode) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Item");
        metadata.register_entity(std::move(descriptor));
        auto& service = user_context->make<Service>(metadata);
        InsertCommand command(std::allocator_arg, Allocator(r), mode == 2 ? "Missing" : "Item");
        if (mode != 3) {
            command.value("id", std::int64_t(1)).value("version", std::int64_t(3));
        }
        auto out = detail::model_json(r);
        detail::model_put(out, "case", String("insert_", r) + debug_string(r, mode));
        detail::model_put(out, "first", outcome(r, service.insert(command)));
        detail::model_put(out, "second", mode == 1 ? outcome(r, service.insert(command)) : Json(r));
        detail::model_put(out, "after", snapshot(service));
        std::cout << out.dump(r) << '\n';
    }
}
