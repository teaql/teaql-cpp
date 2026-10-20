#include <teaql/runtime/data_service.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/query_debug.hpp>
#include "../support/entity_decoder_fixture.hpp"
#include <array>
#include <iostream>

using namespace teaql;

static const Json& field(const Json& value, std::string_view name) {
    return std::get<Json::Object>(value.data).find(name)->second;
}

static bool flag(const Json& value, std::string_view name) {
    return std::get<bool>(field(value, name).data);
}

static Json trace(Memory r, std::span<const TraceNode> nodes) {
    Json::Array output(r);
    for (const auto& node : nodes) {
        auto entry = detail::model_json(r);
        detail::model_put(entry, "kind", unsigned(node.kind));
        detail::model_put(entry, "entity", node.entity_type);
        detail::model_put(entry, "comment", node.comment);
        output.emplace_back(std::move(entry));
    }
    return Json(r, output);
}

static Json schema_rows(Memory r, std::span<const CompactRow> rows) {
    Json::Array output(r);
    for (const auto& row : rows) {
        auto item = detail::model_json(r);
        CompactRow plain(std::allocator_arg, Allocator(r), row.copy_map(r));
        detail::model_put(item, "equals_plain", row == plain);
        auto valid = row.validate_dynamic_properties();
        detail::model_put(item, "validation", valid ? Json(r) : Json(r, valid.error()));
        output.emplace_back(std::move(item));
    }
    return Json(r, output);
}

static std::span<const CompactRow> result_rows(const Vector<CompactRow>& rows) {
    return rows;
}

static std::span<const CompactRow> result_rows(const SmartList<CompactRow>& rows) {
    return rows.data;
}

static Vector<CompactRow> result_rows(const SmartList<decoder_fixture::Item<>>& list) {
    Vector<CompactRow> rows(list.data.get_allocator().resource());
    for (const auto& item : list.data) {
        rows.emplace_back(item.row);
    }
    return rows;
}

static void
dynamic_selection(const UserContext& user_context, SelectQuery& query, std::string_view action) {
    if (action == "add") {
        auto& selection = user_context.make<DynamicFieldSelection>(
            DynamicFieldSelection::all(user_context.memory_resource()));
        query.dynamic_field_selection = detail::ScopedBox<DynamicFieldSelection>(&selection);
    } else if (action == "remove") {
        query.dynamic_field_selection.reset();
    } else if (action != "keep") {
        throw std::logic_error("selection fixture action");
    }
}

struct SelectionCalls {
    using allocator_type = Allocator;
    Json::Array calls;

    SelectionCalls(std::allocator_arg_t, Allocator a) : calls(a.resource()) {}
};

static RuntimeHookResult select_dynamic_fields(const UserContext& user_context,
                                               SelectQuery& query,
                                               SelectionCalls& calls,
                                               std::string_view stage,
                                               std::string_view action) {
    auto r = user_context.memory_resource();
    const bool before = bool(query.dynamic_field_selection);
    dynamic_selection(user_context, query, action);
    auto event = detail::model_json(r);
    detail::model_put(event, "stage", stage);
    detail::model_put(event, "before", before);
    detail::model_put(event, "after", bool(query.dynamic_field_selection));
    calls.calls.emplace_back(std::move(event));
    return RuntimeHookResult::ok(r, {});
}

struct SelectionBehavior final : EntityDataServiceBehavior {
    using allocator_type = Allocator;
    SelectionCalls& calls;
    String action;

    SelectionBehavior(std::allocator_arg_t,
                      Allocator a,
                      SelectionCalls& calls,
                      std::string_view action)
        : calls(calls), action(action, a.resource()) {}

    RuntimeHookResult
    before_select(Memory, const UserContext& user_context, SelectQuery& query) const override {
        return select_dynamic_fields(user_context, query, calls, "behavior", action);
    }
};

struct SelectionPolicy final : RequestPolicy {
    using allocator_type = Allocator;
    SelectionCalls& calls;
    String action;

    SelectionPolicy(std::allocator_arg_t,
                    Allocator a,
                    SelectionCalls& calls,
                    std::string_view action)
        : calls(calls), action(action, a.resource()) {}

    RuntimeHookResult
    enforce_select(Memory, const UserContext& user_context, SelectQuery& query) const override {
        return select_dynamic_fields(user_context, query, calls, "policy", action);
    }
};

