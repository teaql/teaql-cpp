#include <teaql/sql/executor.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;

struct Observation {
    using allocator_type = Allocator;
    unsigned kinds = 0, lookups = 0, opened = 0, closed = 0;
    std::optional<std::size_t> size;
    Json::Array events;

    Observation(std::allocator_arg_t, Allocator a) : events(a.resource()) {}
};

class Dialect final : public SqlDialect {
public:
    Observation& state;
    std::optional<std::size_t> fail_after;

    explicit Dialect(Observation& state) : state(state) {}

    DatabaseKind kind() const override {
        if (++state.kinds > fail_after.value_or(std::numeric_limits<std::size_t>::max())) {
            throw std::runtime_error("controlled kind failure");
        }
        return DatabaseKind::Sqlite;
    }

    String quote_ident(Memory r, std::string_view name) const override {
        String result("\"", r);
        result += name;
        result += '"';
        return result;
    }

    String placeholder(Memory r, std::size_t) const override {
        return String("?", r);
    }
};

class Schema final : public SchemaProvider {
public:
    Observation& state;
    const EntityDescriptor& entity;

    Schema(Observation& state, const EntityDescriptor& entity) : state(state), entity(entity) {}

    const EntityDescriptor* get_entity(std::string_view name) const override {
        ++state.lookups;
        return name == "Order" ? &entity : nullptr;
    }
};

class Store final : public DynamicFieldStore {
public:
    bool is_transaction_bound() const override {
        return false;
    }

    std::uintptr_t binding_key() const override {
        return 17;
    }

    std::optional<std::uint64_t> source_identity() const override {
        return 71;
    }

    DynamicStorageFuture<std::monostate>& ensure_schema(Memory) const override {
        throw std::logic_error("unused fixture SPI");
    }

    DynamicStorageFuture<DynamicStorageRows>& load_values(Memory,
                                                          std::string_view,
                                                          const DynamicFieldDefinitions&,
                                                          std::span<const std::uint64_t>,
                                                          const DynamicFieldSelection&,
                                                          const QueryIntent&) const override {
        throw std::logic_error("unused fixture SPI");
    }

    Result<std::monostate, DynamicFieldError>
    validate_writes(Memory, std::span<const DynamicFieldWrite>) const override {
        throw std::logic_error("unused fixture SPI");
    }

    DynamicStorageFuture<std::monostate>&
    apply_writes(Memory, std::span<const DynamicFieldWrite>, const MutationIntent&) const override {
        throw std::logic_error("unused fixture SPI");
    }
};

struct Cursor {
    using allocator_type = Allocator;
    using Item = Result<StreamChunk, String>;
    Observation& state;
    bool closed = false;

    Cursor(std::allocator_arg_t, Allocator, Observation& state) : state(state) {}

    std::optional<std::optional<Item>> poll_next(WakeSignal&) {
        return std::optional<Item>{};
    }

    void finish_and_wait() noexcept {
        if (!std::exchange(closed, true)) {
            ++state.closed;
        }
    }

    void cancel_and_wait() noexcept {
        finish_and_wait();
    }
};

class PlainTransport : public StreamingSqlTransport<String> {
public:
    Observation& state;

    explicit PlainTransport(Observation& state) : state(state) {}

    DataServiceOperationResult<Rows, String>& fetch_all_compact_sql(Memory,
                                                                    const CompiledQuery&) override {
        throw std::logic_error("unused fixture SPI");
    }

    DataServiceOperationResult<std::uint64_t, String>& execute_sql(Memory,
                                                                   const CompiledQuery&) override {
        throw std::logic_error("unused fixture SPI");
    }

    QueryStream<String>& stream_sql(Memory r, CompiledQuery, std::size_t size) override {
        ++state.opened;
        state.size = size;
        return make_stream<Result<StreamChunk, String>, Cursor>(r, state);
    }
};

class SupportedTransport final : public PlainTransport {
public:
    const Store& store;

    SupportedTransport(Observation& state, const Store& store)
        : PlainTransport(state), store(store) {}

    const DynamicFieldStore* dynamic_field_store() const override {
        return &store;
    }

    Result<QueryStream<String>*, DynamicFieldError> stream_sql_with_dynamic_fields(
        Memory r, CompiledQuery query, std::size_t size, DynamicFieldStreamPlan) override {
        return Result<QueryStream<String>*, DynamicFieldError>::ok(
            r, &stream_sql(r, std::move(query), size));
    }
};

static bool flag(const Json& input, std::string_view name) {
    return std::get<bool>(std::get<Json::Object>(input.data).find(name)->second.data);
}

