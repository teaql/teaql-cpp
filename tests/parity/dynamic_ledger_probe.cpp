#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime.hpp>
#include "fixture_value.hpp"
#include <array>
#include <iostream>

static void put(Json::Object& out, std::string_view key, Json v) {
    out.emplace(String(key, out.get_allocator().resource()), std::move(v));
}

static EntityKey key(Memory r, const Json& input) {
    auto entity = optional(input, "entity");
    return EntityKey(std::allocator_arg,
                     Allocator(r),
                     entity ? text(*entity) : "Item",
                     Value(r, std::uint64_t{7}));
}

static void load(UserContext& user_context, EntityRuntimeState& state, const Json& input) {
    auto r = user_context.memory_resource();
    auto owner = optional(input, "owner");
    const std::array fields{DynamicFieldDefinitions::Field{"name", DataType::Text},
                            DynamicFieldDefinitions::Field{"count", DataType::I64}};
    auto d = std::move(DynamicFieldDefinitions::from_fields(
                           r, owner ? text(*owner) : "Item", "v1", fields))
                 .value();
    const auto* executor = optional(input, "executor");
    auto& definitions = executor
                            ? user_context.make<DynamicFieldDefinitions>(d.with_storage_binding(
                                  r, "profile", std::get<std::uint64_t>(executor->data)))
                            : user_context.make<DynamicFieldDefinitions>(std::move(d));
    Value::Object values(r);
    for (const auto& [code, v] : std::get<Json::Object>(field(input, "values").data)) {
        values.emplace(String(code, r), value(r, v));
    }
    auto fields_value = DynamicFieldValues::from_values(r, definitions, values);
    state.install_loaded_dynamic_fields(fields_value.value());
}

static Json state(Memory r, const EntityRuntimeState& s) {
    EntityKey k(std::allocator_arg, Allocator(r), "Item", Value(r, std::uint64_t{7}));
    auto changes = s.current_change_set();
    Json::Object result(r);
    const auto* values = s.loaded_dynamic_fields();
    put(result, "values", values ? record_to_json_value(r, values->values()) : Json(r));
    put(result, "pending", Json(r, s.has_pending_dynamic_mutations()));
    put(result, "empty", Json(r, changes.is_empty()));
    Json dynamic(r);
    if (const auto* map = changes.dynamic_changes()) {
        Json::Array entries(r);
        for (const auto& [key, fields] : *map) {
            Json::Object entry(r), items(r);
            put(entry, "entity", Json(r, key.entity));
            put(entry, "id", key.id.to_json_value(r));
            for (const auto& [code, mutation] : fields) {
                Json::Object item(r);
                put(item, "op", Json(r, mutation.is_delete() ? "Delete" : "Set"));
                if (mutation.value()) {
                    put(item, "value", mutation.value()->to_json_value(r));
                }
                put(items, code, Json(r, item));
            }
            put(entry, "fields", Json(r, items));
            entries.emplace_back(entry);
        }
        dynamic = Json(r, entries);
    }
    put(result, "dynamic", std::move(dynamic));
    Json::Array fields(r);
    for (const auto& name : s.changed_field_names(k)) {
        fields.emplace_back(name);
    }
    put(result, "fields", Json(r, fields));
    auto native = s.get(k, "native");
    put(result, "native", native ? native->to_json_value(r) : Json(r));
    auto comment = s.get_comment();
    put(result, "comment", comment ? Json(r, *comment) : Json(r));
    return Json(r, result);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    Vector<EntityRuntimeState*> states(r);
    for (const auto& initial : std::get<Json::Array>(field(input, "initial").data)) {
        auto& s = user_context.make<EntityRuntimeState>();
        if (!std::holds_alternative<std::monostate>(initial.data)) {
            load(user_context, s, initial);
        }
        states.push_back(&s);
    }
    Json::Array initial(r), outcomes(r);
    for (const auto* s : states) {
        initial.push_back(state(r, *s));
    }
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        auto i = std::get<std::uint64_t>(field(action, "state").data);
        auto op = text(field(action, "op"));
        Json result(r);
        if (op == "set" || op == "delete") {
            auto changed =
                op == "set"
                    ? states[i]->update_dynamic_field(key(r, action),
                                                      text(field(action, "code")),
                                                      value(r, field(action, "value")))
                    : states[i]->delete_dynamic_field(key(r, action), text(field(action, "code")));
            if (changed) {
                result = Json(r, changed.value());
            } else {
                Json::Object e(r);
                put(e, "error", Json(r, changed.error().code));
                put(e, "field", Json(r, changed.error().field));
                result = Json(r, e);
            }
        } else if (op == "clone") {
            states.push_back(&user_context.make<EntityRuntimeState>(*states[i]));
        } else if (op == "fresh") {
            states.push_back(&user_context.make<EntityRuntimeState>(
                EntityRuntimeState::fresh_with_shared_graph(r, *states[i])));
        } else if (op == "rebind") {
            auto from = std::get<std::uint64_t>(field(action, "from").data);
            *states[i] = states[i]->with_loaded_view_from(*states[from]);
        } else if (op == "adopt") {
            auto from = std::get<std::uint64_t>(field(action, "from").data);
            states[i]->adopt_mutations_from(*states[from]);
        } else if (op == "load") {
            load(user_context, *states[i], field(action, "load"));
        } else if (op == "native") {
            states[i]->set(key(r, action), "native", value(r, field(action, "value")));
        } else if (op == "comment") {
            states[i]->set_comment(text(field(action, "text")));
        } else if (op == "push") {
            states[i]->push_change_set();
        } else if (op == "pop") {
            result = Json(r, bool(states[i]->pop_change_set()));
        } else if (op == "clear") {
            states[i]->clear_current_change_set();
        } else if (op == "commit") {
            states[i]->clear_committed();
        }
        Json::Array views(r), equal(r);
        for (const auto* s : states) {
            views.push_back(state(r, *s));
            equal.emplace_back(*s == *states[0]);
        }
        Json::Object out(r);
        put(out, "result", std::move(result));
        put(out, "states", Json(r, views));
        put(out, "equal", Json(r, equal));
        outcomes.emplace_back(out);
    }
    Json::Object out(r);
    put(out, "initial", Json(r, initial));
    put(out, "outcomes", Json(r, outcomes));
    return Json(r, out);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("Invalid dynamic ledger fixture");
            }
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