struct RecordingCache final : AggregationCacheBackend {
    using allocator_type = Allocator;
    InMemoryAggregationCache inner;
    mutable Json::Array events;

    RecordingCache(std::allocator_arg_t, Allocator a)
        : inner(std::allocator_arg, a, "readonly"), events(a.resource()) {}

    Memory memory_resource() const noexcept override {
        return inner.memory_resource();
    }

    std::string_view name_space() const noexcept override {
        return inner.name_space();
    }

    std::optional<Vector<CompactRow>>
    get(Memory target, std::string_view key, std::uint64_t age) const override {
        auto rows = inner.get(target, key, age);
        auto r = memory_resource();
        auto event = detail::model_json(r);
        detail::model_put(event, "op", "get");
        detail::model_put(event, "age", age);
        detail::model_put(event, "hit", bool(rows));
        detail::model_put(event, "schema", rows ? schema_rows(r, *rows) : Json(r));
        events.emplace_back(std::move(event));
        return rows;
    }

    void put(std::string_view key, std::span<const CompactRow> rows) override {
        auto r = memory_resource();
        auto event = detail::model_json(r);
        detail::model_put(event, "op", "put");
        detail::model_put(event, "schema", schema_rows(r, rows));
        events.emplace_back(std::move(event));
        inner.put(key, rows);
    }

    void invalidate_namespace(std::string_view name) override {
        inner.invalidate_namespace(name);
    }
};

template <class T>
struct Ready {
    using allocator_type = Allocator;
    T result;
    bool pending;
    unsigned* cancellations;

    Ready(std::allocator_arg_t,
          Allocator a,
          T result,
          bool pending = false,
          unsigned* cancellations = nullptr)
        : result(detail::move_in(a.resource(), std::move(result))), pending(pending),
          cancellations(cancellations) {}

    std::optional<T> poll(WakeSignal& wake) {
        if (pending) {
            pending = false;
            wake.notify();
            return {};
        }
        return std::move(result);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {
        if (cancellations) {
            ++*cancellations;
        }
    }
};

struct Mock : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory r;
    Json::Array requests;
    mutable Json::Array classifications;
    bool fail;
    bool schema_probe = false, empty = false;
    bool delayed = false;
    unsigned* cancellations = nullptr;
    String result_kind;

    Mock(std::allocator_arg_t, Allocator a, bool fail)
        : r(a.resource()), requests(r), classifications(r), fail(fail), result_kind(r) {}

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    SqlIntentRedactions query_log_intent(Memory target, const SelectQuery& query) const override {
        if (schema_probe) {
            auto event = detail::model_json(r);
            detail::model_put(event, "entity", query.entity);
            auto kind = query.dynamic_property_definitions
                            ? query.dynamic_property_definitions->data_type("_result")
                            : std::optional<DataType>{};
            detail::model_put(event, "schema", kind ? Json(r, unsigned(*kind)) : Json(r));
            classifications.emplace_back(std::move(event));
        }
        return SqlIntentRedactions::from_unclassified_query(target, query);
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory target,
                                                           QueryRequest request) override {
        auto snapshot = detail::model_json(r);
        const auto& query = request.query;
        detail::model_put(snapshot, "entity", query.entity);
        Json::Array projection(r);
        for (const auto& name : query.projection) {
            projection.emplace_back(name);
        }
        detail::model_put(snapshot, "projection", projection);
        if (query.slice) {
            Json::Array slice(r);
            slice.emplace_back(query.slice->offset);
            slice.emplace_back(query.slice->limit ? Json(r, *query.slice->limit) : Json(r));
            detail::model_put(snapshot, "slice", slice);
        } else {
            detail::model_put(snapshot, "slice", Json(r));
        }
        detail::model_put(snapshot, "request_trace", trace(r, request.trace_chain));
        detail::model_put(snapshot, "query_trace", trace(r, query.trace_chain));
        detail::model_put(snapshot, "comment", request.comment());
        detail::model_put(
            snapshot, "query_comment", query.comment_text ? Json(r, *query.comment_text) : Json(r));
        detail::model_put(snapshot, "capture", request.capture_execution_metadata);
        detail::model_put(snapshot, "debug", request.capture_debug_query);
        detail::model_put(snapshot,
                          "filter",
                          query.predicate ? Json(r, detail::debug_expr(r, *query.predicate))
                                          : Json(r));
        requests.emplace_back(std::move(snapshot));
        using R = Result<QueryResult, String>;
        if (fail) {
            return make_operation<R, Ready<R>>(target,
                                               R::err(target, String("executor failed", target)));
        }
        QueryResult result(
            std::allocator_arg, Allocator(target), ExecutionMetadata::unrecorded_query(target, 1));
        Record values(target);
        values.emplace(String("id", target), Value(target, std::uint64_t(1)));
        if (schema_probe) {
            Value value(target);
            if (result_kind == "u64") {
                value = Value(target, std::uint64_t(7));
            } else if (result_kind == "text") {
                value = Value(target, "result");
            } else if (result_kind == "typed_null") {
                value = Value(target, TypedNull{DataType::U64});
            } else if (result_kind != "null") {
                throw std::logic_error("fixture result kind");
            }
            values.emplace(String("_result", target), std::move(value));
        }
        if (!schema_probe || !empty) {
            result.rows.emplace_back(values);
        }
        return make_operation<R, Ready<R>>(
            target, R::ok(target, std::move(result)), delayed, cancellations);
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unexpected mutation");
    }
};

