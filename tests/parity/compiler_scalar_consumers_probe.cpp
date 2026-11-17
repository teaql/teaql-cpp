#include "compiled_scalar_storage.hpp"
#include "fixture_value.hpp"
#include <iostream>
#include <sqlite3.h>
#include <type_traits>

using namespace compiled_scalar_storage;

static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::declval<const Reading&>().value())>,
                             TEAQL_EXPECTED_VALUE_TYPE>);

static void execute_sql(sqlite3* database, std::string_view sql) {
    if (sqlite3_exec(database, std::string(sql).c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

static Json readings(Memory resource, sqlite3* database) {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database,
                           "SELECT id,gateway,value,version FROM reading_data ORDER BY id",
                           -1,
                           &statement,
                           nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> cleanup(statement, sqlite3_finalize);
    Json::Array output(resource);
    for (auto status = sqlite3_step(statement); status != SQLITE_DONE;
         status = sqlite3_step(statement)) {
        if (status != SQLITE_ROW) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        Json::Object row(resource);
        for (int index = 0; index != sqlite3_column_count(statement); ++index) {
            Json value(resource);
            switch (sqlite3_column_type(statement, index)) {
            case SQLITE_NULL:
                break;
            case SQLITE_INTEGER:
                value = Json(resource, std::int64_t(sqlite3_column_int64(statement, index)));
                break;
            case SQLITE_FLOAT:
                value = Json(resource, sqlite3_column_double(statement, index));
                break;
            case SQLITE_TEXT:
                value = Json(resource,
                             reinterpret_cast<const char*>(sqlite3_column_text(statement, index)));
                break;
            default:
                throw std::runtime_error("Unexpected scalar sample storage");
            }
            row.emplace(String(sqlite3_column_name(statement, index), resource), std::move(value));
        }
        output.emplace_back(Json(resource, std::move(row)));
    }
    return Json(resource, std::move(output));
}

class Logic final : public ReadingCheckerLogic {
public:
    Logic(std::allocator_arg_t, Allocator allocator, const Json& input, Json::Array& calls)
        : ReadingCheckerLogic(std::allocator_arg, allocator),
          input_(detail::clone_in(allocator.resource(), input)), calls_(calls) {}

    void checkAndFixReading(const UserContext&,
                            Reading& entity,
                            CheckObjectStatus,
                            const ObjectLocation& location,
                            CheckResults& results) const override {
        auto resource = memory_resource();
        Json::Object call(resource);
        call.emplace("before",
                     generated_detail::encode(resource, entity.value()).to_json_value(resource));
        call.emplace("gateway", Json(resource, entity.gatewayId()));
        call.emplace("errors", Json(resource, std::uint64_t(results.size())));
        call.emplace("location", Json(resource, location.model_path(resource)));
        calls_.emplace_back(Json(resource, std::move(call)));
        auto mode = text(field(input_, "logic"));
        if (mode == "fix") {
            auto replacement = value(resource, field(input_, "next_typed"));
            auto decoded = GeneratedScalarCodec::decode<TEAQL_EXPECTED_VALUE_TYPE>(
                resource, &replacement, "Reading", "value");
            entity.updateValue(require_value(decoded));
        }
        if (mode == "reject") {
            required(false, "value_issue", location, results);
        }
    }

private:
    Json input_;
    Json::Array& calls_;
};

static Json check(UserContext& user_context, const Json& input) {
    auto resource = user_context.memory_resource();
    auto name = text(field(input, "status"));
    auto status = name == "create"   ? CheckObjectStatus::Create
                  : name == "update" ? CheckObjectStatus::Update
                                     : CheckObjectStatus::Unknown;
    Record row(resource);
    row.emplace("id", Value(resource, std::uint64_t(7)));
    row.emplace("gateway_id",
                Value(resource, std::get<std::uint64_t>(field(input, "gateway").data)));
    row.emplace("version",
                Value(resource, std::int64_t(status == CheckObjectStatus::Create ? 0 : 3)));
    auto decoded = Reading::from_compact_row(
        resource, CompactRow(std::allocator_arg, Allocator(resource), row));
    auto& entity = user_context.make<Reading>(require_value(decoded));
    auto initial = value(resource, field(input, "typed"));
    auto typed = GeneratedScalarCodec::decode<TEAQL_EXPECTED_VALUE_TYPE>(
        resource, &initial, "Reading", "value");
    entity.updateValue(require_value(typed));
    Vector<std::string_view> fields(resource);
    auto mask = text(field(input, "loaded"));
    for (auto field : {"id", "value", "gateway_id", "version"}) {
        if (mask == "identity" && field != std::string_view("id") &&
            field != std::string_view("version")) {
            continue;
        }
        if (mask == "no_value" && field == std::string_view("value")) {
            continue;
        }
        fields.emplace_back(field);
    }
    entity.load_state =
        eval::LoadState::partial(resource, fields)
            .into_indexed(detail::ContextArena::from_memory(resource).loaded_snapshots(),
                          Reading::field_layout(resource))
            .value();
    if (status == CheckObjectStatus::Create) {
        entity.root.mark_as_new(entity.entity_key());
    }
    auto& calls = user_context.make<Json::Array>();
    auto& logic = user_context.make<Logic>(input, calls);
    auto& checker = user_context.make<ReadingChecker>(logic);
    CheckResults results(resource);
    auto location = ObjectLocation::array_root(resource, 2);
    location.member("reading_info");
    checker.check_and_fix_typed(user_context, entity, status, location, results);
    Json::Array diagnostics(resource), loaded(resource), fixes(resource);
    for (auto profile : {JsonFieldNamingProfile::CamelCase,
                         JsonFieldNamingProfile::SnakeCase,
                         JsonFieldNamingProfile::PascalCase}) {
        Json::Array items(resource);
        for (const auto& result : results) {
            items.emplace_back(result.to_wire(resource, profile).to_json(resource));
        }
        diagnostics.emplace_back(Json(resource, std::move(items)));
    }
    for (auto field : {"id", "value", "gateway_id", "version"}) {
        loaded.emplace_back(Json(resource, entity.is_loaded(field)));
    }
    for (const auto& evidence : user_context.last_fix_evidence()) {
        Json::Object item(resource);
        item.emplace("entity", Json(resource, evidence.entity_type));
        item.emplace("path", Json(resource, evidence.model_path));
        item.emplace(
            "source",
            Json(resource, evidence.source == FixEvidenceSource::Context ? "Context" : "Clock"));
        item.emplace("label", Json(resource, evidence.source_label));
        fixes.emplace_back(Json(resource, std::move(item)));
    }
    Json::Object output(resource);
    output.emplace("value",
                   generated_detail::encode(resource, entity.value()).to_json_value(resource));
    output.emplace("gateway", Json(resource, entity.gatewayId()));
    output.emplace("loaded", Json(resource, std::move(loaded)));
    output.emplace("diagnostics", Json(resource, std::move(diagnostics)));
    output.emplace("fixes", Json(resource, std::move(fixes)));
    output.emplace("calls", Json(resource, calls));
    return Json(resource, std::move(output));
}

static Json sample(UserContext& user_context, const Json& input, sqlite3* database) {
    auto resource = user_context.memory_resource();
    require_value(user_context.ensure_schema().wait());
    if (!std::get<bool>(field(input, "root").data)) {
        execute_sql(database, "DELETE FROM gateway_data");
    }
    if (text(field(input, "fault")) == "trigger") {
        execute_sql(
            database,
            "CREATE TRIGGER sample_failure BEFORE INSERT ON reading_data BEGIN SELECT RAISE(ABORT,'fixture sample insert failed'); END");
    }
    auto name = text(field(input, "scale"));
    auto scale = name == "Tiny"    ? SampleDataScale::Tiny
                 : name == "Small" ? SampleDataScale::Small
                                   : SampleDataScale::Medium;
    auto seed = std::get<std::uint64_t>(field(input, "seed").data);
    Json::Array rounds(resource);
    for (std::uint64_t index = 0; index != std::get<std::uint64_t>(field(input, "rounds").data);
         ++index) {
        const auto& result = generateSampleData(user_context, {scale, seed}).wait();
        Json::Object round(resource), report(resource);
        if (result) {
            Json::Object generated(resource);
            Json::Array skipped(resource);
            for (const auto& [name, count] : result.value().generated) {
                generated.emplace(name, Json(resource, std::uint64_t(count)));
            }
            for (const auto& skip : result.value().skipped) {
                Json::Object item(resource);
                item.emplace("entity", Json(resource, skip.entity));
                item.emplace("reason", Json(resource, skip.reason));
                skipped.emplace_back(Json(resource, std::move(item)));
            }
            report.emplace("error", Json(resource));
            report.emplace("generated", Json(resource, std::move(generated)));
            report.emplace("skipped", Json(resource, std::move(skipped)));
        } else {
            report.emplace("error", Json(resource, result.error().to_string(resource)));
        }
        round.emplace("report", Json(resource, std::move(report)));
        round.emplace("rows", readings(resource, database));
        rounds.emplace_back(Json(resource, std::move(round)));
    }
    return Json(resource, std::move(rounds));
}

static Json observe(UserContext& user_context, const Json& input) {
    auto resource = user_context.memory_resource();
    auto connection = user_context.make_resource_ref<SqliteConnection>();
    require_value(serviceRuntimeFromConnection(user_context, connection));
    auto kind = text(field(input, "kind"));
    if (kind == "sample" || !std::holds_alternative<std::monostate>(field(input, "root").data)) {
        user_context.with_active_root(
            "Gateway", kind == "sample" ? 1 : std::get<std::uint64_t>(field(input, "root").data));
    }
    Json::Object output(resource);
    output.emplace("wall_millis", Json(resource, Timestamp::now().millis));
    output.emplace("result",
                   kind == "sample"
                       ? sample(user_context, input, user_context.get(connection).native_handle())
                       : check(user_context, input));
    return Json(resource, std::move(output));
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    ContextLimits limits;
    limits.user.max_bytes = 1024ull * 1024 * 1024;
    limits.max_live_user_bytes = limits.user.max_bytes;
    GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
    for (std::string line; std::getline(std::cin, line);) {
        auto user_context = global.make_user_context();
        {
            auto resource = user_context->memory_resource();
            auto input = Json::parse_result(resource, line);
            std::cout << observe(*user_context, require_value(input)).dump(resource) << '\n';
        }
        user_context.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Scalar Checker/sample observer retains Context memory");
        }
    }
}
