#include <teaql/runtime/data_service.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <array>
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
    bool schema_probe = false, empty = false;
    String result_kind;
    unsigned* external_closes = nullptr;

    Mock(std::allocator_arg_t, Allocator a, std::int64_t error_at, bool last_first)
        : resource(a.resource()), requests(resource), error_at(error_at), last_first(last_first),
          result_kind(a.resource()) {}

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
            if (!owner.schema_probe || !owner.empty) {
                for (unsigned row = 0; row < (owner.schema_probe ? 2u : 1u); ++row) {
                    Record values(resource);
                    values.emplace(String("id", resource),
                                   Value(resource, std::uint64_t(current + 1 + row)));
                    if (owner.schema_probe) {
                        Value value(resource);
                        if (owner.result_kind == "u64") {
                            value = Value(resource, std::uint64_t(7));
                        } else if (owner.result_kind == "text") {
                            value = Value(resource, "result");
                        } else if (owner.result_kind == "typed_null") {
                            value = Value(resource, TypedNull{DataType::U64});
                        } else if (owner.result_kind != "null") {
                            throw std::logic_error("Unknown stream fixture result kind");
                        }
                        values.emplace(String("_result", resource), std::move(value));
                    }
                    chunk.rows.emplace_back(values);
                }
            }
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
                if (owner.external_closes) {
                    ++*owner.external_closes;
                }
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
        detail::model_put(snapshot, "comment", Json(resource, request.comment()));
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
    const auto& input_fields = std::get<Json::Object>(input.data);
    const bool schema_probe = input_fields.contains("schema_probe") && flag(input, "schema_probe");
    executor.schema_probe = schema_probe;
    if (schema_probe) {
        executor.empty = flag(input, "empty");
        executor.result_kind = std::get<String>(field(input, "result_kind").data);
    }
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
    if (schema_probe && std::get<String>(field(input, "schema_kind").data) != "none") {
        const std::array definitions{DynamicPropertyDefinitions::Field{
            "_result",
            std::get<String>(field(input, "schema_kind").data) == "text" ? DataType::Text
                                                                         : DataType::U64}};
        auto result = DynamicPropertyDefinitions::from_fields(r, definitions);
        query.with_dynamic_property_definitions(result.value());
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
            auto consumption =
                schema_probe ? std::string_view(std::get<String>(field(input, "consume").data))
                             : "all";
            if (consumption == "none") {
                stream.cancel();
            } else if (consumption == "pending") {
                if (stream.poll_next().kind !=
                    StreamPoll<Result<StreamChunk, DataServiceError<String>>>::Kind::Pending) {
                    throw std::runtime_error("Expected initial pending stream");
                }
                stream.cancel();
            } else {
                while (auto* next = stream.wait_next()) {
                    auto item = detail::model_json(r);
                    if (!*next) {
                        detail::model_put(item, "error", next->error().to_string(r));
                    } else {
                        detail::model_put(item, "index", std::uint64_t(next->value().chunk_index));
                        detail::model_put(item, "last", next->value().is_last);
                        const auto& rows = next->value().rows;
                        detail::model_put(item,
                                          "id",
                                          rows.empty() ? Json(r)
                                                       : Json(r, *rows[0].get("id")->try_u64()));
                        if (schema_probe) {
                            Json::Array schemas(r);
                            for (const auto& row : rows) {
                                auto schema = detail::model_json(r);
                                CompactRow plain(std::allocator_arg, Allocator(r), row.copy_map(r));
                                detail::model_put(schema, "equals_plain", row == plain);
                                auto valid = row.validate_dynamic_properties();
                                detail::model_put(
                                    schema, "validation", valid ? Json(r) : Json(r, valid.error()));
                                schemas.emplace_back(std::move(schema));
                            }
                            detail::model_put(item, "schema", schemas);
                        }
                    }
                    items.emplace_back(std::move(item));
                    if (consumption == "one") {
                        stream.cancel();
                        break;
                    }
                }
            }
        }
    }
    detail::model_put(output, "items", items);
    detail::model_put(output, "requests", executor.requests);
    detail::model_put(output, "closes", executor.closes);
    detail::model_put(output, "pending", executor.pending);
    return output;
}

static void schema_lifetimes() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    for (unsigned mode = 0; mode < 6; ++mode) {
        GlobalContext global;
        unsigned closes = 0;
        std::optional<EntityQueryStream<String>> handle;
        Ref<CompactRow> retained;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        user_context->disable_sql_log();
        auto& executor = user_context->make<Mock>(mode == 4 ? 1 : mode == 5 ? 0 : -1, false);
        executor.schema_probe = true;
        executor.result_kind = "text";
        executor.external_closes = &closes;
        auto& repo =
            EntityDataService<Mock>::for_executor(*user_context, "Item", *user_context, executor);
        {
            auto query = repo.select();
            const std::array fields{DynamicPropertyDefinitions::Field{"_result", DataType::U64}};
            auto definitions = DynamicPropertyDefinitions::from_fields(r, fields);
            query.with_dynamic_property_definitions(definitions.value())
                .comment("Open readonly stream");
            PurposedSelectQuery request(
                std::allocator_arg, Allocator(r), query, "Verify delayed schema ownership");
            const auto& result = repo.fetch_stream(request).wait();
            if (!result) {
                throw std::runtime_error("Schema lifetime stream did not open");
            }
            handle.emplace(std::allocator_arg, Allocator(r), result.value());
        }
        // The request and definitions are gone before the first delayed poll.
        auto& stream = handle->get();
        if (mode == 1) {
            using Kind = StreamPoll<Result<StreamChunk, DataServiceError<String>>>::Kind;
            if (stream.poll_next().kind != Kind::Pending) {
                throw std::runtime_error("Expected initial pending cursor");
            }
        } else if (mode >= 2) {
            const auto* item = stream.wait_next();
            if (!item || bool(*item) != (mode != 5)) {
                throw std::runtime_error("Schema stream changed provider error behavior");
            }
            if (mode != 5) {
                const auto& rows = item->value().rows;
                if (rows.size() != 2 || rows[0].validate_dynamic_properties() ||
                    rows[1].validate_dynamic_properties()) {
                    throw std::runtime_error("Delayed stream lost readonly schema");
                }
                retained = user_context->make_global<CompactRow>(rows.front());
            }
            if (mode == 3) {
                stream.cancel();
            } else if (mode == 4) {
                unsigned errors = 0, successful = 1;
                while (const auto* next = stream.wait_next()) {
                    if (!*next) {
                        ++errors;
                    } else {
                        ++successful;
                        if (next->value().rows.front().validate_dynamic_properties()) {
                            throw std::runtime_error("A later chunk lost its schema");
                        }
                    }
                }
                if (errors != 1 || successful != 2) {
                    throw std::runtime_error("Stream must continue after a provider item error");
                }
            }
        }
        user_context.reset();
        if (closes != 1) {
            throw std::runtime_error("Schema stream must close exactly once before reclamation");
        }
        bool rejected = false;
        try {
            (void)handle->get();
        } catch (const ReferenceError& error) {
            rejected = error.kind() == ReferenceError::Kind::Closed;
        }
        if (!rejected) {
            throw std::runtime_error("An ordinary stream handle must not extend Context lifetime");
        }
        if (mode >= 2 && mode < 5) {
            auto next_user_context = global.make_user_context();
            if (next_user_context->get(next_user_context->rebind_global(retained))
                    .validate_dynamic_properties()) {
                throw std::runtime_error("Explicit Global stream-row copy lost readonly schema");
            }
        }
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Stream schema lifetime contracts passed\n";
}

int main(int argc, char** argv) {
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