static Json evaluate(UserContext& context, const Json& input) {
    auto r = context.memory_resource();
    context.set_user_identifier("fixture");
    if (!flag(input, "capture")) {
        context.disable_sql_log();
    }
    const auto& options = std::get<Json::Object>(input.data);
    if (options.contains("select_capture")) {
        context.set_sql_log_options(SqlLogOptions{
            .select = flag(input, "select_capture"),
            .mutation = flag(input, "mutation_capture"),
        });
    }
    auto& metadata = context.make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Item");
    descriptor.property("id", DataType::U64).is_id = true;
    metadata.register_entity(std::move(descriptor));
    context.set_metadata(context, metadata);
    if (flag(input, "backend")) {
        auto& cache = context.make<InMemoryAggregationCache>("integration");
        context.insert_resource(context, cache);
    }
    const bool schema_probe = options.contains("schema_probe") && flag(input, "schema_probe");
    RecordingCache* recording = nullptr;
    if (schema_probe) {
        recording = &context.make<RecordingCache>();
        if (flag(input, "record_cache")) {
            context.insert_resource(context, static_cast<AggregationCacheBackend&>(*recording));
        }
    }
    auto& executor = context.make<Mock>(flag(input, "failure"));
    executor.schema_probe = schema_probe;
    if (schema_probe) {
        executor.empty = flag(input, "empty");
        executor.result_kind = std::get<String>(field(input, "result_kind").data);
    }
    const bool dynamic_probe = options.contains("dynamic_probe") && flag(input, "dynamic_probe");
    SelectionCalls* selection_calls = nullptr;
    if (dynamic_probe) {
        selection_calls = &context.make<SelectionCalls>();
        auto& behavior = context.make<SelectionBehavior>(
            *selection_calls, std::get<String>(field(input, "selection_behavior").data));
        auto& registry = context.make<InMemoryEntityDataServiceBehaviorRegistry>();
        registry.register_behavior("Item", context, behavior);
        context.set_entity_data_service_behavior_registry(context, registry);
        auto& policy = context.make<SelectionPolicy>(
            *selection_calls, std::get<String>(field(input, "selection_policy").data));
        context.set_request_policy(context, policy);
    }
    auto& repo = EntityDataService<Mock>::for_executor(context, "Item", context, executor);
    auto query = repo.select();
    if (dynamic_probe && flag(input, "initial_dynamic")) {
        dynamic_selection(context, query, "add");
    }
    if (flag(input, "cache")) {
        query.enable_aggregation_cache();
    }
    if (flag(input, "aggregate")) {
        query.count("count");
    }
    if (auto* limit = std::get_if<std::uint64_t>(&field(input, "limit").data)) {
        query.limit(*limit);
    }
    if (auto* comment = std::get_if<String>(&field(input, "comment").data)) {
        query.comment(*comment);
    }
    if (flag(input, "continuous")) {
        query.order_asc("id").limit(1).optimize_for_continuous_page_fetch();
    }
    if (schema_probe && std::get<String>(field(input, "schema_kind").data) != "none") {
        const std::array fields{DynamicPropertyDefinitions::Field{
            "_result",
            std::get<String>(field(input, "schema_kind").data) == "text" ? DataType::Text
                                                                         : DataType::U64}};
        auto definitions = DynamicPropertyDefinitions::from_fields(r, fields);
        auto& retained = context.make<DynamicPropertyDefinitions>(std::move(definitions).value());
        query.with_dynamic_property_definitions(retained);
    }
    Json::Array outcomes(r);
    auto calls = std::get<std::uint64_t>(field(input, "calls").data);
    const auto& mode = std::get<String>(field(input, "mode").data);
    for (std::uint64_t i = 0; i < calls; ++i) {
        auto page = detail::clone_in(r, query);
        if (flag(input, "continuous")) {
            const auto& object = std::get<Json::Object>(input.data);
            auto offsets = object.find("offsets");
            page.offset(offsets == object.end()
                            ? i
                            : std::get<std::uint64_t>(
                                  std::get<Json::Array>(offsets->second.data).at(i).data));
        }
        auto outcome = detail::model_json(r);
        std::optional<PurposedSelectQuery> wrapped;
        try {
            wrapped.emplace(std::allocator_arg,
                            Allocator(r),
                            page,
                            std::get<String>(field(input, "purpose").data));
        } catch (const std::invalid_argument&) {
            detail::model_put(outcome, "purpose_panic", true);
            outcomes.emplace_back(std::move(outcome));
            continue;
        }
        auto collect = [&](const auto& result, auto size) {
            if (result) {
                detail::model_put(outcome, "rows", std::uint64_t(size(result.value())));
                if (schema_probe) {
                    detail::model_put(
                        outcome, "schema", schema_rows(r, result_rows(result.value())));
                }
            } else {
                detail::model_put(outcome, "error", result.error().to_string(r));
            }
        };
        try {
            if (mode == "typed") {
                collect(repo.fetch_entities<decoder_fixture::Item<>>(*wrapped).wait(),
                        [](const auto& v) {
                            return v.data.size();
                        });
            } else if (mode == "list") {
                collect(repo.fetch_smart_list(*wrapped).wait(), [](const auto& v) {
                    return v.data.size();
                });
            } else if (mode == "aggregates") {
                collect(repo.fetch_smart_list_with_relation_aggregates(*wrapped, {}).wait(),
                        [](const auto& v) {
                            return v.data.size();
                        });
            } else {
                auto& work = mode == "owned" ? repo.fetch_all_owned(std::move(*wrapped))
                                             : repo.fetch_all(*wrapped);
                collect(work.wait(), [](const auto& v) {
                    return v.size();
                });
            }
        } catch (const std::overflow_error&) {
            detail::model_put(outcome, "execution_panic", true);
        }
        outcomes.emplace_back(std::move(outcome));
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "outcomes", outcomes);
    detail::model_put(output, "requests", executor.requests);
    if (schema_probe) {
        detail::model_put(output, "cache_events", recording->events);
        detail::model_put(output, "classifications", executor.classifications);
    }
    if (dynamic_probe) {
        detail::model_put(output, "selection_calls", selection_calls->calls);
    }
    auto plan = context.continuous_page_plan();
    detail::model_put(output, "continuous", plan ? Json(r, *plan) : Json(r));
    return output;
}

