#include <teaql/runtime/data_service.hpp>
#include <iostream>
using namespace teaql;

struct Control {
    unsigned cancelled = 0;
    bool alive = false;
};

template <class T>
struct ForeignState {
    using allocator_type = Allocator;
    const UserContext& caller;
    Control& control;
    bool stopped = false;

    ForeignState(std::allocator_arg_t, Allocator, const UserContext& caller, Control& control)
        : caller(caller), control(control) {}

    std::optional<Result<T, unsigned>> poll(WakeSignal&) {
        return {};
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

    void cancel_and_wait() noexcept {
        if (!stopped) {
            stopped = true;
            ++control.cancelled;
            control.alive = !dynamic_cast<Arena*>(caller.memory_resource())->closed();
        }
    }
};

class ForeignExecutor final : public QueryExecutor<unsigned>, public MutationExecutor<unsigned> {
public:
    GlobalContext& global;
    const UserContext& caller;
    Control& control;
    AsyncOperation<Result<QueryResult, unsigned>>* query_child = nullptr;
    AsyncOperation<Result<MutationResult, unsigned>>* mutation_child = nullptr;

    ForeignExecutor(GlobalContext& global, const UserContext& caller, Control& control)
        : global(global), caller(caller), control(control) {}

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    AsyncOperation<Result<QueryResult, unsigned>>& query(Memory, QueryRequest) override {
        query_child = &make_operation<Result<QueryResult, unsigned>, ForeignState<QueryResult>>(
            global.memory_resource(), caller, control);
        return *query_child;
    }

    AsyncOperation<Result<MutationResult, unsigned>>& mutate(Memory, MutationRequest) override {
        mutation_child =
            &make_operation<Result<MutationResult, unsigned>, ForeignState<MutationResult>>(
                global.memory_resource(), caller, control);
        return *mutation_child;
    }

    void cleanup() noexcept {
        if (query_child) {
            query_child->cancel();
        }
        if (mutation_child) {
            mutation_child->cancel();
        }
    }
};

int main(int argc, char** argv) {
    Control control;
    GlobalContext global;
    auto user = global.make_user_context();
    auto& metadata = user->make<InMemoryMetadataStore>();
    auto& executor = global.make_resource<ForeignExecutor>(global, *user, control);
    auto& borrow = user->make<ResourceBorrow<ForeignExecutor>>(
        executor, *dynamic_cast<Arena*>(global.memory_resource()));
    auto& service = user->make<detail::RuntimeDataService<InMemoryMetadataStore, ForeignExecutor>>(
        metadata, borrow);
    bool rejected = false;
    try {
        if (argc == 2 && std::string_view(argv[1]) == "mutation") {
            service.with_mutation_intent(
                MutationIntent::create(user->memory_resource(), "Inspect foreign mutation cleanup")
                    .value());
            auto& command = user->make<InsertCommand>("Item");
            (void)service.insert(command).poll();
        } else {
            auto& query = user->make<SelectQuery>("Item");
            query.comment("Inspect foreign query cleanup")
                .purpose("Reject a foreign operation domain");
            (void)service.fetch_all(query).poll();
        }
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    const bool stopped_before_rejection = control.cancelled == 1 && control.alive;
    // A failing negative control explicitly cancels before User teardown.
    executor.cleanup();
    user.reset();
    std::cout << "{\"rejected\":" << (rejected ? "true" : "false")
              << ",\"cancelled_before_rejection\":" << (stopped_before_rejection ? "true" : "false")
              << ",\"user_bytes\":" << global.live_user_bytes() << "}\n";
    return rejected && stopped_before_rejection ? 0 : 1;
}
