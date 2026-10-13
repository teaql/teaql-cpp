#include <teaql/data_service.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

struct State {
    Memory owner;
    unsigned calls = 0, polls = 0, notifications = 0;
    std::size_t chunk_size = 0;
    Vector<String> requests;

    explicit State(Memory owner) : owner(owner), requests(owner) {}
};

template <class T>
struct Failure {
    using allocator_type = Allocator;
    State& state;

    Failure(std::allocator_arg_t, Allocator, State& state) : state(state) {}

    std::optional<Result<T, int>> poll(WakeSignal&) {
        ++state.polls;
        return Result<T, int>::err(state.owner, 1);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct EmptyStream {
    using allocator_type = Allocator;
    State& state;

    EmptyStream(std::allocator_arg_t, Allocator, State& state) : state(state) {}

    std::optional<std::optional<Result<StreamChunk, int>>> poll_next(WakeSignal&) {
        ++state.polls;
        return std::optional<Result<StreamChunk, int>>{};
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct Provider final : QueryExecutor<int>, StreamQueryExecutor<int>, GuardedMutationExecutor<int> {
    UserContext& user;
    State& state;

    Provider(UserContext& user, State& state) : user(user), state(state) {}

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    template <class T>
    void record(const T& request) {
        ++state.calls;
        state.requests.push_back(debug_string(state.owner, request));
    }

    template <class T>
    DataServiceOperationResult<T, int>& failure() {
        return make_operation<Result<T, int>, Failure<T>>(user, state);
    }

    DataServiceOperationResult<QueryResult, int>& query(Memory, QueryRequest request) override {
        record(request);
        return failure<QueryResult>();
    }

    QueryStream<int>& query_stream(Memory, QueryRequest request, std::size_t size) override {
        record(request);
        state.chunk_size = size;
        return make_stream<Result<StreamChunk, int>, EmptyStream>(user, state);
    }

    DataServiceOperationResult<MutationResult, int>& mutate(Memory,
                                                            MutationRequest request) override {
        record(request);
        return failure<MutationResult>();
    }

    DataServiceOperationResult<MutationResult, int>&
    mutate_guarded(Memory, GuardedMutationRequest request) override {
        record(request);
        return failure<MutationResult>();
    }
};

static Json counters(const State& state) {
    Json::Object out(state.owner);
    out.emplace("calls", Json(state.owner, state.calls));
    out.emplace("polls", Json(state.owner, state.polls));
    out.emplace("notifications", Json(state.owner, state.notifications));
    out.emplace("chunk_size", Json(state.owner, std::uint64_t(state.chunk_size)));
    return Json(state.owner, std::move(out));
}

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    State state(owner);
    auto& provider = user.make_resource<Provider>(user, state);
    auto name = text(field(input, "name"));
    auto flags = std::get<std::uint64_t>(field(input, "flags").data);
    SelectQuery query(std::allocator_arg, Allocator(owner), name);
    query.comment(name).purpose("inspect executor default fixture");
    QueryRequest request(std::allocator_arg, Allocator(owner), query, flags & 1, flags & 2);
    InsertCommand insert(std::allocator_arg, Allocator(owner), name);
    insert.trace_chain.emplace_back(TraceKind::Comment, name, name, std::nullopt);
    MutationRequest mutation(
        std::allocator_arg, Allocator(owner), insert, "inspect executor mutation default fixture");
    auto observer =
        std::make_obj_using_allocator<ExecutionObserver>(Allocator(owner), [&](ExecutionMetadata) {
            ++state.notifications;
        });
    std::optional<ExecutionObserver> optional;
    if (std::get<bool>(field(input, "observer").data)) {
        optional.emplace(observer);
    }
    auto& queried = provider.query_observed(owner, detail::clone_in(owner, request), optional);
    auto& stream =
        provider.query_stream_observed(owner,
                                       std::move(request),
                                       std::get<std::uint64_t>(field(input, "chunk_size").data),
                                       observer);
    auto& mutated = provider.mutate_observed(owner, detail::clone_in(owner, mutation), optional);
    GuardedMutationRequest guarded(
        std::allocator_arg, Allocator(owner), mutation, Expr::eq(owner, "owner", std::uint64_t(7)));
    auto& guard = provider.mutate_guarded_observed(owner, std::move(guarded), optional);
    auto before = counters(state);
    Json::Array results(owner);
    if (std::get<bool>(field(input, "poll").data)) {
        results.emplace_back(queried.wait().is_err());
        results.emplace_back(!stream.wait_next());
        results.emplace_back(mutated.wait().is_err());
        results.emplace_back(guard.wait().is_err());
    }
    // Fixture states borrow stack counters; quiesce before the counters leave.
    queried.cancel();
    stream.cancel();
    mutated.cancel();
    guard.cancel();
    Json::Array requests(owner);
    for (const auto& recorded : state.requests) {
        requests.emplace_back(recorded);
    }
    Json::Object output(owner);
    output.emplace("before", std::move(before));
    output.emplace("after", counters(state));
    output.emplace("results", Json(owner, std::move(results)));
    output.emplace("requests", Json(owner, std::move(requests)));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid executor-default input");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
