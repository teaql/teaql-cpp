#include <teaql/runtime.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <charconv>
#include <chrono>
#include <iostream>

static Json evaluate(GlobalContext& global, const Json& input) {
    auto micros = [] {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    };
    const auto before = micros();
    auto user = global.make_user_context();
    const auto after = micros();
    const auto owner = user->memory_resource();
    const auto identifier = user->user_identifier().value();
    const auto tail = identifier.substr(identifier.rfind("@pid-") + 5);
    const auto separator = tail.find(".tid-");
    const auto pid = tail.substr(0, separator);
    const auto thread = tail.substr(separator + 5);
    const auto trace = user->trace_id();
    const auto first = trace.find('-');
    const auto second = trace.find('-', first + 1);
    const auto third = trace.find('-', second + 1);
    const auto suffix = trace.substr(third + 1);
    std::uint64_t counter{};
    const auto parsed = std::from_chars(suffix.data(), suffix.data() + suffix.size(), counter, 16);
    std::uint64_t pid_number{}, thread_number{};
    const auto parsed_pid = std::from_chars(pid.data(), pid.data() + pid.size(), pid_number);
    const auto parsed_thread =
        std::from_chars(thread.data(), thread.data() + thread.size(), thread_number);
    const auto options = user->sql_log_options();
    Json::Object output(owner);
    auto put = [&](std::string_view key, bool value) {
        output.emplace(String(key, owner), Json(owner, value));
    };
    put("trace_clock_is_microseconds",
        parsed.ec == std::errc{} && parsed.ptr == suffix.data() + suffix.size() &&
            counter >= static_cast<std::uint64_t>(before) &&
            counter <= static_cast<std::uint64_t>(after));
    put("trace_shape",
        trace.substr(0, first) == "req" && trace.substr(first + 1, second - first - 1) == pid &&
            trace.substr(second + 1, third - second - 1) == thread &&
            trace.find('-', third + 1) == trace.npos);
#if defined(_WIN32)
    put("identifier_pid",
        parsed_pid.ec == std::errc{} && pid_number == static_cast<std::uint64_t>(::_getpid()));
#else
    put("identifier_pid",
        parsed_pid.ec == std::errc{} && pid_number == static_cast<std::uint64_t>(::getpid()));
#endif
    put("identifier_thread_numeric",
        parsed_thread.ec == std::errc{} && parsed_thread.ptr == thread.data() + thread.size());
    output.emplace("timezone", Json(owner, user->timezone().value()));
    put("language_english", user->language() == Language::English);
    put("select_log", options.select);
    put("mutation_log", options.mutation);
    put("empty_sql_logs", user->sql_logs().empty());
    put("empty_graphs", user->initial_graphs().empty() && user->root_graphs().empty());
    put("empty_entities", user->all_entities().empty());
    put("missing_service", !user->has_entity_data_service("Unknown"));
    put("missing_checker", !user->has_checker("Unknown"));
    put("noop_telemetry", user->runtime_telemetry().is_noop());
    put("empty_evidence", user->last_fix_evidence().empty());
    put("missing_local", user->local("absent") == nullptr);
    std::optional<std::string_view> specified;
    if (auto* value = std::get_if<String>(&field(input, "identifier").data)) {
        specified = *value;
    }
    user->set_user_identifier_option(specified);
    user->set_timezone(text(field(input, "timezone")));
    user->set_trace_id(text(field(input, "trace")));
    output.emplace("set_identifier",
                   specified ? Json(owner, user->user_identifier().value()) : Json(owner));
    output.emplace("set_timezone", Json(owner, user->timezone().value()));
    output.emplace("set_trace", Json(owner, user->trace_id()));
    auto& module = user->make<RuntimeModule>();
    module.descriptor(EntityDescriptor(std::allocator_arg, Allocator(owner), "ContextAssembly"));
    module.language(Language::Japanese);
    user->with_module(module);
    output.emplace("module_entity", Json(owner, user->entity("ContextAssembly")->name));
    put("module_language", user->language() == Language::Japanese);
    put("module_service", user->has_entity_data_service("ContextAssembly"));
    // The response must leave this temporary User through an explicit copy.
    return Json(global.memory_resource(), Json(owner, std::move(output)));
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto input_user = global.make_user_context();
        auto input = Json::parse(input_user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid Context defaults fixture");
        }
        std::cout << evaluate(global, *input).dump(global.memory_resource()) << '\n';
    }
}
