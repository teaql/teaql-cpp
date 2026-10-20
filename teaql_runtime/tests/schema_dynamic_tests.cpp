#include <teaql/runtime.hpp>
#include <teaql/runtime/dynamic_fields.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <sqlite3.h>
#include <array>
#include <iostream>

using namespace teaql;
using DynamicResult = Result<std::monostate, DynamicFieldError>;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct SchemaReady {
    Memory owner;

    std::optional<RuntimeSchemaResult> poll(WakeSignal&) {
        return RuntimeSchemaResult::ok(owner, {});
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct OrdinaryProvider : runtime::SchemaProvider {
    AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext& user_context,
                                                       const SchemaInvocation&) const override {
        return make_operation<RuntimeSchemaResult, SchemaReady>(
            user_context.memory_resource(), SchemaReady{user_context.memory_resource()});
    }
};

struct Control {
    bool ready = false;
    bool normal_scope = true;
    bool live_input_on_cancel = false;
    unsigned polls = 0;
    unsigned cancellations = 0;
    WakeSignal* wake = nullptr;
};

struct PendingDynamic {
    Memory owner;
    const UserContext& user_context;
    Control& control;
    std::string_view borrowed_input;

    std::optional<DynamicResult> poll(WakeSignal& wake) {
        ++control.polls;
        control.wake = &wake;
        control.normal_scope &= !detail::is_generated_schema_bootstrap(user_context);
        if (!control.ready) {
            return {};
        }
        return DynamicResult::ok(owner, {});
    }

    void finish_and_wait() noexcept {
        control.wake = nullptr;
    }

    void cancel_and_wait() noexcept {
        ++control.cancellations;
        control.normal_scope &= !detail::is_generated_schema_bootstrap(user_context);
        control.live_input_on_cancel = borrowed_input.size() == 257 && borrowed_input.back() == 'x';
        control.wake = nullptr;
    }
};

struct PendingProvider : DynamicFieldsProvider {
    Control& control;
    GlobalContext* foreign = nullptr;
    mutable DynamicStorageFuture<std::monostate>* child = nullptr;

    explicit PendingProvider(Control& control) : control(control) {}

    DynamicStorageFuture<std::monostate>&
    ensure_schema(const UserContext& user_context) const override {
        auto& input = user_context.make<String>(257, 'x');
        auto owner = foreign ? foreign->memory_resource() : user_context.memory_resource();
        child = &make_operation<DynamicResult, PendingDynamic>(
            owner, PendingDynamic{owner, user_context, control, input});
        return *child;
    }

    DynamicStorageFuture<DynamicFieldBatch>& load_values(const UserContext&,
                                                         std::string_view,
                                                         std::span<const std::uint64_t>,
                                                         const DynamicFieldSelection&,
                                                         const QueryIntent&) const override {
        throw std::logic_error("Schema test must not load dynamic values");
    }
};

static void pending_and_cleanup() {
    for (unsigned mode = 0; mode < 3; ++mode) {
        GlobalContext global;
        auto& control = global.make_resource<Control>();
        auto user_context = global.make_user_context();
        auto& ordinary = user_context->make_resource<OrdinaryProvider>();
        auto& dynamic = user_context->make_resource<PendingProvider>(control);
        user_context->set_schema_provider(*user_context, ordinary);
        user_context->set_dynamic_fields_provider(*user_context, dynamic);
        auto& operation = user_context->ensure_schema();
        check(!operation.poll() && control.polls == 1,
              "Context schema waits for the dynamic provider");
        auto before = operation.wake_signal().ticket();
        control.wake->notify();
        check(operation.wake_signal().ticket() != before,
              "dynamic schema wakeups reach the outer schema operation");
        if (mode == 0) {
            control.ready = true;
            check(bool(operation.wait()), "dynamic schema can resume successfully");
        } else if (mode == 1) {
            operation.cancel();
        }
        user_context.reset();
        check(global.live_user_bytes() == 0 && control.normal_scope && !control.wake,
              "dynamic work joins before User reclamation without bootstrap privilege");
        check(control.cancellations == (mode == 0 ? 0U : 1U),
              "explicit cancellation and Context deletion cancel exactly once");
        if (mode != 0) {
            check(control.live_input_on_cancel,
                  "dynamic cancellation reads its still-live User input");
        }
    }
}

static void foreign_work() {
    GlobalContext global;
    auto& control = global.make_resource<Control>();
    auto user_context = global.make_user_context();
    auto& ordinary = user_context->make_resource<OrdinaryProvider>();
    auto& dynamic = user_context->make_resource<PendingProvider>(control);
    dynamic.foreign = &global;
    user_context->set_schema_provider(*user_context, ordinary);
    user_context->set_dynamic_fields_provider(*user_context, dynamic);
    bool rejected = false;
    try {
        user_context->ensure_schema().wait();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && control.cancellations == 1 && control.live_input_on_cancel &&
              dynamic.child->status() == AsyncStatus::Cancelled,
          "invalid factory work is quiesced before rejecting its foreign domain");
    user_context->set_schema_provider(*user_context, ordinary);
    user_context.reset();
    check(global.live_user_bytes() == 0 && control.cancellations == 1,
          "foreign dynamic rejection releases the pending schema borrow");
}

static int table_count(sqlite3* connection) {
    sqlite3_stmt* statement = nullptr;
    check(sqlite3_prepare_v2(
              connection,
              "SELECT COUNT(*) FROM sqlite_master WHERE name='teaql_dynamic_field_storage_v1'",
              -1,
              &statement,
              nullptr) == SQLITE_OK,
          "inspect actual dynamic table");
    auto status = sqlite3_step(statement);
    int count = status == SQLITE_ROW ? sqlite3_column_int(statement, 0) : -1;
    sqlite3_finalize(statement);
    check(status == SQLITE_ROW, "read actual dynamic table count");
    return count;
}

static void physical_sqlite() {
    for (bool readonly : {false, true}) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        {
            auto owner = user_context->memory_resource();
            auto& connection = user_context->make_resource<SqliteConnection>();
            auto& executor = user_context->make<SqliteMutationExecutor>(connection);
            user_context->insert_resource(*user_context, executor);
            auto& ordinary = user_context->make_resource<OrdinaryProvider>();
            user_context->set_schema_provider(*user_context, ordinary);
            const std::array fields{DynamicFieldDefinitions::Field{"note", DataType::Text}};
            auto& definitions = user_context->make<DynamicFieldDefinitions>(
                std::move(DynamicFieldDefinitions::from_fields(owner, "Item", "r1", fields))
                    .value());
            const std::array<const DynamicFieldDefinitions*, 1> schemas{&definitions};
            using Provider = DatabaseDynamicFieldsProvider<SqliteMutationExecutor>;
            auto& provider = user_context->make<Provider>(
                std::move(Provider::create(owner, "schema-integration", schemas)).value());
            user_context->set_dynamic_fields_provider(*user_context, provider);
            check(table_count(connection.native_handle()) == 0,
                  "fresh SQLite has no extension table");
            if (readonly) {
                check(sqlite3_exec(connection.native_handle(),
                                   "PRAGMA query_only=ON",
                                   nullptr,
                                   nullptr,
                                   nullptr) == SQLITE_OK,
                      "configure real DDL rejection");
            }
            const auto& result = user_context->ensure_schema().wait();
            if (readonly) {
                check(!result && result.error().kind() == RuntimeError::Kind::DynamicField &&
                          result.error().dynamic_field_error().code ==
                              "DYNAMIC_FIELD_STORAGE_FAILED" &&
                          table_count(connection.native_handle()) == 0,
                      "physical dynamic DDL failure remains a typed Context error");
            } else {
                check(bool(result) && table_count(connection.native_handle()) == 1,
                      "Context ensure_schema initializes registered persistent dynamic storage");
                check(bool(user_context->ensure_schema().wait()) &&
                          table_count(connection.native_handle()) == 1,
                      "repeated Context schema initialization is idempotent");
            }
        }
        user_context.reset();
        check(global.live_user_bytes() == 0, "SQLite resources close before User arena release");
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        pending_and_cleanup();
        foreign_work();
        physical_sqlite();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
