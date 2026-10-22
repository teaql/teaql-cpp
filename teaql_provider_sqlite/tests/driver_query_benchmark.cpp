#include "../../tests/support/sqlite_driver_benchmark_fixture.hpp"
#include <sqlite3.h>
#include <iostream>
#include <memory>
#include <chrono>
#include "../../tests/support/generated_allocation_counter.hpp"

using namespace sqlite_driver_benchmark;

struct Measurement {
    Counts allocations;
    std::int64_t nanoseconds = 0;

    template <class Action>
    decltype(auto) run(Action&& action) {
        counts = {};
        const auto start = std::chrono::steady_clock::now();
        measuring = true;
        try {
            decltype(auto) result = action();
            measuring = false;
            const auto end = std::chrono::steady_clock::now();
            allocations = counts;
            nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            return result;
        } catch (...) {
            measuring = false;
            throw;
        }
    }
};

static void seed(SqliteConnection& connection) {
    auto* db = connection.native_handle();
    require(sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr) == SQLITE_OK, "begin setup");
    sqlite3_stmt* raw = nullptr;
    require(sqlite3_prepare_v2(db,
                               "INSERT INTO driver_probe(id,version,name,note) VALUES(?,1,?,?)",
                               -1,
                               &raw,
                               nullptr) == SQLITE_OK,
            "prepare setup");
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    for (int id = 1; id <= 10000; ++id) {
        const auto name = "row-" + std::to_string(id);
        require(sqlite3_bind_int64(raw, 1, id) == SQLITE_OK, "bind setup ID");
        require(sqlite3_bind_text(raw, 2, name.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK,
                "bind setup name");
        require((id % 2 ? sqlite3_bind_text(raw, 3, "nullable note", -1, SQLITE_STATIC)
                        : sqlite3_bind_null(raw, 3)) == SQLITE_OK,
                "bind setup note");
        require(sqlite3_step(raw) == SQLITE_DONE, "insert setup row");
        require(sqlite3_reset(raw) == SQLITE_OK, "reset setup statement");
        require(sqlite3_clear_bindings(raw) == SQLITE_OK, "clear setup bindings");
    }
    statement.reset();
    require(sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) == SQLITE_OK, "commit setup");
}

struct NativeRow {
    std::uint64_t id;
    std::int64_t version;
    std::string name;
    std::optional<std::string> note;
};

// Explicit Global resource; cached keys never borrow query Context strings.
class NativeStatements {
public:
    using allocator_type = Allocator;
    using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

    NativeStatements(std::allocator_arg_t, Allocator a, SqliteConnection& connection)
        : connection_(connection), statements_(a.resource()) {}

    sqlite3_stmt* prepare(const String& sql) {
        if (auto found = statements_.find(std::string_view(sql)); found != statements_.end()) {
            ++hits;
            return found->second.get();
        }
        sqlite3_stmt* raw = nullptr;
        require(sqlite3_prepare_v3(connection_.native_handle(),
                                   sql.data(),
                                   static_cast<int>(sql.size()),
                                   SQLITE_PREPARE_PERSISTENT,
                                   &raw,
                                   nullptr) == SQLITE_OK,
                "prepare cached native statement");
        Statement owned(raw, sqlite3_finalize);
        auto [position, inserted] = statements_.try_emplace(
            String(sql, statements_.get_allocator().resource()), std::move(owned));
        require(inserted, "unique native statement key");
        ++misses;
        return position->second.get();
    }

    void clear() {
        statements_.clear();
    }

    std::size_t hits = 0;
    std::size_t misses = 0;

private:
    SqliteConnection& connection_;
    std::pmr::map<String, Statement, std::less<>> statements_;
};

