#include <teaql/provider_sqlite/runtime_connection.hpp>
#include <sqlite3.h>
#include <iostream>

using namespace teaql;

static CompiledQuery query(Memory resource, std::string_view sql) {
    CompiledQuery result(std::allocator_arg, Allocator(resource));
    result.sql = sql;
    return result;
}

static void scoped_provider_on_global_connection() {
    GlobalContext global;
    auto& connection = global.make_resource<SqliteConnection>(":memory:");
    auto user_context = global.make_user_context();
    auto other_user_context = global.make_user_context();
    auto resource = user_context->memory_resource();
    auto other_resource = other_user_context->memory_resource();
    auto& provider = user_context->make<SqliteMutationExecutor>(connection);
    auto& other_provider = other_user_context->make<SqliteMutationExecutor>(connection);
    // A completed synchronous result may be copied into an explicit destination
    // because it no longer retains its provider or native connection.
    auto& retained_rows = global.make<Result<SqliteMutationExecutor::Rows, MutationExecutorError>>(
        provider.fetch_all_compact(global.memory_resource(),
                                   query(global.memory_resource(), "SELECT 7 AS copied_value")));
    if (!retained_rows ||
        retained_rows.value().get_allocator().resource() != global.memory_resource()) {
        throw std::runtime_error("Synchronous destination results did not belong to Global");
    }
    bool rejected = false;
    try {
        provider.fetch_all_compact_sql(other_resource, query(other_resource, "SELECT 1"));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    if (!rejected) {
        throw std::runtime_error("User provider state escaped through a Global connection");
    }
    rejected = false;
    try {
        provider.fetch_all_compact_sql(global.memory_resource(),
                                       query(global.memory_resource(), "SELECT 1"));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    if (!rejected) {
        throw std::runtime_error("Global work retained shorter-lived User provider state");
    }
    rejected = false;
    try {
        global.make<SqliteMutationExecutor>(provider);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    if (!rejected) {
        throw std::runtime_error("Provider cloning implicitly retained User state in Global");
    }
    rejected = false;
    try {
        auto& user_connection = user_context->make_resource<SqliteConnection>(":memory:");
        global.make<SqliteMutationExecutor>(user_connection);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    if (!rejected) {
        throw std::runtime_error("Global provider retained a shorter-lived connection");
    }
    auto& stream = provider.stream_sql(resource, query(resource, "SELECT 1 UNION ALL SELECT 2"), 1);
    auto* first = stream.wait_next();
    if (!first || !*first || first->value().rows.size() != 1) {
        throw std::runtime_error("Global connection did not provide the first native stream row");
    }
    auto& abandoned = other_provider.execute_sql(
        other_resource, query(other_resource, "CREATE TABLE abandoned_write(id INTEGER)"));
    if (abandoned.poll()) {
        throw std::runtime_error("Second provider did not queue behind the active native cursor");
    }
    other_user_context.reset();
    auto next_user_context = global.make_user_context();
    auto next_resource = next_user_context->memory_resource();
    auto& next_provider = next_user_context->make<SqliteMutationExecutor>(connection);
    auto& pending = next_provider.fetch_all_compact_sql(
        next_resource,
        query(next_resource,
              "SELECT count(*) AS remaining FROM sqlite_master WHERE name='abandoned_write'"));
    if (pending.poll()) {
        throw std::runtime_error("A surviving provider bypassed the active connection lease");
    }
    user_context.reset();
    const auto& rows = pending.wait();
    if (!rows || rows.value().size() != 1 || rows.value()[0].get("remaining")->try_i64() != 0) {
        throw std::runtime_error("Context close failed to release cursor or discard queued writes");
    }
    next_user_context.reset();
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("Global connection retained closed provider storage");
    }
    if (retained_rows.value()[0].get("copied_value")->try_i64() != 7) {
        throw std::runtime_error("Explicit synchronous copies depended on closed User state");
    }
    for (auto* statement = sqlite3_next_stmt(connection.native_handle(), nullptr); statement;
         statement = sqlite3_next_stmt(connection.native_handle(), statement)) {
        if (sqlite3_stmt_busy(statement)) {
            throw std::runtime_error("A Global connection retained a closed User cursor");
        }
    }
}

struct ConnectionHostBuffer {
    using allocator_type = Allocator;
    String data;
    bool& alive;

    ConnectionHostBuffer(std::allocator_arg_t, Allocator allocator, bool& alive)
        : data(32768, 'h', allocator.resource()), alive(alive) {
        alive = true;
    }

    ~ConnectionHostBuffer() {
        alive = false;
    }
};

struct CloseObservation {
    ConnectionHostBuffer& host;
    bool& alive;
    unsigned& closed;

    CloseObservation(ConnectionHostBuffer& host, bool& alive, unsigned& closed)
        : host(host), alive(alive), closed(closed) {}
};

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        scoped_provider_on_global_connection();
        GlobalContext global;
        unsigned closed = 0;
        bool host_alive = false;
        Ref<SqliteConnection> reference;
        {
            auto user_context = global.make_user_context();
            // These payloads precede the connection, so its native close callback
            // must run while their registered objects and arena storage are alive.
            auto& host = user_context->make_resource<ConnectionHostBuffer>(host_alive);
            auto& observation =
                user_context->make_resource<CloseObservation>(host, host_alive, closed);
            {
                auto result =
                    open_generated_sqlite_runtime_connection(*user_context, "sqlite://:memory:");
                if (!result) {
                    throw std::runtime_error(
                        std::string(result.error().to_string(user_context->memory_resource())));
                }
                reference = result.value();
                auto* handle = user_context->get(reference).native_handle();
                auto code = sqlite3_create_function_v2(
                    handle,
                    "observe_configuration_close",
                    0,
                    SQLITE_UTF8,
                    &observation,
                    [](sqlite3_context* context, int, sqlite3_value**) {
                        sqlite3_result_null(context);
                    },
                    nullptr,
                    nullptr,
                    [](void* data) {
                        auto& observation = *static_cast<CloseObservation*>(data);
                        if (!observation.alive || observation.host.data.size() != 32768 ||
                            observation.host.data.back() != 'h') {
                            std::terminate();
                        }
                        ++observation.closed;
                    });
                if (code != SQLITE_OK || closed != 0) {
                    throw std::runtime_error(
                        "Failed to register the native connection close observer");
                }
            }
            user_context.reset();
        }
        if (closed != 1 || host_alive || global.live_user_bytes() != 0) {
            throw std::runtime_error(
                "Native SQLite cleanup did not precede User arena reclamation");
        }
        auto next_user_context = global.make_user_context();
        bool expired = false;
        try {
            (void)next_user_context->get(reference);
        } catch (const ReferenceError&) {
            expired = true;
        }
        if (!expired) {
            throw std::runtime_error("Closed configured connection reference remained usable");
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Configured SQLite cleanup and borrowed connection lifetime passed\n";
    return 0;
}
