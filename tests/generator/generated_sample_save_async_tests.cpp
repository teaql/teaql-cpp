#include "../support/sample_data_fixture.hpp"
#include <iostream>
using namespace teaql;
using namespace cpp_relations;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct SaveStage {
    unsigned gate, started = 0, finished = 0, cancelled = 0;
    bool ready = false;
    WakeSignal* wake = nullptr;

    void release() {
        ready = true;
        if (wake) {
            wake->notify();
        }
    }
};

// Delay only the graph-saver boundary. After release the original saver still
// executes governance, transactions, SQL writes, readback and ledger cleanup.
class DeferredSaver final : public detail::DynGraphSaver {
public:
    using allocator_type = Allocator;
    using Output = detail::GraphSaveResult;

    DeferredSaver(std::allocator_arg_t,
                  Allocator a,
                  const ResourceBorrow<detail::DynGraphSaver>& delegate,
                  SaveStage& stage)
        : delegate_(std::allocator_arg, a, delegate), stage_(stage) {}

    struct Work {
        using allocator_type = Allocator;
        Memory resource;
        const UserContext& user_context;
        GraphNode node;
        std::optional<EntityRuntimeState> root;
        ResourceBorrow<detail::DynGraphSaver> delegate;
        SaveStage& stage;
        unsigned ordinal;
        String host_buffer;
        AsyncOperation<Output>* child = nullptr;
        WakeSignal::Forward forwarding;

        Work(std::allocator_arg_t,
             Allocator a,
             const UserContext& user_context,
             GraphNode node,
             const EntityRuntimeState* root,
             const ResourceBorrow<detail::DynGraphSaver>& delegate,
             SaveStage& stage)
            : resource(a.resource()), user_context(user_context),
              node(std::allocator_arg, a, std::move(node)),
              delegate(std::allocator_arg, a, delegate), stage(stage), ordinal(++stage.started),
              host_buffer(32768, 'h', resource) {
            if (root) {
                this->root.emplace(std::allocator_arg, a, *root);
            }
        }

        std::optional<Output> poll(WakeSignal& wake) {
            stage.wake = &wake;
            if (ordinal >= stage.gate && !stage.ready) {
                return {};
            }
            if (!child) {
                child = root ? &delegate.get().save_ledger_dyn(user_context, std::move(node), *root)
                             : &delegate.get().save_graph_dyn(user_context, std::move(node));
            }
            forwarding.attach(child->wake_signal(), wake);
            if (!child->poll()) {
                return {};
            }
            forwarding.reset();
            return child->take_result();
        }

        void finish_and_wait() noexcept {
            forwarding.reset();
            stage.wake = nullptr;
            ++stage.finished;
        }

        void cancel_and_wait() noexcept {
            if (host_buffer.size() != 32768 || host_buffer.back() != 'h') {
                std::terminate();
            }
            forwarding.reset();
            if (child) {
                child->cancel();
            }
            stage.wake = nullptr;
            ++stage.cancelled;
        }
    };

    AsyncOperation<Output>& save_graph_dyn(const UserContext& user_context,
                                           GraphNode node) const override {
        return make_operation<Output, Work>(user_context.memory_resource(),
                                            user_context,
                                            std::move(node),
                                            nullptr,
                                            delegate_,
                                            stage_);
    }

    AsyncOperation<Output>& save_ledger_dyn(const UserContext& user_context,
                                            GraphNode node,
                                            const EntityRuntimeState& root) const override {
        return make_operation<Output, Work>(user_context.memory_resource(),
                                            user_context,
                                            std::move(node),
                                            &root,
                                            delegate_,
                                            stage_);
    }

private:
    ResourceBorrow<detail::DynGraphSaver> delegate_;
    SaveStage& stage_;
};

static unsigned persisted(sqlite3* database, const char* table) {
    std::string query = "SELECT count(*) FROM " + std::string(table);
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(database, query.c_str(), -1, &raw, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    if (sqlite3_step(raw) != SQLITE_ROW) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return static_cast<unsigned>(sqlite3_column_int(raw, 0));
}

static void pending_sample_save(unsigned gate, bool cancel, bool explicit_cancel) {
    GlobalContext global;
    SaveStage stage{gate};
    auto user_context = global.make_user_context();
    auto* database = teaql::test::initialize_sample_database(*user_context, true);
    auto original = user_context->require_resource<detail::DynGraphSaver>();
    auto& saver = user_context->make<DeferredSaver>(require_value(original), stage);
    user_context->insert_resource(*user_context, static_cast<detail::DynGraphSaver&>(saver));
    auto& operation = generateSampleData(*user_context, {SampleDataScale::Tiny, 0});
    check(!operation.poll() && stage.started == gate && stage.finished == gate - 1,
          "sample execution suspends at the requested actual audited save");
    check(!operation.poll() && stage.started == gate,
          "repeated pending polls do not recreate an entity or allocate another ID");
    unsigned expected_device = std::min(gate - 1, 20u);
    unsigned expected_profile = gate > 20 ? std::min(gate - 21, 20u) : 0;
    unsigned expected_school = gate > 40 ? gate - 41 : 0;
    check(persisted(database, "device_data") == expected_device &&
              persisted(database, "profile_data") == expected_profile &&
              persisted(database, "school_data") == expected_school,
          "only completed prior saves persist while the current save is pending");
    if (cancel) {
        if (explicit_cancel) {
            operation.cancel();
        }
        user_context.reset();
        check(stage.cancelled == 1 && stage.finished == gate - 1 && stage.started == gate &&
                  !stage.wake,
              "cancel or actual Context deletion quiesces pending save host memory once");
        return;
    }
    auto ticket = operation.wake_signal().ticket();
    stage.release();
    check(operation.wake_signal().ticket() != ticket,
          "save wake reaches the generated sample sequence across all adapter layers");
    const auto& result = operation.wait();
    check(result && result.value().generated.size() == 3 && result.value().skipped.empty() &&
              stage.started == 60 && stage.finished == 60 && stage.cancelled == 0 && !stage.wake,
          "resumption completes all original saves and reports without duplicate work");
    for (const auto& [name, count] : result.value().generated) {
        check(count == 20, "sample counts include only successful saves");
    }
    check(persisted(database, "device_data") == 20 && persisted(database, "profile_data") == 20 &&
              persisted(database, "school_data") == 20,
          "resumed real saver commits the complete source fanout");
    user_context.reset();
    check(stage.cancelled == 0, "completed saves leave no pending work for Context close");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (auto gate : {1u, 23u, 41u, 60u}) {
            pending_sample_save(gate, false, false);
            pending_sample_save(gate, true, false);
            pending_sample_save(gate, true, true);
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Sample audited-save pending, wake and cleanup contracts passed\n";
}
