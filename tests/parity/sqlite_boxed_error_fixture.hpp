#pragma once
#include <teaql/provider_sqlite/boxed_error.hpp>

namespace boxed_fixture {
struct FixtureCause {
    using allocator_type = teaql::Allocator;
    teaql::String message;
    std::optional<teaql::SqliteBoxedError> child;

    FixtureCause(std::allocator_arg_t,
                 teaql::Allocator a,
                 std::string_view text,
                 std::optional<teaql::SqliteBoxedError> child)
        : message(text, a.resource()),
          child(teaql::detail::move_in(a.resource(), std::move(child))) {}

    FixtureCause(std::allocator_arg_t, teaql::Allocator a, const FixtureCause& other)
        : message(other.message, a.resource()),
          child(teaql::detail::clone_in(a.resource(), other.child)) {}

    teaql::String to_string(teaql::Memory owner) const {
        return teaql::String(message, owner);
    }

    teaql::SqliteErrorCauseView source() const noexcept {
        return child ? child->view() : teaql::SqliteErrorCauseView{};
    }
};

inline void write_debug(teaql::DebugWriter& writer, const FixtureCause& error) {
    writer.structure("FixtureCause", [&](auto& fields) {
        fields.field("message", [&](auto& out) {
            out.text(error.message);
        });
        fields.field("child", [&](auto& out) {
            if (error.child) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *error.child);
                });
            } else {
                out.raw("None");
            }
        });
    });
}
}
