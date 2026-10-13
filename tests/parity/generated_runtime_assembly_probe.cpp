#include "../generated_checkers/include/cpp_checker_fixture.hpp"
#include "../generated_inheritance/include/cpp_inheritance.hpp"
#include "../generated_relations/include/cpp_relations.hpp"
#include "fixture_value.hpp"
#include <sqlite3.h>
#include <iostream>
using namespace teaql;

template <class Executor, class Factory>
static Json run(UserContext& user_context,
                const Json& input,
                Factory factory,
                std::string_view root_name,
                std::span<const std::string_view> entities,
                std::string_view table) {
    auto r = user_context.memory_resource();
    auto& module = factory(user_context, text(field(input, "mode")));
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto& ids = user_context.make<SqliteIdSpaceGenerator>(transport);
    user_context.set_internal_id_generator(user_context, ids);
    auto dialect = user_context.require_resource<SqliteDialect>();
    if (!dialect) {
        throw std::runtime_error(std::string(dialect.error().to_string(r)));
    }
    auto& state = user_context.make<SqlExecutorState>();
    auto executor =
        user_context.make_ref<Executor>(dialect.value().get(), transport, module.metadata, state);
    user_context.register_executor(executor);
    auto* database = connection.native_handle();
    Json::Array registration(r);
    for (auto name : entities) {
        Json::Object item(r);
        item.emplace(String("entity", r), Json(r, name));
        item.emplace(String("metadata", r), Json(r, bool(user_context.require_entity(name))));
        item.emplace(String("checker", r), Json(r, user_context.has_checker(name)));
        item.emplace(String("behavior", r),
                     Json(r, bool(user_context.entity_data_service_behavior(name))));
        registration.emplace_back(item);
    }
    Json::Array rounds(r);
    for (std::uint64_t index = 0; index < std::get<std::uint64_t>(field(input, "rounds").data);
         ++index) {
        const auto& result = user_context.ensure_schema().wait();
        std::string query = "SELECT id,name,version FROM " + std::string(table) + " ORDER BY id";
        sqlite3_stmt* raw = nullptr;
        Json::Array rows(r);
        if (sqlite3_prepare_v2(database, query.c_str(), -1, &raw, nullptr) == SQLITE_OK) {
            std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw,
                                                                                 sqlite3_finalize);
            int status;
            while ((status = sqlite3_step(raw)) == SQLITE_ROW) {
                Json::Array row(r);
                row.emplace_back(static_cast<std::int64_t>(sqlite3_column_int64(raw, 0)));
                row.emplace_back(
                    std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(raw, 1))));
                row.emplace_back(static_cast<std::int64_t>(sqlite3_column_int64(raw, 2)));
                rows.emplace_back(row);
            }
            if (status != SQLITE_DONE) {
                throw std::runtime_error(sqlite3_errmsg(database));
            }
        } else if (raw) {
            sqlite3_finalize(raw);
        }
        auto root = user_context.require_active_root(root_name);
        Json::Object round(r);
        round.emplace(String("error", r), result ? Json(r) : Json(r, result.error().to_string(r)));
        round.emplace(String("root", r), root ? Json(r, root.value().get().id) : Json(r));
        round.emplace(String("rows", r), Json(r, rows));
        rounds.emplace_back(round);
        if (index == 0 && std::get<bool>(field(input, "edit").data)) {
            auto update = "UPDATE " + std::string(table) +
                          " SET name='application owned root',version=7 WHERE id=1";
            if (sqlite3_exec(database, update.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(database));
            }
        }
    }
    Json::Object result(r);
    result.emplace(String("registration", r), Json(r, registration));
    result.emplace(String("rounds", r), Json(r, rounds));
    return Json(r, result);
}

#define ASSEMBLY_FACTORY(domain)                                                                   \
    [](UserContext& user_context, std::string_view mode) -> RuntimeModule& {                       \
        if (mode == "plain") {                                                                     \
            return domain::module(user_context);                                                   \
        }                                                                                          \
        if (mode == "checkers") {                                                                  \
            return domain::moduleWithCheckers(user_context);                                       \
        }                                                                                          \
        if (mode == "behaviors") {                                                                 \
            return domain::moduleWithBehaviors(user_context);                                      \
        }                                                                                          \
        return domain::moduleWithBehaviorsAndCheckers(user_context);                               \
    }

static Json observe(UserContext& user_context, const Json& input) {
    auto model = text(field(input, "model"));
    if (model == "checkers") {
        constexpr std::array<std::string_view, 2> entities{"Tenant", "Note"};
        return run<cpp_checker_fixture::DataServiceExecutor>(user_context,
                                                             input,
                                                             ASSEMBLY_FACTORY(cpp_checker_fixture),
                                                             "Tenant",
                                                             entities,
                                                             "tenant_data");
    }
    constexpr std::array<std::string_view, 4> inherited{
        "Platform", "School", "Academy", "ResearchAcademy"};
    if (model == "inheritance") {
        return run<cpp_inheritance::DataServiceExecutor>(user_context,
                                                         input,
                                                         ASSEMBLY_FACTORY(cpp_inheritance),
                                                         "Platform",
                                                         inherited,
                                                         "platform_data");
    }
    constexpr std::array<std::string_view, 4> relations{"Platform", "School", "Profile", "Device"};
    return run<cpp_relations::DataServiceExecutor>(user_context,
                                                   input,
                                                   ASSEMBLY_FACTORY(cpp_relations),
                                                   "Platform",
                                                   relations,
                                                   "platform_data");
}

#undef ASSEMBLY_FACTORY

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid assembly input");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
