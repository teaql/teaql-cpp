#include <teaql/runtime/data_service.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <iostream>

using namespace teaql;

static const Json& field(const Json& input, std::string_view name) {
    return std::get<Json::Object>(input.data).find(name)->second;
}

static bool flag(const Json& input, std::string_view name) {
    return std::get<bool>(field(input, name).data);
}

static Json trace(Memory r, std::span<const TraceNode> nodes) {
    Json::Array array(r);
    for (const auto& node : nodes) {
        auto value = detail::model_json(r);
        detail::model_put(value, "kind", unsigned(node.kind));
        detail::model_put(value, "entity", node.entity_type);
        detail::model_put(value, "comment", node.comment);
        array.emplace_back(std::move(value));
    }
    return Json(r, array);
}

struct Mock : QueryExecutor<String>, MutationExecutor<String>, StreamQueryExecutor<String> {
    using allocator_type = Allocator;
    Memory resource;
    Json::Array requests;
    std::int64_t error_at;
    bool last_first;
    unsigned closes = 0, pending = 0;

    Mock(std::allocator_arg_t, Allocator a, std::int64_t error_at, bool last_first)
        : resource(a.resource()), requests(resource), error_at(error_at), last_first(last_first) {}

    Memory memory_resource() const {
        return resource;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Cursor {
        using allocator_type = Allocator;
        Memory resource;
        Mock& owner;
        unsigned index = 0;
        bool pending = true, closed = false;

        Cursor(std::allocator_arg_t, Allocator a, Mock& owner)
            : resource(a.resource()), owner(owner) {}

        std::optional<std::optional<Result<StreamChunk, String>>> poll_next(WakeSignal& wake) {
            using Item = Result<StreamChunk, String>;
            if (pending) {
                pending = false;
                ++owner.pending;
                wake.notify();
                return {};
            }
            if (index == 3) {
                return std::optional<Item>{};
            }
            auto current = index++;
            if (current == owner.error_at) {
                return std::optional<Item>(Item::err(resource, String("cursor failed", resource)));
            }
            StreamChunk chunk(
                std::allocator_arg, Allocator(resource), current, current == 2 || owner.last_first);
            chunk.rows.emplace_back();
            chunk.rows.back().insert("id", Value(resource, std::uint64_t(current + 1)));
            return std::optional<Item>(Item::ok(resource, std::move(chunk)));
        }

        void finish_and_wait() noexcept {
            close();
        }

        void cancel_and_wait() noexcept {
            close();
        }

        void close() noexcept {
            if (!closed) {
                closed = true;
                ++owner.closes;
            }
        }
    };

    QueryStream<String>& stream(Memory r, QueryRequest request, std::size_t size, bool observed) {
        const auto& query = request.query;
        auto snapshot = detail::model_json(resource);
        detail::model_put(snapshot, "entity", query.entity);
        Json::Array projection(resource);
        for (const auto& name : query.projection) {
            projection.emplace_back(name);
        }
        detail::model_put(snapshot, "projection", projection);
        if (query.slice) {
            Json::Array slice(resource);
            slice.emplace_back(query.slice->offset);
            slice.emplace_back(query.slice->limit ? Json(resource, *query.slice->limit)
                                                  : Json(resource));
            detail::model_put(snapshot, "slice", slice);
        } else {
            detail::model_put(snapshot, "slice", Json(resource));
        }
        detail::model_put(snapshot, "query_trace", trace(resource, query.trace_chain));
        detail::model_put(snapshot, "request_trace", trace(resource, request.trace_chain));
        detail::model_put(snapshot,
                          "query_comment",
                          query.comment_text ? Json(resource, *query.comment_text)
                                             : Json(resource));
        detail::model_put(snapshot,
                          "comment",
                          request.comment ? Json(resource, *request.comment) : Json(resource));
        detail::model_put(snapshot, "capture", request.capture_execution_metadata);
        detail::model_put(snapshot, "debug", request.capture_debug_query);
        detail::model_put(snapshot, "size", std::uint64_t(size));
        detail::model_put(snapshot, "observed", observed);
        requests.emplace_back(std::move(snapshot));
        return make_resource_stream<Result<StreamChunk, String>, Cursor>(r, *this);
    }

    QueryStream<String>& query_stream(Memory r, QueryRequest q, std::size_t size) override {
        return stream(r, std::move(q), size, false);
    }

    QueryStream<String>&
    query_stream_observed(Memory r, QueryRequest q, std::size_t size, ExecutionObserver) override {
        return stream(r, std::move(q), size, true);
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory, QueryRequest) override {
        throw std::logic_error("streaming must not materialize rows");
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unexpected mutation");
    }
};

static Json evaluate(UserContext& context, const Json& input) {
    auto r = context.memory_resource();
    context.set_user_identifier("reader");
    if (!flag(input, "capture")) {
        context.disable_sql_log();
    }
    auto& metadata = context.make<InMemoryMetadataStore>();
    EntityDescriptor item(std::allocator_arg, Allocator(r), "Item");
    item.property("id", DataType::U64).is_id = true;
    auto& relation = item.relation("children", "Item");
    relation.local_key = "id";
    relation.foreign_key = "id";
    relation.many = true;
    metadata.register_entity(std::move(item));
    context.set_metadata(context, metadata);
    auto& cache = context.make<InMemoryAggregationCache>("unused-stream-cache");
    context.insert_resource(context, cache);
    const auto& error_value = field(input, "error_at").data;
    const auto error_at = std::get_if<std::int64_t>(&error_value)
                              ? std::get<std::int64_t>(error_value)
                              : std::int64_t(std::get<std::uint64_t>(error_value));
    auto& executor = context.make<Mock>(error_at, flag(input, "last_first"));
    auto& repo = EntityDataService<Mock>::for_executor(context, "Item", context, executor);
    auto query = repo.select();
    query.enable_aggregation_cache().comment("business");
    if (auto* size = std::get_if<std::uint64_t>(&field(input, "size").data)) {
        query.stream_config = StreamConfig{std::size_t(*size)};
    }
    if (auto* limit = std::get_if<std::uint64_t>(&field(input, "limit").data)) {
        query.limit(*limit);
    }
    const auto& fields = std::get<Json::Object>(input.data);
    if (auto hard_limit = fields.find("hard_limit"); hard_limit != fields.end()) {
        query.hard_limit(std::get<std::uint64_t>(hard_limit->second.data));
    }
    const auto& shape = std::get<String>(field(input, "shape").data);
    SelectQuery nested(std::allocator_arg, Allocator(r), "Item");
    if (shape == "relation") {
        query.relation("children");
    } else if (shape == "child") {
        query.child_enhancement(nested);
    } else if (shape == "object") {
        query.object_group_by("object", "id", nested);
    } else if (shape == "aggregate") {
        query.count("count");
    } else if (shape == "grouped") {
        query.group_by("id").count("count");
    } else if (shape == "continuous") {
        query.order_asc("id").optimize_for_continuous_page_fetch();
    }
    auto output = detail::model_json(r);
    Json::Array items(r);
    std::optional<PurposedSelectQuery> purpose;
    try {
        purpose.emplace(std::allocator_arg,
                        Allocator(r),
                        query,
                        std::get<String>(field(input, "purpose").data));
    } catch (const std::invalid_argument&) {
        detail::model_put(output, "purpose_panic", true);
    }
    if (purpose) {
        auto& result = repo.fetch_stream(*purpose).wait();
        if (!result) {
            detail::model_put(output, "start_error", result.error().to_string(r));
        } else {
            auto& stream = result.value().get();
            while (auto* next = stream.wait_next()) {
                auto item = detail::model_json(r);
                if (!*next) {
                    detail::model_put(item, "error", next->error().to_string(r));
                } else {
                    detail::model_put(item, "index", std::uint64_t(next->value().chunk_index));
                    detail::model_put(item, "last", next->value().is_last);
                    detail::model_put(item, "id", *next->value().rows[0].get("id")->try_u64());
                }
                items.emplace_back(std::move(item));
            }
        }
    }
    detail::model_put(output, "items", items);
    detail::model_put(output, "requests", executor.requests);
    detail::model_put(output, "closes", executor.closes);
    detail::model_put(output, "pending", executor.pending);
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