static std::vector<NativeRow>
native_rows(NativeStatements& cache, const CompiledQuery& query, const String& sql, bool full) {
    auto* raw = cache.prepare(sql);
    auto release = [](sqlite3_stmt* statement) {
        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);
    };
    std::unique_ptr<sqlite3_stmt, decltype(release)> checkout(raw, release);
    int index = 1;
    for (const auto& parameter : query.params) {
        int code;
        if (const auto* value = std::get_if<std::int64_t>(&parameter.data)) {
            code = sqlite3_bind_int64(raw, index, *value);
        } else if (const auto* value = std::get_if<std::uint64_t>(&parameter.data)) {
            require(*value <= static_cast<std::uint64_t>(INT64_MAX),
                    "native unsigned binding range");
            code = sqlite3_bind_int64(raw, index, static_cast<std::int64_t>(*value));
        } else if (const auto* value = std::get_if<String>(&parameter.data)) {
            code = sqlite3_bind_text(
                raw, index, value->data(), static_cast<int>(value->size()), SQLITE_TRANSIENT);
        } else {
            throw std::runtime_error("unexpected matched-driver binding type");
        }
        require(code == SQLITE_OK, "bind matched native parameter");
        ++index;
    }
    std::vector<NativeRow> result;
    int status;
    while ((status = sqlite3_step(raw)) == SQLITE_ROW) {
        auto text = [&](int column) {
            const auto* bytes = reinterpret_cast<const char*>(sqlite3_column_text(raw, column));
            return std::string(bytes, static_cast<std::size_t>(sqlite3_column_bytes(raw, column)));
        };
        NativeRow row{static_cast<std::uint64_t>(sqlite3_column_int64(raw, 0)),
                      sqlite3_column_int64(raw, 1),
                      text(2),
                      {}};
        if (full && sqlite3_column_type(raw, 3) != SQLITE_NULL) {
            row.note = text(3);
        }
        result.push_back(std::move(row));
    }
    require(status == SQLITE_DONE, "complete matched native query");
    return result;
}

static NativeRow plain_row(const CompactRow& row) {
    NativeRow value{row.get("id")->try_u64().value(),
                    row.get("version")->try_i64().value(),
                    std::string(row.get("name")->try_text().value()),
                    {}};
    if (const auto* note = row.get("note"); note && !note->is_null()) {
        value.note = std::string(note->try_text().value());
    }
    return value;
}

