#include <teaql/provider_sqlite/from_sql_error.hpp>
#include <teaql/provider_sqlite/driver_error.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static SqliteFromSqlError make_error(Memory owner, const Json& input) {
    auto kind = text(field(input, "kind"));
    if (kind == "invalid") {
        return SqliteFromSqlError::invalid_type(owner);
    }
    if (kind == "range") {
        return SqliteFromSqlError::out_of_range(owner, number<std::int64_t>(field(input, "value")));
    }
    if (kind == "blob") {
        return SqliteFromSqlError::invalid_blob_size(owner,
                                                     number<std::size_t>(field(input, "expected")),
                                                     number<std::size_t>(field(input, "actual")));
    }
    Vector<std::uint8_t> bytes(owner);
    for (const auto& value : std::get<Json::Array>(field(input, "bytes").data)) {
        bytes.push_back(static_cast<std::uint8_t>(std::get<std::uint64_t>(value.data)));
    }
    auto utf8 = SqliteUtf8Error::validate(
        std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
    if (!utf8) {
        throw std::invalid_argument("UTF-8 conversion fixture is valid");
    }
    if (kind == "utf8") {
        return SqliteFromSqlError::utf8_error(owner, *utf8);
    }
    if (kind == "other") {
        return SqliteFromSqlError::other(owner,
                                         SqliteBoxedError::make<SqliteUtf8Error>(owner, *utf8));
    }
    throw std::invalid_argument("Unknown column-conversion fixture");
}

static Json chain(Memory owner, SqliteErrorCauseView source) {
    Json::Array values(owner);
    while (source) {
        Json::Object value(owner);
        value.emplace("debug", formatted_debug(owner, source));
        value.emplace("display", Json(owner, source.to_string(owner)));
        values.emplace_back(Json(owner, std::move(value)));
        source = source.source();
    }
    return Json(owner, std::move(values));
}

static Json evaluate(Memory owner, const Json& input, bool raw_debug) {
    auto error = make_error(owner, input);
    auto other = make_error(owner, input);
    Json::Object model(owner);
    model.emplace("debug", formatted_debug(owner, error));
    model.emplace("display", Json(owner, error.to_string(owner)));
    model.emplace("self_equal", Json(owner, error == error));
    model.emplace("fresh_equal", Json(owner, error == other));
    model.emplace("chain", chain(owner, error.source()));
    auto converted = std::move(error).into_driver_error(owner);
    auto range = converted.kind() == SqliteDriverError::Kind::IntegralValueOutOfRange;
    Json::Object result(owner), driver(owner);
    result.emplace("model", Json(owner, std::move(model)));
    driver.emplace("kind",
                   Json(owner, range ? "IntegralValueOutOfRange" : "FromSqlConversionFailure"));
    driver.emplace(
        "unknown_index",
        Json(owner,
             range ? converted.simple_error().index() == SIZE_MAX
                   : std::get<SqliteDriverError::BoxedFailure>(converted.payload()).index ==
                         SIZE_MAX));
    auto blob = !range && std::get<SqliteDriverError::BoxedFailure>(converted.payload()).type ==
                              SqliteValueType::Blob;
    driver.emplace("type", range ? Json(owner) : Json(owner, blob ? "Blob" : "Null"));
    driver.emplace("blob_type", Json(owner, blob));
    driver.emplace("display", Json(owner, converted.to_string(owner)));
    driver.emplace("chain", chain(owner, converted.source()));
    driver.emplace("self_equal", Json(owner, converted == converted));
    driver.emplace("native_none", Json(owner, converted.sqlite_error() == nullptr));
    result.emplace("converted", Json(owner, std::move(driver)));
    if (raw_debug) {
        result.emplace("converted_raw_debug", formatted_debug(owner, converted));
    }
    return Json(owner, std::move(result));
}

int main(int argc, char** argv) {
    const bool raw_debug = argc == 2 && std::string_view(argv[1]) == "--raw-debug";
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::invalid_argument("Invalid column-conversion fixture JSON");
        }
        std::cout << evaluate(owner, *input, raw_debug).dump(owner) << '\n';
    }
}
