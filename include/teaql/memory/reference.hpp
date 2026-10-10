#pragma once
#include <teaql/memory/limits.hpp>
#include <exception>
#include <source_location>
#include <span>
#include <string_view>

namespace teaql {
class UserContext;
class GlobalContext;

// A process-local value, not an owner, address or persistent entity/business ID.
// Resolution always requires a live Context.
template <class T>
class Ref {
public:
    Ref() = default;

    explicit Ref(std::uint64_t id) noexcept : id_(id) {}

    std::uint64_t id() const noexcept {
        return id_;
    }

    friend bool operator==(Ref, Ref) = default;

private:
    std::uint64_t id_ = 0;
};

static_assert(sizeof(Ref<int>) == sizeof(std::uint64_t));

class ReferenceError final : public std::exception {
public:
    enum class Kind { Invalid, ForeignRuntime, ForeignContext, Closed, Dead, Type, NotGlobal };

    ReferenceError(Kind kind,
                   std::uint64_t id,
                   std::source_location access,
                   std::source_location closed = {}) noexcept
        : kind_(kind), id_(id), access_(access), closed_(closed) {}

    Kind kind() const noexcept {
        return kind_;
    }

    std::uint64_t id() const noexcept {
        return id_;
    }

    std::source_location access_location() const noexcept {
        return access_;
    }

    std::source_location close_location() const noexcept {
        return closed_;
    }

    const char* what() const noexcept override {
        switch (kind_) {
        case Kind::Invalid:
            return "Invalid or unissued Context reference";
        case Kind::ForeignRuntime:
            return "Reference belongs to another application";
        case Kind::ForeignContext:
            return "Reference belongs to another UserContext";
        case Kind::Closed:
            return "Reference access Context has closed";
        case Kind::Dead:
            return "Referenced object is not live";
        case Kind::Type:
            return "Reference object type does not match";
        case Kind::NotGlobal:
            return "Only Global references can be rebound";
        }
        return "Invalid Context reference";
    }

private:
    Kind kind_;
    std::uint64_t id_;
    std::source_location access_;
    std::source_location closed_;
};

namespace detail {
// 23 runtime bits, 20 access-Context bits, 1 owner bit, 20 object-slot bits.
inline constexpr unsigned owner_shift = 20;
inline constexpr unsigned context_shift = 21;
inline constexpr unsigned runtime_shift = 41;
inline constexpr std::uint32_t runtime_identity_limit = (1u << 23) - 1;

inline std::uint64_t reference_id(std::uint32_t runtime,
                                  std::uint32_t context,
                                  bool global,
                                  std::uint32_t slot = 0) noexcept {
    return (std::uint64_t(runtime) << runtime_shift) | (std::uint64_t(context) << context_shift) |
           (std::uint64_t(global) << owner_shift) | slot;
}

inline std::uint32_t reference_runtime(std::uint64_t id) noexcept {
    return static_cast<std::uint32_t>(id >> runtime_shift);
}

inline std::uint32_t reference_context(std::uint64_t id) noexcept {
    return static_cast<std::uint32_t>((id >> context_shift) & context_identity_limit);
}

inline bool reference_global(std::uint64_t id) noexcept {
    return (id & (std::uint64_t{1} << owner_shift)) != 0;
}

inline std::uint32_t reference_slot(std::uint64_t id) noexcept {
    return static_cast<std::uint32_t>(id & object_slot_limit);
}

struct ContextState {
    enum class Phase { Constructing, Open, Closing, Closed };
    Phase phase = Phase::Constructing;
    bool published = false;
    std::source_location closed_at;
};

std::uint32_t issue_runtime_identity();
}

// Immutable payload views use a Context scope token, not another object ID.
class StringRef {
public:
    StringRef() = default;

    std::size_t size() const noexcept {
        return size_;
    }

    std::uint64_t scope_id() const noexcept {
        return scope_;
    }

private:
    friend class UserContext;

    StringRef(std::uint64_t scope, std::string_view value) noexcept
        : scope_(scope), data_(value.data()), size_(value.size()) {}

    std::uint64_t scope_ = 0;
    const char* data_ = nullptr;
    std::size_t size_ = 0;
};

class BytesRef {
public:
    BytesRef() = default;

    std::size_t size() const noexcept {
        return size_;
    }

    std::uint64_t scope_id() const noexcept {
        return scope_;
    }

private:
    friend class UserContext;

    BytesRef(std::uint64_t scope, std::span<const std::byte> value) noexcept
        : scope_(scope), data_(value.data()), size_(value.size()) {}

    std::uint64_t scope_ = 0;
    const std::byte* data_ = nullptr;
    std::size_t size_ = 0;
};

// Immediate construction rejects pointers to temporary runtime buffers.
class StaticString {
public:
    template <std::size_t N>
    consteval StaticString(const char (&text)[N]) : data_(text), size_(N - 1) {}

    std::string_view view() const noexcept {
        return {data_, size_};
    }

private:
    const char* data_;
    std::size_t size_;
};
}
