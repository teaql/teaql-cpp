#include "../../tests/support/move_only_error_fixture.hpp"
#include <teaql/runtime/error_debug.hpp>
#include <teaql/sql/error_debug.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

using RuntimeEnvelope = DataServiceError<MoveOnlyError>;
using SqlEnvelope = SqlExecutorError<MoveOnlyError>;
using NestedEnvelope = DataServiceError<SqlEnvelope>;
using Failure = Result<std::uint64_t, NestedEnvelope>;

static_assert(!detail::domain_copyable<MoveOnlyError>);
static_assert(!detail::domain_copyable<RuntimeEnvelope>);
static_assert(!detail::domain_copyable<SqlEnvelope>);
static_assert(!detail::domain_copyable<NestedEnvelope>);
static_assert(!detail::domain_copyable<Failure>);
static_assert(detail::domain_copyable<DataServiceError<String>>);
static_assert(detail::domain_copyable<SqlExecutorError<String>>);
static_assert(detail::domain_copyable<Result<std::uint64_t, DataServiceError<String>>>);

template <class T, class Inspect>
static void reject_foreign_move(Memory other, T& source, Inspect&& inspect) {
    bool rejected = false;
    try {
        (void)detail::move_in(other, std::move(source));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && inspect(source), "foreign retention rejects without consuming source error");
}

static void scoped_move_only_errors() {
    unsigned destroyed = 0;
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto foreign = global.make_user_context();
    auto other = foreign->memory_resource();
    auto raw = user->make_ref<RuntimeEnvelope>(RuntimeEnvelope::executor(
        owner,
        MoveOnlyError(std::allocator_arg, Allocator(owner), String(4096, 'r', owner), destroyed)));
    auto sql = user->make_ref<SqlEnvelope>(SqlEnvelope::transport(
        owner,
        MoveOnlyError(std::allocator_arg, Allocator(owner), "native transport error", destroyed)));
    check(user->get(raw).executor_error().message.size() == 4096 &&
              user->get(sql).to_string(owner) == "Transport error: native transport error" &&
              destroyed == 0,
          "both Context-allocated error wrappers accept noncopyable native errors");
    {
        String diagnostic(owner);
        DebugWriter writer(diagnostic, {});
        write_debug(writer, user->get(sql), [](auto& out, const MoveOnlyError& error) {
            out.tuple("Native", [&](auto& inner) {
                inner.text(error.message);
            });
        });
        check(diagnostic == "Transport(Native(\"native transport error\"))" && destroyed == 0 &&
                  user->get(sql).transport_error().active,
              "custom structural diagnostics borrow a move-only payload without consuming it");
        auto text_error = SqlExecutorError<String>::transport(owner, String("quoted\"", owner));
        check(debug_string(owner, text_error) == "Transport(\"quoted\\\"\")",
              "default executor-error diagnostics use the payload's structural writer");
        auto persisted = SqlExecutorError<String>::persisted_record(owner, "missing");
        check(debug_string(owner, persisted) == "PersistedRecord(\"missing\")",
              "persisted-record diagnostics retain the independent error variant");
    }
    ContextError context_error(
        std::allocator_arg, Allocator(owner), ContextError::Kind::MissingResource, "missing");
    auto runtime_error = RuntimeError::schema(owner, "missing schema");
    check(!context_error.source() && !runtime_error.source() && !user->get(raw).source() &&
              !user->get(sql).source(),
          "all source Error implementations retain the default absent cause chain");
    reject_foreign_move(other, user->get(raw), [](const auto& value) {
        return value.kind() == RuntimeEnvelope::Kind::Executor &&
               value.executor_error().message.size() == 4096;
    });
    reject_foreign_move(other, user->get(sql), [](const auto& value) {
        return value.kind() == SqlEnvelope::Kind::Transport &&
               value.transport_error().message == "native transport error";
    });
    auto nested =
        user->make_ref<NestedEnvelope>(NestedEnvelope::executor(owner, std::move(user->get(sql))));
    auto failure = user->make_ref<Failure>(Failure::err(owner, std::move(user->get(nested))));
    check(user->get(failure).is_err() &&
              user->get(failure).error().executor_error().transport_error().message ==
                  "native transport error",
          "move-only error retains all classification through SQL, runtime and Result layers");
    reject_foreign_move(other, user->get(failure), [](const auto& value) {
        return value.is_err() && value.error().executor_error().transport_error().active;
    });
    auto target = user->make_ref<RuntimeEnvelope>(
        RuntimeEnvelope::runtime(owner, RuntimeError::schema(owner, "initial")));
    user->get(target) = std::move(user->get(raw));
    check(user->get(target).executor_error().message.size() == 4096 && destroyed == 0,
          "same-domain move assignment transfers the complete native error");
    {
        auto foreign_value = RuntimeEnvelope::executor(
            other, MoveOnlyError(std::allocator_arg, Allocator(other), "foreign error", destroyed));
        bool rejected = false;
        try {
            user->get(target) = std::move(foreign_value);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected && user->get(target).executor_error().message.size() == 4096 &&
                  foreign_value.executor_error().message == "foreign error" && destroyed == 0,
              "failed foreign assignment preserves both source and destination");
    }
    check(destroyed == 1, "rejected foreign native error is destroyed exactly once");
    user->close();
    check(destroyed == 3 && user->allocation_stats().live_bytes == 0,
          "User close destroys transferred native errors exactly once before freeing blocks");
    user.reset();
    foreign.reset();
    check(global.live_user_bytes() == 0, "nested move-only error results retain no User memory");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    struct Restore {
        Memory previous;

        ~Restore() {
            std::pmr::set_default_resource(previous);
        }
    } restore{previous};

    try {
        scoped_move_only_errors();
        std::cout << "Generic move-only error contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
