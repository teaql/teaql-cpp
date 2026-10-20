#include "../generated_constants_numeric/include/cpp_constants_numeric.hpp"
#include <sqlite3.h>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

using namespace teaql;
using NativeError = MutationExecutorError;
using TerminalResult = Result<std::monostate, NativeError>;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct SaveControl {
    unsigned gate, started = 0, completed = 0, cancelled = 0, joined = 0;
    bool ready = false, fail = false, scope_ok = true, destroyed = false;
    std::atomic<unsigned> workers{0}, reads{0};
    WakeSignal* wake = nullptr;
};

class GatedTransaction final : public SqlTransportTransaction<NativeError> {
public:
    using allocator_type = Allocator;

    GatedTransaction(std::allocator_arg_t,
                     Allocator a,
                     SqlTransportTransaction<NativeError>& delegate,
                     const UserContext& user_context,
                     SaveControl& control)
        : SqlTransportTransaction<NativeError>(a.resource()), delegate_(delegate),
          user_context_(user_context), control_(control) {}

    struct Commit {
        using allocator_type = Allocator;
        Memory resource;
        GatedTransaction& owner;
        unsigned ordinal;
        String host_buffer;
        std::mutex mutex;
        std::condition_variable condition;
        bool stop = false, started = false;
        std::thread worker;
        AsyncOperation<TerminalResult>* child = nullptr;
        WakeSignal::Forward forwarding;

        Commit(std::allocator_arg_t, Allocator a, GatedTransaction& owner)
            : resource(a.resource()), owner(owner), ordinal(++owner.control_.started),
              host_buffer(32768, 'h', resource) {}

        ~Commit() {
            join();
        }

        void start_worker() {
            worker = std::thread([this] {
                std::unique_lock lock(mutex);
                ++owner.control_.workers;
                if (host_buffer.size() != 32768 || host_buffer.back() != 'h') {
                    std::terminate();
                }
                ++owner.control_.reads;
                started = true;
                condition.notify_all();
                condition.wait(lock, [this] {
                    return stop;
                });
                if (host_buffer.front() != 'h') {
                    std::terminate();
                }
                --owner.control_.workers;
            });
            std::unique_lock lock(mutex);
            condition.wait(lock, [this] {
                return started;
            });
        }

        void join() noexcept {
            if (worker.joinable()) {
                {
                    std::lock_guard lock(mutex);
                    stop = true;
                }
                condition.notify_all();
                worker.join();
                ++owner.control_.joined;
            }
        }

        std::optional<TerminalResult> poll(WakeSignal& wake) {
            auto& control = owner.control_;
            control.scope_ok &= detail::is_generated_schema_bootstrap(owner.user_context_);
            if (ordinal == control.gate) {
                if (!started) {
                    start_worker();
                }
                control.wake = &wake;
                if (!control.ready) {
                    return {};
                }
                join();
                control.wake = nullptr;
                if (control.fail) {
                    owner.delegate_.cancel_sql();
                    return TerminalResult::err(
                        resource,
                        NativeError::sqlite(resource, SQLITE_ABORT, "fixture commit failure"));
                }
            }
            if (!child) {
                child = &owner.delegate_.commit_sql(resource);
            }
            forwarding.attach(child->wake_signal(), wake);
            if (!child->poll()) {
                return {};
            }
            forwarding.reset();
            return child->take_result();
        }

        void finish_and_wait() noexcept {
            join();
            owner.control_.wake = nullptr;
            ++owner.control_.completed;
        }

        void cancel_and_wait() noexcept {
            owner.control_.scope_ok &= detail::is_generated_schema_bootstrap(owner.user_context_);
            join();
            if (child) {
                child->cancel();
            }
            forwarding.reset();
            owner.delegate_.cancel_sql();
            owner.control_.wake = nullptr;
            ++owner.control_.cancelled;
        }
    };

protected:
    DataServiceOperationResult<Rows, NativeError>&
    fetch_all_compact_sql_transaction(Memory r, const CompiledQuery& query) override {
        return delegate_.fetch_all_compact_sql(r, query);
    }

    DataServiceOperationResult<std::uint64_t, NativeError>&
    execute_sql_transaction(Memory r, const CompiledQuery& query) override {
        return delegate_.execute_sql(r, query);
    }

