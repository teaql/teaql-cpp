#include <teaql/provider_sqlite/error.hpp>

namespace teaql {
MutationExecutorError::MutationExecutorError(std::allocator_arg_t,
                                             Allocator a,
                                             const MutationExecutorError& other)
    : kind_(other.kind_), message_(other.message_, a.resource()),
      compile_(detail::clone_in(a.resource(), other.compile_)), sqlite_code_(other.sqlite_code_) {}

MutationExecutorError MutationExecutorError::sqlite(Memory r, int code, std::string_view message) {
    MutationExecutorError error(r, Kind::Sqlite, message);
    error.sqlite_code_ = code;
    return error;
}

MutationExecutorError MutationExecutorError::sql_compile(Memory r, SqlCompileError error) {
    MutationExecutorError result(r, Kind::SqlCompile, {});
    result.compile_.emplace(detail::move_in(r, std::move(error)));
    return result;
}

MutationExecutorError MutationExecutorError::unsupported_value(Memory r, std::string_view value) {
    return {r, Kind::UnsupportedValue, value};
}

MutationExecutorError MutationExecutorError::unsupported_column_type(Memory r,
                                                                     std::string_view value) {
    return {r, Kind::UnsupportedColumnType, value};
}

MutationExecutorError MutationExecutorError::bind(Memory r, std::string_view value) {
    return {r, Kind::Bind, value};
}

MutationExecutorError MutationExecutorError::lock(Memory r, std::string_view value) {
    return {r, Kind::Lock, value};
}

String MutationExecutorError::to_string(Memory r) const {
    std::string_view prefix;
    switch (kind_) {
    case Kind::SqlCompile:
        return compile_error().to_string(r);
    case Kind::Sqlite:
        break;
    case Kind::UnsupportedValue:
        prefix = "unsupported rusqlite bind value for mutation executor: ";
        break;
    case Kind::UnsupportedColumnType:
        prefix = "unsupported rusqlite column type for record decoding: ";
        break;
    case Kind::Bind:
        prefix = "rusqlite bind error: ";
        break;
    case Kind::Lock:
        prefix = "rusqlite connection lock error: ";
        break;
    }
    String result(prefix, r);
    result += message_;
    return result;
}
}
