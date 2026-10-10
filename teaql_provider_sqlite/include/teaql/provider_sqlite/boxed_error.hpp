#pragma once
#include <teaql/provider_sqlite/native_diagnostics.hpp>
#include <teaql/memory/arena.hpp>

namespace teaql {
// Context-owned erased Error payload. No shared owner or diagnostic-text proxy:
// the concrete object supplies its own Display, Debug and borrowed cause chain.
class SqliteBoxedError {
public:
    using allocator_type = Allocator;

    template <class T, class... Args>
    static SqliteBoxedError make(Memory owner, Args&&... args) {
        static_assert(std::uses_allocator_v<T, Allocator> || std::is_trivially_destructible_v<T>,
                      "Boxed error storage must be allocator-aware or scalar");
        static_assert(std::is_nothrow_destructible_v<T>);
        owner = context_resource(owner);
        auto allocator = std::pmr::polymorphic_allocator<Model<T>>(owner);
        return SqliteBoxedError(
            owner,
            allocator.template new_object<Model<T>>(std::in_place, std::forward<Args>(args)...));
    }

    SqliteBoxedError(std::allocator_arg_t, Allocator a, const SqliteBoxedError& other)
        : owner_(context_resource(a.resource())), object_(other.require_object().copy(owner_)) {}

    SqliteBoxedError(std::allocator_arg_t, Allocator a, SqliteBoxedError&& other)
        : SqliteBoxedError(detail::move_to(a.resource(), std::move(other))) {}

    SqliteBoxedError(const SqliteBoxedError&) = delete;

    SqliteBoxedError(SqliteBoxedError&& other) noexcept
        : owner_(other.owner_), object_(std::exchange(other.object_, nullptr)) {}

    SqliteBoxedError& operator=(SqliteBoxedError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    ~SqliteBoxedError() {
        reset();
    }

    Memory memory_resource() const noexcept {
        return owner_;
    }

    SqliteErrorCauseView view() const noexcept {
        return object_ ? object_->view() : SqliteErrorCauseView{};
    }

    String to_string(Memory destination) const {
        return require_object().display(destination);
    }

    void write_debug_to(DebugWriter& writer) const {
        require_object().debug(writer);
    }

    void reset() noexcept {
        if (object_) {
            auto* old = std::exchange(object_, nullptr);
            old->destroy(owner_);
        }
    }

    explicit operator bool() const noexcept {
        return object_ != nullptr;
    }

private:
    static Memory context_resource(Memory owner) {
        auto* arena = dynamic_cast<Arena*>(owner);
        if (!arena || arena->closed()) {
            throw std::invalid_argument("Boxed SQLite errors require a live Context arena");
        }
        return owner;
    }

    struct Object {
        virtual String display(Memory) const = 0;
        virtual void debug(DebugWriter&) const = 0;
        virtual SqliteErrorCauseView view() const noexcept = 0;
        virtual Object* copy(Memory) const = 0;
        virtual void destroy(Memory) noexcept = 0;
        virtual ~Object() = default;
    };

    template <class T>
    struct Model final : Object {
        using allocator_type = Allocator;

        template <class... Args>
        Model(std::allocator_arg_t, Allocator a, std::in_place_t, Args&&... args)
            : value(std::make_obj_using_allocator<T>(a, std::forward<Args>(args)...)) {}

        String display(Memory owner) const override {
            return value.to_string(owner);
        }

        void debug(DebugWriter& writer) const override {
            write_debug(writer, value);
        }

        SqliteErrorCauseView view() const noexcept override {
            return SqliteErrorCauseView::from(value);
        }

        Object* copy(Memory owner) const override {
            if constexpr (detail::domain_copyable<T>) {
                return std::pmr::polymorphic_allocator<Model<T>>(owner)
                    .template new_object<Model<T>>(std::in_place, value);
            } else {
                throw std::invalid_argument(
                    "Boxed error payload has no explicit destination-scope copy");
            }
        }

        void destroy(Memory owner) noexcept override {
            std::pmr::polymorphic_allocator<Model<T>>(owner).delete_object(this);
        }

        T value;
    };

    SqliteBoxedError(Memory owner, Object* object) noexcept : owner_(owner), object_(object) {}

    const Object& require_object() const {
        if (!object_) {
            throw std::logic_error("Consumed SQLite boxed error payload");
        }
        return *object_;
    }

    Memory owner_;
    Object* object_;
};

inline void write_debug(DebugWriter& writer, const SqliteBoxedError& error) {
    error.write_debug_to(writer);
}
}
