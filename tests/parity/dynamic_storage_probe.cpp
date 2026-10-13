#include <teaql/data_service.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <array>
#include <iostream>

struct Store final : DynamicFieldStore {
    std::uintptr_t key;
    std::optional<std::uint64_t> identity;

    explicit Store(const Json& input) : key(std::get<std::uint64_t>(field(input, "key").data)) {
        const auto& id = field(input, "identity");
        if (!std::holds_alternative<std::monostate>(id.data)) {
            identity = std::get<std::uint64_t>(id.data);
        }
    }

    bool is_transaction_bound() const override {
        return false;
    }

    std::uintptr_t binding_key() const override {
        return key;
    }

    std::optional<std::uint64_t> source_identity() const override {
        return identity;
    }

    DynamicStorageFuture<std::monostate>& ensure_schema(Memory) const override {
        throw std::logic_error("unexpected I/O");
    }

    DynamicStorageFuture<DynamicStorageRows>& load_values(Memory,
                                                          std::string_view,
                                                          const DynamicFieldDefinitions&,
                                                          std::span<const std::uint64_t>,
                                                          const DynamicFieldSelection&,
                                                          const QueryIntent&) const override {
        throw std::logic_error("unexpected I/O");
    }

    Result<std::monostate, DynamicFieldError>
    validate_writes(Memory r, std::span<const DynamicFieldWrite>) const override {
        return Result<std::monostate, DynamicFieldError>::ok(r, {});
    }

    DynamicStorageFuture<std::monostate>&
    apply_writes(Memory, std::span<const DynamicFieldWrite>, const MutationIntent&) const override {
        throw std::logic_error("unexpected I/O");
    }
};

static void put(Json::Object& out, std::string_view key, Json v) {
    out.emplace(String(key, out.get_allocator().resource()), std::move(v));
}

static Json error(Memory r, const DynamicFieldError& e) {
    Json::Object out(r);
    put(out, "error", Json(r, e.code));
    put(out, "field", Json(r, e.field));
    return Json(r, out);
}

static Record record(Memory r, const Json& input) {
    Record out(r);
    for (const auto& [k, v] : std::get<Json::Object>(input.data)) {
        out.emplace(String(k, r), value(r, v));
    }
    return out;
}

static Json rows_state(Memory r, std::span<const CompactRow> rows) {
    Json::Array output(r);
    for (const auto& row : rows) {
        Json::Object entry(r);
        put(entry, "native", compact_row_to_json_value(r, row));
        auto fields = row.loaded_dynamic_fields();
        put(entry, "fields", fields ? record_to_json_value(r, fields->values()) : Json(r));
        output.emplace_back(entry);
    }
    return Json(r, output);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    const std::array fields{DynamicFieldDefinitions::Field{"name", DataType::Text},
                            DynamicFieldDefinitions::Field{"count", DataType::I64}};
    auto& definitions = user_context.make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "Item", "v1", fields)).value());
    if (text(field(input, "mode")) == "write_debug") {
        DynamicFieldWrite::Changes changes(r);
        for (const auto& [name, v] : std::get<Json::Object>(field(input, "changes").data)) {
            changes.emplace(String(name, r),
                            std::holds_alternative<std::monostate>(v.data)
                                ? DynamicFieldMutation::erase(r)
                                : DynamicFieldMutation::set(r, value(r, v)));
        }
        DynamicFieldWrite write(std::allocator_arg,
                                Allocator(r),
                                text(field(input, "namespace")),
                                definitions,
                                999,
                                changes);
        Json::Object out(r);
        put(out, "debug", Json(r, debug_string(r, write)));
        DebugOptions options;
        options.pretty = true;
        put(out, "pretty", Json(r, debug_string(r, write, options)));
        return Json(r, out);
    }
    auto selection =
        Result<DynamicFieldSelection, DynamicFieldError>::ok(r, DynamicFieldSelection::all(r));
    const auto& selected = field(input, "selection");
    if (!std::holds_alternative<std::monostate>(selected.data)) {
        Vector<DynamicFieldSelection::Field> items(r);
        for (const auto& pair : std::get<Json::Array>(selected.data)) {
            const auto& p = std::get<Json::Array>(pair.data);
            items.emplace_back(text(p[0]), DataType(std::get<std::uint64_t>(p[1].data)));
        }
        selection = DynamicFieldSelection::fields(r, items);
    }
    if (!selection) {
        return error(r, selection.error());
    }
    Store store(field(input, "store"));
    auto intent = std::move(QueryIntent::create(r, "read extensions", "cursor hydration")).value();
    auto created = DynamicFieldStreamPlan::create(
        r, text(field(input, "namespace")), definitions, selection.value(), store, intent);
    if (!created) {
        return error(r, created.error());
    }
    const auto& plan = created.value();
    Json::Object out(r);
    put(out, "namespace", Json(r, plan.storage_namespace()));
    put(out,
        "bound",
        Json(
            r,
            plan.definitions().storage_binding_matches(plan.storage_namespace(), *store.identity)));
    Json::Array validations(r);
    for (const auto& entry : std::get<Json::Array>(field(input, "validate").data)) {
        Store other(entry);
        auto valid = plan.validate_store(other);
        validations.push_back(valid ? Json(r) : error(r, valid.error()));
    }
    put(out, "validations", Json(r, validations));
    Vector<CompactRow> rows(r);
    for (const auto& row : std::get<Json::Array>(field(input, "rows").data)) {
        rows.emplace_back(record(r, row));
    }
    auto ids_result = plan.owner_ids(rows);
    Json ids(r);
    if (ids_result) {
        Json::Array items(r);
        for (auto id : ids_result.value()) {
            items.emplace_back(id);
        }
        Json::Object obj(r);
        put(obj, "ids", Json(r, items));
        ids = Json(r, obj);
    } else {
        ids = error(r, ids_result.error());
    }
    put(out, "ids", std::move(ids));
    put(out, "initial", rows_state(r, rows));
    Vector<std::uint64_t> owners(r);
    for (const auto& id : std::get<Json::Array>(field(input, "ids").data)) {
        owners.push_back(std::get<std::uint64_t>(id.data));
    }
    DynamicStorageRows stored(r);
    for (const auto& [id, v] : std::get<Json::Object>(field(input, "stored").data)) {
        std::uint64_t owner = 0;
        auto parsed = std::from_chars(id.data(), id.data() + id.size(), owner);
        if (parsed.ec != std::errc{}) {
            throw std::runtime_error("invalid stored owner");
        }
        stored.emplace(owner, record(r, v));
    }
    auto merged = plan.merge(rows, owners, stored);
    put(out, "merge", merged ? Json(r) : error(r, merged.error()));
    put(out, "final", rows_state(r, rows));
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
                throw std::runtime_error("Invalid dynamic storage fixture");
            }
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