    AsyncOperation<TerminalResult>& commit_sql_transaction(Memory r) override {
        return make_resource_operation<TerminalResult, Commit>(r, *this);
    }

    AsyncOperation<TerminalResult>& rollback_sql_transaction(Memory r) override {
        return delegate_.rollback_sql(r);
    }

    void cancel_sql_transaction() noexcept override {
        delegate_.cancel_sql();
    }

private:
    SqlTransportTransaction<NativeError>& delegate_;
    const UserContext& user_context_;
    SaveControl& control_;
};

class GatedTransport final : public SqlTransactionTransport<NativeError> {
public:
    using allocator_type = Allocator;

    GatedTransport(std::allocator_arg_t,
                   Allocator,
                   SqliteMutationExecutor& delegate,
                   const UserContext& user_context,
                   SaveControl& control)
        : delegate_(delegate), user_context_(user_context), control_(control) {}

    DataServiceOperationResult<Rows, NativeError>&
    fetch_all_compact_sql(Memory r, const CompiledQuery& query) override {
        return delegate_.fetch_all_compact_sql(r, query);
    }

    DataServiceOperationResult<std::uint64_t, NativeError>&
    execute_sql(Memory r, const CompiledQuery& query) override {
        return delegate_.execute_sql(r, query);
    }

    struct Begin {
        using allocator_type = Allocator;
        using Output = Result<SqlTransactionHandle<NativeError>, NativeError>;
        Memory resource;
        GatedTransport& owner;
        AsyncOperation<Output>& child;
        WakeSignal::Forward forwarding;

        Begin(std::allocator_arg_t, Allocator a, GatedTransport& owner)
            : resource(a.resource()), owner(owner), child(owner.delegate_.begin_sql(resource)) {}

        std::optional<Output> poll(WakeSignal& wake) {
            forwarding.attach(child.wake_signal(), wake);
            if (!child.poll()) {
                return {};
            }
            forwarding.reset();
            auto result = child.take_result();
            if (!result) {
                return Output::err(resource, std::move(result).error());
            }
            auto& transaction = detail::async_arena(resource).make_resource<GatedTransaction>(
                result.value().get(), owner.user_context_, owner.control_);
            return Output::ok(resource,
                              SqlTransactionHandle<NativeError>(
                                  std::allocator_arg, Allocator(resource), transaction));
        }

        void finish_and_wait() noexcept {
            forwarding.reset();
        }

        void cancel_and_wait() noexcept {
            child.cancel();
            forwarding.reset();
        }
    };

    DataServiceOperationResult<SqlTransactionHandle<NativeError>, NativeError>&
    begin_sql(Memory r) override {
        return make_operation<Result<SqlTransactionHandle<NativeError>, NativeError>, Begin>(r,
                                                                                             *this);
    }

private:
    SqliteMutationExecutor& delegate_;
    const UserContext& user_context_;
    SaveControl& control_;
};

struct CleanupWitness {
    using allocator_type = Allocator;
    sqlite3* database;
    SaveControl& control;

    CleanupWitness(std::allocator_arg_t, Allocator, sqlite3* database, SaveControl& control)
        : database(database), control(control) {}

    ~CleanupWitness() {
        if (control.workers != 0 || !sqlite3_get_autocommit(database)) {
            std::terminate();
        }
        for (auto* statement = sqlite3_next_stmt(database, nullptr); statement;
             statement = sqlite3_next_stmt(database, statement)) {
            if (sqlite3_stmt_busy(statement)) {
                std::terminate();
            }
        }
        control.destroyed = true;
    }
};

static unsigned rows(sqlite3* database) {
    sqlite3_stmt* raw = nullptr;
    check(
        sqlite3_prepare_v2(
            database,
            "SELECT (SELECT count(*) FROM platform_data) + (SELECT count(*) FROM priority_data) + (SELECT count(*) FROM label_data)",
            -1,
            &raw,
            nullptr) == SQLITE_OK,
        "row count prepares");
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    check(sqlite3_step(raw) == SQLITE_ROW, "row count executes");
    return static_cast<unsigned>(sqlite3_column_int(raw, 0));
}

