#include <teaql/provider_sqlite/simple_error.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;
using Error = detail::SqliteSimpleError;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void retained_alternatives() {
    GlobalContext global;
    std::array<Ref<Error>, 12> retained;
    std::array<Ref<String>, 12> diagnostics;
    std::array<Ref<String>, 12> displays;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        String text(8192, 'n', owner);
        std::array errors{Error::unit(owner, Error::Kind::SqliteSingleThreadedMode),
                          Error::integral_range(owner, 1, INT64_MIN),
                          Error::named(owner, Error::Kind::InvalidParameterName, text),
                          Error::unit(owner, Error::Kind::QueryReturnedNoRows),
                          Error::unit(owner, Error::Kind::QueryReturnedMoreThanOneRow),
                          Error::named(owner, Error::Kind::InvalidColumnName, text),
                          Error::indexed(owner, Error::Kind::StatementChangedRows, UINT32_MAX),
                          Error::function_type(owner, 1, SqliteValueType::Blob),
                          Error::unit(owner, Error::Kind::InvalidQuery),
                          Error::unit(owner, Error::Kind::UnwindingPanic),
                          Error::unit(owner, Error::Kind::GetAuxWrongType),
                          Error::indexed(owner, Error::Kind::InvalidDatabaseIndex, UINT32_MAX)};
        for (std::size_t index = 0; index < errors.size(); ++index) {
            retained[index] = user->make_global<Error>(errors[index]);
            diagnostics[index] = user->make_global<String>(debug_string(owner, errors[index]));
            displays[index] = user->make_global<String>(errors[index].to_string(owner));
        }
        auto unspecified = Error::integral_range(owner, SIZE_MAX, -1);
        check(unspecified.to_string(owner) == "Integer -1 out of range",
              "native unknown-column sentinel omits the index without changing its payload");
    }
    check(global.live_user_bytes() == 0, "retained alternatives keep no source User blocks");
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    for (std::size_t index = 0; index < retained.size(); ++index) {
        const auto& error = user->get(user->rebind_global(retained[index]));
        Error copied(std::allocator_arg, Allocator(owner), error);
        check(
            static_cast<std::size_t>(error.kind()) == index && error == copied &&
                error.memory_resource() == global.memory_resource() && error.source() == nullptr &&
                debug_string(owner, error) == user->get(user->rebind_global(diagnostics[index])) &&
                error.to_string(owner) == user->get(user->rebind_global(displays[index])),
            "every scalar/text/unit tag retains its active fields, diagnostics and scoped storage");
        if (index == 2 || index == 5) {
            check(error.name().size() == 8192 && copied.memory_resource() == owner,
                  "long error names are copied explicitly into each destination scope");
        }
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        retained_alternatives();
        std::pmr::set_default_resource(previous);
        std::cout << "SQLite scalar/text/unit error scope contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
