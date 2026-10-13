#include "../support/dynamic_hydration_fixture.hpp"
#include "fixture_value.hpp"
#include <sqlite3.h>
#include <iostream>

using dynamic_hydration_fixture::School;
using dynamic_hydration_fixture::Executor;

static void put(Json::Object& out, std::string_view name, Json v) {
    out.emplace(String(name, out.get_allocator().resource()), std::move(v));
}

static Json error(Memory r, std::string_view message) {
    Json::Object result(r);
    put(result, "error", Json(r, message));
    return Json(r, result);
}

template <class T>
static T completed(AsyncOperation<T>& operation) {
    operation.wait();
    return operation.take_result();
}

static Json describe(Memory r, const Vector<School>& entities) {
    Json::Array views(r), sharing(r);
    for (const auto& e : entities) {
        Json::Object view(r), loaded(r);
        for (std::string_view name : {"id", "name", "version", "address", "#title", "#count"}) {
            put(loaded, name, Json(r, e.is_field_loaded(name)));
        }
        put(view, "loaded", Json(r, loaded));
        if (auto* fields = e.dynamic_field_values()) {
            put(view, "values", record_to_json_value(r, fields->values()));
        } else {
            put(view, "values", Json(r));
        }
        put(view, "bits", Json(r, e.loaded_state_snapshot()->get().bits()));
        put(view, "dirty", Json(r, e.dirty_fields().has_value()));
        views.emplace_back(view);
        Json::Array row(r);
        for (const auto& other : entities) {
            row.emplace_back(e.loaded_state_snapshot()->identity() ==
                             other.loaded_state_snapshot()->identity());
        }
        sharing.emplace_back(row);
    }
    Json::Object result(r);
    put(result, "views", Json(r, views));
    put(result, "sharing", Json(r, sharing));
    return Json(r, result);
}

