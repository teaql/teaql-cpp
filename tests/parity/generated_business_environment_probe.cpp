#include "../generated_business_ids/include/cpp_business_ids.hpp"
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

static_assert(cpp_business_ids::BUSINESS_ID_KEY_HEX_ENV == "TEAQL_BUSINESS_ID_KEY_V1_HEX");
static_assert(cpp_business_ids::BUSINESS_ID_KEY_VERSION_ENV == "TEAQL_BUSINESS_ID_KEY_VERSION");
static_assert(
    std::same_as<
        decltype(cpp_business_ids::serviceRuntimeFromConnection(
            std::declval<UserContext&>(), std::declval<Ref<SqliteConnection>>())),
        Result<Ref<cpp_business_ids::DataServiceExecutor>, cpp_business_ids::ServiceRuntimeError>>);

static Json runtime_error(Memory resource, const RuntimeError& error) {
    Json::Object output(resource);
    output.emplace("display", Json(resource, error.to_string(resource)));
    output.emplace("debug", Json(resource, debug_string(resource, error)));
    return Json(resource, std::move(output));
}

static Json observe(GlobalContext& global, const Json& input) {
    auto output_resource = global.memory_resource();
    Json::Array rounds(output_resource);
    Ref<SqliteConnection> connection;
    for (std::uint64_t round = 0; round < std::get<std::uint64_t>(field(input, "rounds").data);
         ++round) {
        auto user_context = global.make_user_context();
        auto resource = user_context->memory_resource();
        {
            auto assembled = [&]() {
                auto entry = text(field(input, "entry"));
                if (entry == "pool") {
                    if (connection.id() == 0) {
                        connection =
                            user_context->make_global_resource<SqliteConnection>(":memory:");
                    } else {
                        connection = user_context->rebind_global(connection);
                    }
                    return cpp_business_ids::serviceRuntimeFromConnection(*user_context,
                                                                          connection);
                }
                if (entry == "config") {
                    cpp_business_ids::ServiceRuntimeConfig config(
                        std::allocator_arg, Allocator(resource), ":memory:");
                    return cpp_business_ids::serviceRuntime(*user_context, config);
                }
                if (entry == "env") {
                    return cpp_business_ids::serviceRuntimeFromEnv(*user_context);
                }
                throw std::invalid_argument("Unknown runtime entry");
            }();
            Json::Object output(resource);
            output.emplace("assembled", Json(resource, bool(assembled)));
            if (!assembled) {
                if (user_context->require_resource<cpp_business_ids::DataServiceExecutor>()) {
                    throw std::runtime_error("Failed key startup registered an executor");
                }
                if (!user_context->business_ids() || !user_context->entity("Receipt")) {
                    throw std::runtime_error("Key failure occurred before modeled infrastructure");
                }
                output.emplace("display", Json(resource, assembled.error().to_string(resource)));
                output.emplace("debug", Json(resource, debug_string(resource, assembled.error())));
                auto cause = assembled.error().source(resource);
                output.emplace("source", cause ? Json(resource, *cause) : Json(resource));
            } else {
                auto& clock = user_context->make<FixedBusinessClock>(
                    UtcDateTime::from_timestamp(1'700'000'000, 0).value());
                user_context->set_business_clock(*user_context, clock);
                auto Q = user_context->queries<cpp_business_ids::query_factory>();
                const auto& before = Q.receiptsMinimal()
                                         .comment("Inspect configured numbered runtime")
                                         .purpose("Verify explicit schema and connection reuse")
                                         .executeForCount(*user_context)
                                         .wait();
                Json::Object before_schema(resource);
                if (before) {
                    before_schema.emplace("count", Json(resource, before.value()));
                    output.emplace("before_schema", Json(resource, std::move(before_schema)));
                } else {
                    output.emplace("before_schema",
                                   runtime_error(resource, before.error().runtime_error()));
                }
                const auto& schema = user_context->ensure_schema().wait();
                if (!schema) {
                    output.emplace("schema_error", runtime_error(resource, schema.error()));
                } else {
                    auto& receipt = Q.receipts()
                                        .comment("Create configured numbered receipt")
                                        .purpose("Verify generated runtime infrastructure")
                                        .newEntity(*user_context);
                    String name("receipt-", resource);
                    name += std::to_string(round);
                    receipt.updateName(name);
                    const auto& saved =
                        cpp_business_ids::AuditedSaveExt<cpp_business_ids::Receipt>::save(
                            std::move(receipt).audit_as("Save configured numbered receipt"),
                            *user_context)
                            .wait();
                    if (saved) {
                        const auto& value = saved.value();
                        Json::Object row(resource);
                        row.emplace("id", Json(resource, value.id()));
                        row.emplace("name", Json(resource, value.name()));
                        row.emplace("number", Json(resource, value.receiptNumber().asStr()));
                        row.emplace("tenant", Json(resource, value.tenantId()));
                        row.emplace("version", Json(resource, value.version()));
                        output.emplace("saved", Json(resource, std::move(row)));
                    } else {
                        output.emplace("saved", runtime_error(resource, saved.error()));
                    }
                    const auto& rows = cpp_business_ids::require_value(
                        Q.receipts()
                            .orderByIdAsc()
                            .comment("Read configured numbered receipts")
                            .purpose("Verify persistence across runtime scopes")
                            .executeForList(*user_context)
                            .wait());
                    Json::Array observed(resource);
                    for (const auto& value : rows.data) {
                        Json::Object row(resource);
                        row.emplace("id", Json(resource, value.id()));
                        row.emplace("name", Json(resource, value.name()));
                        row.emplace("number", Json(resource, value.receiptNumber().asStr()));
                        row.emplace("tenant", Json(resource, value.tenantId()));
                        row.emplace("version", Json(resource, value.version()));
                        observed.push_back(Json(resource, std::move(row)));
                    }
                    output.emplace("rows", Json(resource, std::move(observed)));
                }
            }
            rounds.push_back(Json(output_resource, Json(resource, std::move(output))));
        }
        user_context.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Configured Business-ID runtime retained User bytes");
        }
    }
    Json::Object output(output_resource);
    output.emplace("rounds", Json(output_resource, std::move(rounds)));
    return Json(output_resource, std::move(output));
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        std::string line;
        while (std::getline(std::cin, line)) {
            GlobalContext global;
            auto resource = global.memory_resource();
            auto input = Json::parse(resource, line);
            if (!input) {
                throw std::runtime_error("Invalid configuration observation input");
            }
            try {
                auto output = observe(global, input.value());
                std::cout << output.dump(resource) << '\n';
            } catch (...) {
                if (global.live_user_bytes() != 0) {
                    throw std::runtime_error("Interrupted startup retained User storage");
                }
                throw;
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    return 0;
}
