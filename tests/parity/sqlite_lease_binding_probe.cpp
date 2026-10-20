#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <sqlite3.h>
#include <iostream>

using namespace teaql;

static const Json& field(const Json& input, std::string_view name) {
    return std::get<Json::Object>(input.data).find(name)->second;
}

static std::string_view text(const Json& input) {
    return std::get<String>(input.data);
}

static const Json* optional_field(const Json& input, std::string_view name) {
    const auto& fields = std::get<Json::Object>(input.data);
    const auto found = fields.find(name);
    return found == fields.end() ? nullptr : &found->second;
}

static Json rows(Memory resource, const Vector<CompactRow>& input) {
    Json::Array output(resource);
    for (const auto& row : input) {
        Json::Array values(resource);
        for (const auto& value : row.values()) {
            values.emplace_back(value.to_json_value(resource));
        }
        output.emplace_back(values);
    }
    return Json(resource, output);
}

template <class T>
static Json outcome(Memory resource, const Result<T, MutationExecutorError>& item) {
    auto output = detail::model_json(resource);
    if (!item) {
        detail::model_put(output, "error", item.error().to_string(resource));
    } else if constexpr (std::is_same_v<T, std::uint64_t>) {
        detail::model_put(output, "affected", item.value());
    } else if constexpr (std::is_same_v<T, StreamChunk>) {
        detail::model_put(output, "rows", rows(resource, item.value().rows));
        detail::model_put(output, "chunk_index", item.value().chunk_index);
        detail::model_put(output, "is_last", item.value().is_last);
    } else {
        detail::model_put(output, "rows", rows(resource, item.value()));
    }
    return Json(resource, output);
}

static Json observe(GlobalContext& global, const Json& input) {
    auto transaction_user_context = global.make_user_context();
    auto waiting_user_context = global.make_user_context();
    auto resource = waiting_user_context->memory_resource();
    auto transaction_resource = transaction_user_context->memory_resource();
    auto& connection = global.make_resource<SqliteConnection>();
    auto& provider = global.make<SqliteMutationExecutor>(connection);
    bool held = std::get<bool>(field(input, "held").data);
    const auto* poison_input = optional_field(input, "poison");
    const bool poison = poison_input && std::get<bool>(poison_input->data);
    if (poison) {
        if (held) {
            throw std::runtime_error("Poison controls require an idle connection");
        }
        auto& setup = waiting_user_context->make<CompiledQuery>();
        setup.sql = "CREATE TABLE poison_fixture(ts TIMESTAMP)";
        provider.execute(resource, setup).value();
        setup.sql = "INSERT INTO poison_fixture VALUES ('2024-01-01 00:00:/0')";
        provider.execute(resource, setup).value();
        setup.sql = "SELECT ts FROM poison_fixture";
        bool caught = false;
        try {
            provider.fetch_all_compact_sql(resource, setup).wait().value();
        } catch (const std::out_of_range&) {
            caught = true;
        }
        if (!caught) {
            throw std::runtime_error("Checked timestamp did not fail inside the native lock");
        }
    }
    SqlTransaction<MutationExecutorError>* transaction = nullptr;
    if (held) {
        transaction = &provider.begin_sql(transaction_resource).wait().value().get();
    }
    auto& query = waiting_user_context->make<CompiledQuery>();
    query.sql = "SELECT ?";
    const auto* binding_input = optional_field(input, "binding");
    const auto binding = binding_input ? text(*binding_input) : "list";
    if (binding == "list") {
        Value::List list(resource);
        list.emplace_back(std::int64_t(1));
        query.params.emplace_back(Value(resource, list));
    } else if (binding == "object") {
        query.params.emplace_back(Value(resource, Value::Object(resource)));
    } else if (binding == "u64") {
        query.params.emplace_back(std::uint64_t(UINT64_MAX));
    } else if (binding == "valid") {
        query.params.emplace_back(std::int64_t(1));
    } else {
        throw std::runtime_error("Unknown fixture binding");
    }
    const auto mode = text(field(input, "mode"));
    const auto end = text(field(input, "end"));
    auto finish = [&](auto& work, auto poll_result) {
        Json::Array pending(resource);
        Json result(resource);
        bool completed = false;
        auto cancel = [&] {
            if constexpr (requires { work.cancel(); }) {
                work.cancel();
            }
        };
        auto poll = [&] {
            const auto* item = poll_result(work);
            if (!item) {
                return false;
            }
            result = outcome(resource, *item);
            completed = true;
            if (mode == "stream") {
                cancel();
            }
            return true;
        };
        for (unsigned index = 0; index != 2; ++index) {
            const bool ready = poll();
            pending.emplace_back(!ready);
            if (ready) {
                break;
            }
        }
        const bool cancelled = end == "cancel";
        if (cancelled) {
            cancel();
        } else if (transaction) {
            if (end == "commit") {
                transaction->commit_sql(transaction_resource).wait().value();
            } else if (end == "rollback") {
                transaction->rollback_sql(transaction_resource).wait().value();
            }
        }
        transaction_user_context.reset();
        if (!cancelled && !completed && !poll()) {
            throw std::runtime_error("Lease did not resume after transaction release");
        }
        auto output = detail::model_json(resource);
        detail::model_put(output, "pending", pending);
        detail::model_put(output, "cancelled", cancelled);
        detail::model_put(output, "result", result);
        detail::model_put(
            output, "autocommit", bool(sqlite3_get_autocommit(connection.native_handle())));
        return Json(global.memory_resource(), output);
    };
    if (mode == "execute") {
        return finish(provider.execute_sql(resource, query), [](auto& work) {
            return work.poll();
        });
    }
    if (mode == "fetch") {
        return finish(provider.fetch_all_compact_sql(resource, query), [](auto& work) {
            return work.poll();
        });
    }
    if (mode == "repeated") {
        return finish(provider.fetch_repeated_compact_sql(resource, query, 0, query.params),
                      [](auto& work) {
                          return work.poll();
                      });
    }
    if (mode == "sync_execute" || mode == "sync_fetch") {
        if (held) {
            throw std::runtime_error("Synchronous controls require an idle connection");
        }
        if (mode == "sync_execute") {
            auto result = provider.execute(resource, query);
            return finish(result, [](auto& value) {
                return &value;
            });
        }
        auto result = provider.fetch_all_compact(resource, query);
        return finish(result, [](auto& value) {
            return &value;
        });
    }
    auto& stream = provider.stream_sql(resource, detail::clone_in(resource, query), 1);
    return finish(stream, [](auto& work) {
        const auto next = work.poll_next();
        if (next.kind == StreamPoll<Result<StreamChunk, MutationExecutorError>>::Kind::End) {
            throw std::runtime_error("Stream ended before its binding error");
        }
        return next.item;
    });
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto input = Json::parse(global.memory_resource(), line);
        if (!input) {
            throw std::runtime_error("Invalid fixture JSON");
        }
        const auto output = observe(global, *input);
        std::cout << output.dump(global.memory_resource()) << '\n';
    }
}