static void allocation_layers(GlobalContext& global,
                              SqliteMutationExecutor& provider,
                              bool full,
                              std::size_t count,
                              bool logging) {
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto query = make_query(r, full, count);
    auto& dialect = user_context->make_resource<SqliteDialect>();
    auto compiled =
        dialect.compile_select(r, Probe::entity_descriptor(r), query.as_query().prepare_for_list());
    Measurement measurement;
    auto report = [&](std::string_view phase) {
        const auto& a = measurement.allocations;
        std::cout << "LAYER," << logging << ',' << full << ',' << count << ',' << phase << ','
                  << a.requests << ',' << a.bytes << ',' << a.heap_calls << ',' << a.heap_bytes
                  << '\n';
    };
    const auto& fetched = measurement.run([&]() -> const auto& {
        return provider.fetch_all_compact_sql(r, compiled).wait();
    });
    require(bool(fetched) && fetched.value().size() == count, "compact fetch count");
    report("compact_fetch");
    std::vector<NativeRow> expected;
    for (const auto& row : fetched.value()) {
        expected.push_back(plain_row(row));
    }
    auto prepared = detail::clone_in(r, fetched.value());
    auto plain = measurement.run([&] {
        std::vector<NativeRow> result;
        for (const auto& row : prepared) {
            result.push_back(plain_row(row));
        }
        return result;
    });
    require(plain.size() == count, "plain decode count");
    for (std::size_t i = 0; i < count; ++i) {
        require(plain[i].id == expected[i].id && plain[i].version == expected[i].version &&
                    plain[i].name == expected[i].name && plain[i].note == expected[i].note,
                "plain compact values");
    }
    report("plain_compact_decode");
    auto typed_inputs = detail::clone_in(r, fetched.value());
    auto& root = user_context->make<EntityRuntimeState>();
    auto typed = measurement.run([&] {
        Vector<Probe> result(r);
        for (auto& row : typed_inputs) {
            auto decoded = Probe::from_compact_row_with_context(r, std::move(row), AnyRef(root));
            require(bool(decoded), "prepared typed decode");
            result.push_back(std::move(decoded).value());
        }
        return result;
    });
    require(typed.size() == count, "typed decode count");
    const auto snapshot = typed.front().loaded_state_snapshot();
    require(snapshot.has_value(), "typed indexed snapshot");
    for (std::size_t i = 0; i < count; ++i) {
        const auto& entity = typed[i];
        require(entity.id == expected[i].id && entity.version == expected[i].version &&
                    std::string_view(entity.name) == expected[i].name &&
                    entity.note.has_value() == expected[i].note.has_value(),
                "typed compact values");
        if (entity.note) {
            require(std::string_view(*entity.note) == *expected[i].note, "typed compact note");
        }
        require(entity.is_field_loaded("note") == full &&
                    entity.loaded_state_snapshot()->identity() == snapshot->identity() &&
                    !entity.dirty_fields(),
                "typed shape and clean state");
    }
    report("runtime_compact_decode");
    std::vector<const EntityRuntimeState*> states;
    for (const auto& entity : typed) {
        require(entity.state.has_value(), "hydration runtime state");
        states.push_back(&*entity.state);
    }
    for (auto phase : {"first_state_clone", "repeat_state_clone"}) {
        auto cloned = measurement.run([&] {
            Vector<EntityRuntimeState> result(r);
            for (const auto* state : states) {
                result.emplace_back(*state);
            }
            return result;
        });
        for (std::size_t i = 0; i < count; ++i) {
            EntityKey key(
                std::allocator_arg, Allocator(r), Probe::ENTITY_NAME, Value(r, typed[i].id));
            const auto original = detail::EntityRuntimeAccess::original_values_for(cloned[i], key);
            require(original.has_value(), "cloned state retains original");
            const auto& fields = original->fields;
            require(fields.at(String("id", r)).try_u64() == typed[i].id &&
                        fields.at(String("version", r)).try_i64() == typed[i].version &&
                        fields.at(String("name", r)).try_text() ==
                            std::string_view(typed[i].name) &&
                        fields.contains(String("note", r)) == full,
                    "cloned original fields");
            if (full) {
                const auto value = fields.at(String("note", r)).try_text();
                require(value.has_value() == typed[i].note.has_value(), "cloned note nullability");
                if (value) {
                    require(*value == *typed[i].note, "cloned note value");
                }
            }
            require(cloned[i].current_change_set().is_empty() && !typed[i].dirty_fields(),
                    "clone and original remain clean");
        }
        report(phase);
    }
    // Positive control: the clean-state observer must read actual mutation state.
    auto& first = typed.front();
    EntityKey key(std::allocator_arg, Allocator(r), Probe::ENTITY_NAME, Value(r, first.id));
    require(first.runtime_state_any().has_value(), "typed runtime state is exposed");
    first.state->set(key, "name", Value(r, "changed for observer control"));
    auto dirty = first.dirty_fields();
    require(dirty && dirty->contains(String("name", r)), "dirty observer detects a real change");
    first.state->clear_current_change_set();
    require(!first.dirty_fields(), "dirty observer detects cleared changes");
}