enum class Action { Resume, Fail, Cancel, DeleteContext };

static void pending_bootstrap(unsigned gate, Action action) {
    GlobalContext global;
    SaveControl control{gate};
    auto& connection = global.make_resource<SqliteConnection>();
    auto& native = global.make<SqliteMutationExecutor>(connection);
    auto* database = connection.native_handle();
    auto user_context = global.make_user_context();
    user_context->disable_sql_log();
    user_context->make_resource<CleanupWitness>(database, control);
    auto& module = cpp_constants_numeric::moduleWithBehaviorsAndCheckers(*user_context);
    module.apply_to(*user_context);
    user_context->set_metadata(*user_context, module.metadata);
    use_sqlite_provider(*user_context, global, native);
    auto& ids = user_context->make<SqliteIdSpaceGenerator>(native);
    user_context->set_internal_id_generator(*user_context, ids);
    auto& transport = user_context->make<GatedTransport>(native, *user_context, control);
    auto& state = user_context->make<SqlExecutorState>();
    using Executor = SqlDataServiceExecutor<NativeError, GatedTransport>;
    auto executor = user_context->make_ref<Executor>(
        user_context->require_resource<SqliteDialect>().value().get(),
        transport,
        module.metadata,
        state);
    // Generated Q resolves its exact domain executor. Only transaction saving
    // uses the gated adapter; ordinary reads retain the generated executor.
    auto& query_executor = user_context->make<cpp_constants_numeric::ServiceRuntimeExecutor>(
        user_context->require_resource<SqliteDialect>().value().get(),
        native,
        module.metadata,
        state);
    user_context->insert_resource(*user_context, query_executor);
    user_context->register_executor(executor);
    auto& operation = user_context->ensure_schema();
    check(!operation.poll() && control.started == gate && control.completed == gate - 1 &&
              control.workers == 1 && control.reads == 1 && !sqlite3_get_autocommit(database),
          "actual bootstrap write is pending before commit with User-owned host memory in use");
    check(rows(database) == gate, "pending transaction has written its actual modeled row");
    check(!operation.poll() && control.started == gate && control.reads == 1,
          "repeated polls do not duplicate writes, commits or host workers");
    check(!detail::is_generated_schema_bootstrap(*user_context),
          "pending commit does not retain bootstrap privilege between polls");
    if (action == Action::Cancel || action == Action::DeleteContext) {
        if (action == Action::Cancel) {
            operation.cancel();
            check(sqlite3_get_autocommit(database) && control.workers == 0,
                  "explicit cancel joins native host work and abandons the transaction");
        }
        user_context.reset();
        check(
            control.cancelled == 1 && control.joined == 1 && control.destroyed &&
                control.scope_ok && !control.wake && control.workers == 0 &&
                rows(database) == gate - 1,
            "Context teardown joins exactly once, rolls back pending row and retains prior commits");
        return;
    }
    control.ready = true;
    control.fail = action == Action::Fail;
    auto ticket = operation.wake_signal().ticket();
    control.wake->notify();
    check(operation.wake_signal().ticket() != ticket,
          "commit wake reaches schema/bootstrap parents");
    const auto& result = operation.wait();
    if (action == Action::Fail) {
        check(!result &&
                  result.error()
                          .to_string(user_context->memory_resource())
                          .find("fixture commit failure") != String::npos &&
                  rows(database) == gate - 1,
              "terminal failure joins host memory and rolls back only the current transaction");
    } else {
        check(result && rows(database) == 5 && control.started == 5 && control.completed == 5 &&
                  user_context->require_active_root("Platform").value().get().id == 1,
              "resumption commits all constants and installs the actual generated root");
    }
    user_context.reset();
    check(
        control.joined == 1 && control.cancelled == 0 && control.destroyed && control.scope_ok &&
            !control.wake && control.workers == 0 && sqlite3_get_autocommit(database),
        "terminal operations leave no callback, worker, statement lease or transaction at destruction");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (unsigned gate : {1u, 2u, 3u, 4u, 5u}) {
            for (auto action :
                 {Action::Resume, Action::Fail, Action::Cancel, Action::DeleteContext}) {
                pending_bootstrap(gate, action);
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "20 active bootstrap save cleanup scenarios passed\n";
}
