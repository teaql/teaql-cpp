#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/mutation_debug.hpp>
#include "fixture_value.hpp"
#include <iostream>

static Record record(Memory r, const Json& input) {
    Record result(r);
    for (const auto& [name, input_value] : std::get<Json::Object>(input.data)) {
        result.emplace(name, value(r, input_value));
    }
    return result;
}

static Json record_output(Memory r, const Record& values) {
    Json::Array result(r);
    for (const auto& [name, value] : values) {
        Json::Array pair(r);
        pair.emplace_back(name);
        pair.emplace_back(detail::debug_value(r, value));
        result.emplace_back(pair);
    }
    return Json(r, std::move(result));
}

static std::optional<std::int64_t> version(const Json& input) {
    return std::holds_alternative<String>(input.data) ? std::optional(number<std::int64_t>(input))
                                                      : std::nullopt;
}

static TraceNode trace(Memory r, const Json& input) {
    auto kind = TraceKind(std::get<std::uint64_t>(field(input, "trace_kind").data));
    const auto& raw_id = field(input, "id");
    auto id = std::holds_alternative<String>(raw_id.data)
                  ? std::optional(number<std::uint64_t>(raw_id))
                  : std::nullopt;
    if (std::get<bool>(field(input, "default").data)) {
        return TraceNode(std::allocator_arg,
                         Allocator(r),
                         text(field(input, "entity")),
                         text(field(input, "comment")),
                         id);
    }
    return TraceNode(std::allocator_arg,
                     Allocator(r),
                     kind,
                     text(field(input, "entity")),
                     text(field(input, "comment")),
                     id);
}

static TraceChain traces(Memory r, const Json& input) {
    TraceChain result(r);
    for (const auto& node : std::get<Json::Array>(input.data)) {
        result.emplace_back(trace(r, node));
    }
    return result;
}

template <class T>
static Json equality(Memory r, const T& left, const T& right) {
    auto lcopy = detail::clone_in(r, left);
    auto rcopy = detail::clone_in(r, right);
    Json::Array result(r);
    for (bool equal :
         {left == right, left == lcopy, right == rcopy, left == left, right == right}) {
        result.emplace_back(equal);
    }
    return Json(r, std::move(result));
}

template <class T>
static T command(Memory r, const Json& input) {
    T result = [&]() -> T {
        auto name = text(field(input, "entity"));
        if constexpr (std::same_as<T, InsertCommand> || std::same_as<T, BatchInsertCommand>) {
            return T(std::allocator_arg, Allocator(r), name);
        } else if constexpr (std::same_as<T, BatchUpdateCommand>) {
            Vector<String> fields(r);
            for (const auto& field_name : std::get<Json::Array>(field(input, "fields").data)) {
                fields.emplace_back(text(field_name));
            }
            return T(std::allocator_arg, Allocator(r), name, fields);
        } else if constexpr (std::same_as<T, RecoverCommand>) {
            return T(std::allocator_arg,
                     Allocator(r),
                     name,
                     value(r, field(input, "id")),
                     *version(field(input, "version")));
        } else {
            return T(std::allocator_arg, Allocator(r), name, value(r, field(input, "id")));
        }
    }();
    if constexpr (std::same_as<T, InsertCommand> || std::same_as<T, UpdateCommand>) {
        result.values = MutationValues(r, record(r, field(input, "values")));
    }
    if constexpr (std::same_as<T, UpdateCommand> || std::same_as<T, DeleteCommand>) {
        result.expected_version_value = version(field(input, "version"));
    }
    if constexpr (std::same_as<T, DeleteCommand>) {
        result.soft_delete = std::get<bool>(field(input, "soft").data);
    }
    if constexpr (std::same_as<T, UpdateCommand>) {
        const auto& old = field(input, "old");
        if (!std::holds_alternative<std::monostate>(old.data)) {
            result.old_values.emplace(r, record(r, old));
        }
    }
    if constexpr (std::same_as<T, BatchInsertCommand> || std::same_as<T, BatchUpdateCommand>) {
        for (const auto& values : std::get<Json::Array>(field(input, "values").data)) {
            result.batch_values.emplace_back(record(r, values));
        }
        for (const auto& chain : std::get<Json::Array>(field(input, "trace").data)) {
            result.trace_chains.emplace_back(traces(r, chain));
        }
    } else {
        result.trace_chain = traces(r, field(input, "trace"));
    }
    if constexpr (std::same_as<T, BatchUpdateCommand>) {
        for (const auto& id : std::get<Json::Array>(field(input, "ids").data)) {
            result.batch_ids.emplace_back(value(r, id));
        }
        for (const auto& v : std::get<Json::Array>(field(input, "versions").data)) {
            result.batch_expected_versions.emplace_back(version(v));
        }
        for (const auto& old : std::get<Json::Array>(field(input, "old").data)) {
            if (std::holds_alternative<std::monostate>(old.data)) {
                result.batch_old_values.emplace_back();
            } else {
                result.batch_old_values.emplace_back(std::in_place, r, record(r, old));
            }
        }
    }
    return result;
}

template <class T>
static Json command_equality(Memory r, const Json& input) {
    return equality(r, command<T>(r, field(input, "left")), command<T>(r, field(input, "right")));
}

