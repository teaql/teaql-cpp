#pragma once
#include <teaql/runtime/context.hpp>
#include <charconv>

namespace teaql {
// Application-owned adapter for the original sample module's Info log facade.
// The message is a callback borrow; retained text needs an explicit scoped copy.
class SampleDataLogSink {
public:
    virtual ~SampleDataLogSink() = default;
    virtual Memory memory_resource() const noexcept = 0;

    virtual bool enabled(const UserContext&) const {
        return true;
    }

    virtual void info(const UserContext& user_context, std::string_view message) const = 0;
};

namespace detail {
inline void sample_log_number(String& message, std::uint64_t value) {
    char buffer[32];
    auto converted = std::to_chars(buffer, buffer + sizeof(buffer), value);
    message.append(buffer, converted.ptr);
}

template <class Format>
void sample_info(const UserContext& user_context, Format&& format) {
    auto* sink = user_context.get_resource<SampleDataLogSink>();
    if (!sink) {
        return;
    }
    CheckerCallScope context_call(checker_call_arena(user_context));
    CheckerCallScope owner_call(resource_arena(sink->memory_resource()));
    if (!sink->enabled(user_context)) {
        return;
    }
    auto message = std::invoke(std::forward<Format>(format), user_context.memory_resource());
    sink->info(user_context, message);
}
}
}
