#include <teaql/provider_sqlite/error_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include "sqlite_boxed_error_fixture.hpp"
#include <iostream>

static SqliteDriverError driver_error(Memory owner, const Json& input) {
    auto integer = [](const Json& value) {
        if (auto* signed_value = std::get_if<std::int64_t>(&value.data)) {
            return static_cast<int>(*signed_value);
        }
        return static_cast<int>(std::get<std::uint64_t>(value.data));
    };
    auto kind = text(field(input, "kind"));
    auto native = [&] {
        auto error = SqliteNativeError::from_extended_code(integer(field(input, "code")));
        if (auto* category = optional(input, "category")) {
            error.code =
                static_cast<SqliteNativeError::Code>(std::get<std::uint64_t>(category->data));
        }
        return error;
    };
    if (kind == "native") {
        std::optional<std::string_view> message;
        if (auto* value = std::get_if<String>(&field(input, "message").data)) {
            message = *value;
        }
        return SqliteDriverError::native_failure(owner, native(), message);
    }
    if (kind == "input") {
        return SqliteDriverError::sql_input(owner,
                                            native(),
                                            text(field(input, "message")),
                                            text(field(input, "sql")),
                                            integer(field(input, "offset")));
    }
    if (kind == "count") {
        return SqliteDriverError::invalid_parameter_count(
            owner,
            number<std::size_t>(field(input, "given")),
            number<std::size_t>(field(input, "expected")));
    }
    if (kind == "boxed") {
        std::optional<SqliteBoxedError> next;
        const auto& messages = std::get<Json::Array>(field(input, "messages").data);
        for (auto position = messages.rbegin(); position != messages.rend(); ++position) {
            next.emplace(SqliteBoxedError::make<boxed_fixture::FixtureCause>(
                owner, text(*position), std::move(next)));
        }
        if (!next) {
            throw std::invalid_argument("Empty boxed fixture chain");
        }
        auto category = std::get<std::uint64_t>(field(input, "category").data);
        if (category == 0) {
            return SqliteDriverError::from_sql_conversion_failure(
                owner,
                number<std::size_t>(field(input, "index")),
                static_cast<SqliteValueType>(std::get<std::uint64_t>(field(input, "type").data)),
                std::move(*next));
        }
        if (category == 1) {
            return SqliteDriverError::to_sql_conversion_failure(owner, std::move(*next));
        }
        if (category == 2) {
            return SqliteDriverError::user_function_error(owner, std::move(*next));
        }
        throw std::invalid_argument("Unknown boxed fixture category");
    }
    if (kind == "simple") {
        auto variant = std::get<std::uint64_t>(field(input, "variant").data);
        switch (variant) {
        case 0:
            return SqliteDriverError::sqlite_single_threaded_mode(owner);
        case 1:
            return SqliteDriverError::integral_value_out_of_range(
                owner,
                number<std::size_t>(field(input, "index")),
                number<std::int64_t>(field(input, "value")));
        case 2:
            return SqliteDriverError::invalid_parameter_name(owner, text(field(input, "name")));
        case 3:
            return SqliteDriverError::query_returned_no_rows(owner);
        case 4:
            return SqliteDriverError::query_returned_more_than_one_row(owner);
        case 5:
            return SqliteDriverError::invalid_column_name(owner, text(field(input, "name")));
        case 6:
            return SqliteDriverError::statement_changed_rows(
                owner, number<std::size_t>(field(input, "index")));
        case 7:
            return SqliteDriverError::invalid_function_parameter_type(
                owner,
                number<std::size_t>(field(input, "index")),
                static_cast<SqliteValueType>(std::get<std::uint64_t>(field(input, "type").data)));
        case 8:
            return SqliteDriverError::invalid_query(owner);
        case 9:
            return SqliteDriverError::unwinding_panic(owner);
        case 10:
            return SqliteDriverError::get_aux_wrong_type(owner);
        case 11:
            return SqliteDriverError::invalid_database_index(
                owner, number<std::size_t>(field(input, "index")));
        default:
            throw std::invalid_argument("Unknown enabled alternative fixture");
        }
    }
    if (kind == "multiple") {
        return SqliteDriverError::multiple_statement(owner);
    }
    if (kind == "returned") {
        return SqliteDriverError::execute_returned_results(owner);
    }
    if (kind == "index") {
        return SqliteDriverError::invalid_column_index(owner,
                                                       number<std::size_t>(field(input, "index")));
    }
    if (kind == "type") {
        return SqliteDriverError::invalid_column_type(
            owner,
            number<std::size_t>(field(input, "index")),
            text(field(input, "name")),
            static_cast<SqliteValueType>(std::get<std::uint64_t>(field(input, "type").data)));
    }
    if (kind == "utf8" || kind == "nul" || kind == "path") {
        Vector<std::uint8_t> bytes(owner);
        for (const auto& byte : std::get<Json::Array>(field(input, "bytes").data)) {
            auto value = std::get<std::uint64_t>(byte.data);
            if (value > UINT8_MAX) {
                throw std::invalid_argument("Invalid driver fixture byte");
            }
            bytes.push_back(static_cast<std::uint8_t>(value));
        }
        std::string_view view(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        if (kind == "utf8") {
            auto error = SqliteUtf8Error::validate(view);
            if (!error) {
                throw std::invalid_argument("UTF-8 error fixture is valid UTF-8");
            }
            return SqliteDriverError::utf8_error(
                owner, number<std::size_t>(field(input, "index")), *error);
        }
        if (kind == "nul") {
            auto error = SqliteNulError::from_bytes(owner, bytes);
            if (!error) {
                throw std::invalid_argument("NUL error fixture contains no NUL");
            }
            return SqliteDriverError::nul_error(owner, std::move(*error));
        }
        return SqliteDriverError::invalid_path(
            owner, SqlitePath(std::allocator_arg, Allocator(owner), view));
    }
    throw std::invalid_argument("Unknown driver error fixture");
}

static Json evaluate(Memory owner, const Json& input) {
    if (auto* field_value = optional(input, "outer")) {
        const auto kind = text(*field_value);
        const auto message = text(field(input, "text"));
        auto wrapper = [&] {
            if (kind == "unsupported") {
                return MutationExecutorError::unsupported_value(owner, message);
            }
            if (kind == "column") {
                return MutationExecutorError::unsupported_column_type(owner, message);
            }
            if (kind == "bind") {
                return MutationExecutorError::bind(owner, message);
            }
            if (kind == "lock") {
                return MutationExecutorError::lock(owner, message);
            }
            SqlCompileError compile(std::allocator_arg,
                                    Allocator(owner),
                                    kind == "shape" ? SqlCompileError::Code::InvalidSchemaShape
                                                    : SqlCompileError::Code::UnknownEntity,
                                    message);
            compile.reason = "reason:";
            compile.reason += message;
            return MutationExecutorError::sql_compile(owner, std::move(compile));
        }();
        Json::Object result(owner);
        result.emplace("driver", Json(owner));
        result.emplace("wrapper_debug", formatted_debug(owner, wrapper));
        result.emplace("wrapper_display", Json(owner, wrapper.to_string(owner)));
        result.emplace("wrapper_source_none", Json(owner, wrapper.source() == nullptr));
        return Json(owner, std::move(result));
    }
    auto error = driver_error(owner, input);
    auto other = driver_error(owner, input);
    Json::Object driver(owner);
    driver.emplace("debug", formatted_debug(owner, error));
    driver.emplace("display", Json(owner, error.to_string(owner)));
    driver.emplace("self_equal", Json(owner, error == error));
    driver.emplace("same_payload_equal", Json(owner, error == other));
    if (auto* changed = optional(input, "changed")) {
        auto different = driver_error(owner, *changed);
        driver.emplace("changed_equal", Json(owner, error == different));
    }
    driver.emplace("source_some", Json(owner, error.source() != nullptr));
    driver.emplace("source_display",
                   error.source() ? Json(owner, error.source()->to_string(owner)) : Json(owner));
    driver.emplace("source_source_none",
                   error.source() ? Json(owner, error.source()->source() == nullptr) : Json(owner));
    driver.emplace("native",
                   error.sqlite_error() ? formatted_debug(owner, *error.sqlite_error())
                                        : Json(owner));
    driver.emplace("native_code",
                   error.sqlite_error_code()
                       ? Json(owner, debug_string(owner, *error.sqlite_error_code()))
                       : Json(owner));
    driver.emplace("native_extended",
                   error.sqlite_extended_error_code()
                       ? Json(owner, static_cast<std::int64_t>(*error.sqlite_extended_error_code()))
                       : Json(owner));
    if (text(field(input, "kind")) == "boxed") {
        Json::Array chain(owner);
        auto cursor = error.source();
        while (cursor) {
            Json::Object cause(owner);
            cause.emplace("debug", formatted_debug(owner, cursor));
            cause.emplace("display", Json(owner, cursor.to_string(owner)));
            chain.emplace_back(Json(owner, std::move(cause)));
            cursor = cursor.source();
        }
        driver.emplace("cause_chain", Json(owner, std::move(chain)));
    }
    auto wrapper = MutationExecutorError::sqlite(owner, std::move(error));
    Json::Object result(owner);
    result.emplace("driver", Json(owner, std::move(driver)));
    result.emplace("wrapper_debug", formatted_debug(owner, wrapper));
    result.emplace("wrapper_display", Json(owner, wrapper.to_string(owner)));
    result.emplace("wrapper_source_none", Json(owner, wrapper.source() == nullptr));
    return Json(owner, std::move(result));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::invalid_argument("Invalid structured driver fixture JSON");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
