#include <teaql/provider_sqlite/driver_error.hpp>
#include <teaql/provider_sqlite/error_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;
static_assert(!detail::domain_copyable<SqliteErrorCauseView>);

static void check(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

static void independent_native_fields() {
    GlobalContext global;
    std::array<Ref<SqliteDriverError>, 2> retained;
    const SqliteNativeError raw{SqliteNativeError::Code::DatabaseBusy, 1555};
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        String long_text(8192, 'n', owner);
        retained[0] =
            user->make_global<SqliteDriverError>(SqliteDriverError::native_failure(owner, raw, {}));
        retained[1] = user->make_global<SqliteDriverError>(
            SqliteDriverError::sql_input(owner, raw, long_text, long_text, -1));
        auto derived = SqliteDriverError::native_failure(owner, 1555, {});
        auto explicit_error = SqliteDriverError::native_failure(owner, raw, {});
        check(explicit_error != derived &&
                  explicit_error.to_string(owner) == derived.to_string(owner),
              "native category participates in equality independently of Display's extended code");
    }
    check(global.live_user_bytes() == 0, "native payload retention releases every User block");
    auto reader = global.make_user_context();
    auto owner = reader->memory_resource();
    const auto& failure = reader->get(reader->rebind_global(retained[0]));
    const auto& input = reader->get(reader->rebind_global(retained[1]));
    check(failure.sqlite_error() && *failure.sqlite_error() == raw &&
              failure.sqlite_error_code() == raw.code &&
              failure.sqlite_extended_error_code() == raw.extended_code,
          "retained accessors preserve independently supplied native fields");
    check(input.sql_input_error().error == raw && !input.sqlite_error() &&
              input.sql_input_error().message.size() == 8192 &&
              input.sql_input_error().sql.get_allocator().resource() == global.memory_resource(),
          "SQL input retains complete raw error and owned text without native-failure accessors");
    check(debug_string(owner, failure).find("DatabaseBusy") != String::npos &&
              debug_string(owner, input).find("DatabaseBusy") != String::npos &&
              failure.source().to_string(owner) == raw.to_string(owner) &&
              input.source().to_string(owner) == raw.to_string(owner),
          "source and diagnostics preserve separate category and extended-code responsibilities");
}