static Json describe_row(Memory r, CompactRow row) {
    Json::Object out(r);
    auto dynamic = row.take_loaded_dynamic_fields();
    put(out, "dynamic", dynamic ? record_to_json_value(r, dynamic->values()) : Json(r));
    auto loaded = row.take_loaded_relation("peers");
    if (loaded) {
        Json::Array rows(r);
        for (auto& child : loaded->data) {
            rows.emplace_back(describe_row(r, std::move(child)));
        }
        put(out, "loaded", Json(r, rows));
    } else {
        put(out, "loaded", Json(r));
    }
    put(out, "native", record_to_json_value(r, row.into_map(r)));
    return Json(r, out);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto selected =
        Result<DynamicFieldSelection, DynamicFieldError>::ok(r, DynamicFieldSelection::all(r));
    if (!std::holds_alternative<std::monostate>(field(input, "selection").data)) {
        Vector<DynamicFieldSelection::Field> fields(r);
        for (const auto& item : std::get<Json::Array>(field(input, "selection").data)) {
            const auto& p = std::get<Json::Array>(item.data);
            fields.emplace_back(text(p[0]), DataType(std::get<std::uint64_t>(p[1].data)));
        }
        selected = DynamicFieldSelection::fields(r, fields);
        if (!selected) {
            return error(r, selected.error().to_string(r));
        }
    }
    const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text},
                            DynamicFieldDefinitions::Field{"count", DataType::I64}};
    auto& definition = user_context.make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "School", "v1", fields)).value());
    if (text(field(input, "mode")) == "decode") {
        Vector<CompactRow> rows(r);
        for (const auto& item : std::get<Json::Array>(field(input, "rows").data)) {
            Record native(r);
            native.emplace(String("id", r),
                           Value(r, std::get<std::uint64_t>(field(item, "id").data)));
            native.emplace(String("name", r), Value(r, "school"));
            rows.emplace_back(native);
            Value::Object payload(r);
            for (const auto& [name, v] : std::get<Json::Object>(field(item, "values").data)) {
                payload.emplace(String(name, r), value(r, v));
            }
            auto values = DynamicFieldValues::from_values(r, definition, payload);
            if (!values) {
                return error(r, values.error().to_string(r));
            }
            rows.back().set_loaded_dynamic_fields(values.value());
        }
        auto& root = user_context.make<EntityRuntimeState>();
        auto decoded = decode_compact_rows_with_read_metadata<School>(r, std::move(rows), root);
        return decoded ? describe(r, decoded.value()) : error(r, decoded.error().to_string(r));
    }
    SqliteConnection* connection = nullptr;
    auto& executor = dynamic_hydration_fixture::setup(user_context, &connection, false);
    if (text(field(input, "mode")) == "relation") {
        auto& metadata = dynamic_cast<InMemoryMetadataStore&>(
            const_cast<SchemaProvider&>(executor.schema_provider));
        auto descriptor = School::entity_descriptor(r);
        descriptor.relation("peers", "School").many = std::get<bool>(field(input, "many").data);
        metadata.register_entity(std::move(descriptor));
    }
    if (sqlite3_exec(connection->native_handle(),
                     "CREATE TABLE school_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER)",
                     nullptr,
                     nullptr,
                     nullptr) != SQLITE_OK) {
        throw std::runtime_error("fixture schema");
    }
    for (const auto& id : std::get<Json::Array>(field(input, "ids").data)) {
        auto sql = std::string("INSERT INTO school_data VALUES(") +
                   std::to_string(std::get<std::uint64_t>(id.data)) + ",'school',1)";
        if (sqlite3_exec(connection->native_handle(), sql.c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
            throw std::runtime_error("fixture seed");
        }
    }
    DynamicStorageRows stored(r);
    for (const auto& [id, p] : std::get<Json::Object>(field(input, "stored").data)) {
        std::uint64_t parsed = 0;
        auto conversion = std::from_chars(id.data(), id.data() + id.size(), parsed);
        if (conversion.ec != std::errc{}) {
            throw std::runtime_error("fixture owner");
        }
        auto& row = stored.try_emplace(parsed).first->second;
        for (const auto& [name, v] : std::get<Json::Object>(p.data)) {
            row.emplace(String(name, r), value(r, v));
        }
    }
    auto provider = text(field(input, "provider"));
    if (provider == "database") {
        executor.dynamic_field_store()->ensure_schema(r).wait();
        auto& tx = executor.transport.begin_sql(r).wait().value().get();
        Vector<DynamicFieldWrite> writes(r);
        for (const auto& [id, p] : stored) {
            DynamicFieldWrite::Changes changes(r);
            for (const auto& [name, v] : p) {
                changes.emplace(String(name, r), DynamicFieldMutation::set(r, v));
            }
            writes.emplace_back("profile", definition, id, changes);
        }
        auto reason = std::move(MutationIntent::create(r, "seed extensions")).value();
        auto written = completed(tx.dynamic_field_store()->apply_writes(r, writes, reason));
        if (!written) {
            return error(r, written.error().to_string(r));
        }
        tx.commit_sql(r).wait();
        using Provider = DatabaseDynamicFieldsProvider<Executor>;
        const std::array<const DynamicFieldDefinitions*, 1> definitions{&definition};
        auto& configured = user_context.make<Provider>(
            std::move(Provider::create(r, "profile", definitions)).value());
        user_context.set_dynamic_fields_provider(user_context, configured);
    } else if (provider == "fixture") {
        const std::array owners{InMemoryDynamicFieldsProvider::Owner{&definition, &stored}};
        auto created = InMemoryDynamicFieldsProvider::from_owners(r, owners);
        if (!created) {
            return error(r, created.error().to_string(r));
        }
        auto& configured =
            user_context.make<InMemoryDynamicFieldsProvider>(std::move(created).value());
        user_context.set_dynamic_fields_provider(user_context, configured);
    }
    SelectQuery query(std::allocator_arg, Allocator(r), "School");
    query.project("name").order_asc("id").select_dynamic_fields(selected.value());
    query.comment_text.emplace("inspect dynamic schools", r);
    if (auto* filtered = std::get_if<std::uint64_t>(&field(input, "filter").data)) {
        query.filter(Expr::eq(r, "id", Value(r, *filtered)));
    }
    if (text(field(input, "mode")) == "child") {
        query.dynamic_field_selection.reset();
        query.project("id");
        for (const auto& child : std::get<Json::Array>(field(input, "children").data)) {
            auto selection = Result<DynamicFieldSelection, DynamicFieldError>::ok(
                r, DynamicFieldSelection::all(r));
            if (!std::holds_alternative<std::monostate>(field(child, "selection").data)) {
                Vector<DynamicFieldSelection::Field> fields(r);
                for (const auto& item : std::get<Json::Array>(field(child, "selection").data)) {
                    const auto& p = std::get<Json::Array>(item.data);
                    fields.emplace_back(text(p[0]), DataType(std::get<std::uint64_t>(p[1].data)));
                }
                selection = DynamicFieldSelection::fields(r, fields);
                if (!selection) {
                    return error(r, selection.error().to_string(r));
                }
            }
            SelectQuery enhancement(std::allocator_arg, Allocator(r), "School");
            enhancement.project("id")
                .project_expr("name", Expr::value(r, text(field(child, "name"))))
                .select_dynamic_fields(selection.value());
            if (auto* filter = std::get_if<std::uint64_t>(&field(child, "filter").data)) {
                enhancement.filter(Expr::eq(r, "id", Value(r, *filter)));
            }
            query.child_enhancements.emplace_back(std::move(enhancement));
        }
    }
    if (text(field(input, "mode")) == "relation") {
        query.dynamic_field_selection.reset();
        query.project("id");
        SelectQuery relation(std::allocator_arg, Allocator(r), "School");
        relation.project("id").project("name").order_asc("id").select_dynamic_fields(
            selected.value());
        if (auto* filter = std::get_if<std::uint64_t>(&field(input, "child_filter").data)) {
            relation.filter(Expr::eq(r, "id", Value(r, *filter)));
        }
        for (std::uint64_t i = 0; i < std::get<std::uint64_t>(field(input, "depth").data); ++i) {
            SelectQuery outer(std::allocator_arg, Allocator(r), "School");
            outer.project("id").project("name");
            outer.relations.emplace_back("peers", relation);
            relation = std::move(outer);
        }
        query.relations.emplace_back("peers", relation);
    }
    auto service = user_context.entity_data_service<Executor>("School");
    if (text(field(input, "mode")) == "child" || text(field(input, "mode")) == "relation") {
        PurposedSelectQuery purposed(std::allocator_arg, Allocator(r), query, "hydrate schools");
        auto result = completed(service.value().get().fetch_all(purposed));
        if (!result) {
            return error(r, result.error().to_string(r));
        }
        Json::Array rows(r);
        for (const auto& row : result.value()) {
            rows.emplace_back(describe_row(r, detail::clone_in(r, row)));
        }
        Json::Object out(r);
        put(out, "rows", Json(r, rows));
        return Json(r, out);
    }
    if (text(field(input, "mode")) == "stream") {
        if (!std::get<bool>(field(input, "missing_id").data)) {
            query.project("id");
        }
        query.stream_config =
            StreamConfig{std::size_t(std::get<std::uint64_t>(field(input, "chunk").data))};
        PurposedSelectQuery purposed(std::allocator_arg, Allocator(r), query, "hydrate schools");
        auto result = completed(service.value().get().fetch_stream(purposed));
        if (!result) {
            return error(r, result.error().to_string(r));
        }
        auto& stream = result.value().get();
        Json::Array chunks(r);
        while (auto* next = stream.wait_next()) {
            if (!*next) {
                chunks.emplace_back(error(r, next->error().to_string(r)));
                break;
            }
            Json::Object chunk(r);
            put(chunk, "index", Json(r, std::uint64_t(next->value().chunk_index)));
            put(chunk, "last", Json(r, next->value().is_last));
            Json::Array rows(r);
            for (const auto& row : next->value().rows) {
                Json::Object entry(r);
                put(entry, "id", row.get("id")->to_json_value(r));
                put(entry,
                    "values",
                    record_to_json_value(r, row.loaded_dynamic_fields()->values()));
                rows.emplace_back(entry);
            }
            put(chunk, "rows", Json(r, rows));
            chunks.emplace_back(chunk);
        }
        stream.cancel();
        Json::Object out(r);
        put(out, "chunks", Json(r, chunks));
        return Json(r, out);
    }
    PurposedSelectQuery purposed(std::allocator_arg, Allocator(r), query, "hydrate schools");
    auto& work = text(field(input, "mode")) == "enhanced"
                     ? service.value().get().fetch_enhanced_entities<School>(purposed)
                     : service.value().get().fetch_entities<School>(purposed);
    auto result = completed(work);
    return result ? describe(r, result.value().data) : error(r, result.error().to_string(r));
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("invalid fixture");
            }
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
