#include "../support/runtime_mutation_fixture.hpp"
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

static Json trace_out(Memory r, std::span<const TraceNode> frames) {
    Json::Array output(r);
    for (const auto& frame : frames) {
        auto item = detail::model_json(r);
        auto kind = frame.kind == TraceKind::Entity        ? "Entity"
                    : frame.kind == TraceKind::AuditReason ? "AuditReason"
                                                           : "Comment";
        detail::model_put(item, "kind", kind);
        detail::model_put(item, "entity", frame.entity_type);
        detail::model_put(item, "id", frame.entity_id ? Json(r, *frame.entity_id) : Json(r));
        detail::model_put(item, "comment", frame.comment);
        output.emplace_back(std::move(item));
    }
    return Json(r, output);
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& state = user_context.make<mutation_fixture::State>();
    auto& executor = user_context.make_resource<mutation_fixture::Executor>(state);
    auto& metadata = user_context.make<detail::UserContextMetadata>(user_context);
    ResourceBorrow<mutation_fixture::Executor> borrow(
        std::allocator_arg, Allocator(r), executor, detail::resource_arena(r));
    auto& service = user_context.make<
        detail::RuntimeDataService<detail::UserContextMetadata, mutation_fixture::Executor>>(
        metadata, borrow);
    auto intent = MutationIntent::create(r, "execute physical command");
    service.with_mutation_intent(intent.value());
    TraceChain frames(r);
    for (const auto& frame : std::get<Json::Array>(field(input, "trace").data)) {
        auto name = text(field(frame, "kind"));
        auto kind = name == "Entity"        ? TraceKind::Entity
                    : name == "AuditReason" ? TraceKind::AuditReason
                                            : TraceKind::Comment;
        const auto& raw_id = field(frame, "id");
        std::optional<std::uint64_t> id;
        if (!std::holds_alternative<std::monostate>(raw_id.data)) {
            id = detail::model_integer<std::uint64_t>(raw_id, "id");
        }
        frames.emplace_back(kind, text(field(frame, "entity")), text(field(frame, "comment")), id);
    }
    auto id = value(r, field(input, "id"));
    auto entry = text(field(input, "entry"));
    auto run = [&]<class C>(C& command) -> std::uint64_t {
        command.trace_chain = detail::clone_in(r, frames);
        if constexpr (std::same_as<C, InsertCommand>) {
            if (std::get<bool>(field(input, "has_id").data)) {
                command.values.fields.emplace(String("id", r), Value(r, id));
            }
        }
        auto& work = [&]() -> auto& {
            if (entry.ends_with("s")) {
                Vector<C> commands(r);
                commands.emplace_back(command);
                commands.emplace_back(command);
                if constexpr (std::same_as<C, InsertCommand>) {
                    return service.insert_many(commands);
                } else if constexpr (std::same_as<C, UpdateCommand>) {
                    return service.update_many(commands);
                } else if constexpr (std::same_as<C, DeleteCommand>) {
                    return service.delete_many(commands);
                } else {
                    return service.recover_many(commands);
                }
            }
            if constexpr (std::same_as<C, InsertCommand>) {
                if (entry == "batch_insert") {
                    auto& batch = user_context.make<BatchInsertCommand>("Payment");
                    batch.batch_values.emplace_back(command.values);
                    batch.batch_values.emplace_back(command.values);
                    batch.trace_chains.emplace_back(frames);
                    return service.batch_insert(batch);
                }
                return service.insert(command);
            } else if constexpr (std::same_as<C, UpdateCommand>) {
                if (entry == "batch_update") {
                    auto& batch = user_context.make<BatchUpdateCommand>("Payment");
                    batch.batch_ids.emplace_back(id);
                    batch.batch_ids.emplace_back(id);
                    batch.batch_values.emplace_back();
                    batch.batch_values.emplace_back();
                    batch.trace_chains.emplace_back(frames);
                    return service.batch_update(batch);
                }
                return service.update(command);
            } else if constexpr (std::same_as<C, DeleteCommand>) {
                return service.delete_command(command);
            } else {
                return service.recover(command);
            }
        }();
        const auto& result = work.wait();
        if (!result) {
            throw std::runtime_error(std::string(result.error().to_string(r)));
        }
        if (command.trace_chain != frames) {
            throw std::logic_error("runtime changed caller-owned lineage");
        }
        return result.value();
    };
    std::uint64_t rows;
    if (entry == "insert" || entry == "inserts" || entry == "batch_insert") {
        auto& command = user_context.make<InsertCommand>("Payment");
        rows = run(command);
    } else if (entry == "update" || entry == "updates" || entry == "batch_update") {
        auto& command = user_context.make<UpdateCommand>("Payment", id);
        rows = run(command);
    } else if (entry == "delete" || entry == "deletes") {
        auto& command = user_context.make<DeleteCommand>("Payment", id);
        rows = run(command);
    } else {
        auto& command = user_context.make<RecoverCommand>("Payment", id, 1);
        rows = run(command);
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "rows", rows);
    detail::model_put(output, "input", trace_out(r, frames));
    Json::Array requests(r);
    for (const auto& request : state.requests) {
        auto item = detail::model_json(r);
        detail::model_put(item, "raw", trace_out(r, request.trace_chain()));
        detail::model_put(item, "execution", trace_out(r, request.execution_trace_chain()));
        requests.emplace_back(std::move(item));
    }
    detail::model_put(output, "requests", requests);
    return output;
}

int main() {
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        user_context->disable_sql_log();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        std::cout << evaluate(*user_context, *input).dump(r) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
