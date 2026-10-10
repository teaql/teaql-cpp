#include <teaql/sql/error_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

struct Error {
    using allocator_type = Allocator;
    String message;

    Error(std::allocator_arg_t, Allocator a, std::string_view text) : message(text, a.resource()) {}

    Error(std::allocator_arg_t, Allocator a, const Error& other)
        : message(other.message, a.resource()) {}

    Error(std::allocator_arg_t, Allocator a, Error&& other)
        : message(detail::move_in(a.resource(), std::move(other.message))) {}

    Error(Error&&) noexcept = default;

    Memory memory_resource() const {
        return message.get_allocator().resource();
    }

    String to_string(Memory owner) const {
        return String(message, owner);
    }
};

static void write_debug(DebugWriter& writer, const Error& error) {
    writer.tuple("Error", [&](auto& out) {
        out.text(error.message);
    });
}

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto name = text(field(input, "text"));
    auto kind = text(field(input, "kind"));
    using Envelope = SqlExecutorError<Error>;
    auto error = [&]() -> Envelope {
        if (kind == "transport") {
            return Envelope::transport(owner, Error(std::allocator_arg, Allocator(owner), name));
        }
        if (kind == "persisted") {
            return Envelope::persisted_record(owner, name);
        }
        auto code = kind == "empty"     ? SqlCompileError::Code::EmptyInList
                    : kind == "version" ? SqlCompileError::Code::InvalidRecoverVersion
                    : kind == "shape"   ? SqlCompileError::Code::InvalidSchemaShape
                                        : SqlCompileError::Code::UnknownField;
        SqlCompileError compiled(std::allocator_arg, Allocator(owner), code, name);
        compiled.version = number<std::int64_t>(field(input, "version"));
        compiled.reason = name;
        return Envelope::compile(owner, std::move(compiled));
    }();
    Json::Object output(owner);
    output.emplace("debug", formatted_debug(owner, error));
    output.emplace("display", Json(owner, error.to_string(owner)));
    output.emplace("source_none", Json(owner, error.source() == nullptr));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid SQL executor error fixture");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
