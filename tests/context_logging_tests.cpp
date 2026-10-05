#include <teaql/runtime/context.hpp>
#include <teaql/runtime/log_manager.hpp>
#include <teaql/core/detail/debug.hpp>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <unistd.h>
using namespace teaql;

static void check(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}

template <class F>
void rejected(F&& f) {
    bool fail = false;
    try {
        f();
    } catch (const std::logic_error&) {
        fail = true;
    }
    check(fail, "closed Context rejects logging API");
}

static void quiet() {
    setenv("TEAQL_LOG_ENDPOINT", "off", 1);
    unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");
    unsetenv("TEAQL_SQL_LOG_TABLES");
    unsetenv("TEAQL_SQL_LOG");
    setenv("TEAQL_LOG_FORMAT", "debug", 1);
}

static ExecutionMetadata metadata(Memory r) {
    ExecutionMetadata m(std::allocator_arg, Allocator(r), DataServiceOperation::Query);
    m.backend = "sqlite";
    m.parameterized_query.emplace("SELECT ?, ?", r);
    m.debug_query.emplace("RAW private-customer private-password", r);
    m.params.emplace_back(Value(r, "private-customer"));
    m.params.emplace_back(Value(r, "private-password"));
    m.sql_log.generated_sql = true;
    m.sql_log.parameter_policies = {SqlParameterLogPolicy::Masked,
                                    SqlParameterLogPolicy::Credential};
    m.result_count = 0;
    m.trace_chain.emplace_back(TraceKind::Request, "Order", "private-customer private-password", 7);
    m.started_at = ExecutionMetadata::SystemTime(std::chrono::nanoseconds(-1));
    m.ended_at = ExecutionMetadata::SystemTime(std::chrono::nanoseconds(1500));
    return m;
}

static void lifetime_and_options() {
    quiet();
    GlobalContext global;
    auto& retained = global.make<UnifiedLogBuffer>();
    auto user = global.make_user_context();
    user->insert_resource(global, retained);
    auto r = user->memory_resource();
    user->with_user_identifier("operator whose identity lives in the User arena");
    {
        auto m = metadata(r);
        user->record_metadata_log(m);
    }
    check(retained.entries.size() == 1, "unified buffer receives safe entry");
    check(retained.entries[0].memory_resource() == global.memory_resource(),
          "registered Global buffer deep copies into its explicit domain");
    {
        auto copy = user->sql_logs();
        copy[0].sql = "altered snapshot";
        check(user->sql_logs()[0].sql == "SELECT ?, ?", "sql_logs returns a deep snapshot");
    }
    user->set_sql_log_options(SqlLogOptions::disabled());
    check(user->sql_logs().size() == 1, "setting disabled options does not clear history");
    user->disable_sql_log();
    check(user->sql_logs().empty() && retained.entries.size() == 1,
          "disable clears SQL history without clearing unified history");
    user->enable_select_sql_log();
    {
        auto m = metadata(r);
        user->record_metadata_log(m);
    }
    user->clear_sql_logs();
    check(retained.entries.size() == 2, "clear leaves separately registered buffer unchanged");
    auto& snapshot = global.make<Vector<SqlLogEntry>>(user->sql_logs());
    check(snapshot.empty(), "explicit cross-domain snapshot");
    user->close();
    check(user->allocation_stats().live_blocks == 0, "log and identity blocks returned at close");
    auto& item = retained.entries[0];
    auto& sql = std::get<SqlLogEntry>(item.payload);
    check(sql.params.size() == 2 && sql.debug_sql.find("private-password") == String::npos &&
              sql.debug_sql.find("private-customer") == String::npos,
          "Global buffer retains only projected values after source destruction");
    check(item.user_identifier == "operator whose identity lives in the User arena" &&
              !item.trace_chain.empty(),
          "nested identity and trace survive through explicit deep copy");
    rejected([&] {
        user->sql_logs();
    });
    rejected([&] {
        user->clear_sql_logs();
    });
    rejected([&] {
        user->user_identifier();
    });
    rejected([&] {
        user->set_user_identifier("closed");
    });
    rejected([&] {
        user->enable_all_sql_log();
    });
    rejected([&] {
        user->record_metadata_log(metadata(global.memory_resource()));
    });
}

