#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <sqlite3.h>
#include <iostream>

static void put(Json::Object& out, std::string_view key, Json v) {
    out.emplace(String(key, out.get_allocator().resource()), std::move(v));
}

static Json error(Memory r, const DynamicFieldError& e) {
    Json::Object out(r);
    put(out, "error", Json(r, e.code));
    put(out, "field", Json(r, e.field));
    return Json(r, out);
}

static Json output(Memory r, const Result<DynamicStorageRows, DynamicFieldError>& rows) {
    if (!rows) {
        return error(r, rows.error());
    }
    Json::Object out(r), values(r);
    for (const auto& [id, fields] : rows.value()) {
        put(values, std::to_string(id), record_to_json_value(r, fields));
    }
    put(out, "rows", Json(r, values));
    return Json(r, out);
}

static std::uint64_t id(const Json& v) {
    return number<std::uint64_t>(v);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& provider = user_context.make<SqliteMutationExecutor>(connection);
    if (!provider.ensure_schema(r).wait()) {
        throw std::runtime_error("fixture schema failed");
    }
    Vector<DynamicFieldDefinitions::Field> fields(r);
    for (const auto& pair : std::get<Json::Array>(field(input, "fields").data)) {
        const auto& p = std::get<Json::Array>(pair.data);
        fields.emplace_back(text(p[0]), DataType(std::get<std::uint64_t>(p[1].data)));
    }
    auto& definitions = user_context.make<DynamicFieldDefinitions>(
        std::move(
            DynamicFieldDefinitions::from_fields(r, "Item", text(field(input, "revision")), fields))
            .value());
    Vector<DynamicFieldWrite> writes(r);
    for (const auto& item : std::get<Json::Array>(field(input, "writes").data)) {
        DynamicFieldWrite::Changes changes(r);
        for (const auto& [code, v] : std::get<Json::Object>(field(item, "changes").data)) {
            if (std::holds_alternative<std::monostate>(v.data)) {
                changes.emplace(String(code, r), DynamicFieldMutation::erase(r));
            } else {
                auto type = optional(v, "type");
                auto operand =
                    text(field(v, "kind")) == "typed_null"
                        ? Value(r,
                                TypedNull{type ? DataType(std::get<std::uint64_t>(type->data))
                                               : DataType::Text})
                        : value(r, v);
                changes.emplace(String(code, r), DynamicFieldMutation::set(r, operand));
            }
        }
        writes.emplace_back(
            text(field(item, "namespace")), definitions, id(field(item, "id")), changes);
    }
    const auto& selected = field(input, "selection");
    auto selected_value =
        Result<DynamicFieldSelection, DynamicFieldError>::ok(r, DynamicFieldSelection::all(r));
    if (!std::holds_alternative<std::monostate>(selected.data)) {
        Vector<DynamicFieldSelection::Field> types(r);
        for (const auto& pair : std::get<Json::Array>(selected.data)) {
            const auto& p = std::get<Json::Array>(pair.data);
            types.emplace_back(text(p[0]), DataType(std::get<std::uint64_t>(p[1].data)));
        }
        selected_value = DynamicFieldSelection::fields(r, types);
    }
    const auto& selection = selected_value.value();
    Vector<std::uint64_t> ids(r);
    for (const auto& v : std::get<Json::Array>(field(input, "ids").data)) {
        ids.push_back(id(v));
    }
    auto ns = text(field(input, "namespace"));
    auto read = std::move(QueryIntent::create(r, "read dynamic fields", "verify storage")).value();
    auto intent = std::move(MutationIntent::create(r, "write dynamic fields")).value();
    auto& tx = provider.begin_sql(r).wait().value().get();
    auto* store = tx.dynamic_field_store();
    Json::Object out(r);
    auto validation = store->validate_writes(r, writes);
    put(out, "validation", validation ? Json(r) : error(r, validation.error()));
    auto& applied = store->apply_writes(r, writes, intent).wait();
    put(out, "apply", applied ? Json(r) : error(r, applied.error()));
    put(out,
        "inside",
        output(r, store->load_values(r, ns, definitions, ids, selection, read).wait()));
    auto& queued = provider.load_values(r, ns, definitions, ids, selection, read);
    put(out, "pending", Json(r, !queued.poll()));
    if (std::get<bool>(field(input, "rollback").data)) {
        if (!tx.rollback_sql(r).wait()) {
            throw std::runtime_error("fixture rollback failed");
        }
    } else {
        if (!tx.commit_sql(r).wait()) {
            throw std::runtime_error("fixture commit failed");
        }
    }
    put(out, "after", output(r, queued.wait()));
    const auto& corrupt = field(input, "corrupt");
    if (auto* sql = std::get_if<String>(&corrupt.data)) {
        if (sqlite3_exec(connection.native_handle(), sql->c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
            throw std::runtime_error("fixture corruption SQL failed");
        }
    }
    put(out,
        "final",
        output(r, provider.load_values(r, ns, definitions, ids, selection, read).wait()));
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
                throw std::runtime_error("Invalid SQLite dynamic fixture");
            }
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
