#pragma once
#include <teaql/core/result.hpp>
#include <teaql/core/detail/text.hpp>
#include <exception>
#include <stdexcept>

namespace teaql {
enum class RequestKind { Query, Mutation };

struct RequestIntentError {
    using allocator_type = Allocator;
    RequestKind request_kind;
    String field;

    RequestIntentError(std::allocator_arg_t, Allocator a, RequestKind kind, std::string_view field)
        : request_kind(kind), field(field, a.resource()) {}

    RequestIntentError(std::allocator_arg_t, Allocator a, const RequestIntentError& other)
        : RequestIntentError(std::allocator_arg, a, other.request_kind, other.field) {}

    RequestIntentError(const RequestIntentError&) = delete;
    RequestIntentError(RequestIntentError&&) noexcept = default;

    RequestIntentError(std::allocator_arg_t, Allocator a, RequestIntentError&& other)
        : request_kind(other.request_kind), field(std::move(other.field), a.resource()) {}

    std::string_view code() const noexcept {
        return field == "purpose" ? "QUERY_PURPOSE_REQUIRED" : "REQUEST_COMMENT_REQUIRED";
    }

    Memory memory_resource() const noexcept {
        return field.get_allocator().resource();
    }

    String to_string(Memory destination) const {
        String text(code(), destination);
        text += request_kind == RequestKind::Query ? ": Query" : ": Mutation";
        text += " Request requires a non-blank ";
        text += field;
        text += "; supply it at the request entry point";
        return text;
    }

    const std::exception* source() const noexcept {
        return nullptr;
    }

    bool operator==(const RequestIntentError& other) const {
        return request_kind == other.request_kind && field == other.field;
    }
};

class QueryIntent {
public:
    using allocator_type = Allocator;

    QueryIntent(std::allocator_arg_t, Allocator a, const QueryIntent& other)
        : QueryIntent(a.resource(), other.comment_, other.purpose_) {
        require_valid();
    }

    QueryIntent(std::allocator_arg_t, Allocator a, QueryIntent&& other)
        : comment_(std::move(other.comment_), a.resource()),
          purpose_(std::move(other.purpose_), a.resource()) {
        require_valid();
    }

    QueryIntent(const QueryIntent&) = delete;
    QueryIntent(QueryIntent&&) noexcept = default;

    static Result<QueryIntent, RequestIntentError>
    create(Memory resource, std::string_view comment, std::string_view purpose) {
        return from_optional(resource, comment, purpose);
    }

    static Result<QueryIntent, RequestIntentError>
    from_optional(Memory resource,
                  std::optional<std::string_view> comment,
                  std::optional<std::string_view> purpose) {
        using Result = teaql::Result<QueryIntent, RequestIntentError>;
        auto valid = [](auto value) {
            if (!value) {
                return false;
            }
            auto text = *value;
            detail::trim(text);
            return !text.empty();
        };
        if (!valid(comment)) {
            return Result::err(
                resource,
                RequestIntentError(
                    std::allocator_arg, Allocator(resource), RequestKind::Query, "comment"));
        }
        if (!valid(purpose)) {
            return Result::err(
                resource,
                RequestIntentError(
                    std::allocator_arg, Allocator(resource), RequestKind::Query, "purpose"));
        }
        return Result::ok(resource, QueryIntent(resource, *comment, *purpose));
    }

    std::optional<RequestIntentError> validation_error(Memory destination) const {
        auto comment = std::string_view(comment_);
        detail::trim(comment);
        if (comment.empty()) {
            return RequestIntentError(
                std::allocator_arg, Allocator(destination), RequestKind::Query, "comment");
        }
        auto purpose = std::string_view(purpose_);
        detail::trim(purpose);
        if (purpose.empty()) {
            return RequestIntentError(
                std::allocator_arg, Allocator(destination), RequestKind::Query, "purpose");
        }
        return std::nullopt;
    }

    void require_valid() const {
        if (auto error = validation_error(memory_resource())) {
            throw std::invalid_argument(std::string(error->to_string(memory_resource())));
        }
    }

    std::string_view comment() const noexcept {
        return comment_;
    }

    std::string_view purpose() const noexcept {
        return purpose_;
    }

    Memory memory_resource() const noexcept {
        return comment_.get_allocator().resource();
    }

    bool operator==(const QueryIntent& other) const {
        return comment_ == other.comment_ && purpose_ == other.purpose_;
    }

private:
    QueryIntent(Memory resource, std::string_view comment, std::string_view purpose)
        : comment_(comment, resource), purpose_(purpose, resource) {}

    String comment_, purpose_;
};

class MutationIntent {
public:
    using allocator_type = Allocator;

    MutationIntent(std::allocator_arg_t, Allocator a, const MutationIntent& other)
        : MutationIntent(a.resource(), other.comment_) {
        require_valid();
    }

    MutationIntent(std::allocator_arg_t, Allocator a, MutationIntent&& other)
        : comment_(std::move(other.comment_), a.resource()) {
        require_valid();
    }

    MutationIntent(const MutationIntent&) = delete;
    MutationIntent(MutationIntent&&) noexcept = default;

    static Result<MutationIntent, RequestIntentError> create(Memory resource,
                                                             std::string_view comment) {
        return from_optional(resource, comment);
    }

    static Result<MutationIntent, RequestIntentError>
    from_optional(Memory resource, std::optional<std::string_view> comment) {
        using Result = teaql::Result<MutationIntent, RequestIntentError>;
        auto text = comment.value_or(std::string_view{});
        detail::trim(text);
        if (text.empty()) {
            return Result::err(
                resource,
                RequestIntentError(
                    std::allocator_arg, Allocator(resource), RequestKind::Mutation, "comment"));
        }
        return Result::ok(resource, MutationIntent(resource, *comment));
    }

    std::optional<RequestIntentError> validation_error(Memory destination) const {
        auto comment = std::string_view(comment_);
        detail::trim(comment);
        if (comment.empty()) {
            return RequestIntentError(
                std::allocator_arg, Allocator(destination), RequestKind::Mutation, "comment");
        }
        return std::nullopt;
    }

    void require_valid() const {
        if (auto error = validation_error(memory_resource())) {
            throw std::invalid_argument(std::string(error->to_string(memory_resource())));
        }
    }

    std::string_view comment() const noexcept {
        return comment_;
    }

    Memory memory_resource() const noexcept {
        return comment_.get_allocator().resource();
    }

    bool operator==(const MutationIntent& other) const {
        return comment_ == other.comment_;
    }

private:
    MutationIntent(Memory resource, std::string_view comment) : comment_(comment, resource) {}

    String comment_;
};

}
