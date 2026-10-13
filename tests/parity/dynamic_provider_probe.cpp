#include <teaql/runtime/dynamic_fields.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>

struct Schema final : teaql::SchemaProvider {
    const teaql::EntityDescriptor* get_entity(std::string_view) const override {
        return nullptr;
    }
};

using Executor =
    teaql::SqlDataServiceExecutor<teaql::MutationExecutorError, teaql::SqliteMutationExecutor>;
#include "fixture_value.hpp"
#include <iostream>
#include <array>

static void put(Json::Object& out, std::string_view key, Json value) {
    out.emplace(String(key, out.get_allocator().resource()), std::move(value));
}

static Json error(Memory r, const DynamicFieldError& e) {
    Json::Object out(r);
    put(out, "error", Json(r, e.code));
    put(out, "field", Json(r, e.field));
    return Json(r, out);
}

template <class T>
static T completed(AsyncOperation<T>& operation) {
    operation.wait();
    return operation.take_result();
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& executor = user_context.make<SqliteMutationExecutor>(connection);
    if (std::get<bool>(field(input, "registered").data)) {
        auto& dialect = user_context.make_resource<SqliteDialect>();
        auto& schema = user_context.make_resource<Schema>();
        auto& state = user_context.make<SqlExecutorState>();
        auto& sql_executor = user_context.make<Executor>(dialect, executor, schema, state);
        user_context.insert_resource(user_context, sql_executor);
    }
    const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text},
                            DynamicFieldDefinitions::Field{"count", DataType::I64}};
    auto& definitions = user_context.make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "Item", "v1", fields)).value());
    auto selection =
        Result<DynamicFieldSelection, DynamicFieldError>::ok(r, DynamicFieldSelection::all(r));
    if (!std::holds_alternative<std::monostate>(field(input, "selection").data)) {
        Vector<DynamicFieldSelection::Field> fields(r);
        for (const auto& f : std::get<Json::Array>(field(input, "selection").data)) {
            const auto& pair = std::get<Json::Array>(f.data);
            fields.emplace_back(text(pair[0]), DataType(std::get<std::uint64_t>(pair[1].data)));
        }
        auto created = DynamicFieldSelection::fields(r, fields);
        if (!created) {
            return error(r, created.error());
        }
        selection = std::move(created);
    }
    const auto namespace_name = text(field(input, "namespace"));
    const bool duplicate = std::get<bool>(field(input, "duplicate").data);
    const DynamicFieldsProvider* provider;
    if (text(field(input, "mode")) == "fixture") {
        DynamicStorageRows rows(r);
        for (const auto& [id, payload] : std::get<Json::Object>(field(input, "stored").data)) {
            std::uint64_t parsed = 0;
            auto result = std::from_chars(id.data(), id.data() + id.size(), parsed);
            if (result.ec != std::errc{}) {
                throw std::runtime_error("invalid owner");
            }
            auto& row = rows.try_emplace(parsed).first->second;
            for (const auto& [code, v] : std::get<Json::Object>(payload.data)) {
                row.emplace(String(code, r), value(r, v));
            }
        }
        Vector<InMemoryDynamicFieldsProvider::Owner> owners(r);
        owners.push_back({&definitions, &rows});
        if (duplicate) {
            owners.push_back(owners.front());
        }
        auto created = InMemoryDynamicFieldsProvider::from_owners(r, owners);
        if (!created) {
            return error(r, created.error());
        }
        provider = &user_context.make<InMemoryDynamicFieldsProvider>(std::move(created).value());
    } else {
        Vector<const DynamicFieldDefinitions*> owners(r);
        owners.push_back(&definitions);
        if (duplicate) {
            owners.push_back(&definitions);
        }
        using Provider = DatabaseDynamicFieldsProvider<Executor>;
        auto created = Provider::create(r, namespace_name, owners);
        if (!created) {
            return error(r, created.error());
        }
        provider = &user_context.make<Provider>(std::move(created).value());
    }
    auto owner = text(field(input, "owner"));
    auto intent = std::move(QueryIntent::create(r, "inspect extensions", "hydrate views")).value();
    Vector<std::uint64_t> ids(r);
    for (const auto& id : std::get<Json::Array>(field(input, "ids").data)) {
        ids.push_back(std::get<std::uint64_t>(id.data));
    }
    Json::Object out(r);
    auto schema = completed(provider->ensure_schema(user_context));
    put(out, "schema", schema ? Json(r) : error(r, schema.error()));
    auto load =
        completed(provider->load_values(user_context, owner, ids, selection.value(), intent));
    if (!load) {
        put(out, "load", error(r, load.error()));
    } else {
        Json::Object result(r), rows(r);
        put(result,
            "bound",
            Json(r,
                 load.value().definitions.get().storage_binding_matches(
                     namespace_name, *executor.source_identity())));
        for (const auto& [id, row] : load.value().rows) {
            Json::Object payload(r);
            for (const auto& [code, v] : row) {
                put(payload, code, v.to_json_value(r));
            }
            put(rows, std::to_string(id), Json(r, payload));
        }
        put(result, "rows", Json(r, rows));
        put(out, "load", Json(r, result));
    }
    auto& foreign = user_context.make<SqliteMutationExecutor>(connection);
    auto stream = provider->prepare_stream_read(
        user_context,
        std::get<bool>(field(input, "foreign").data) ? foreign : executor,
        owner,
        selection.value(),
        intent);
    put(out, "stream", stream ? Json(r) : error(r, stream.error()));
    auto& transaction = executor.begin_sql(r).wait().value().get();
    auto binding = text(field(input, "binding"));
    auto expected = [&] {
        if (binding == "unbound") {
            return DynamicFieldDefinitions(std::allocator_arg, Allocator(r), definitions);
        }
        if (binding == "wrong_schema") {
            return std::move(DynamicFieldDefinitions::from_fields(r, "Item", "v2", fields)).value();
        }
        return definitions.with_storage_binding(
            r, binding == "wrong_profile" ? "other" : namespace_name, *executor.source_identity());
    }();
    auto& managed_expected = user_context.make<DynamicFieldDefinitions>(std::move(expected));
    DynamicFieldWrite::Changes changes(r);
    changes.emplace(String("title", r), DynamicFieldMutation::set(r, Value(r, "proposed")));
    auto graph = provider->prepare_graph_write(
        user_context, *transaction.dynamic_field_store(), managed_expected, 7, changes);
    if (!graph) {
        put(out, "graph", error(r, graph.error()));
    } else {
        Json::Object result(r);
        put(result, "namespace", Json(r, graph.value().namespace_name));
        put(result, "id", Json(r, graph.value().owner_id));
        put(result, "changes", Json(r, std::uint64_t(graph.value().changes.size())));
        put(out, "graph", Json(r, result));
    }
    transaction.rollback_sql(r).wait();
    put(out, "supports_graph", Json(r, provider->supports_graph_save()));
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
                throw std::runtime_error("invalid input");
            }
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