static void identity_and_quiescence() {
    quiet();
    setenv("USER", "captured-edge-user", 1);
    GlobalContext global;
    auto user = global.make_user_context();
    auto id = std::string(*user->user_identifier());
    setenv("USER", "later-user", 1);
    check(user->user_identifier() == id && id.starts_with("captured-edge-user@pid-"),
          "default identity captures environment at construction");
    user->with_user_identifier_option(std::string_view(""));
    check(user->user_identifier() == "", "empty identity remains Some");
    user->with_user_identifier_option({});
    check(!user->user_identifier(), "None remains distinct");
    setenv("USER", "\xff", 1);
    setenv("USERNAME", "fallback-name", 1);
    {
        auto other = global.make_user_context();
        check(other->user_identifier()->starts_with("fallback-name@pid-"),
              "invalid UTF8 USER falls back to USERNAME");
    }
    unsetenv("USER");
    unsetenv("USERNAME");
    {
        auto other = global.make_user_context();
        check(other->user_identifier()->starts_with("main@pid-"), "missing environment uses main");
    }
    auto& buffer = global.make<UnifiedLogBuffer>();
    user->insert_resource(global, buffer);
    Arena::Quiescence stop;
    stop.attach(*dynamic_cast<Arena*>(user->memory_resource()), user.get(), [](void* p) noexcept {
        try {
            auto& c = *static_cast<UserContext*>(p);
            auto m = metadata(c.memory_resource());
            c.record_metadata_log(m);
        } catch (...) {
            std::terminate();
        }
    });
    user.reset();
    check(buffer.entries.size() == 1,
          "quiescence records final diagnostics before releasing Context");
}

static std::string file_text(const char* path) {
    std::ifstream in(path);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

static void output_and_failure_order() {
    quiet();
    char path[] = "/tmp/teaql-context-log-XXXXXX";
    int fd = mkstemp(path);
    check(fd >= 0, "temporary log file");
    close(fd);

    struct Remove {
        const char* path;

        ~Remove() {
            std::remove(path);
        }
    } remove{path};

    setenv("TEAQL_LOG_ENDPOINT", path, 1);
    setenv("TEAQL_SQL_DEBUG_ENDPOINT", path, 1);
    setenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS",
           "I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK",
           1);
    {
        GlobalContext global;
        auto user = global.make_user_context();
        {
            auto m = metadata(user->memory_resource());
            user->record_metadata_log(m);
        }
        auto logs = user->sql_logs();
        check(logs.size() == 1 && logs[0].debug_sql.find("private-customer") == String::npos,
              "plaintext endpoint does not upgrade retained SQL");
    }
    auto text = file_text(path);
    auto first = text.find("SqlLogEntry {");
    auto second = text.find("SqlLogEntry {", first + 1);
    check(first != text.npos && second != text.npos && text.find("private-customer") > second &&
              text.find("private-customer") != text.npos,
          "ordinary output precedes sensitive output");
    check(text.find("private-password") == text.npos && text.find("RAW private") == text.npos,
          "credentials and untrusted diagnostic SQL never leave projection");
    quiet();
    setenv("TEAQL_LOG_ENDPOINT", "stdout", 1);
    GlobalContext global;
    auto user = global.make_user_context();
    auto& buffer = user->make<UnifiedLogBuffer>();
    user->insert_resource(*user, buffer);
    auto m = metadata(user->memory_resource());
    int saved = dup(fileno(stdout));
    auto* full = std::fopen("/dev/full", "wb");
    check(saved >= 0 && full, "stdout failure setup");
    std::fflush(stdout);
    dup2(fileno(full), fileno(stdout));
    bool failed = false;
    try {
        user->record_metadata_log(m);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    std::fflush(stdout);
    dup2(saved, fileno(stdout));
    close(saved);
    std::fclose(full);
    std::clearerr(stdout);
    check(failed && user->sql_logs().size() == 1 && buffer.entries.size() == 1,
          "retention happens before ordinary output failure");
}

int main() {
    try {
        auto* prior = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        lifetime_and_options();
        identity_and_quiescence();
        output_and_failure_order();
        std::pmr::set_default_resource(prior);
        std::cout << "Context logging contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
