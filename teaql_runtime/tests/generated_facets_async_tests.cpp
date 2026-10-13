#include "../../tests/support/dynamic_json_runtime_fixture.hpp"
#include <iostream>
using namespace cpp_dynamic_json;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct DeferredStage {
    bool ready = false;
    unsigned started = 0, finished = 0, cancelled = 0;
    WakeSignal* wake = nullptr;

    void release() {
        ready = true;
        if (wake) {
            wake->notify();
        }
    }
};

struct DeferredExecutor : QueryExecutor<String>,
                          MutationExecutor<String>,
                          TransactionExecutor<String> {
    using allocator_type = Allocator;
    Memory resource;
    DeferredStage& outer;
    DeferredStage& facet;

    DeferredExecutor(std::allocator_arg_t, Allocator a, DeferredStage& outer, DeferredStage& facet)
        : resource(a.resource()), outer(outer), facet(facet) {}

    Memory memory_resource() const {
        return resource;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Work {
        using allocator_type = Allocator;
        using Output = Result<QueryResult, String>;
        Memory resource;
        DeferredStage& stage;
        String entity;
        String host_buffer;

        Work(std::allocator_arg_t, Allocator a, DeferredStage& stage, std::string_view entity)
            : resource(a.resource()), stage(stage), entity(entity, resource),
              host_buffer(32768, 'h', resource) {
            ++stage.started;
        }

        std::optional<Output> poll(WakeSignal& wake) {
            stage.wake = &wake;
            if (!stage.ready) {
                return {};
            }
            QueryResult result(std::allocator_arg,
                               Allocator(resource),
                               ExecutionMetadata::unrecorded_query(resource, 0));
            result.rows.emplace_back();
            result.rows.back().insert("id",
                                      Value(resource, std::int64_t(entity == "Order" ? 10 : 2)));
            result.rows.back().insert("version", Value(resource, std::int64_t(1)));
            return Output::ok(resource, std::move(result));
        }

        void finish_and_wait() noexcept {
            stage.wake = nullptr;
            ++stage.finished;
        }

        void cancel_and_wait() noexcept {
            if (host_buffer.size() != 32768 || host_buffer.back() != 'h') {
                std::terminate();
            }
            stage.wake = nullptr;
            ++stage.cancelled;
        }
    };

    DataServiceOperationResult<QueryResult, String>& query(Memory r,
                                                           QueryRequest request) override {
        auto& stage = request.query.entity == "Order" ? outer : facet;
        return make_operation<Work::Output, Work>(r, stage, request.query.entity);
    }

    DataServiceOperationResult<TransactionHandle<String>, String>& begin(Memory) override {
        throw std::logic_error("Unused transaction in deferred facet test");
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("Unused mutation in deferred facet test");
    }
};

using OuterQuery = EntityQuery<Order, DeferredExecutor>;
using FacetQuery = EntityQuery<Person, DeferredExecutor>;

static auto deferred_request(UserContext& user_context) {
    return OuterQuery(user_context, true)
        .facetBy("customer", "_people", FacetQuery(user_context, true).comment("deferred facet"))
        .comment("deferred outer")
        .purpose("verify facet cleanup");
}

static void initialize(UserContext& user_context, DeferredStage& outer, DeferredStage& facet) {
    teaql::test::initialize_dynamic_json_database(user_context);
    auto reference = user_context.make_ref<DeferredExecutor>(outer, facet);
    user_context.register_executor(reference);
}

static void deferred_completion() {
    GlobalContext global;
    DeferredStage outer, facet;
    auto user_context = global.make_user_context();
    initialize(*user_context, outer, facet);
    auto query = deferred_request(*user_context);
    auto& operation = query.executeForList(*user_context);
    check(!operation.poll() && outer.started == 1 && facet.started == 0,
          "facet work must not start before the outer query completes");
    auto ticket = operation.wake_signal().ticket();
    outer.release();
    check(operation.wake_signal().ticket() != ticket, "outer wake must reach the awaiting list");
    check(!operation.poll() && outer.finished == 1 && facet.started == 1,
          "completed outer rows remain owned while a facet actually suspends");
    auto copied = query;
    copied.facetBy(
        "customer", "_added_later", FacetQuery(*user_context, true).comment("new facet"));
    ticket = operation.wake_signal().ticket();
    facet.release();
    check(operation.wake_signal().ticket() != ticket, "facet wake must reach the awaiting list");
    const auto& result = operation.wait();
    check(
        result && result.value().facets.size() == 1 &&
            result.value().facets.at("_people").data.size() == 1 && outer.finished == 1 &&
            facet.finished == 1,
        "completion attaches the original snapshot, consumes results and finishes both children once");
}

static void cancelled_context(unsigned phase, bool explicit_cancel) {
    GlobalContext global;
    DeferredStage outer, facet;
    auto user_context = global.make_user_context();
    initialize(*user_context, outer, facet);
    auto& operation = deferred_request(*user_context).executeForList(*user_context);
    check(!operation.poll(), "outer cancellation test must actually suspend");
    if (phase == 1) {
        outer.release();
        check(!operation.poll() && facet.started == 1,
              "facet cancellation test must actually suspend");
    }
    if (explicit_cancel) {
        operation.cancel();
    }
    user_context.reset();
    check((phase == 0 ? outer.cancelled : facet.cancelled) == 1 && !outer.wake && !facet.wake,
          "actual Context destruction cancels the live child once before reclaiming host memory");
    check(phase == 0 ? facet.started == 0 : outer.finished == 1,
          "cancellation preserves the already-completed and never-started stage boundaries");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        deferred_completion();
        for (unsigned phase = 0; phase < 2; ++phase) {
            cancelled_context(phase, false);
            cancelled_context(phase, true);
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Deferred facet wake and Context cleanup contracts passed\n";
}