static void scoped_payloads() {
    GlobalContext global;
    std::array<Ref<SqliteDriverError>, 25> retained;
    std::array<Ref<String>, 25> diagnostics;
    std::array<Ref<MutationExecutorError>, 25> wrappers;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        String long_text(8192, 's', owner);
        Vector<std::uint8_t> bytes(8192, 'n', owner);
        bytes[4095] = 0;
        auto nul = SqliteNulError::from_bytes(owner, bytes);
        auto utf8 = SqliteUtf8Error::validate(std::string_view("\xff", 1));
        String path_text(long_text, owner);
        path_text += static_cast<char>(0xff);
        std::array errors{
            SqliteDriverError::native_failure(owner, 1555, long_text),
            SqliteDriverError::sql_input(owner, 1, long_text, long_text, -1),
            SqliteDriverError::invalid_parameter_count(owner, 1, 0),
            SqliteDriverError::multiple_statement(owner),
            SqliteDriverError::execute_returned_results(owner),
            SqliteDriverError::invalid_column_index(owner, UINT32_MAX),
            SqliteDriverError::invalid_column_type(owner, 1, long_text, SqliteValueType::Blob),
            SqliteDriverError::utf8_error(owner, 1, *utf8),
            SqliteDriverError::nul_error(owner, std::move(*nul)),
            SqliteDriverError::invalid_path(
                owner, SqlitePath(std::allocator_arg, Allocator(owner), path_text)),
            SqliteDriverError::sqlite_single_threaded_mode(owner),
            SqliteDriverError::integral_value_out_of_range(owner, 1, INT64_MIN),
            SqliteDriverError::invalid_parameter_name(owner, long_text),
            SqliteDriverError::query_returned_no_rows(owner),
            SqliteDriverError::query_returned_more_than_one_row(owner),
            SqliteDriverError::invalid_column_name(owner, long_text),
            SqliteDriverError::statement_changed_rows(owner, UINT32_MAX),
            SqliteDriverError::invalid_function_parameter_type(owner, 1, SqliteValueType::Blob),
            SqliteDriverError::invalid_query(owner),
            SqliteDriverError::unwinding_panic(owner),
            SqliteDriverError::get_aux_wrong_type(owner),
            SqliteDriverError::invalid_database_index(owner, UINT32_MAX),
            SqliteDriverError::from_sql_conversion_failure(
                owner,
                1,
                SqliteValueType::Text,
                SqliteBoxedError::make<SqliteUtf8Error>(owner, *utf8)),
            SqliteDriverError::to_sql_conversion_failure(
                owner, SqliteBoxedError::make<SqliteUtf8Error>(owner, *utf8)),
            SqliteDriverError::user_function_error(
                owner, SqliteBoxedError::make<SqliteUtf8Error>(owner, *utf8))};
        for (std::size_t index = 0; index < errors.size(); ++index) {
            retained[index] = user->make_global<SqliteDriverError>(errors[index]);
            diagnostics[index] = user->make_global<String>(debug_string(owner, errors[index]));
            auto wrapper = MutationExecutorError::sqlite(
                owner, SqliteDriverError(std::allocator_arg, Allocator(owner), errors[index]));
            wrappers[index] = user->make_global<MutationExecutorError>(wrapper);
        }
        auto absent = SqliteDriverError::native_failure(owner, 5, {});
        auto empty = SqliteDriverError::native_failure(owner, 5, "");
        check(absent != empty && empty.to_string(owner).empty(),
              "missing and present-empty native messages retain separate source payloads");
        check(!absent.to_string(owner).empty(),
              "absent native messages use the structured native error Display");
        auto moved = user->make_global<SqliteDriverError>(std::move(errors[1]));
        check(user->get(user->rebind_global(moved))
                      .sql_input_error()
                      .sql.get_allocator()
                      .resource() == global.memory_resource(),
              "foreign move explicitly copies every SQL input field into its destination");
    }
    check(global.live_user_bytes() == 0, "retained structured errors keep no source User blocks");
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    for (std::size_t index = 0; index < retained.size(); ++index) {
        const auto& error = user->get(user->rebind_global(retained[index]));
        const auto& wrapper = user->get(user->rebind_global(wrappers[index]));
        check(error.memory_resource() == global.memory_resource() &&
                  static_cast<std::size_t>(error.kind()) == index &&
                  debug_string(owner, error) == user->get(user->rebind_global(diagnostics[index])),
              "structured driver payload and diagnostics survive actual User deletion");
        check(
            wrapper.source() == nullptr &&
                wrapper.driver_error().memory_resource() == global.memory_resource() &&
                debug_string(owner, wrapper.driver_error()) == debug_string(owner, error) &&
                wrapper.to_string(owner) == error.to_string(owner),
            "every wrapped driver path retains its structure and independent provider cause after User deletion");
        SqliteDriverError copied(std::allocator_arg, Allocator(owner), error);
        check((copied == error) == (index != 3 && index < 22),
              "driver PartialEq preserves the original non-reflexive MultipleStatement case");
        check(
            (error.source() != nullptr) == (index < 2 || index == 7 || index == 8 || index >= 22),
            "native, UTF-8 and NUL sources stay distinct from the provider wrapper's empty cause");
        if (index == 1) {
            check(
                error.sql_input_error().message.get_allocator().resource() ==
                        global.memory_resource() &&
                    error.sql_input_error().sql.get_allocator().resource() ==
                        global.memory_resource() &&
                    error.sql_input_error().offset == -1 && !error.sqlite_error(),
                "SQL input retains both text fields and signed offset without masquerading as SqliteFailure");
        }
        if (index == 0) {
            check(
                error.sqlite_error_code() == SqliteNativeError::Code::ConstraintViolation &&
                    error.sqlite_extended_error_code() == 1555 &&
                    error.source()->source() == nullptr,
                "native primary category, extended code and nested default cause remain distinct");
        }
        if (index == 7) {
            const auto& payload = std::get<SqliteDriverError::Utf8Error>(error.payload());
            check(payload.index == 1 && payload.error.valid_up_to() == 0 &&
                      payload.error.error_len() == 1 && error.source()->source() == nullptr,
                  "retained UTF-8 errors expose scalar fields and the original empty nested cause");
        }
        if (index == 8) {
            const auto& payload = std::get<SqliteDriverError::NulError>(error.payload());
            check(payload.error.memory_resource() == global.memory_resource() &&
                      payload.error.nul_position() == 4095 && payload.error.bytes().size() == 8192,
                  "all original NUL bytes belong to the explicit retained scope");
            SqliteNulError copied(std::allocator_arg, Allocator(owner), payload.error);
            auto bytes = std::move(copied).into_vec();
            check(
                bytes.get_allocator().resource() == owner && bytes.size() == 8192 &&
                    bytes[4095] == 0,
                "consuming NUL recovery preserves the complete byte buffer and destination allocator");
        }
        if (index == 9) {
            const auto& payload = std::get<SqliteDriverError::InvalidPath>(error.payload());
            check(
                payload.path.memory_resource() == global.memory_resource() &&
                    payload.path.as_encoded_bytes().size() == 8193 &&
                    static_cast<unsigned char>(payload.path.as_encoded_bytes().back()) == 0xff,
                "invalid Unix path bytes survive retention without replacement or default allocation");
        }
        if (index >= 10 && index < 22) {
            check(
                error.simple_error().memory_resource() == global.memory_resource() &&
                    static_cast<std::size_t>(error.simple_error().kind()) == index - 10 &&
                    !error.sqlite_error() && !error.sqlite_error_code() &&
                    !error.sqlite_extended_error_code() && error.source() == nullptr,
                "all twelve scalar/text/unit driver tags remain distinct and have no native error or cause");
            if (index == 12 || index == 15) {
                check(error.simple_error().name().size() == 8192,
                      "named driver errors retain every byte in their explicit destination scope");
            }
        }
        if (index >= 22) {
            const auto& payload = std::get<SqliteDriverError::BoxedFailure>(error.payload());
            check(
                payload.error.memory_resource() == global.memory_resource() &&
                    error.source().to_string(owner).starts_with("invalid utf-8 sequence") &&
                    error.source().source() == nullptr && !error.sqlite_error(),
                "all three boxed driver tags retain concrete causes and destination ownership after User deletion");
        }
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        scoped_payloads();
        independent_native_fields();
        std::pmr::set_default_resource(previous);
        std::cout << "SQLite structured driver error contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
