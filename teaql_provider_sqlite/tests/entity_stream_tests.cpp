#include <teaql/runtime/data_service.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct StreamExecutor : QueryExecutor<String>,
                        MutationExecutor<String>,
                        StreamQueryExecutor<String> {
    using allocator_type = Allocator;
    Memory resource;
    Memory foreign = nullptr;
    QueryRequest* request = nullptr;
    WakeSignal* wake = nullptr;
    unsigned calls = 0, observed = 0, closes = 0;
    unsigned* external_closes = nullptr;
    std::size_t size = 0;
    bool pending = false;
    bool exhaust_objects = false;
    int error_at = -1;

    StreamExecutor(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    Memory memory_resource() const {
        return resource;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Cursor {
        using allocator_type = Allocator;
        Memory resource;
        StreamExecutor& owner;
        unsigned index = 0;
        bool closed = false;

        Cursor(std::allocator_arg_t, Allocator a, StreamExecutor& owner)
            : resource(a.resource()), owner(owner) {}

        std::optional<std::optional<Result<StreamChunk, String>>> poll_next(WakeSignal& wake) {
            using Item = Result<StreamChunk, String>;
            owner.wake = &wake;
            if (owner.pending) {
                return {};
            }
            if (index == 3) {
                return std::optional<Item>{};
            }
            const auto current = index++;
            if (int(current) == owner.error_at) {
                return std::optional<Item>(Item::err(resource, String("cursor failed", resource)));
            }
            StreamChunk chunk(std::allocator_arg, Allocator(resource), current, current == 2);
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
                owner.wake = nullptr;
                ++owner.closes;
                if (owner.external_closes) {
                    ++*owner.external_closes;
                }
            }
        }
    };

    QueryStream<String>&
    query_stream(Memory r, QueryRequest value, std::size_t chunk_size) override {
        ++calls;
        size = chunk_size;
        request = &detail::resource_arena(r).make<QueryRequest>(std::move(value));
        auto& stream =
            make_resource_stream<Result<StreamChunk, String>, Cursor>(foreign ? foreign : r, *this);
        if (exhaust_objects) {
            try {
                for (;;) {
                    detail::resource_arena(r).make<int>();
                }
            } catch (const MemoryLimitError& error) {
                check(error.kind() == MemoryLimitError::Kind::Objects,
                      "exhaust only object identities");
            }
        }
        return stream;
    }

    QueryStream<String>& query_stream_observed(Memory r,
                                               QueryRequest request,
                                               std::size_t size,
                                               ExecutionObserver observer) override {
        ++observed;
        auto metadata = ExecutionMetadata::unrecorded_query(r, 0);
        observer(std::move(metadata));
        return query_stream(r, std::move(request), size);
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory, QueryRequest) override {
        throw std::logic_error("streaming must not materialize rows");
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unexpected mutation");
    }
};

static void install(UserContext& user) {
    auto& metadata = user.make<InMemoryMetadataStore>();
    EntityDescriptor item(std::allocator_arg, Allocator(user.memory_resource()), "Item");
    item.property("id", DataType::U64).is_id = true;
    auto& children = item.relation("children", "Item");
    children.local_key = "id";
    children.foreign_key = "id";
    children.many = true;
    metadata.register_entity(std::move(item));
    user.set_metadata(user, metadata);
}

static void mapped_cursor_contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    install(*user);
    user->set_user_identifier("reader");
    auto r = user->memory_resource();
    auto& executor = user->make<StreamExecutor>();
    auto& repo = EntityDataService<StreamExecutor>::for_executor(*user, "Item", *user, executor);
    auto& query = user->make<SelectQuery>("Item");
    query.comment("inspect stream records");
    query.stream(2);
    auto& purposed = user->make<PurposedSelectQuery>(query, "Inspect root records");
    auto& startup = repo.fetch_stream(purposed);
    check(executor.calls == 0, "stream preparation remains lazy");
    query.entity = "Changed";
    auto& result = startup.wait();
    check(result && executor.calls == 1 && executor.observed == 1 && executor.size == 2,
          "the stream entry delegates one prepared request and chunk size");
    check(executor.request->query.entity == "Item" &&
              executor.request->query.trace_chain.back().kind == TraceKind::Purpose &&
              executor.request->trace_chain.back().kind == TraceKind::Purpose &&
              executor.request->comment() == "inspect stream records" &&
              executor.request->purpose() == "Inspect root records",
          "stream request retains query provenance and captured purpose");
    auto& stream = result.value().get();
    executor.pending = true;
    check(stream.poll_next().kind ==
              StreamPoll<Result<StreamChunk, DataServiceError<String>>>::Kind::Pending,
          "mapped streams preserve Pending");
    auto ticket = stream.wake_signal().ticket();
    executor.wake->notify();
    check(stream.wake_signal().ticket() != ticket, "native readiness reaches the public stream");
    executor.pending = false;
    executor.error_at = 1;
    auto* first = stream.wait_next();
    check(first && *first && first->value().rows[0].get("id")->try_u64() == 1,
          "the first chunk is not accumulated into a full list");
    auto* error = stream.wait_next();
    check(error && !*error && error->error().executor_error() == "cursor failed",
          "executor errors remain stream items");
    auto* last = stream.wait_next();
    check(last && *last && last->value().is_last && last->value().chunk_index == 2 &&
              first->value().rows[0].get("id")->try_u64() == 1,
          "error items do not suppress subsequent chunks and prior borrows remain stable");
    check(!stream.wait_next() && executor.closes == 1, "EOF closes the provider exactly once");
    bool escaped = false;
    try {
        global.make<EntityQueryStream<String>>(result.value());
    } catch (const std::invalid_argument&) {
        escaped = true;
    }
    check(escaped, "a stream handle cannot escape into Global");

    user->disable_sql_log();
    auto& plain = user->make<SelectQuery>("Item");
    plain.comment("inspect stream records");
    plain.count("count").enable_aggregation_cache();
    auto& allowed = user->make<PurposedSelectQuery>(plain, "Stream an aggregate root");
    auto& uncaptured = repo.fetch_stream(allowed).wait();
    check(uncaptured && executor.observed == 1 && executor.size == 1000 &&
              executor.request->comment() == "inspect stream records" &&
              executor.request->purpose() == "Stream an aggregate root" &&
              executor.request->trace_chain.empty() && !executor.request->query.trace_chain.empty(),
          "uncaptured streams retain owned intent while omitting diagnostic trace capture");
    uncaptured.value().get().cancel();
    auto before = executor.calls;
    for (unsigned shape = 0; shape != 3; ++shape) {
        auto& unsupported = user->make<SelectQuery>("Item");
        unsupported.comment("inspect stream records");
        auto& nested = user->make<SelectQuery>("Item");
        if (shape == 0) {
            unsupported.relation("children");
        } else if (shape == 1) {
            unsupported.child_enhancement(nested);
        } else {
            unsupported.object_group_by("object", "id", nested);
        }
        auto& input = user->make<PurposedSelectQuery>(unsupported, "Reject enhancements");
        auto& rejected = repo.fetch_stream(input).wait();
        check(!rejected && rejected.error().runtime_error().kind() == RuntimeError::Kind::Graph,
              "stream enhancement rejection precedes provider creation");
    }
    check(executor.calls == before, "rejected enhanced shapes create no cursor");
}

