#include "../support/runtime_mutation_fixture.hpp"
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

struct Cache final : AggregationCacheBackend {
    mutation_fixture::State& state;

    explicit Cache(mutation_fixture::State& state) : state(state) {}

    Memory memory_resource() const noexcept override {
        return state.resource;
    }

    std::string_view name_space() const noexcept override {
        return "custom";
    }

    std::optional<Vector<CompactRow>> get(Memory, std::string_view, std::uint64_t) const override {
        return {};
    }

    void put(std::string_view, std::span<const CompactRow>) override {}

    void invalidate_namespace(std::string_view name) override {
        String note("cache:", state.resource);
        note += name;
        state.note(note);
    }
};

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    user_context.disable_sql_log();
    auto& state = user_context.make<mutation_fixture::State>();
    state.failure = detail::model_integer<int>(field(input, "failure"), "failure");
    for (const auto& count : std::get<Json::Array>(field(input, "counts").data)) {
        state.affected.push_back(detail::model_integer<std::uint64_t>(count, "count"));
    }
    auto& executor = user_context.make_resource<mutation_fixture::Executor>(state);
    ResourceBorrow<mutation_fixture::Executor> borrow(
        std::allocator_arg, Allocator(r), executor, detail::resource_arena(r));
    auto& service = user_context.make<detail::ContextDataService<mutation_fixture::Executor>>(
        user_context, borrow);
    if (std::get<bool>(field(input, "intent").data)) {
        auto intent = MutationIntent::create(r, "change payment");
        service.with_mutation_intent(intent.value());
    }
    auto caches = detail::model_integer<unsigned>(field(input, "caches"), "caches");
    if ((caches & 1) != 0) {
        auto& cache = user_context.make_resource<Cache>(state);
        user_context.insert_resource(user_context, static_cast<AggregationCacheBackend&>(cache));
    }
    auto& memory = user_context.make<InMemoryAggregationCache>("mem");
    constexpr std::string_view keys[]{"mem::entity:Payment::row",
                                      "mem::entity:Payments::row",
                                      "mem::entity:Other::row",
                                      "foreign::entity:Payment::row",
                                      "mem::entity:Payment"};
    for (auto key : keys) {
        memory.put(key, {});
    }
    if ((caches & 2) != 0) {
        user_context.insert_resource(user_context, memory);
    }
    Vector<std::optional<std::int64_t>> versions(r);
    for (const auto& version : std::get<Json::Array>(field(input, "versions").data)) {
        versions.emplace_back(std::holds_alternative<std::monostate>(version.data)
                                  ? std::nullopt
                                  : std::optional<std::int64_t>(
                                        detail::model_integer<std::int64_t>(version, "version")));
    }
    auto n = detail::model_integer<std::size_t>(field(input, "length"), "length");
    auto m = detail::model_integer<std::size_t>(field(input, "ids"), "ids");
    auto entry = text(field(input, "entry"));
    Value id(r, std::uint64_t(7));
    auto outcome = detail::model_json(r);
    auto collect = [&](auto& work) {
        const auto& result = work.wait();
        if (result) {
            detail::model_put(outcome, "rows", result.value());
        } else {
            detail::model_put(outcome, "error", result.error().to_string(r));
        }
    };
    try {
        if (entry == "batch_insert") {
            auto& command = user_context.make<BatchInsertCommand>("Payment");
            for (std::size_t i = 0; i < n; ++i) {
                command.batch_values.emplace_back();
            }
            collect(service.batch_insert(command));
        } else if (entry == "batch_update") {
            auto& command = user_context.make<BatchUpdateCommand>("Payment");
            for (std::size_t i = 0; i < n; ++i) {
                command.batch_values.emplace_back();
            }
            for (std::size_t i = 0; i < m; ++i) {
                command.batch_ids.emplace_back(id);
            }
            command.batch_expected_versions = detail::clone_in(r, versions);
            collect(service.batch_update(command));
        } else if (entry == "insert") {
            auto& command = user_context.make<InsertCommand>("Payment");
            collect(service.insert(command));
        } else if (entry == "update") {
            auto& command = user_context.make<UpdateCommand>("Payment", id);
            command.expected_version_value = versions.empty() ? std::nullopt : versions.front();
            collect(service.update(command));
        } else if (entry == "delete") {
            auto& command = user_context.make<DeleteCommand>("Payment", id);
            command.expected_version_value = versions.empty() ? std::nullopt : versions.front();
            collect(service.delete_command(command));
        } else {
            auto& command = user_context.make<RecoverCommand>("Payment", id, 1);
            collect(service.recover(command));
        }
    } catch (const std::out_of_range&) {
        detail::model_put(outcome, "panic", true);
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "outcome", outcome);
    detail::model_put(output, "requests", state.requests.size());
    Json::Array notes(r);
    for (const auto& note : state.notes) {
        notes.emplace_back(note);
    }
    detail::model_put(output, "notes", notes);
    Json cache(r);
    if ((caches & 2) != 0) {
        Json::Array entries(r);
        for (auto key : keys) {
            entries.emplace_back(memory.get(r, key, 0).has_value());
        }
        cache = Json(r, entries);
    }
    detail::model_put(output, "cache", cache);
    return output;
}

int main() {
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        std::cout << evaluate(*user_context, *input).dump(r) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
