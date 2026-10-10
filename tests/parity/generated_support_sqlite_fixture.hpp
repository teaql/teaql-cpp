#pragma once
#include "generated_support_fixture.hpp"
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>

namespace support_fixture {
using SqlExecutor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;

class SqlRuntime final : public TeaqlRuntime {
public:
    using allocator_type = Allocator;
    using Output = Result<SmartList<CompactRow>, RuntimeError>;
    using ChildOutput =
        Result<SmartList<CompactRow>, DataServiceError<typename SqlExecutor::Error>>;
    UserContext& context;
    SqlExecutor& executor;
    mutable Json::Array calls;

    SqlRuntime(std::allocator_arg_t, Allocator a, UserContext& context, SqlExecutor& executor)
        : context(context), executor(executor), calls(a.resource()) {}

    const UserContext& user_context() const override {
        return context;
    }

    class Work {
    public:
        using allocator_type = Allocator;

        Work(std::allocator_arg_t, Allocator a, AsyncOperation<ChildOutput>& child)
            : resource_(a.resource()), child_(child) {}

        std::optional<Output> poll(WakeSignal& wake) {
            forward_.attach(child_.wake_signal(), wake);
            auto* result = child_.poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return Output::err(
                    resource_,
                    RuntimeError::graph(resource_, result->error().to_string(resource_)));
            }
            return Output::ok(resource_, detail::clone_in(resource_, result->value()));
        }

        void finish_and_wait() noexcept {
            forward_.reset();
        }

        void cancel_and_wait() noexcept {
            forward_.reset();
            child_.cancel();
        }

    private:
        Memory resource_;
        AsyncOperation<ChildOutput>& child_;
        WakeSignal::Forward forward_;
    };

    AsyncOperation<Output>&
    fetch_facet_smart_list(std::string_view entity,
                           const PurposedSelectQuery& query,
                           std::span<const RelationAggregate> relation_aggregates,
                           std::span<const TraceNode> chain) const override {
        auto r = context.memory_resource();
        auto call = detail::model_json(r);
        detail::model_put(call, "entity", entity);
        detail::model_put(call, "query", detail::debug_query(r, query.as_query()));
        detail::model_put(call, "aggregates", aggregates(r, relation_aggregates));
        detail::model_put(call, "trace", trace(r, chain));
        calls.emplace_back(std::move(call));
        auto& service =
            EntityDataService<SqlExecutor>::for_executor(context, entity, context, executor);
        service.with_trace_context(chain);
        auto& child = service.fetch_smart_list_with_relation_aggregates(query, relation_aggregates);
        return make_operation<Output, Work>(r, child);
    }
};

class SqlFixture {
public:
    UserContext& user;
    Memory resource;
    SqliteConnection& connection;
    SqliteMutationExecutor& transport;
    InMemoryMetadataStore& metadata;
    SqliteDialect& dialect;
    SqlExecutorState& state;
    SqlExecutor& executor;
    SqlRuntime& runtime;

    SqlFixture(UserContext& user, const Json& descriptors)
        : user(user), resource(user.memory_resource()),
          connection(user.make_resource<SqliteConnection>()),
          transport(user.make<SqliteMutationExecutor>(connection)),
          metadata(user.make<InMemoryMetadataStore>()),
          dialect(user.make_resource<SqliteDialect>()), state(user.make<SqlExecutorState>()),
          executor(user.make<SqlExecutor>(dialect, transport, metadata, state)),
          runtime(user.make_resource<SqlRuntime>(user, executor)) {
        user.disable_sql_log();
        for (const auto& d : std::get<Json::Array>(descriptors.data)) {
            metadata.register_entity(key_entity(resource, d));
        }
        user.set_metadata(user, metadata);
        CompiledQuery sql(std::allocator_arg, Allocator(resource));
        for (auto statement :
             {"CREATE TABLE parents(id INTEGER PRIMARY KEY,label TEXT)",
              "CREATE TABLE children(id INTEGER PRIMARY KEY,parent_id INTEGER,label TEXT)",
              "INSERT INTO parents VALUES(1,'one'),(2,'two')",
              "INSERT INTO children VALUES(10,1,'first'),(11,1,'second'),(20,2,'third')"}) {
            sql.sql = statement;
            if (!transport.execute(resource, sql)) {
                throw std::runtime_error("SQLite facet fixture setup failed");
            }
        }
    }
};
}
