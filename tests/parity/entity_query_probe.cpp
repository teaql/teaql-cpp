#include <teaql/runtime/data_service.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/query_debug.hpp>
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

template <class T>
struct Ready {
    using allocator_type = Allocator;
    T result;

    Ready(std::allocator_arg_t, Allocator a, T result)
        : result(detail::move_in(a.resource(), std::move(result))) {}

    std::optional<T> poll(WakeSignal&) {
        return std::move(result);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct Mock : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory r;
    Json::Array requests;
    bool fail;

    Mock(std::allocator_arg_t, Allocator a, bool fail) : r(a.resource()), requests(r), fail(fail) {}

    DataServiceCapabilities capabilities() const override {
        return {};
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
        result.rows.emplace_back(values);
        return make_operation<R, Ready<R>>(target, R::ok(target, std::move(result)));
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
    auto& executor = context.make<Mock>(flag(input, "failure"));
    auto& repo = EntityDataService<Mock>::for_executor(context, "Item", context, executor);
    auto query = repo.select();
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
            } else {
                detail::model_put(outcome, "error", result.error().to_string(r));
            }
        };
        try {
            if (mode == "list") {
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
    auto plan = context.continuous_page_plan();
    detail::model_put(output, "continuous", plan ? Json(r, *plan) : Json(r));
    return output;
}

int main() {
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