template <class T>
static Json observe(
    UserContext& user_context, const Json& input, Observation& state, T& transport, Store& store) {
    auto r = user_context.memory_resource();
    const auto& fields = std::get<Json::Object>(input.data);
    auto& dialect = user_context.make_resource<Dialect>(state);
    if (const auto& value = fields.find("fail_after")->second;
        !std::holds_alternative<std::monostate>(value.data)) {
        dialect.fail_after = std::get<std::uint64_t>(value.data);
    }
    auto& entity = user_context.make<EntityDescriptor>("Order");
    entity.property("id", DataType::U64).id();
    auto& schema = user_context.make_resource<Schema>(state, entity);
    auto& cache = user_context.make<SqlExecutorState>();
    auto& executor =
        user_context.make<SqlDataServiceExecutor<String, T>>(dialect, transport, schema, cache);
    auto caps = executor.capabilities();
    auto out = detail::model_json(r);
    detail::model_put(out, "store", executor.dynamic_field_store() != nullptr);
    Json::Array capabilities(r);
    for (bool value : {caps.query,
                       caps.mutation,
                       caps.transaction,
                       caps.schema,
                       caps.id_generation,
                       caps.batch_mutation,
                       caps.returning,
                       caps.small_parent_relation_probes}) {
        capabilities.emplace_back(Json(r, value));
    }
    detail::model_put(out, "capabilities", std::move(capabilities));
    bool opened_result = false;
    Json::Array items(r);
    const auto& gate = std::get<String>(fields.find("gate")->second.data);
    std::string_view owner = gate == "unknown" ? "Unknown" : "Order";
    std::array definitions_fields{DynamicFieldDefinitions::Field{"extra", DataType::Text}};
    auto definitions_result = DynamicFieldDefinitions::from_fields(
        r, gate == "owner" ? "Other" : owner, "r1", definitions_fields);
    auto& definitions =
        user_context.make<DynamicFieldDefinitions>(std::move(definitions_result).value());
    auto selection = DynamicFieldSelection::all(r);
    auto intent =
        QueryIntent::create(r, "Inspect dynamic SQL stream", "Verify executor diagnostics");
    auto plan =
        DynamicFieldStreamPlan::create(r, "fixture", definitions, selection, store, intent.value());
    SelectQuery query(std::allocator_arg, Allocator(r), owner);
    query.limit(1);
    if (gate != "selection") {
        query.select_dynamic_fields(selection);
    }
    if (gate == "compile") {
        query.project_expr("bad", Expr(r, Expr::Kind::Function));
    }
    QueryRequest request(
        std::allocator_arg, Allocator(r), query, intent.value(), false, flag(input, "capture"));
    std::optional<ExecutionObserver> observer;
    if (flag(input, "observe")) {
        observer.emplace(std::allocator_arg, Allocator(r), [r, &state](ExecutionMetadata metadata) {
            auto event = detail::model_json(r);
            detail::model_put(event,
                              "outcome",
                              metadata.sql_log.execution_outcome
                                  ? Json(r, unsigned(*metadata.sql_log.execution_outcome))
                                  : Json(r));
            detail::model_put(
                event, "count", metadata.result_count ? Json(r, *metadata.result_count) : Json(r));
            detail::model_put(event, "closed_before", state.opened == state.closed);
            state.events.emplace_back(std::move(event));
        });
    }
    try {
        auto result = executor.query_stream_with_dynamic_fields(
            r,
            std::move(request),
            std::get<std::uint64_t>(fields.find("size")->second.data),
            std::move(plan).value(),
            std::move(observer));
        if (!result) {
            detail::model_put(out, "error", result.error().to_string(r));
        } else {
            opened_result = true;
            auto& cursor = *result.value();
            if (flag(input, "poll")) {
                for (;;) {
                    auto next = cursor.poll_next();
                    if (next.kind !=
                        StreamPoll<Result<StreamChunk, SqlExecutorError<String>>>::Kind::Item) {
                        break;
                    }
                    auto item = detail::model_json(r);
                    if (!*next.item) {
                        detail::model_put(item, "error", next.item->error().to_string(r));
                    } else {
                        detail::model_put(item, "rows", next.item->value().rows.size());
                    }
                    items.emplace_back(std::move(item));
                }
            }
            cursor.cancel();
        }
    } catch (const std::exception&) {
        detail::model_put(out, "panic", true);
    }
    detail::model_put(out, "opened_result", opened_result);
    detail::model_put(out, "items", std::move(items));
    detail::model_put(out, "kinds", state.kinds);
    detail::model_put(out, "lookups", state.lookups);
    detail::model_put(out, "opened", state.opened);
    detail::model_put(out, "closed", state.closed);
    detail::model_put(out, "size", state.size ? Json(r, *state.size) : Json(r));
    detail::model_put(out, "events", state.events);
    return out;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    for (std::string line; std::getline(std::cin, line);) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("Invalid dynamic stream fixture");
        }
        auto& state = user_context->make<Observation>();
        auto& store = user_context->make_resource<Store>();
        if (flag(*input, "support")) {
            auto& transport = user_context->make_resource<SupportedTransport>(state, store);
            std::cout << observe(*user_context, *input, state, transport, store)
                             .dump(user_context->memory_resource())
                      << '\n';
        } else {
            auto& transport = user_context->make_resource<PlainTransport>(state);
            std::cout << observe(*user_context, *input, state, transport, store)
                             .dump(user_context->memory_resource())
                      << '\n';
        }
    }
    std::pmr::set_default_resource(previous);
}
