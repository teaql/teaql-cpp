#pragma once
#include <teaql/core/query.hpp>
#include <teaql/memory/function.hpp>
#include <teaql/memory/clone.hpp>
#include <typeinfo>

namespace teaql {
class DebugWriter;
enum class SqlParameterLogPolicy { Unknown, Plain, Masked, Credential };
enum class SqlExecutionOutcome { Success, Failure, Cancelled };
std::string_view as_str(SqlExecutionOutcome);
bool is_credential_log_name(std::string_view);

// Opaque runtime state borrowed from the explicit Context. Same-domain copies
// retain identity; explicit cross-domain copies retain logical identity while
// cloning the concrete allocator-aware state into the destination lifetime.
class SqlProjectionState {
public:
    using allocator_type = Allocator;

    explicit SqlProjectionState(Memory r) : resource_(r) {}

    SqlProjectionState(std::allocator_arg_t, Allocator a) : SqlProjectionState(a.resource()) {}

    SqlProjectionState(std::allocator_arg_t, Allocator a, const SqlProjectionState& other)
        : resource_(a.resource()), state_(other.state_), table_(other.table_),
          identity_(other.identity_) {
        if (state_ && resource_ != other.resource_) {
            state_ = table_->clone(detail::context_arena(resource_), state_);
        }
    }

    SqlProjectionState(std::allocator_arg_t, Allocator a, SqlProjectionState&& other)
        : SqlProjectionState(std::allocator_arg, a, std::as_const(other)) {}

    SqlProjectionState(const SqlProjectionState&) = default; // Same-domain borrow.
    SqlProjectionState(SqlProjectionState&&) noexcept = default;

    SqlProjectionState& operator=(SqlProjectionState&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    template <class T, class... Args>
    static SqlProjectionState make(Memory r, Args&&... args) {
        static_assert(std::uses_allocator_v<T, Allocator> || std::is_trivially_destructible_v<T>,
                      "Projection state must propagate its allocator");
        SqlProjectionState result(r);
        auto& arena = detail::context_arena(r);
        auto allocated = arena.make_with_identity<T>(std::forward<Args>(args)...);
        result.state_ = &allocated.object;
        result.identity_ = allocated.identity;
        result.table_ = &table<T>;
        return result;
    }

    template <class T>
    const T* get() const {
        return table_ && *table_->type == typeid(T) ? static_cast<const T*>(state_) : nullptr;
    }

    Memory memory_resource() const {
        return resource_;
    }

    bool operator==(const SqlProjectionState& other) const {
        return identity_ == other.identity_;
    }

    static constexpr std::string_view debug() {
        return "SqlProjectionState(<private>)";
    }

private:
    struct Table {
        const std::type_info* type;
        const void* (*clone)(Arena&, const void*);
    };
    template <class T>
    static inline const Table table{
        &typeid(T), [](Arena& arena, const void* source) -> const void* {
            if constexpr (detail::domain_copyable<T> &&
                          (std::uses_allocator_v<T, Allocator> || std::is_arithmetic_v<T> ||
                           std::is_enum_v<T> || std::is_empty_v<T>)) {
                return &arena.make<T>(*static_cast<const T*>(source));
            } else {
                throw std::invalid_argument(
                    "Cross-context projection state requires copyable allocator-aware data");
            }
        }};
    Memory resource_;
    const void* state_ = nullptr;
    const Table* table_ = nullptr;
    std::uint64_t identity_ = 0;
};
struct SqlLogContext;

class SqlIntentRedactions {
public:
    using allocator_type = Allocator;

    explicit SqlIntentRedactions(Memory r) : values_(r) {}

    SqlIntentRedactions(std::allocator_arg_t, Allocator a) : SqlIntentRedactions(a.resource()) {}

    SqlIntentRedactions(std::allocator_arg_t, Allocator a, const SqlIntentRedactions& other)
        : values_(other.values_, a.resource()) {}

    SqlIntentRedactions(std::allocator_arg_t, Allocator a, SqlIntentRedactions&& other)
        : SqlIntentRedactions(detail::move_to(a.resource(), std::move(other))) {}

    SqlIntentRedactions(const SqlIntentRedactions&) = delete;
    SqlIntentRedactions(SqlIntentRedactions&&) noexcept = default;

    SqlIntentRedactions& operator=(SqlIntentRedactions&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    static SqlParameterLogPolicy field_policy(const EntityDescriptor&, std::string_view);

    static SqlIntentRedactions from_unclassified_query(Memory, const SelectQuery&);
    static SqlIntentRedactions
    from_bindings(Memory, const SqlLogContext&, std::span<const Value>, std::string_view sql);
    void extend(const SqlIntentRedactions&);
    void capture_target_id(const Value&);
    void extend_secrets(bool allow_plaintext, Vector<String>& secrets) const;

    bool is_empty() const {
        return values_.empty();
    }

    void clear() {
        values_.clear();
    }

    String debug(Memory) const;

    Memory memory_resource() const {
        return values_.get_allocator().resource();
    }

    bool operator==(const SqlIntentRedactions&) const = default;

private:
    friend void write_debug(DebugWriter&, const SqlIntentRedactions&);

    struct Entry {
        using allocator_type = Allocator;
        String text;
        bool forced;

        Entry(std::allocator_arg_t, Allocator a, std::string_view text, bool forced)
            : text(text, a.resource()), forced(forced) {}

        Entry(std::allocator_arg_t, Allocator a, const Entry& other)
            : Entry(std::allocator_arg, a, other.text, other.forced) {}

        Entry(Entry&&) noexcept = default;
        bool operator==(const Entry&) const = default;
    };

    Vector<Entry> values_;
};

struct SqlLogContext {
    using allocator_type = Allocator;
    SqlProjectionState projection_state;
    SqlIntentRedactions intent_redactions;
    std::optional<SqlExecutionOutcome> execution_outcome;
    std::optional<String> database_kind;
    Vector<SqlParameterLogPolicy> parameter_policies;
    bool generated_sql = false;
    Vector<bool> masked_parameters;
    std::optional<String> mode, omission_reason;

    explicit SqlLogContext(Memory r)
        : projection_state(r), intent_redactions(r), parameter_policies(r), masked_parameters(r) {}

    SqlLogContext(std::allocator_arg_t, Allocator a) : SqlLogContext(a.resource()) {}

    SqlLogContext(std::allocator_arg_t, Allocator, const SqlLogContext&);

    SqlLogContext(std::allocator_arg_t, Allocator a, SqlLogContext&& other)
        : SqlLogContext(detail::move_to(a.resource(), std::move(other))) {}

    SqlLogContext(const SqlLogContext&) = delete;
    SqlLogContext(SqlLogContext&&) noexcept = default;

    SqlLogContext& operator=(SqlLogContext&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return parameter_policies.get_allocator().resource();
    }

    bool operator==(const SqlLogContext&) const = default;
};
}