static std::array<Measurement, 2> verify(GlobalContext& global,
                                         NativeStatements& cache,
                                         SqliteMutationExecutor& provider,
                                         bool full,
                                         std::size_t count,
                                         bool logging,
                                         bool sampled = false,
                                         bool runtime_first = false) {
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    if (!logging) {
        user_context->disable_sql_log();
    }
    auto& metadata = install_metadata(*user_context);
    auto& recording = user_context->make_resource<RecordingTransport>(provider);
    auto& dialect = user_context->make_resource<SqliteDialect>();
    auto& state = user_context->make<SqlExecutorState>();
    SqlTransport<MutationExecutorError>& transport =
        sampled ? static_cast<SqlTransport<MutationExecutorError>&>(provider)
                : static_cast<SqlTransport<MutationExecutorError>&>(recording);
    auto& executor = user_context->make<Executor>(dialect, transport, metadata, state);
    auto& service = EntityDataService<Executor>::for_executor(
        *user_context, Probe::ENTITY_NAME, *user_context, executor);
    auto query = make_query(r, full, count);
    auto descriptor = Probe::entity_descriptor(r);
    auto compiled = dialect.compile_select(r, descriptor, query.as_query().prepare_for_list());
    const auto sql = compiled.sql_with_comment(r);
    std::array<Measurement, 2> measurements;
    std::vector<NativeRow> native;
    using QueryResult =
        std::remove_reference_t<decltype(service.fetch_enhanced_entities<Probe>(query).wait())>;
    const QueryResult* result_pointer = nullptr;
    auto run_native = [&] {
        native = measurements[0].run([&] {
            return native_rows(cache, compiled, sql, full);
        });
    };
    auto run_runtime = [&] {
        result_pointer = &measurements[1].run([&]() -> const auto& {
            return service.fetch_enhanced_entities<Probe>(query).wait();
        });
    };
    if (runtime_first) {
        run_runtime();
        run_native();
    } else {
        run_native();
        run_runtime();
    }
    const auto& result = *result_pointer;
    require(bool(result), "typed driver query succeeds");
    if (!sampled) {
        require(recording.queries.size() == 1, "one actual transport statement per query");
        require(recording.queries.front().sql_with_comment(r) == compiled.sql_with_comment(r),
                "exact actual SQL including comment matches native lane");
        require(recording.queries.front().params == compiled.params,
                "exact actual driver bindings match native lane");
    }
    require(native.size() == count, "matched native row count");
    require(result.value().data.size() == count, "correct driver row count");
    const auto snapshot = result.value().data.front().loaded_state_snapshot();
    require(snapshot.has_value(), "indexed snapshot installed");
    std::uint64_t expected = 1;
    for (const auto& entity : result.value().data) {
        const auto& raw = native.at(expected - 1);
        require(entity.id == raw.id && entity.version == raw.version &&
                    std::string_view(entity.name) == raw.name &&
                    entity.note.has_value() == raw.note.has_value(),
                "native and runtime ordered tuples match");
        if (entity.note) {
            require(std::string_view(*entity.note) == *raw.note,
                    "native and runtime nullable payload matches");
        }
        require(entity.id == expected && entity.version == 1 &&
                    std::string_view(entity.name) == "row-" + std::to_string(expected),
                "ordered typed values");
        require(entity.note.has_value() == (full && expected % 2 == 1), "NULL versus sparse note");
        if (entity.note) {
            require(*entity.note == "nullable note", "typed note payload");
        }
        require(entity.is_field_loaded("note") == full, "note projection loadedness");
        require(entity.loaded_state_snapshot()->identity() == snapshot->identity(),
                "shared immutable shape");
        require(!entity.dirty_fields(), "freshly decoded entity is clean");
        ++expected;
    }
    const auto logs = user_context->sql_logs();
    if (logging) {
        require(logs.size() == 1, "one default diagnostic per runtime list query");
        require(logs.front().sql == compiled.sql && logs.front().params != compiled.params,
                "diagnostics retain SQL and mask actual bindings");
        require(logs.front().comment &&
                    *logs.front().comment == "what: matched typed load-state driver probe" &&
                    logs.front().purpose &&
                    *logs.front().purpose == "why: isolate governed runtime query overhead",
                "diagnostics preserve exact query intent");
    } else {
        require(logs.empty(), "disabled logging retains no SQL diagnostic");
    }
    user_context->clear_sql_logs();
    if (!sampled) {
        std::cout << "DRIVER_CORRECTNESS," << logging << ',' << full << ',' << count << '\n';
    }
    return measurements;
}

static std::int64_t percentile(const std::vector<std::int64_t>& sorted, std::size_t percent) {
    require(!sorted.empty() && percent >= 1 && percent <= 100, "valid percentile input");
    return sorted[(sorted.size() * percent + 99) / 100 - 1];
}

