#include "../support/context_query_fixture.hpp"
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;
using namespace context_query_fixture;

struct HelperWork {
    using allocator_type = Allocator;
    Memory resource;
    unsigned& calls;
    unsigned pending;
    bool failure;

    HelperWork(
        std::allocator_arg_t, Allocator allocator, unsigned& calls, unsigned pending, bool failure)
        : resource(allocator.resource()), calls(calls), pending(pending), failure(failure) {}

    std::optional<Result<std::uint64_t, DataServiceError<String>>> poll(WakeSignal& wake) {
        ++calls;
        if (pending) {
            --pending;
            wake.notify();
            return {};
        }
        using R = Result<std::uint64_t, DataServiceError<String>>;
        if (failure) {
            return R::err(
                resource,
                DataServiceError<String>::executor(resource, String("work failed", resource)));
        }
        return R::ok(resource, 7);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

static Json evaluate(UserContext& user_context, const Json& input) {
    auto resource = user_context.memory_resource();
    user_context.disable_sql_log();
    auto output = detail::model_json(resource);
    bool observe = text(field(input, "mode")) == "observe";
    bool noop = observe && std::get<bool>(field(input, "noop").data);
    auto& state = user_context.make<State>(Config{.noop = noop});
    auto& telemetry = user_context.make<Telemetry>(state);
    user_context.set_runtime_telemetry(user_context, telemetry);
    auto& executor = user_context.make<Mock>(state);
    auto& repository = service(user_context, executor);
    if (!observe) {
        const auto& user = field(input, "user");
        user_context.set_user_identifier_option({});
        if (!std::holds_alternative<std::monostate>(user.data)) {
            user_context.set_user_identifier(text(user));
        }
        const auto& comment = field(input, "comment");
        std::optional<std::string_view> business_comment;
        if (!std::holds_alternative<std::monostate>(comment.data)) {
            business_comment = text(comment);
        }
        Vector<TraceNode> trace(resource);
        constexpr std::string_view names[]{"Operation",
                                           "Request",
                                           "Relation",
                                           "Entity",
                                           "Provider",
                                           "Sql",
                                           "Comment",
                                           "Purpose",
                                           "AuditReason"};
        for (const auto& node : std::get<Json::Array>(field(input, "trace").data)) {
            auto kind = std::find(std::begin(names), std::end(names), text(field(node, "kind")));
            if (kind == std::end(names)) {
                throw std::logic_error("invalid fixture trace kind");
            }
            std::optional<std::uint64_t> identity;
            if (!std::holds_alternative<std::monostate>(field(node, "id").data)) {
                identity = number<std::uint64_t>(field(node, "id"));
            }
            trace.emplace_back(TraceKind(kind - std::begin(names)),
                               text(field(node, "entity")),
                               text(field(node, "comment")),
                               identity);
        }
        auto result = repository.resolve_final_comment(trace, business_comment);
        detail::model_put(output, "comment", result ? Json(resource, *result) : Json(resource));
        return output;
    }
    unsigned names = 0, work_polls = 0;
    auto pending = unsigned(std::get<std::uint64_t>(field(input, "pending").data));
    auto polls = unsigned(std::get<std::uint64_t>(field(input, "polls").data));
    bool failure = std::get<bool>(field(input, "failure").data);
    bool name_panic = std::get<bool>(field(input, "name_panic").data);
    using R = Result<std::uint64_t, DataServiceError<String>>;
    auto& work = make_operation<R, HelperWork>(resource, work_polls, pending, failure);
    auto name = [&] {
        ++names;
        if (name_panic) {
            throw std::runtime_error("name failed");
        }
        return String("custom helper", resource);
    };
    auto& observed = repository.observe("custom", name, "Item", work);
    detail::model_put(output, "names_before_poll", names);
    bool ready = false;
    try {
        for (unsigned index = 0; index < polls; ++index) {
            if (auto* result = observed.poll()) {
                ready = true;
                if (*result) {
                    detail::model_put(output, "value", result->value());
                } else {
                    detail::model_put(output, "error", result->error().to_string(resource));
                }
                break;
            }
        }
        detail::model_put(output, "cancelled", !ready);
        detail::model_put(output, "panic", false);
    } catch (const std::runtime_error&) {
        detail::model_put(output, "panic", true);
    }
    observed.cancel();
    detail::model_put(output, "names", names);
    detail::model_put(output, "work_polls", work_polls);
    Json::Array families(resource);
    for (const auto& operation : state.operations) {
        families.emplace_back(operation.family);
    }
    detail::model_put(output, "families", families);
    if (state.request) {
        throw std::logic_error("helper observation unexpectedly dispatched a query");
    }
    return output;
}

int main() {
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto resource = user_context->memory_resource();
        auto input = Json::parse(resource, line);
        std::cout << evaluate(*user_context, *input).dump(resource) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
