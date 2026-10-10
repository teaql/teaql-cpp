#include <teaql/provider_sqlite/boxed_error.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct CountedError {
    using allocator_type = Allocator;
    String message;
    std::optional<SqliteBoxedError> cause;
    unsigned* destroyed;

    CountedError(std::allocator_arg_t,
                 Allocator a,
                 std::string_view text,
                 unsigned& count,
                 std::optional<SqliteBoxedError> child = {})
        : message(text, a.resource()), cause(detail::move_in(a.resource(), std::move(child))),
          destroyed(&count) {}

    CountedError(std::allocator_arg_t, Allocator a, const CountedError& other)
        : message(other.message, a.resource()), cause(detail::clone_in(a.resource(), other.cause)),
          destroyed(other.destroyed) {}

    ~CountedError() {
        ++*destroyed;
    }

    Memory memory_resource() const noexcept {
        return message.get_allocator().resource();
    }

    String to_string(Memory owner) const {
        return String(message, owner);
    }

    SqliteErrorCauseView source() const noexcept {
        return cause ? cause->view() : SqliteErrorCauseView{};
    }
};

static void write_debug(DebugWriter& writer, const CountedError& error) {
    writer.structure("CountedError", [&](auto& fields) {
        fields.field("message", [&](auto& out) {
            out.text(error.message);
        });
    });
}

struct NoncopyError {
    using allocator_type = Allocator;
    String message;
    unsigned* destroyed;

    NoncopyError(std::allocator_arg_t, Allocator a, std::string_view text, unsigned& count)
        : message(text, a.resource()), destroyed(&count) {}

    NoncopyError(const NoncopyError&) = delete;

    NoncopyError(NoncopyError&& other) noexcept
        : message(std::move(other.message)), destroyed(std::exchange(other.destroyed, nullptr)) {}

    ~NoncopyError() {
        if (destroyed) {
            ++*destroyed;
        }
    }

    String to_string(Memory owner) const {
        return String(message, owner);
    }

    SqliteErrorCauseView source() const noexcept {
        return {};
    }
};

static void write_debug(DebugWriter& writer, const NoncopyError& error) {
    writer.tuple("NoncopyError", [&](auto& out) {
        out.text(error.message);
    });
}

static void error_ownership() {
    unsigned destroyed = 0;
    unsigned noncopy_destroyed = 0;
    {
        GlobalContext global;
        Ref<SqliteBoxedError> retained;
        {
            auto user = global.make_user_context();
            auto owner = user->memory_resource();
            String text(8192, 'e', owner);
            auto leaf = SqliteBoxedError::make<CountedError>(owner, "leaf", destroyed);
            std::optional<SqliteBoxedError> leaf_owner(std::move(leaf));
            auto middle = SqliteBoxedError::make<CountedError>(
                owner, "middle", destroyed, std::move(leaf_owner));
            std::optional<SqliteBoxedError> middle_owner(std::move(middle));
            auto top = SqliteBoxedError::make<CountedError>(
                owner, text, destroyed, std::move(middle_owner));
            retained = user->make_global<SqliteBoxedError>(top);
            check(top.view()->source()->source()->to_string(owner) == "leaf" &&
                      top.view()->source()->source()->source() == nullptr,
                  "erasure preserves the actual nested Error chain");
            auto single = SqliteBoxedError::make<NoncopyError>(owner, text, noncopy_destroyed);
            bool rejected = false;
            try {
                user->make_global<SqliteBoxedError>(single);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            check(rejected && single.to_string(owner).size() == 8192 && noncopy_destroyed == 0,
                  "unsupported retention rejects without consuming a noncopy payload");
            auto moved = SqliteBoxedError(std::move(single));
            check(!single && moved.to_string(owner).size() == 8192,
                  "same-scope moving transfers the concrete object without sharing ownership");
            moved.reset();
            moved.reset();
            check(noncopy_destroyed == 1, "eager reset destroys a noncopy resource exactly once");
        }
        check(global.live_user_bytes() == 0 && destroyed == 3 && noncopy_destroyed == 1,
              "actual User deletion destroys the source chain before returning blocks");
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& error = user->get(user->rebind_global(retained));
        check(error.memory_resource() == global.memory_resource() &&
                  error.to_string(owner).size() == 8192 &&
                  error.view()->source()->to_string(owner) == "middle" &&
                  error.view()->source()->source()->to_string(owner) == "leaf",
              "Global retention deep-copies every concrete cause object");
        auto diagnostic = debug_string(owner, error);
        check(diagnostic.starts_with("CountedError { message: \"") && diagnostic.size() > 8192,
              "boxed Debug calls the original concrete formatter");
        error.reset();
        check(destroyed == 6,
              "retained chain cleanup is eager and recursive in its owning Context");
    }
    check(destroyed == 6 && noncopy_destroyed == 1,
          "Context close does not repeat destruction of reset boxes");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        error_ownership();
        std::pmr::set_default_resource(previous);
        std::cout << "SQLite boxed Error ownership contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