static void summary(const std::vector<Measurement>& samples,
                    bool logging,
                    bool full,
                    std::size_t count,
                    std::string_view lane) {
    require(samples.size() == 31, "complete summary sample set");
    std::vector<std::int64_t> times;
    std::vector<std::array<std::size_t, 4>> allocations;
    long double total = 0;
    for (const auto& sample : samples) {
        times.push_back(sample.nanoseconds);
        total += sample.nanoseconds;
        const auto& a = sample.allocations;
        allocations.push_back({a.requests, a.bytes, a.heap_calls, a.heap_bytes});
    }
    std::sort(times.begin(), times.end());
    std::sort(allocations.begin(), allocations.end());
    const auto& median = allocations[allocations.size() / 2];
    require(total > 0, "positive measured duration");
    std::cout << "SUMMARY," << logging << ',' << full << ',' << count << ',' << lane << ','
              << samples.size() << ',' << percentile(times, 50) << ',' << percentile(times, 95)
              << ',' << 31000000000.0L / total;
    for (auto value : median) {
        std::cout << ',' << value;
    }
    std::cout << '\n';
}

int main(int argc, char** argv) {
    require(percentile({1, 2, 3, 4}, 50) == 2 && percentile({1, 2, 3, 4}, 95) == 4 &&
                percentile({7}, 50) == 7,
            "nearest-rank percentile boundaries");
    require(argc == 1 || (argc == 2 && std::string_view(argv[1]) == "--measure"),
            "usage: driver_query_benchmark [--measure]");
    const bool sample = argc == 2;
    ContextLimits limits;
    limits.user.max_bytes = 512ull * 1024 * 1024;
    limits.max_live_user_bytes = limits.user.max_bytes;
    GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
    auto& connection = global.make_resource<SqliteConnection>();
    auto& provider = global.make<SqliteMutationExecutor>(connection);
    auto& cache = global.make_resource<NativeStatements>(connection);
    {
        auto user_context = global.make_user_context();
        install_metadata(*user_context);
        use_sqlite_provider(*user_context, global, provider);
        require(bool(user_context->ensure_schema().wait()), "runtime schema initialization");
    }
    seed(connection);
    for (bool logging : {false, true}) {
        for (bool full : {false, true}) {
            for (std::size_t count : {1, 100, 10000}) {
                verify(global, cache, provider, full, count, logging);
                if (sample) {
                    // The correctness lane has already touched SQLite. This is a fresh
                    // runtime/shape observation with default logs, not a cold database.
                    const auto first =
                        verify(global, cache, provider, full, count, true, true, true);
                    std::cout << "FIRST_RUNTIME_DEFAULT_LOG," << logging << ',' << full << ','
                              << count << ',' << first[1].nanoseconds << '\n';
                }
                allocation_layers(global, provider, full, count, logging);
                if (sample) {
                    for (int warmup = 0; warmup < 20; ++warmup) {
                        verify(global, cache, provider, full, count, logging, true, warmup % 2);
                    }
                    std::array<std::vector<Measurement>, 2> retained;
                    for (int iteration = 0; iteration < 31; ++iteration) {
                        const auto samples = verify(
                            global, cache, provider, full, count, logging, true, iteration % 2);
                        for (std::size_t lane = 0; lane < samples.size(); ++lane) {
                            const auto& value = samples[lane];
                            retained[lane].push_back(value);
                            std::cout
                                << "SAMPLE," << logging << ',' << full << ',' << count << ','
                                << iteration << ',' << (lane == 0 ? "native" : "runtime") << ','
                                << value.nanoseconds << ',' << value.allocations.requests << ','
                                << value.allocations.bytes << ',' << value.allocations.heap_calls
                                << ',' << value.allocations.heap_bytes << '\n';
                        }
                    }
                    summary(retained[0], logging, full, count, "native");
                    summary(retained[1], logging, full, count, "runtime");
                }
            }
        }
    }
    require(cache.misses == 6 && cache.hits >= 6, "six native cached shapes are reused");
    auto statement_count = [&] {
        std::size_t count = 0;
        for (auto* statement = sqlite3_next_stmt(connection.native_handle(), nullptr); statement;
             statement = sqlite3_next_stmt(connection.native_handle(), statement)) {
            require(!sqlite3_stmt_busy(statement), "no live cursor after benchmark");
            ++count;
        }
        return count;
    };
    const auto before = statement_count();
    cache.clear();
    require(before == statement_count() + 6, "native cached statements are finalized");
}
