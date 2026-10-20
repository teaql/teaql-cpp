#include <teaql/runtime/context.hpp>
#include <teaql/sql/detail/diagnostic.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

static ExecutionMetadata metadata(Memory r, std::size_t id, bool compiled) {
    auto value = ExecutionMetadata::unrecorded_query(r, id);
    value.started_at = SystemTime(std::chrono::seconds(7));
    value.ended_at = value.started_at;
    value.comment.emplace("statement-", r);
    *value.comment += std::to_string(id);
    value.backend = "fixture";
    String parameter("value-", r);
    parameter += std::to_string(id);
    value.params.emplace_back(Value(r, parameter));
    if (compiled) {
        value.parameterized_query.emplace("SELECT ?", r);
    }
    return value;
}

static Json describe(Memory r, const ExecutionMetadata& value) {
    auto output = detail::model_json(r);
    detail::model_put(output, "count", value.result_count ? Json(r, *value.result_count) : Json(r));
    detail::model_put(output, "comment", value.comment ? Json(r, *value.comment) : Json(r));
    detail::model_put(
        output, "sql", value.parameterized_query ? Json(r, *value.parameterized_query) : Json(r));
    detail::model_put(output, "backend", value.backend);
    Json::Array params(r);
    for (const auto& item : value.params) {
        params.emplace_back(item.to_json_value(r));
    }
    detail::model_put(output, "params", params);
    Json outcome(r);
    if (value.sql_log.execution_outcome) {
        switch (*value.sql_log.execution_outcome) {
        case SqlExecutionOutcome::Success:
            outcome = Json(r, "Success");
            break;
        case SqlExecutionOutcome::Failure:
            outcome = Json(r, "Failure");
            break;
        case SqlExecutionOutcome::Cancelled:
            outcome = Json(r, "Cancelled");
            break;
        }
    }
    detail::model_put(output, "outcome", outcome);
    auto epoch = SystemTime(std::chrono::seconds(7));
    detail::model_put(output, "started_preserved", value.started_at == epoch);
    detail::model_put(output, "ended_updated", value.ended_at > epoch);
    detail::model_put(output, "debug", value.debug_query ? Json(r, *value.debug_query) : Json(r));
    return output;
}

template <class T>
static void unwind_guard(UserContext& user_context, Ref<T> guard, bool& live) {
    struct Owner {
        UserContext& user_context;
        Ref<T> guard;
        bool& live;

        ~Owner() {
            live = false;
            user_context.destroy(guard);
        }
    };

    try {
        Owner owner{user_context, guard, live};
        throw std::runtime_error("controlled consumer failure");
    } catch (const std::runtime_error&) {
    }
}

static void unwind_context(std::unique_ptr<UserContext>& user_context) {
    struct Owner {
        std::unique_ptr<UserContext>& user_context;

        ~Owner() {
            user_context.reset();
        }
    };

    try {
        Owner owner{user_context};
        throw std::runtime_error("controlled consumer failure");
    } catch (const std::runtime_error&) {
    }
}

static Json evaluate(GlobalContext& global, const Json& input) {
    auto result_resource = global.memory_resource();
    Json::Array entries(result_resource), returned(result_resource);
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto flag = [&](std::string_view key) {
        return std::get<bool>(field(input, key).data);
    };
    std::optional<ExecutionObserver> observer;
    if (flag("observer")) {
        observer.emplace(std::allocator_arg, Allocator(r), [&](ExecutionMetadata value) {
            entries.emplace_back(describe(result_resource, value));
            if (flag("panic_sink")) {
                throw std::runtime_error("controlled observer failure");
            }
        });
    }
    bool has_recorder = false;
    bool live = true;
    if (text(field(input, "mode")) == "statement") {
        std::optional<ExecutionMetadata> value;
        if (flag("metadata")) {
            value.emplace(metadata(r, 7, flag("compiled")));
        }
        auto guard = user_context->make_ref<detail::StatementDiagnostic>(std::move(value),
                                                                         std::move(observer));
        value.reset();
        observer.reset();
        for (const auto& item : std::get<Json::Array>(field(input, "actions").data)) {
            auto action = text(item);
            if (action == "success") {
                auto success = user_context->get(guard).success();
                returned.emplace_back(success ? describe(result_resource, *success)
                                              : Json(result_resource));
            } else if (action == "fail") {
                user_context->get(guard).fail();
            } else if (action == "drop") {
                user_context->destroy(guard);
                live = false;
            } else if (action == "unwind") {
                unwind_guard(*user_context, guard, live);
            } else if (action == "close") {
                user_context.reset();
            } else if (action == "unwind_close") {
                unwind_context(user_context);
            } else {
                throw std::invalid_argument("Unknown statement diagnostic action");
            }
        }
        if (user_context && live) {
            user_context->destroy(guard);
        }
    } else {
        auto journal = user_context->make_ref<detail::FailureJournal>(std::move(observer));
        observer.reset();
        auto recorder = user_context->get(journal).recorder();
        has_recorder = bool(recorder);
        std::size_t id = 0;
        for (const auto& item : std::get<Json::Array>(field(input, "actions").data)) {
            auto action = text(item);
            if (action == "record") {
                ++id;
                if (recorder) {
                    (*recorder)(metadata(r, id, flag("compiled")));
                }
            } else if (action == "disarm") {
                user_context->get(journal).disarm();
            } else if (action == "drop") {
                user_context->destroy(journal);
                live = false;
            } else if (action == "unwind") {
                unwind_guard(*user_context, journal, live);
            } else if (action == "close" || action == "unwind_close") {
                recorder.reset();
                if (action == "close") {
                    user_context.reset();
                } else {
                    unwind_context(user_context);
                }
            } else {
                throw std::invalid_argument("Unknown journal diagnostic action");
            }
        }
        recorder.reset();
        if (user_context && live) {
            user_context->destroy(journal);
        }
    }
    user_context.reset();
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("Diagnostic guard retained User bytes after actual deletion");
    }
    auto output = detail::model_json(result_resource);
    detail::model_put(output, "entries", entries);
    detail::model_put(output, "returned", returned);
    detail::model_put(output, "recorder", has_recorder);
    return output;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto input = Json::parse(global.memory_resource(), line);
        if (!input) {
            return 2;
        }
        std::cout << evaluate(global, input.value()).dump(global.memory_resource()) << '\n';
    }
}
