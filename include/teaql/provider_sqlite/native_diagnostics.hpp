#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/memory/scoped.hpp>

namespace teaql {
// Standard Rust error payloads used by the locked SQLite driver. UTF-8 error
// data is scalar; NUL error bytes retain their explicitly selected Context.
class SqliteUtf8Error {
public:
    static std::optional<SqliteUtf8Error> validate(std::string_view bytes) noexcept;

    std::size_t valid_up_to() const noexcept {
        return position_;
    }

    std::optional<std::size_t> error_len() const noexcept {
        if (length_) {
            return *length_;
        }
        return {};
    }

    String to_string(Memory) const;

    const std::exception* source() const noexcept {
        return nullptr;
    }

    friend bool operator==(const SqliteUtf8Error&, const SqliteUtf8Error&) = default;

private:
    SqliteUtf8Error(std::size_t position, std::optional<std::uint8_t> length)
        : position_(position), length_(length) {}

    std::size_t position_;
    std::optional<std::uint8_t> length_;
};

class SqliteNulError {
public:
    using allocator_type = Allocator;
    static std::optional<SqliteNulError> from_bytes(Memory, std::span<const std::uint8_t>);
    SqliteNulError(std::allocator_arg_t, Allocator, const SqliteNulError&);

    SqliteNulError(std::allocator_arg_t, Allocator a, SqliteNulError&& other)
        : SqliteNulError(detail::move_to(a.resource(), std::move(other))) {}

    SqliteNulError(const SqliteNulError&) = delete;
    SqliteNulError(SqliteNulError&&) noexcept = default;

    SqliteNulError& operator=(SqliteNulError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    std::size_t nul_position() const noexcept {
        return position_;
    }

    std::span<const std::uint8_t> bytes() const noexcept {
        return bytes_;
    }

    Vector<std::uint8_t> into_vec() && {
        return std::move(bytes_);
    }

    Memory memory_resource() const noexcept {
        return bytes_.get_allocator().resource();
    }

    String to_string(Memory) const;

    const std::exception* source() const noexcept {
        return nullptr;
    }

    friend bool operator==(const SqliteNulError& left, const SqliteNulError& right) {
        return left.position_ == right.position_ && left.bytes_ == right.bytes_;
    }

private:
    SqliteNulError(Memory r, std::size_t position, std::span<const std::uint8_t> bytes)
        : position_(position), bytes_(bytes.begin(), bytes.end(), r) {}

    std::size_t position_;
    Vector<std::uint8_t> bytes_;
};

// Unix PathBuf error payload: preserve native bytes, not only lossy text.
class SqlitePath {
public:
    using allocator_type = Allocator;

    SqlitePath(std::allocator_arg_t, Allocator a, std::string_view bytes)
        : bytes_(bytes, a.resource()) {}

    SqlitePath(std::allocator_arg_t, Allocator a, const SqlitePath& other)
        : bytes_(other.bytes_, a.resource()) {}

    SqlitePath(std::allocator_arg_t, Allocator a, SqlitePath&& other)
        : SqlitePath(detail::move_to(a.resource(), std::move(other))) {}

    SqlitePath(const SqlitePath&) = delete;
    SqlitePath(SqlitePath&&) noexcept = default;

    SqlitePath& operator=(SqlitePath&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    std::string_view as_encoded_bytes() const noexcept {
        return bytes_;
    }

    Memory memory_resource() const noexcept {
        return bytes_.get_allocator().resource();
    }

    String to_string_lossy(Memory) const;
    friend bool operator==(const SqlitePath&, const SqlitePath&) noexcept;

private:
    String bytes_;
};

void write_debug(DebugWriter&, const SqliteUtf8Error&);
void write_debug(DebugWriter&, const SqliteNulError&);
void write_debug(DebugWriter&, const SqlitePath&);

struct SqliteNativeError;

// A borrowed Error trait view. Copying the view never retains its payload;
// cross-domain retention must copy the complete owning driver error instead.
class SqliteErrorCauseView {
public:
    using allocator_type = Allocator;
    SqliteErrorCauseView() noexcept = default;
    SqliteErrorCauseView(const SqliteErrorCauseView&) noexcept = default;
    SqliteErrorCauseView(std::allocator_arg_t, Allocator, const SqliteErrorCauseView&) = delete;

    template <class T>
        requires(std::same_as<T, SqliteNativeError> || std::same_as<T, SqliteUtf8Error> ||
                 std::same_as<T, SqliteNulError> ||
                 requires(const T& value) {
                     { value.source() } -> std::same_as<SqliteErrorCauseView>;
                 })
    static SqliteErrorCauseView from(const T& value) noexcept {
        return SqliteErrorCauseView(&value, &table<T>);
    }

    explicit operator bool() const noexcept {
        return object_ != nullptr;
    }

    friend bool operator==(SqliteErrorCauseView value, std::nullptr_t) noexcept {
        return !value;
    }

    const SqliteErrorCauseView* operator->() const noexcept {
        return this;
    }

    String to_string(Memory r) const {
        if (!object_) {
            throw std::logic_error("Empty SQLite cause view");
        }
        return table_->display(r, object_);
    }

    SqliteErrorCauseView source() const noexcept {
        if (object_) {
            return table_->source(object_);
        }
        return {};
    }

    void write_debug_to(DebugWriter& writer) const {
        if (!object_) {
            throw std::logic_error("Empty SQLite cause view");
        }
        table_->debug(writer, object_);
    }

private:
    struct Table {
        String (*display)(Memory, const void*);
        void (*debug)(DebugWriter&, const void*);
        SqliteErrorCauseView (*source)(const void*) noexcept;
    };
    template <class T>
    static const Table table;

    SqliteErrorCauseView(const void* object, const Table* table) noexcept
        : object_(object), table_(table) {}

    const void* object_ = nullptr;
    const Table* table_ = nullptr;
};

template <class T>
inline const SqliteErrorCauseView::Table SqliteErrorCauseView::table{
    [](Memory owner, const void* value) {
        return static_cast<const T*>(value)->to_string(owner);
    },
    [](DebugWriter& writer, const void* value) {
        write_debug(writer, *static_cast<const T*>(value));
    },
    [](const void* value) noexcept -> SqliteErrorCauseView {
        if constexpr (requires(const T& error) {
                          { error.source() } -> std::same_as<SqliteErrorCauseView>;
                      }) {
            static_assert(noexcept(std::declval<const T&>().source()),
                          "Error cause access must not throw");
            return static_cast<const T*>(value)->source();
        } else {
            return {};
        }
    }};

inline void write_debug(DebugWriter& writer, SqliteErrorCauseView view) {
    view.write_debug_to(writer);
}
}
