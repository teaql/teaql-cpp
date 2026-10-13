#include "../support/sample_data_fixture.hpp"
#include "../generated_inheritance/include/cpp_inheritance.hpp"
#include <iostream>

using namespace teaql;
using namespace cpp_relations;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void real_sample_save() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    teaql::test::initialize_sample_database(*user_context, true);
    auto Q = user_context->queries<query_factory>();
    auto id = std::uint64_t(1);
    Ref<SampleDataReport> retained;
    for (unsigned round = 1; round <= 2; ++round) {
        const auto& result = generateSampleData(*user_context, {SampleDataScale::Tiny, 7}).wait();
        if (!result) {
            throw std::runtime_error(
                std::string(result.error().to_string(user_context->memory_resource())));
        }
        const auto& report = result.value();
        check(report.skipped.empty() && report.generated.size() == 3,
              "all business models generate after root loading");
        for (const auto& [name, count] : report.generated) {
            check(count == 20 && name.get_allocator().resource() == user_context->memory_resource(),
                  "sample report counts are per invocation and Context owned");
        }
        auto& schools = require_value(Q.schools()
                                          .comment("sample readback")
                                          .purpose("check persisted samples")
                                          .executeForList(*user_context)
                                          .wait());
        check(schools.len() == round * 20, "repeated sample calls create another batch");
        for (const auto& school : schools.data) {
            check(school.platformId() == id && school.name().starts_with("string() ") &&
                      school.version() == 1,
                  "generated samples persist through typed audited save");
        }
        retained = user_context->make_global<SampleDataReport>(report);
    }
    user_context.reset();
    auto next_user_context = global.make_user_context();
    check(next_user_context->get(next_user_context->rebind_global(retained)).generated.size() == 3,
          "explicit Global report copy survives actual Context deletion");
}

static void inherited_sample_save() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    cpp_inheritance::setup(*user_context);
    auto Q = user_context->queries<cpp_inheritance::query_factory>();
    auto id =
        cpp_inheritance::require_value(user_context->require_active_root("Platform")).get().id;
    const auto& result =
        cpp_inheritance::generateSampleData(*user_context, {SampleDataScale::Tiny, 7}).wait();
    if (!result) {
        throw std::runtime_error(
            std::string(result.error().to_string(user_context->memory_resource())));
    }
    check(result.value().generated.size() == 3 && result.value().skipped.empty(),
          "inherited sample includes ancestor dependencies and scalar defaults");
    auto& rows =
        cpp_inheritance::require_value(Q.researchAcademies()
                                           .comment("inherited sample readback")
                                           .purpose("verify ancestor fields and numeric operands")
                                           .executeForList(*user_context)
                                           .wait());
    check(rows.len() == 20, "research sample persists the source fanout");
    for (const auto& row : rows.data) {
        check(row.platformId() == id && row.name().starts_with("string() ") &&
                  row.campusCode().starts_with("string() ") && row.rank() >= 8 &&
                  row.rank() <= 27 && row.budget() >= Decimal::from_i64(8) &&
                  row.budget() <= Decimal::from_i64(27),
              "inherited field defaults survive the actual governed ledger save");
    }
}

struct SampleStage {
    bool ready = false, fail = false;
    unsigned started = 0, finished = 0, cancelled = 0;
    WakeSignal* wake = nullptr;

    void release() {
        ready = true;
        if (wake) {
            wake->notify();
        }
    }
};

struct SampleStageResource {
    using allocator_type = Allocator;
    SampleStage& stage;

    SampleStageResource(std::allocator_arg_t, Allocator, SampleStage& stage) : stage(stage) {}
};

struct DeferredRootRecipe {
    using Model = Platform;
    static constexpr bool load_references = true;
    static constexpr std::string_view name = "platform";
    using Output = Result<SmartList<Platform>, RuntimeError>;

    struct Work {
        using allocator_type = Allocator;
        Memory resource;
        SampleStage& stage;
        String host_buffer;

        Work(std::allocator_arg_t, Allocator a, SampleStage& stage)
            : resource(a.resource()), stage(stage), host_buffer(32768, 'h', resource) {
            ++stage.started;
        }

