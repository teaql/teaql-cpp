#include <teaql/data_service/executor.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}
enum class Error { Query, Mutation, Guard, Schema, Id, Begin, Commit, Rollback };

template <class T>
struct Ready {
    using allocator_type = Allocator;
    T output;
    int* polls;

    Ready(std::allocator_arg_t, Allocator a, T&& output, int& polls)
        : output(detail::move_in(a.resource(), std::move(output))), polls(&polls) {}

    std::optional<T> poll(WakeSignal&) {
        ++*polls;
        return std::move(output);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

template <class T>
DataServiceOperationResult<T, Error>&
ready(UserContext& context, Result<T, Error> value, int& polls) {
    return make_operation<Result<T, Error>, Ready<Result<T, Error>>>(
        context, std::move(value), polls);
}

struct Tx final : DataServiceTransaction<Error> {
    void cancel_transaction() noexcept override {}

    UserContext& context;
    int& polls;
    int queries = 0, mutations = 0;

    explicit Tx(UserContext& c, int& polls)
        : DataServiceTransaction(c.memory_resource()), context(c), polls(polls) {}

    DataServiceCapabilities capabilities() const override {
        DataServiceCapabilities c;
        c.query = c.mutation = c.transaction = true;
        return c;
    }

    DataServiceOperationResult<QueryResult, Error>& query_transaction(Memory r,
                                                                      QueryRequest) override {
        ++queries;
        return ready(context, Result<QueryResult, Error>::err(r, Error::Query), polls);
    }

    DataServiceOperationResult<MutationResult, Error>&
    mutate_transaction(Memory r, MutationRequest) override {
        ++mutations;
        return ready(context, Result<MutationResult, Error>::err(r, Error::Mutation), polls);
    }

    DataServiceOperationResult<std::monostate, Error>& commit_transaction(Memory r) override {
        return ready(context, Result<std::monostate, Error>::err(r, Error::Commit), polls);
    }

    DataServiceOperationResult<std::monostate, Error>& rollback_transaction(Memory r) override {
        return ready(context, Result<std::monostate, Error>::ok(r, {}), polls);
    }
};

struct Chunks {
    using allocator_type = Allocator;
    Memory resource;
    std::size_t next = 0;

    Chunks(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    std::optional<std::optional<Result<StreamChunk, Error>>> poll_next(WakeSignal&) {
        if (next == 2) {
            return std::optional<Result<StreamChunk, Error>>{};
        }
        StreamChunk chunk(std::allocator_arg, Allocator(resource), next, next == 1);
        ++next;
        return std::optional<Result<StreamChunk, Error>>(
            Result<StreamChunk, Error>::ok(resource, std::move(chunk)));
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct Provider final : QueryExecutor<Error>,
                        StreamQueryExecutor<Error>,
                        GuardedMutationExecutor<Error>,
                        TransactionExecutor<Error>,
                        SchemaExecutor<Error>,
                        IdGeneratorExecutor<Error>,
                        SchemaProvider {
    UserContext& context;
    EntityDescriptor& descriptor;
    int calls = 0, polls = 0;
    std::size_t chunk_size = 0;
    bool saw_guard = false;

    Provider(UserContext& context, EntityDescriptor& descriptor)
        : context(context), descriptor(descriptor) {}

    DataServiceCapabilities capabilities() const override {
        DataServiceCapabilities c;
        c.query = c.mutation = c.transaction = c.schema = c.id_generation = true;
        return c;
    }

    DataServiceOperationResult<QueryResult, Error>& query(Memory r, QueryRequest request) override {
        ++calls;
        check(request.query.entity == "Customer", "query request reaches executor");
        return ready(context, Result<QueryResult, Error>::err(r, Error::Query), polls);
    }

    QueryStream<Error>& query_stream(Memory, QueryRequest, std::size_t size) override {
        ++calls;
        chunk_size = size;
        return make_stream<Result<StreamChunk, Error>, Chunks>(context);
    }

    DataServiceOperationResult<MutationResult, Error>& mutate(Memory r,
                                                              MutationRequest request) override {
        ++calls;
        check(request.comment() == "insert reason", "mutation trace reaches executor");
        return ready(context, Result<MutationResult, Error>::err(r, Error::Mutation), polls);
    }

    DataServiceOperationResult<MutationResult, Error>&
    mutate_guarded(Memory r, GuardedMutationRequest request) override {
        ++calls;
        saw_guard = request.guard == Expr::eq(r, "owner", std::uint64_t(7));
        return ready(context, Result<MutationResult, Error>::err(r, Error::Guard), polls);
    }

    DataServiceOperationResult<TransactionHandle<Error>, Error>& begin(Memory r) override {
        ++calls;
        auto& tx = context.make_resource<Tx>(context, polls);
        TransactionHandle<Error> handle(std::allocator_arg, Allocator(r), tx);
        return ready(
            context, Result<TransactionHandle<Error>, Error>::ok(r, std::move(handle)), polls);
    }

    DataServiceOperationResult<SchemaResult, Error>& ensure_schema(Memory r,
                                                                   SchemaRequest request) override {
        ++calls;
        check(request.entity_name == "Customer", "schema invocation carries entity name");
        return ready(context, Result<SchemaResult, Error>::ok(r, SchemaResult{true}), polls);
    }

    DataServiceOperationResult<std::uint64_t, Error>& next_id(Memory r,
                                                              std::string_view entity) override {
        ++calls;
        check(entity == "Customer", "ID invocation carries entity name");
        return ready(context, Result<std::uint64_t, Error>::ok(r, 123), polls);
    }

    const EntityDescriptor* get_entity(std::string_view name) const override {
        return name == descriptor.name ? &descriptor : nullptr;
    }
};

static void contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& descriptor = global.make<EntityDescriptor>("Customer");
    auto& provider = user->make_resource<Provider>(*user, descriptor);
    auto& query = user->make<SelectQuery>("Customer");
    query.comment("Exercise the executor SPI fixture");
    query.purpose("Check independent provider and transaction execution");
    query.filter(Expr::eq(r, "name", "UNKNOWN-SECRET"));
    auto request = [&] {
        return QueryRequest(std::allocator_arg, Allocator(r), query, false, true);
    };
    int observed = 0;
    auto observer =
        std::make_obj_using_allocator<ExecutionObserver>(Allocator(r), [&](ExecutionMetadata) {
            ++observed;
        });
    auto intent = provider.query_log_intent(r, query);
    Vector<String> secrets(r);
    intent.extend_secrets(true, secrets);
    check(provider.calls == 0 && provider.polls == 0 && secrets.size() == 1 &&
              secrets[0] == "UNKNOWN-SECRET",
          "default intent must not execute or grant debug plaintext");
    auto& operation = provider.query_observed(r, request(), observer);
    check(provider.polls == 0, "SPI returns an unpolled future");
    check(operation.wait().error() == Error::Query && observed == 0,
          "default query observer preserves third-party executors without invented metadata");
    auto& insert = user->make<InsertCommand>("Customer");
    insert.audit_as("insert reason");
    auto mutation = [&] {
        return MutationRequest(std::allocator_arg, Allocator(r), insert, "insert reason");
    };
    check(provider.mutate_observed(r, mutation(), observer).wait().error() == Error::Mutation &&
              observed == 0,
          "default mutation observer delegates once");
    GuardedMutationRequest guarded(
        std::allocator_arg, Allocator(r), mutation(), Expr::eq(r, "owner", std::uint64_t(7)));
    check(provider.mutate_guarded_observed(r, std::move(guarded), observer).wait().error() ==
                  Error::Guard &&
              provider.saw_guard && observed == 0,
          "guard is an independent trusted predicate at the provider boundary");
    auto& stream = provider.query_stream_observed(r, request(), 17, observer);
    auto* a = stream.wait_next();
    auto* b = stream.wait_next();
    check(a && b && a->value().chunk_index == 0 && b->value().chunk_index == 1 &&
              b->value().is_last && !stream.wait_next() && provider.chunk_size == 17 &&
              observed == 0,
          "stream SPI preserves chunk size and metadata-free default observer");
    auto& begin = provider.begin(r);
    const auto& handle = begin.wait().value();
    auto& tx = handle.get();
    check(&handle.get_as<QueryExecutor<Error>>() == static_cast<QueryExecutor<Error>*>(&tx),
          "erased transaction retains its implemented query capability");
    bool missing_guard = false;
    try {
        (void)handle.get_as<GuardedMutationExecutor<Error>>();
    } catch (const std::logic_error&) {
        missing_guard = true;
    }
    check(missing_guard && !tx.consumed(),
          "ordinary transaction does not advertise or emulate guarded mutation");
    check(tx.query(r, request()).wait().error() == Error::Query &&
              tx.mutate(r, mutation()).wait().error() == Error::Mutation,
          "associated transaction supports query and mutation independently");
    bool cross_domain = false;
    try {
        auto illegal = detail::clone_in(global.memory_resource(), handle);
        (void)illegal;
    } catch (const std::invalid_argument&) {
        cross_domain = true;
    }
    check(cross_domain, "active transactions cannot be retained by cross-domain value copying");
    bool wrong_domain = false;
    try {
        tx.commit(global.memory_resource());
    } catch (const std::invalid_argument&) {
        wrong_domain = true;
    }
    check(wrong_domain && !tx.consumed(),
          "invalid lifetime destination cannot consume or extend a transaction");
    auto& pending_query = tx.query(r, request());
    bool pending_rejected = false;
    try {
        tx.commit(r);
    } catch (const std::logic_error&) {
        pending_rejected = true;
    }
    check(pending_rejected && !tx.consumed(),
          "pending query borrow prevents consuming its transaction");
    pending_query.cancel();
    auto& commit = tx.commit(r);
    check(tx.consumed() && commit.status() == AsyncStatus::Pending,
          "terminal invocation consumes transaction before polling");
    bool query_rejected = false, rollback_rejected = false;
    try {
        tx.query(r, request());
    } catch (const std::logic_error&) {
        query_rejected = true;
    }
    try {
        tx.rollback(r);
    } catch (const std::logic_error&) {
        rollback_rejected = true;
    }
    check(query_rejected && rollback_rejected && commit.wait().error() == Error::Commit,
          "commit failure keeps transaction consumed and preserves error type");
    const auto& second = provider.begin(r).wait().value();
    check(second.get().rollback(r).wait().is_ok(), "independent transaction rollback future");
    SchemaRequest schema(std::allocator_arg, Allocator(r), "Customer");
    check(provider.ensure_schema(r, std::move(schema)).wait().value().changed,
          "schema result preserves changed flag");
    check(provider.next_id(r, "Customer").wait().value() == 123 &&
              provider.get_entity("Customer") == &descriptor && !provider.get_entity("Missing"),
          "ID generation and schema lookup remain independent provider SPI");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        contracts();
        std::pmr::set_default_resource(previous);
        std::cout << "Executor SPI contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