template <class T>
static Json wrapper(Memory r, const Json& input) {
    T left(r, record(r, field(input, "left"))), right(r, record(r, field(input, "right")));
    Json::Object result(r);
    result.emplace("equal", equality(r, left, right));
    auto copied = left.copy_map(r);
    result.emplace("roundtrip", record_output(r, copied));
    if constexpr (std::same_as<T, MutationValues>) {
        Json::Array borrowed(r);
        for (const auto& [name, value] : std::as_const(left)) {
            Json::Array pair(r);
            pair.emplace_back(name);
            pair.emplace_back(detail::debug_value(r, value));
            borrowed.emplace_back(pair);
        }
        auto consumed = std::move(left).into_map(r);
        if (record_output(r, consumed) != Json(r, borrowed)) {
            throw std::runtime_error("borrowed and consuming iteration differ");
        }
        result.emplace("iteration", Json(r, borrowed));
    } else {
        result.emplace("iteration", Json(r));
    }
    return Json(r, std::move(result));
}

template <class T>
static Json builders(Memory r, const Json& input) {
    auto name = text(field(input, "entity"));
    auto command = [&]() {
        if constexpr (std::same_as<T, InsertCommand>) {
            return T(std::allocator_arg, Allocator(r), name);
        } else {
            return T(std::allocator_arg, Allocator(r), name, value(r, field(input, "id")));
        }
    }();
    Json::Array states(r);
    states.emplace_back(debug_string(r, command));
    for (const auto& step : std::get<Json::Array>(field(input, "steps").data)) {
        auto op = text(field(step, "op"));
        if constexpr (std::same_as<T, UpdateCommand> || std::same_as<T, DeleteCommand>) {
            if (op == "version") {
                command.expected_version(*version(field(step, "version")));
                states.emplace_back(debug_string(r, command));
                continue;
            }
        }
        if constexpr (std::same_as<T, DeleteCommand>) {
            command.hard_delete();
        } else {
            command.value(text(field(step, "field")), value(r, field(step, "value")));
        }
        states.emplace_back(debug_string(r, command));
    }
    return Json(r, std::move(states));
}

template <class T>
static Json map_methods(Memory r) {
    T values(r);
    auto& fields = values.fields;
    Json::Array states(r);
    states.emplace_back(record_output(r, fields));
    fields.insert_or_assign(String("z", r), Value(r, "first"));
    fields.insert_or_assign(String("a", r), Value(r, UINT64_MAX));
    states.emplace_back(record_output(r, fields));
    auto replaced = debug_string(r, fields.find("z")->second);
    fields.insert_or_assign(String("z", r), Value(r, false));
    Json::Object replacement(r);
    String replaced_option("Some(", r);
    replaced_option.append(replaced).append(")");
    replacement.emplace("replaced", Json(r, std::move(replaced_option)));
    replacement.emplace("fields", record_output(r, fields));
    states.emplace_back(std::move(replacement));
    fields.find("z")->second = Value(r);
    states.emplace_back(record_output(r, fields));
    auto removed = debug_string(r, fields.find("a")->second);
    fields.erase(fields.find("a"));
    Json::Object removal(r);
    String removed_option("Some(", r);
    removed_option.append(removed).append(")");
    removal.emplace("removed", Json(r, std::move(removed_option)));
    removal.emplace("missing", Json(r, fields.find("missing") == fields.end() ? "None" : "Some"));
    removal.emplace("fields", record_output(r, fields));
    states.emplace_back(std::move(removal));
    fields.clear();
    states.emplace_back(record_output(r, fields));
    return Json(r, std::move(states));
}

static Json observe(Memory r, const Json& input) {
    auto kind = text(field(input, "kind"));
    if (kind == "trace") {
        auto left = trace(r, field(input, "left"));
        auto right = trace(r, field(input, "right"));
        const char* names[] = {"Operation",
                               "Request",
                               "Relation",
                               "Entity",
                               "Provider",
                               "Sql",
                               "Comment",
                               "Purpose",
                               "AuditReason"};
        Json::Object result(r);
        result.emplace("equal", equality(r, left, right));
        result.emplace("kind", Json(r, names[unsigned(left.kind)]));
        result.emplace("entity", Json(r, left.entity_type));
        result.emplace("comment", Json(r, left.comment));
        result.emplace("id", left.entity_id ? Json(r, std::to_string(*left.entity_id)) : Json(r));
        return Json(r, std::move(result));
    }
    auto type = text(field(input, "type"));
    if (kind == "builders") {
        if (type == "insert") {
            return builders<InsertCommand>(r, input);
        }
        if (type == "update") {
            return builders<UpdateCommand>(r, input);
        }
        return builders<DeleteCommand>(r, input);
    }
    if (kind == "map_methods") {
        if (type == "mutation") {
            return map_methods<MutationValues>(r);
        }
        if (type == "generated") {
            return map_methods<GeneratedValues>(r);
        }
        return map_methods<EntitySnapshot>(r);
    }
    if (kind == "wrapper") {
        if (type == "mutation") {
            return wrapper<MutationValues>(r, input);
        }
        if (type == "generated") {
            return wrapper<GeneratedValues>(r, input);
        }
        return wrapper<EntitySnapshot>(r, input);
    }
    if (type == "insert") {
        return command_equality<InsertCommand>(r, input);
    }
    if (type == "update") {
        return command_equality<UpdateCommand>(r, input);
    }
    if (type == "delete") {
        return command_equality<DeleteCommand>(r, input);
    }
    if (type == "recover") {
        return command_equality<RecoverCommand>(r, input);
    }
    if (type == "batch_insert") {
        return command_equality<BatchInsertCommand>(r, input);
    }
    return command_equality<BatchUpdateCommand>(r, input);
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto input = Json::parse(user->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid JSON fixture");
            }
            std::cout << observe(user->memory_resource(), *input).dump(user->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