        std::optional<Output> poll(WakeSignal& wake) {
            stage.wake = &wake;
            if (!stage.ready) {
                return {};
            }
            if (stage.fail) {
                return Output::err(resource, RuntimeError::graph(resource, "root query failure"));
            }
            SmartList<Platform> rows(std::allocator_arg, Allocator(resource));
            rows.data.emplace_back();
            rows.data.back().updateId(7).updateName("deferred root");
            return Output::ok(resource, std::move(rows));
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

    static AsyncOperation<Output>& query(UserContext& user_context) {
        auto& stage =
            require_value(user_context.require_resource<SampleStageResource>()).get().stage;
        return make_operation<Output, Work>(user_context.memory_resource(), stage);
    }
};

static void deferred_sample_query(bool fail, bool cancel, bool explicit_cancel) {
    GlobalContext global;
    SampleStage stage;
    stage.fail = fail;
    auto user_context = global.make_user_context();
    auto resource = user_context->make_ref<SampleStageResource>(stage);
    user_context->insert_resource(resource);
    auto& state = user_context->make<SampleDataState>();
    auto& operation = run_generated_sample_model<DeferredRootRecipe>(*user_context, state);
    check(!operation.poll() && stage.started == 1, "sample root query must really suspend");
    if (cancel) {
        if (explicit_cancel) {
            operation.cancel();
        }
        user_context.reset();
        check(stage.cancelled == 1 && stage.finished == 0 && !stage.wake,
              "pending sample query is cancelled once before arena reclamation");
        return;
    }
    auto ticket = operation.wake_signal().ticket();
    stage.release();
    check(operation.wake_signal().ticket() != ticket, "query wake reaches sample model operation");
    check(bool(operation.wait()) && stage.finished == 1 && stage.cancelled == 0 && !stage.wake,
          "query completion quiesces its host memory once");
    check(state.ids("platform").size() == (fail ? 0 : 1),
          "failed reference load defaults to an empty pool");
    user_context.reset();
    check(stage.finished == 1 && stage.cancelled == 0,
          "completed query is not cancelled during Context deletion");
}

static void sample_state_binding() {
    GlobalContext global;
    SampleStage stage;
    auto user_context = global.make_user_context();
    auto resource = user_context->make_ref<SampleStageResource>(stage);
    user_context->insert_resource(resource);
    bool rejected = false;
    {
        SampleDataState stack_state(std::allocator_arg, Allocator(user_context->memory_resource()));
        try {
            (void)run_generated_sample_model<DeferredRootRecipe>(*user_context, stack_state);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
    }
    check(rejected && stage.started == 0, "sample operation cannot retain a stack state");
    auto state = user_context->make_ref<SampleDataState>();
    auto& operation =
        run_generated_sample_model<DeferredRootRecipe>(*user_context, user_context->get(state));
    check(!operation.poll(), "state destruction contract must suspend a child");
    user_context->destroy(state);
    rejected = false;
    try {
        (void)operation.poll();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected && stage.cancelled == 1 && !stage.wake,
          "destroyed sample state rejects access and quiesces the pending query");
    user_context.reset();
    check(stage.cancelled == 1, "state failure does not double-cancel during Context deletion");
}

static SampleDataFuture& local_sample_steps(UserContext& user_context) {
    const std::array<GeneratedSampleStep, 2> steps{run_generated_sample_model<DeferredRootRecipe>,
                                                   run_generated_sample_model<DeferredRootRecipe>};
    return run_generated_sample_data(user_context, SampleDataPlan::small(), steps);
}

static void deferred_sample_sequence(bool cancel) {
    GlobalContext global;
    SampleStage stage;
    auto user_context = global.make_user_context();
    auto resource = user_context->make_ref<SampleStageResource>(stage);
    user_context->insert_resource(resource);
    auto& operation = local_sample_steps(*user_context);
    check(!operation.poll() && stage.started == 1,
          "sequence copies stack descriptors and starts only its first step");
    if (cancel) {
        user_context.reset();
        check(stage.cancelled == 1 && stage.started == 1 && !stage.wake,
              "sequence cancellation leaves later steps unstarted");
        return;
    }
    auto ticket = operation.wake_signal().ticket();
    stage.release();
    check(operation.wake_signal().ticket() != ticket,
          "child wake crosses model and sequence boundaries");
    const auto& result = operation.wait();
    check(result && stage.started == 2 && stage.finished == 2 && !stage.wake,
          "sequence consumes each completed step exactly once");
    user_context.reset();
    check(stage.cancelled == 0, "completed sequence owns no live external callbacks");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        real_sample_save();
        inherited_sample_save();
        deferred_sample_query(false, false, false);
        deferred_sample_query(true, false, false);
        deferred_sample_query(false, true, false);
        deferred_sample_query(false, true, true);
        sample_state_binding();
        deferred_sample_sequence(false);
        deferred_sample_sequence(true);
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated sample SQLite execution passed\n";
}
