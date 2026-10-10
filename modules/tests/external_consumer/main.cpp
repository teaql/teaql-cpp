#include <cstdint>
#include <iostream>
#include <memory>
#include <memory_resource>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <variant>

#if defined(TEAQL_CONSUME_MODULES)
import teaql_provider_sqlite;
#else
#include <teaql/core.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/runtime.hpp>
#endif

const std::type_info& installed_header_value_type();
bool installed_header_value_matches(const teaql::Value&, std::string_view);

namespace {
void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void consume_installed_package() {
    teaql::GlobalContext global;
    auto user = global.make_user_context();
    auto reference = user->make_ref<teaql::Value>("installed consumer owns this long string");
    check(typeid(user->get(reference)) == installed_header_value_type(),
          "Installed header and imported types have matching RTTI");
    auto retained = user->make_global<teaql::Value>(user->get(reference));
    {
        auto& connection = user->make_resource<teaql::SqliteConnection>();
        auto& provider = user->make<teaql::SqliteMutationExecutor>(connection);
        auto& query = user->make<teaql::CompiledQuery>();
        query.sql = "SELECT 42 AS answer";
        auto& operation = provider.fetch_all_compact_sql(user->memory_resource(), query);
        const auto& rows = operation.wait().value();
        check(rows.size() == 1 && rows[0].get("answer")->try_u64() == 42,
              "Installed SQLite provider executes and decodes through the async SPI");
        check(rows[0].memory_resource() == user->memory_resource(),
              "Installed provider results use the UserContext domain");
    }
    auto leap_day = teaql::Date::from_ymd(2024, 2, 29);
    check(leap_day && leap_day->epoch_days() == 19782,
          "Installed consumer links canonical out-of-line core functions");
    user.reset();
    auto next = global.make_user_context();
    check(installed_header_value_matches(next->get(next->rebind_global(retained)),
                                         "installed consumer owns this long string"),
          "Explicit destination copy survives actual UserContext deletion");
    bool expired = false;
    try {
        (void)next->get(reference);
    } catch (const teaql::ReferenceError&) {
        expired = true;
    }
    check(expired, "Installed checked references reject a deleted UserContext");
}
}

int main() {
    try {
        consume_installed_package();
        std::cout << "Installed package, ABI, SQLite and Context contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