static void schema_lifetimes() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    {
        GlobalContext global;
        std::array<Ref<CompactRow>, 4> retained;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        user_context->disable_sql_log();
        auto& executor = user_context->make<Mock>(false);
        executor.schema_probe = true;
        executor.result_kind = "text";
        auto& cache = user_context->make<InMemoryAggregationCache>("lifetime");
        user_context->insert_resource(*user_context, cache);
        auto& repo =
            EntityDataService<Mock>::for_executor(*user_context, "Item", *user_context, executor);
        for (unsigned mode = 0; mode < retained.size(); ++mode) {
            Ref<CompactRow> row;
            {
                auto query = repo.select();
                const std::array fields{
                    DynamicPropertyDefinitions::Field{"_result", DataType::U64}};
                auto definitions = DynamicPropertyDefinitions::from_fields(r, fields);
                query.with_dynamic_property_definitions(definitions.value());
                query.comment("Inspect schema lifetime").enable_aggregation_cache();
                PurposedSelectQuery wrapped(
                    std::allocator_arg, Allocator(r), query, "Retain explicit readonly schema");
                // Exercise a miss and a hit on cache-capable entries.
                for (unsigned call = 0; call < 2; ++call) {
                    if (mode == 0 || mode == 1) {
                        const auto& result =
                            mode == 0 ? repo.fetch_all(wrapped).wait()
                                      : repo.fetch_all_owned(detail::clone_in(r, wrapped)).wait();
                        if (!result || result.value().size() != 1) {
                            throw std::runtime_error("Schema fixture must return one row");
                        }
                        row = user_context->make_ref<CompactRow>(result.value().front());
                    } else {
                        const auto& result =
                            mode == 2 ? repo.fetch_smart_list(wrapped).wait()
                                      : repo.fetch_smart_list_with_relation_aggregates(wrapped, {})
                                            .wait();
                        if (!result || result.value().data.size() != 1) {
                            throw std::runtime_error("Schema fixture must return one list row");
                        }
                        row = user_context->make_ref<CompactRow>(result.value().data.front());
                    }
                }
                const auto& typed = repo.fetch_entities<decoder_fixture::Item<>>(wrapped).wait();
                if (!typed || typed.value().data.size() != 1 || !typed.value().data.front().state ||
                    typed.value().data.front().row.validate_dynamic_properties()) {
                    throw std::runtime_error(
                        "Typed decoder must preserve runtime state and schema");
                }
            }
            if (user_context->get(row).validate_dynamic_properties()) {
                throw std::runtime_error("Schema must survive its temporary query");
            }
            retained[mode] = user_context->make_global<CompactRow>(user_context->get(row));
        }
        user_context.reset();
        auto next_user_context = global.make_user_context();
        for (const auto& row : retained) {
            if (next_user_context->get(next_user_context->rebind_global(row))
                    .validate_dynamic_properties()) {
                throw std::runtime_error("Explicit Global copy must retain its schema");
            }
        }
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Query schema lifetime contracts passed\n";
}

