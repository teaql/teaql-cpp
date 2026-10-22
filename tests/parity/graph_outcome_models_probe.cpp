#include <teaql/runtime/detail/ledger_execution_outcome_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/core/json.hpp>
#include <iostream>

using namespace teaql;

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        GlobalContext global;
        for (std::uint64_t generated = 0; generated != 3; ++generated) {
            for (std::string_view payload :
                 {"absent", "readbacks_empty", "fields_empty", "null", "text"}) {
                auto user_context = global.make_user_context();
                auto r = user_context->memory_resource();
                auto& outcome = user_context->make<detail::LedgerExecutionOutcome>();
                for (std::uint64_t index = 0; index != generated; ++index) {
                    outcome.generated_ids.emplace(EntityKey(std::allocator_arg,
                                                            Allocator(r),
                                                            index == 0 ? "Order" : "Child",
                                                            Value(r, std::uint64_t(7))),
                                                  Value(r, 100 + index));
                }
                if (payload != "absent") {
                    outcome.dynamic_readbacks = detail::make_box<detail::DynamicReadbacks>(r);
                    if (payload != "readbacks_empty") {
                        const DynamicFieldDefinitions::Field types[] = {{"note", DataType::Text}};
                        auto definitions =
                            DynamicFieldDefinitions::from_fields(r, "Order", "1", types);
                        if (!definitions) {
                            throw std::runtime_error("invalid outcome definitions");
                        }
                        auto& owned = user_context->make<DynamicFieldDefinitions>(
                            std::move(definitions).value());
                        DynamicFieldValues::Values values(r);
                        if (payload == "null") {
                            values.emplace(String("note", r), Value(r));
                        } else if (payload == "text") {
                            values.emplace(String("note", r), Value(r, "retained outcome payload"));
                        }
                        auto fields = DynamicFieldValues::from_values(r, owned, values);
                        if (!fields) {
                            throw std::runtime_error("invalid outcome values");
                        }
                        outcome.dynamic_readbacks->rows.emplace(
                            EntityKey(std::allocator_arg,
                                      Allocator(r),
                                      "Order",
                                      Value(r, std::uint64_t(7))),
                            std::move(fields).value());
                    }
                }
                if (outcome.size() != generated || outcome.empty() != (generated == 0)) {
                    throw std::runtime_error("outcome generated-ID map mismatch");
                }
                if (generated != 0) {
                    EntityKey key(
                        std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t(7)));
                    auto found = outcome.find(key);
                    if (found == outcome.end() || found->second != Value(r, std::uint64_t(100))) {
                        throw std::runtime_error("outcome generated-ID lookup mismatch");
                    }
                }
                Json::Object output(r);
                output.emplace(String("generated", r), Json(r, std::int64_t(generated)));
                output.emplace(String("payload", r), Json(r, payload));
                output.emplace(String("debug", r), Json(r, debug_string(r, outcome)));
                output.emplace(String("dynamic_debug", r),
                               outcome.dynamic_readbacks
                                   ? Json(r, debug_string(r, *outcome.dynamic_readbacks))
                                   : Json(r));
                std::cout << Json(r, output).dump(r) << '\n';
            }
        }
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