static void close_and_domain_contracts() {
    GlobalContext global;
    unsigned closes = 0;
    std::optional<EntityQueryStream<String>> saved;
    auto user = global.make_user_context();
    install(*user);
    auto& executor = user->make<StreamExecutor>();
    executor.external_closes = &closes;
    auto& repo = EntityDataService<StreamExecutor>::for_executor(*user, "Item", *user, executor);
    auto& query = user->make<SelectQuery>("Item");
    query.comment("inspect stream records");
    auto& input = user->make<PurposedSelectQuery>(query, "Inspect shutdown");
    auto& abandoned = repo.fetch_stream(input);
    abandoned.cancel();
    check(executor.calls == 0, "cancel before startup does not create a provider cursor");
    auto peer = global.make_user_context();
    executor.foreign = peer->memory_resource();
    bool foreign = false;
    try {
        repo.fetch_stream(input).wait();
    } catch (const std::invalid_argument&) {
        foreign = true;
    }
    check(foreign && closes == 1, "foreign cursors are cancelled before domain rejection");
    executor.foreign = nullptr;
    auto& result = repo.fetch_stream(input).wait();
    saved.emplace(std::allocator_arg, Allocator(user->memory_resource()), result.value());
    executor.pending = true;
    check(saved->get().poll_next().kind ==
              StreamPoll<Result<StreamChunk, DataServiceError<String>>>::Kind::Pending,
          "public cursor work may be pending during shutdown");
    user->close();
    check(closes == 2, "Context close cancels the pending cursor exactly once");
    user.reset();
    bool expired = false;
    try {
        (void)saved->get();
    } catch (const ReferenceError& error) {
        expired = error.kind() == ReferenceError::Kind::Closed;
    }
    check(expired, "a copied stream handle rejects access after its User has been destroyed");
}