static void dynamic_lifetimes() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    for (bool selected : {false, true}) {
        for (unsigned mode = 0; mode < 4; ++mode) {
            GlobalContext global;
            unsigned cancellations = 0;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            user_context->disable_sql_log();
            auto& calls = user_context->make<SelectionCalls>();
            auto& behavior = user_context->make<SelectionBehavior>(calls, "keep");
            auto& registry = user_context->make<InMemoryEntityDataServiceBehaviorRegistry>();
            registry.register_behavior("Item", *user_context, behavior);
            user_context->set_entity_data_service_behavior_registry(*user_context, registry);
            auto& policy = user_context->make<SelectionPolicy>(calls, selected ? "remove" : "add");
            user_context->set_request_policy(*user_context, policy);
            auto& executor = user_context->make<Mock>(false);
            executor.delayed = true;
            executor.cancellations = &cancellations;
            auto& repo = EntityDataService<Mock>::for_executor(
                *user_context, "Item", *user_context, executor);
            {
                auto query = repo.select();
                if (selected) {
                    dynamic_selection(*user_context, query, "add");
                }
                query.comment("Load a prepared dynamic selection");
                PurposedSelectQuery request(std::allocator_arg,
                                            Allocator(r),
                                            query,
                                            "Verify suspended typed query cleanup");
                auto& work = repo.fetch_entities<decoder_fixture::Item<>>(request);
                if (mode != 0) {
                    if (work.poll() || calls.calls.size() != 2 || executor.requests.size() != 1) {
                        throw std::runtime_error("Typed query must prepare once before waiting");
                    }
                    if (mode == 2) {
                        work.cancel();
                    } else if (mode == 3) {
                        const auto* result = work.poll();
                        if (!result || bool(*result) != selected || calls.calls.size() != 2) {
                            throw std::runtime_error(
                                "Typed decode must use the prepared selection");
                        }
                        if (selected && (result->value().data.size() != 1 ||
                                         !result->value().data.front().state)) {
                            throw std::runtime_error("Typed decode lost its fresh runtime state");
                        }
                    }
                }
            }
            user_context.reset();
            const unsigned expected = mode == 1 || mode == 2 ? 1 : 0;
            if (cancellations != expected) {
                throw std::runtime_error(
                    "Typed provider must be cancelled once before arena reclamation");
            }
        }
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Dynamic typed lifetime contracts passed\n";
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--dynamic-self-test") {
        dynamic_lifetimes();
        return 0;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--schema-self-test") {
        schema_lifetimes();
        return 0;
    }
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("invalid fixture JSON");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
