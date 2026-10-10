#include <teaql/sql.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static Json evaluate(Memory owner, const Json& input) {
    SqlBindings bindings(owner);
    Json::Object output(owner);
    output.emplace("initial", formatted_debug(owner, bindings));
    EntityDescriptor entity(std::allocator_arg, Allocator(owner), "Device");
    entity.mask_fields({"masked"});
    entity.property("plain", DataType::Text);
    entity.property("masked", DataType::Text);
    entity.property("token", DataType::Text, "access_token");
    std::optional<SqlParameterLogPolicy> scope;
    if (auto* index = std::get_if<std::uint64_t>(&field(input, "scope").data)) {
        scope = static_cast<SqlParameterLogPolicy>(*index);
    }
    {
        detail::SqlBindingScope active(bindings, scope, false);
        for (const auto& item : std::get<Json::Array>(field(input, "params").data)) {
            auto owned = value(owner, item);
            if (auto* name = optional(item, "field")) {
                bindings.push_field(entity, text(*name), std::move(owned));
            } else {
                bindings.push(std::move(owned));
            }
        }
        if (std::get<bool>(field(input, "untrusted").data)) {
            bindings.mark_untrusted_sql();
        }
        output.emplace("bindings", formatted_debug(owner, bindings));
        Json::Array values(owner);
        for (const auto& item : bindings) {
            values.emplace_back(formatted_debug(owner, item));
        }
        output.emplace("deref", Json(owner, std::move(values)));
        SqlBindings cloned(std::allocator_arg, Allocator(owner), bindings);
        cloned.push(Value(owner, "clone-only"));
        output.emplace("clone", formatted_debug(owner, cloned));
    }
    CompiledQuery query(std::allocator_arg, Allocator(owner));
    query.log_context = bindings.log_context(owner, DatabaseKind::Sqlite);
    query.sql = text(field(input, "sql"));
    query.params = std::move(bindings).into_values();
    if (auto* comment = std::get_if<String>(&field(input, "comment").data)) {
        query.comment.emplace(*comment, owner);
    }
    output.emplace("before", formatted_debug(owner, query));
    if (auto* outcome = std::get_if<std::uint64_t>(&field(input, "outcome").data)) {
        query.log_context.execution_outcome = static_cast<SqlExecutionOutcome>(*outcome);
    }
    if (auto* mode = std::get_if<String>(&field(input, "mode").data)) {
        query.log_context.mode.emplace(*mode, owner);
    }
    if (auto* reason = std::get_if<String>(&field(input, "omission").data)) {
        query.log_context.omission_reason.emplace(*reason, owner);
    }
    for (const auto& masked : std::get<Json::Array>(field(input, "masked").data)) {
        query.log_context.masked_parameters.push_back(std::get<bool>(masked.data));
    }
    SelectQuery intent(std::allocator_arg, Allocator(owner), "Device");
    intent.comment("intent-comment");
    intent.filter(Expr::eq(owner, "token", "private intent must not appear in diagnostics"));
    query.log_context.intent_redactions =
        SqlIntentRedactions::from_unclassified_query(owner, intent);
    query.log_context.projection_state =
        SqlProjectionState::make<String>(owner, "private projection payload");
    output.emplace(
        "clone_equal",
        Json(owner, query == CompiledQuery(std::allocator_arg, Allocator(owner), query)));
    output.emplace("query", formatted_debug(owner, query));
    output.emplace("debug_sql", Json(owner, query.debug_sql(owner, DatabaseKind::Sqlite)));
    output.emplace("sql_with_comment", Json(owner, query.sql_with_comment(owner)));
    Vector<SqlCompileError> errors(owner);
    for (unsigned index = 0; index < 12; ++index) {
        auto& error = errors.emplace_back(static_cast<SqlCompileError::Code>(index),
                                          text(field(input, "text")));
        error.version = number<std::int64_t>(field(input, "version"));
        error.data_type = static_cast<DataType>(std::get<std::uint64_t>(field(input, "type").data));
        error.reason = "reason-";
        error.reason += text(field(input, "text"));
        error.identifier = "id-";
        error.identifier += text(field(input, "text"));
        error.actual_bytes = std::get<std::uint64_t>(field(input, "bytes").data);
        error.max_bytes = 63;
    }
    Json::Array diagnostics(owner), displays(owner), sources(owner), clones(owner), matrix(owner);
    for (const auto& error : errors) {
        diagnostics.emplace_back(formatted_debug(owner, error));
        displays.emplace_back(error.to_string(owner));
        sources.emplace_back(error.source() == nullptr);
        clones.emplace_back(error == SqlCompileError(std::allocator_arg, Allocator(owner), error));
        Json::Array row(owner);
        for (const auto& other : errors) {
            row.emplace_back(error == other);
        }
        matrix.emplace_back(std::move(row));
    }
    output.emplace("errors", Json(owner, std::move(diagnostics)));
    output.emplace("error_display", Json(owner, std::move(displays)));
    output.emplace("error_source_none", Json(owner, std::move(sources)));
    output.emplace("error_clone_equal", Json(owner, std::move(clones)));
    output.emplace("error_matrix", Json(owner, std::move(matrix)));
    Json::Array kinds(owner);
    for (auto kind : {DatabaseKind::PostgreSql, DatabaseKind::Sqlite, DatabaseKind::MySql}) {
        kinds.emplace_back(formatted_debug(owner, kind));
    }
    output.emplace("kinds", Json(owner, std::move(kinds)));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("Invalid SQL model fixture");
        }
        std::cout << evaluate(user->memory_resource(), *input).dump(user->memory_resource())
                  << '\n';
    }
}
