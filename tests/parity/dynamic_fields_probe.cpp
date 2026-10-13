#include <teaql/core/dynamic_fields.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

static Vector<DynamicFieldDefinitions::Field> types(Memory r, const Json& input) {
    Vector<DynamicFieldDefinitions::Field> result(r);
    for (const auto& pair : std::get<Json::Array>(input.data)) {
        const auto& p = std::get<Json::Array>(pair.data);
        result.emplace_back(text(p[0]), DataType(std::get<std::uint64_t>(p[1].data)));
    }
    return result;
}

static Value::Object values(Memory r, const Json& input) {
    Value::Object result(r);
    for (const auto& [key, v] : std::get<Json::Object>(input.data)) {
        result.emplace(String(key, r), value(r, v));
    }
    return result;
}

static Json error(Memory r, const DynamicFieldError& e) {
    Json::Object result(r);
    result.emplace(String("error", r), Json(r, e.code));
    result.emplace(String("field", r), Json(r, e.field));
    return Json(r, result);
}

static Json view(Memory r, const DynamicFieldValues& v, const Json& lookups) {
    Json::Object result(r);
    Json::Array codes(r), fields(r);
    for (const auto& code : v.selected_codes()) {
        codes.emplace_back(code);
    }
    for (const auto& code : std::get<Json::Array>(lookups.data)) {
        auto queried = v.field(text(code));
        if (!queried) {
            fields.push_back(error(r, queried.error()));
            continue;
        }
        const auto& field_value = queried.value();
        Json::Object info(r);
        info.emplace(String("type", r), Json(r, std::uint64_t(field_value.data_type())));
        info.emplace(String("state", r),
                     Json(r,
                          field_value.state() == DynamicFieldState::Value  ? "Value"
                          : field_value.state() == DynamicFieldState::Null ? "Null"
                                                                           : "NotLoaded"));
        info.emplace(String("value", r),
                     field_value.value() ? field_value.value()->to_json_value(r) : Json(r));
        info.emplace(String("loaded", r), Json(r, field_value.is_loaded()));
        fields.emplace_back(info);
    }
    result.emplace(String("codes", r), Json(r, codes));
    result.emplace(String("fields", r), Json(r, fields));
    return Json(r, result);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto definitions = DynamicFieldDefinitions::from_fields(r,
                                                            text(field(input, "owner")),
                                                            text(field(input, "revision")),
                                                            types(r, field(input, "definitions")));
    if (!definitions) {
        return error(r, definitions.error());
    }
    auto& schema = user_context.make<DynamicFieldDefinitions>(std::move(definitions).value());
    auto mode = text(field(input, "mode"));
    if (mode == "selection") {
        const auto& selected = field(input, "selection");
        auto selection = std::holds_alternative<std::monostate>(selected.data)
                             ? Result<DynamicFieldSelection, DynamicFieldError>::ok(
                                   r, DynamicFieldSelection::all(r))
                             : DynamicFieldSelection::fields(r, types(r, selected));
        if (!selection) {
            return error(r, selection.error());
        }
        auto checked = selection.value().validate(schema);
        if (!checked) {
            return error(r, checked.error());
        }
        Json::Array contains(r);
        for (const auto& code : std::get<Json::Array>(field(input, "lookups").data)) {
            contains.emplace_back(selection.value().contains(text(code)));
        }
        Json::Object result(r);
        result.emplace(String("contains", r), Json(r, contains));
        return Json(r, result);
    }
    if (mode == "binding") {
        auto a = schema.with_storage_binding_shared(r, "profile", 7);
        auto same = a.get().with_storage_binding_shared(r, "profile", 7);
        auto b = a.get().with_storage_binding_shared(r, "profile", 8);
        Json::Object result(r);
        result.emplace(String("same_schema", r), Json(r, a.get().same_schema(b.get())));
        result.emplace(String("equal", r), Json(r, a.get() == b.get()));
        result.emplace(String("same_identity", r), Json(r, a.identity() == same.identity()));
        result.emplace(String("match", r), Json(r, a.get().storage_binding_matches("profile", 7)));
        result.emplace(String("wrong_profile", r),
                       Json(r, a.get().storage_binding_matches("other", 7)));
        result.emplace(String("unbound", r), Json(r, schema.storage_binding_matches("profile", 7)));
        return Json(r, result);
    }
    Vector<Value::Object> rows(r);
    for (const auto& row : std::get<Json::Array>(field(input, "rows").data)) {
        rows.push_back(values(r, row));
    }
    auto created = DynamicFieldValues::from_batch(r, schema, rows);
    if (!created) {
        return error(r, created.error());
    }
    Vector<DynamicFieldValues*> batch(r);
    for (auto& row : created.value()) {
        batch.push_back(&user_context.make<DynamicFieldValues>(std::move(row)));
    }
    Json::Array initial(r), shared(r), outcomes(r), final(r);
    for (const auto* a : batch) {
        initial.push_back(view(r, *a, field(input, "lookups")));
        Json::Array sharing(r);
        for (const auto* b : batch) {
            sharing.emplace_back(a->selection_identity() == b->selection_identity());
        }
        shared.emplace_back(sharing);
    }
    DynamicFieldMergeShapes shapes(std::allocator_arg, Allocator(r));
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        auto& row = *batch.at(std::get<std::uint64_t>(field(action, "row").data));
        auto before = row.selection_identity();
        auto op = text(field(action, "op"));
        auto result =
            op == "assign"
                ? row.assign(text(field(action, "code")), value(r, field(action, "value")))
            : op == "delete"
                ? row.erase(text(field(action, "code")))
                : row.merge_loaded(*batch.at(std::get<std::uint64_t>(field(action, "other").data)),
                                   shapes);
        Json::Object info(r);
        info.emplace(String("result", r), result ? Json(r) : error(r, result.error()));
        info.emplace(String("same_shape", r), Json(r, before == row.selection_identity()));
        outcomes.emplace_back(info);
    }
    for (const auto* row : batch) {
        final.push_back(view(r, *row, field(input, "lookups")));
    }
    Json::Array final_shared(r);
    for (const auto* a : batch) {
        Json::Array sharing(r);
        for (const auto* b : batch) {
            sharing.emplace_back(a->selection_identity() == b->selection_identity());
        }
        final_shared.emplace_back(sharing);
    }
    Json::Object result(r);
    result.emplace(String("initial", r), Json(r, initial));
    result.emplace(String("shared", r), Json(r, shared));
    result.emplace(String("outcomes", r), Json(r, outcomes));
    result.emplace(String("final", r), Json(r, final));
    result.emplace(String("final_shared", r), Json(r, final_shared));
    return Json(r, result);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("Invalid dynamic field probe input");
            }
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
