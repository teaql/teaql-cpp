#include <teaql/provider_sqlite/error.hpp>
#include <teaql/provider_sqlite/error_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;
static_assert(noexcept(std::declval<const MutationExecutorError&>().source()));

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void scoped_errors() {
    GlobalContext global;
    std::array<Ref<MutationExecutorError>, 6> retained;
    std::array<Ref<String>, 6> expected;
    std::array<Ref<String>, 6> diagnostics;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        String text(8192, 'e', owner);
        SqlCompileError compile(
            std::allocator_arg, Allocator(owner), SqlCompileError::Code::InvalidSchemaShape, text);
        compile.reason = text;
        std::array errors{MutationExecutorError::sqlite(owner, 5, text),
                          MutationExecutorError::sql_compile(owner, std::move(compile)),
                          MutationExecutorError::unsupported_value(owner, "list"),
                          MutationExecutorError::unsupported_column_type(owner, text),
                          MutationExecutorError::bind(owner, text),
                          MutationExecutorError::lock(owner, text)};
        for (unsigned index = 0; index < errors.size(); ++index) {
            check(errors[index].source() == nullptr,
                  "every source provider variant has an empty Error cause");
            retained[index] = user->make_global<MutationExecutorError>(errors[index]);
            expected[index] = user->make_global<String>(errors[index].to_string(owner));
            diagnostics[index] = user->make_global<String>(debug_string(owner, errors[index]));
        }
        check(
            errors[0].sqlite_extended_code() == 5 && errors[1].compile_error().source() == nullptr,
            "native SQLite code and structured compile error remain distinct despite the empty wrapper cause");
    }
    check(global.live_user_bytes() == 0, "retained provider errors keep no source User blocks");
    auto user = global.make_user_context();
    for (unsigned index = 0; index < retained.size(); ++index) {
        const auto& error = user->get(user->rebind_global(retained[index]));
        check(
            error.kind() == static_cast<MutationExecutorError::Kind>(index) &&
                error.source() == nullptr && error.memory_resource() == global.memory_resource() &&
                error.to_string(user->memory_resource()) ==
                    user->get(user->rebind_global(expected[index])) &&
                debug_string(user->memory_resource(), error) ==
                    user->get(user->rebind_global(diagnostics[index])),
            "all six provider error kinds retain exact fields and diagnostics after actual User deletion");
        if (index == 1) {
            check(error.compile_error().reason.get_allocator().resource() ==
                      global.memory_resource(),
                  "nested compile-error fields use the explicit destination allocator");
        }
        if (index == 0) {
            const auto& driver = error.driver_error();
            const auto& native = std::get<SqliteDriverError::NativeFailure>(driver.payload());
            check(
                driver.memory_resource() == global.memory_resource() && native.message &&
                    native.message->get_allocator().resource() == global.memory_resource() &&
                    driver.source() != nullptr && error.source() == nullptr,
                "wrapper copies structured native fields and retains its own empty cause independently");
        }
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        scoped_errors();
        std::pmr::set_default_resource(previous);
        std::cout << "SQLite provider error contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
