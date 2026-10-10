#pragma once
// Fault-injection transport from the original SQL diagnostic tests.
// It exercises the production executor; it does not emulate executor behavior.
#include <teaql/sql/executor.hpp>
#include <teaql/runtime/context.hpp>
#include <algorithm>

namespace original_sql {
using namespace teaql;
using Outcome = SqlExecutionOutcome;
using Error = SqlExecutorError<String>;

inline void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class Dialect final : public SqlDialect {
public:
    DatabaseKind kind() const override {
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
    using allocator_type = Allocator;

    Schema(std::allocator_arg_t, Allocator, const EntityDescriptor& entity) : entity_(entity) {}

    const EntityDescriptor* get_entity(std::string_view name) const override {
        return name == "Customer" ? &entity_ : nullptr;
    }

private:
    const EntityDescriptor& entity_;
};

enum class Mode {
    Success,
    QueryFailure,
    PendingQuery,
    QueryPanic,
    FailWrite,
    PendingWrite,
    FailRead,
    EmptyRead,
    PendingRead
};

struct Controls {
    Mode mode;
    unsigned at = 1, writes = 0, reads = 0;
    bool released = true, query_fixture = false;
};

class Transport;

struct Read {
    using allocator_type = Allocator;
    using Rows = Vector<CompactRow>;
    Memory resource;
    Controls& controls;
    const CompiledQuery& query;
    bool started = false;

    Read(std::allocator_arg_t, Allocator a, Controls& controls, const CompiledQuery& query)
        : resource(a.resource()), controls(controls), query(query) {}

    std::optional<Result<Rows, String>> poll(WakeSignal&) {
        if (!started) {
            started = true;
            ++controls.reads;
            controls.released = false;
            if (controls.query_fixture) {
                check(query.params.size() >= 1 && query.params[0].try_text() == "Riverside",
                      "query driver value unchanged");
            }
        }
        if (controls.mode == Mode::PendingQuery || controls.mode == Mode::PendingRead) {
            return {};
        }
        if (controls.mode == Mode::QueryPanic) {
            throw std::runtime_error("controlled driver unwind");
        }
        if (controls.mode == Mode::QueryFailure || controls.mode == Mode::FailRead) {
            return Result<Rows, String>::err(resource,
                                             String(controls.query_fixture
                                                        ? "original driver failure"
                                                        : "original readback error",
                                                    resource));
        }
        Rows rows(resource);
        if (!controls.query_fixture && controls.mode != Mode::EmptyRead) {
            Record record(resource);
            record.emplace("id", Value(resource, std::uint64_t(1)));
            record.emplace("name", Value(resource, "Riverside"));
            rows.emplace_back(record);
        }
        return Result<Rows, String>::ok(resource, std::move(rows));
    }

    void finish_and_wait() noexcept {
        controls.released = true;
    }

    void cancel_and_wait() noexcept {
        controls.released = true;
    }
};

struct Write {
    using allocator_type = Allocator;
    Memory resource;
    Controls& controls;
    const CompiledQuery& query;
    bool started = false;
    unsigned index = 0;

    Write(std::allocator_arg_t, Allocator a, Controls& controls, const CompiledQuery& query)
        : resource(a.resource()), controls(controls), query(query) {}

    std::optional<Result<std::uint64_t, String>> poll(WakeSignal&) {
        if (!started) {
            started = true;
            controls.released = false;
            index = ++controls.writes;
            if (std::ranges::find(query.log_context.parameter_policies,
                                  SqlParameterLogPolicy::Masked) !=
                query.log_context.parameter_policies.end()) {
                check(std::ranges::any_of(query.params,
                                          [](const Value& value) {
                                              return value.try_text() == "Riverside";
                                          }),
                      "mutation driver value unchanged");
            }
        }
        if (controls.mode == Mode::PendingWrite && index == controls.at) {
            return {};
        }
        if (controls.mode == Mode::FailWrite && index == controls.at) {
            return Result<std::uint64_t, String>::err(resource,
                                                      String("original write error", resource));
        }
        return Result<std::uint64_t, String>::ok(resource, std::uint64_t(1));
    }

    void finish_and_wait() noexcept {
        controls.released = true;
    }

    void cancel_and_wait() noexcept {
        controls.released = true;
    }
};

struct Terminal {
    using allocator_type = Allocator;
    Memory resource;

    Terminal(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    std::optional<Result<std::monostate, String>> poll(WakeSignal&) {
        return Result<std::monostate, String>::ok(resource, std::monostate{});
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

class NativeTransaction final : public SqlTransportTransaction<String> {
public:
    NativeTransaction(Memory r, Controls& controls)
        : SqlTransportTransaction<String>(r), controls_(controls) {}

protected:
    DataServiceOperationResult<Rows, String>&
    fetch_all_compact_sql_transaction(Memory r, const CompiledQuery& query) override {
        return make_operation<Result<Rows, String>, Read>(r, controls_, query);
    }

    DataServiceOperationResult<std::uint64_t, String>&
    execute_sql_transaction(Memory r, const CompiledQuery& query) override {
        return make_operation<Result<std::uint64_t, String>, Write>(r, controls_, query);
    }

    DataServiceOperationResult<std::monostate, String>& commit_sql_transaction(Memory r) override {
        return make_operation<Result<std::monostate, String>, Terminal>(r);
    }

    DataServiceOperationResult<std::monostate, String>&
    rollback_sql_transaction(Memory r) override {
        return make_operation<Result<std::monostate, String>, Terminal>(r);
    }

    void cancel_sql_transaction() noexcept override {}

private:
    Controls& controls_;
};

class Transport final : public SqlTransactionTransport<String> {
public:
    using allocator_type = Allocator;

    Transport(std::allocator_arg_t, Allocator, Controls& controls) : controls_(controls) {}

    DataServiceOperationResult<Rows, String>&
    fetch_all_compact_sql(Memory r, const CompiledQuery& query) override {
        return make_operation<Result<Rows, String>, Read>(r, controls_, query);
    }

    DataServiceOperationResult<std::uint64_t, String>&
    execute_sql(Memory r, const CompiledQuery& query) override {
        return make_operation<Result<std::uint64_t, String>, Write>(r, controls_, query);
    }

    struct Begin {
        using allocator_type = Allocator;
        Memory resource;
        Controls& controls;

        Begin(std::allocator_arg_t, Allocator a, Controls& controls)
            : resource(a.resource()), controls(controls) {}

        std::optional<Result<SqlTransactionHandle<String>, String>> poll(WakeSignal&) {
            auto& tx =
                detail::async_arena(resource).make_resource<NativeTransaction>(resource, controls);
            return Result<SqlTransactionHandle<String>, String>::ok(
                resource,
                SqlTransactionHandle<String>(std::allocator_arg, Allocator(resource), tx));
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {}
    };

    DataServiceOperationResult<SqlTransactionHandle<String>, String>& begin_sql(Memory r) override {
        return make_operation<Result<SqlTransactionHandle<String>, String>, Begin>(r, controls_);
    }

private:
    Controls& controls_;
};

using Executor = SqlDataServiceExecutor<String, Transport>;

struct Fixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user = global.make_user_context();
    Memory resource = user->memory_resource();
    Controls& controls;
    Vector<ExecutionMetadata>& entries;
    EntityDescriptor& entity;
    Dialect& dialect;
    Schema& schema;
    Transport& transport;
    SqlExecutorState& cache;
    Executor& executor;
    ExecutionObserver observer;
    unsigned held_resource_notifications = 0;

    explicit Fixture(Mode mode,
                     bool panic_sink = false,
                     bool query_fixture = false,
                     unsigned at = 1)
        : controls(user->make<Controls>(Controls{mode, at, 0, 0, !query_fixture, query_fixture})),
          entries(user->make<Vector<ExecutionMetadata>>()),
          entity(user->make<EntityDescriptor>("Customer")), dialect(user->make_resource<Dialect>()),
          schema(user->make<Schema>(entity)), transport(user->make<Transport>(controls)),
          cache(user->make<SqlExecutorState>()),
          executor(user->make<Executor>(dialect, transport, schema, cache)),
          observer(std::allocator_arg,
                   Allocator(resource),
                   [this, panic_sink](ExecutionMetadata metadata) {
                       if (!controls.released) {
                           ++held_resource_notifications;
                       }
                       entries.emplace_back(std::move(metadata));
                       if (panic_sink) {
                           throw std::runtime_error("controlled sink failure");
                       }
                   }) {
        if (!query_fixture) {
            entity.property("id", DataType::U64).id();
            entity.property("version", DataType::I64).version();
        }
        entity.property("name", DataType::Text);
        if (!query_fixture) {
            entity.property("status", DataType::Text);
        }
        entity.mask_fields({"name"});
    }

    DataServiceTransaction<Error>& begin() {
        auto& result = executor.begin(resource).wait();
        check(bool(result), "begin original fixture transaction");
        return result.value().get();
    }

    void released_before_observers() const {
        check(held_resource_notifications == 0 && controls.released,
              "provider lease released before observer");
    }
};

inline MutationRequest insert(Memory r, std::uint64_t id) {
    InsertCommand command(std::allocator_arg, Allocator(r), "Customer");
    command.value("id", id).value("version", std::int64_t(1)).value("name", "Riverside");
    command.trace_chain.emplace_back(TraceKind::AuditReason, "Customer", "audited test", id);
    return MutationRequest(std::allocator_arg, Allocator(r), std::move(command));
}

inline MutationRequest batch(Memory r) {
    MutationRequest::Batch children(r);
    for (std::uint64_t id : {1, 2, 3}) {
        children.emplace_back(insert(r, id));
    }
    return MutationRequest(std::allocator_arg, Allocator(r), std::move(children));
}

inline void driver_error(const Error& error, std::string_view expected) {
    check(error.kind() == Error::Kind::Transport && error.transport_error() == expected,
          "original typed driver error retained");
}
}
