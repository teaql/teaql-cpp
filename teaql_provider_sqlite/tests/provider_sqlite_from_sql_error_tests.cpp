#include <teaql/provider_sqlite/from_sql_error.hpp>
#include <teaql/provider_sqlite/error_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void retained_conversions() {
    GlobalContext global;
    std::array<Ref<SqliteDriverError>, 5> retained;
    std::array<Ref<String>, 5> diagnostics;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto utf8 = SqliteUtf8Error::validate(std::string_view("\xff", 1));
        auto direct_utf8 = SqliteDriverError::from_utf8_error(owner, *utf8);
        check(direct_utf8.kind() == SqliteDriverError::Kind::Utf8Error &&
                  std::get<SqliteDriverError::Utf8Error>(direct_utf8.payload()).index == SIZE_MAX &&
                  direct_utf8.to_string(owner) == utf8->to_string(owner),
              "bare UTF-8 conversion preserves its distinct driver tag and unknown local column");
        const std::uint8_t nul_bytes[]{'a', 0, 'b'};
        auto nul = SqliteNulError::from_bytes(owner, nul_bytes);
        auto direct_nul = SqliteDriverError::from_nul_error(owner, std::move(*nul));
        check(
            direct_nul.kind() == SqliteDriverError::Kind::NulError &&
                std::get<SqliteDriverError::NulError>(direct_nul.payload()).error.bytes().size() ==
                    3,
            "bare NUL conversion preserves the original owned error bytes");
        std::array errors{SqliteFromSqlError::invalid_type(owner),
                          SqliteFromSqlError::out_of_range(owner, INT64_MIN),
                          SqliteFromSqlError::utf8_error(owner, *utf8),
                          SqliteFromSqlError::invalid_blob_size(owner, 16, 8),
                          SqliteFromSqlError::other(
                              owner, SqliteBoxedError::make<SqliteUtf8Error>(owner, *utf8))};
        for (std::size_t index = 0; index < errors.size(); ++index) {
            auto driver = std::move(errors[index]).into_driver_error(owner);
            retained[index] = user->make_global<SqliteDriverError>(driver);
            diagnostics[index] = user->make_global<String>(debug_string(owner, driver));
        }
    }
    check(global.live_user_bytes() == 0, "retained conversions keep no User blocks");
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    for (std::size_t index = 0; index < retained.size(); ++index) {
        const auto& driver = user->get(user->rebind_global(retained[index]));
        check(debug_string(owner, driver) == user->get(user->rebind_global(diagnostics[index])) &&
                  driver.memory_resource() == global.memory_resource(),
              "conversion shape and diagnostics survive actual User deletion");
        if (index == 1) {
            check(driver.kind() == SqliteDriverError::Kind::IntegralValueOutOfRange &&
                      driver.simple_error().index() == SIZE_MAX &&
                      driver.simple_error().value() == INT64_MIN && driver.source() == nullptr,
                  "range conversion preserves unknown local index and signed value");
        } else {
            const auto& payload = std::get<SqliteDriverError::BoxedFailure>(driver.payload());
            check(payload.index == SIZE_MAX &&
                      payload.type ==
                          (index == 3 ? SqliteValueType::Blob : SqliteValueType::Null) &&
                      payload.error.memory_resource() == global.memory_resource(),
                  "boxed conversion preserves source type policy and destination ownership");
            if (index == 2) {
                check(driver.source().source() != nullptr &&
                          driver.source().source().source() == nullptr,
                      "FromSqlError UTF-8 conversion keeps the intermediate conversion cause");
            }
            if (index == 4) {
                check(
                    driver.source().source() == nullptr,
                    "Other conversion unwraps its concrete source without adding an artificial cause");
            }
        }
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        retained_conversions();
        std::pmr::set_default_resource(previous);
        std::cout << "SQLite column-conversion scope contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