static void sqlite_stream_contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& connection = user->make_resource<SqliteConnection>();
    auto& transport = user->make<SqliteMutationExecutor>(connection);
    auto& sql = user->make<CompiledQuery>();
    for (auto text : {"CREATE TABLE items(id INTEGER PRIMARY KEY)",
                      "INSERT INTO items VALUES(1),(2),(3),(4),(5)"}) {
        sql.sql = text;
        check(bool(transport.execute(r, sql)), "SQLite stream fixture setup");
    }
    auto& metadata = user->make<InMemoryMetadataStore>();
    EntityDescriptor item(std::allocator_arg, Allocator(r), "Item");
    item.table("items");
    item.property("id", DataType::U64).is_id = true;
    metadata.register_entity(std::move(item));
    user->set_metadata(*user, metadata);
    auto& dialect = user->make_resource<SqliteDialect>();
    auto& state = user->make<SqlExecutorState>();
    using E = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;
    auto& executor = user->make<E>(dialect, transport, metadata, state);
    auto& repo = EntityDataService<E>::for_executor(*user, "Item", *user, executor);
    auto& query = user->make<SelectQuery>("Item");
    query.comment("inspect stream records");
    query.order_asc("id").stream(2);
    auto& purpose = user->make<PurposedSelectQuery>(query, "Inspect SQLite chunks");
    auto& result = repo.fetch_stream(purpose).wait();
    check(bool(result), "SQLite stream opens through the public runtime");
    auto& stream = result.value().get();
    unsigned rows = 0, chunks = 0;
    while (auto* next = stream.wait_next()) {
        check(bool(*next), "SQLite stream yields successful chunks");
        check(next->value().chunk_index == chunks++, "SQLite chunk indices preserve order");
        for (const auto& row : next->value().rows) {
            check(row.get("id")->try_u64() == ++rows, "SQLite streamed rows preserve root order");
        }
    }
    check(rows == 5 && chunks == 3 && user->sql_logs().size() == 1,
          "stream completion releases the cursor and records one Context diagnostic");
    sql.sql = "INSERT INTO items VALUES(6)";
    check(bool(transport.execute(r, sql)), "the SQLite connection is idle after stream EOF");
}

template <class E>
concept HasPublicStream =
    requires(const EntityDataService<E>& service, const PurposedSelectQuery& query) {
        service.fetch_stream(query);
    };
static_assert(
    HasPublicStream<SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>>);
static_assert(!HasPublicStream<SqlDataServiceExecutor<MutationExecutorError>>);

static void failed_handoff_contract() {
    GlobalContext global;
    MemoryLimits limits;
    limits.max_objects = 128;
    auto user = global.make_user_context(limits);
    install(*user);
    auto& executor = user->make<StreamExecutor>();
    executor.exhaust_objects = true;
    auto& repo = EntityDataService<StreamExecutor>::for_executor(*user, "Item", *user, executor);
    auto& query = user->make<SelectQuery>("Item");
    query.comment("inspect stream records");
    auto& input = user->make<PurposedSelectQuery>(query, "Reject failed cursor handoff");
    bool exhausted = false;
    try {
        repo.fetch_stream(input).wait();
    } catch (const MemoryLimitError& error) {
        exhausted = error.kind() == MemoryLimitError::Kind::Objects;
    }
    check(exhausted && executor.calls == 1 && executor.closes == 1,
          "allocation failure after cursor creation cancels it before returning the error");
}

int main() {
    try {
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        mapped_cursor_contracts();
        close_and_domain_contracts();
        sqlite_stream_contracts();
        failed_handoff_contract();
        std::pmr::set_default_resource(old);
        std::cout << "Public entity stream contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
