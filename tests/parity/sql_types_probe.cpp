#include <teaql/sql/bindings.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;

static Json outcome(Memory r, SqlRenderResult result) {
    auto out = detail::model_json(r);
    if (result) {
        detail::model_put(out, "ok", result.value());
    } else {
        detail::model_put(out, "error", result.error());
    }
    return out;
}

static Json evaluate(Memory r, const Json& input) {
    auto kind = text(field(input, "kind"));
    auto out = detail::model_json(r);
    if (kind == "value") {
        return outcome(r,
                       render_sql_value(r, value(r, field(input, "value")), DatabaseKind::Sqlite));
    }
    if (kind == "render") {
        CompiledQuery query(std::allocator_arg, Allocator(r));
        query.sql = text(field(input, "sql"));
        for (const auto& param : std::get<Json::Array>(field(input, "params").data)) {
            query.params.emplace_back(value(r, param));
        }
        if (auto* comment = optional(input, "comment")) {
            if (auto* s = std::get_if<String>(&comment->data)) {
                query.comment.emplace(*s, r);
            }
        }
        Json::Array called(r);
        auto render = [&](std::size_t i) {
            called.emplace_back(i);
            if (auto* failure = optional(input, "literal_failure")) {
                if (std::get<std::uint64_t>(failure->data) == i) {
                    return SqlRenderResult::err(r, "redacted literal omitted");
                }
            }
            return render_sql_value(r, query.params[i], DatabaseKind::Sqlite);
        };
        auto rendered = render_sql_with(
            r, query.sql_with_comment(r), DatabaseKind::Sqlite, query.params.size(), render);
        detail::model_put(out, "with_comment", query.sql_with_comment(r));
        detail::model_put(out, "debug", query.debug_sql(r, DatabaseKind::Sqlite));
        detail::model_put(out, "render", outcome(r, std::move(rendered)));
        detail::model_put(out, "called", called);
        return out;
    }
    if (kind == "bindings") {
        EntityDescriptor entity(std::allocator_arg, Allocator(r), "Record");
        for (const auto& property : std::get<Json::Array>(field(input, "properties").data)) {
            entity.property(text(field(property, "name")), DataType::Text)
                .column(text(field(property, "column")));
        }
        entity.audit_mask_fields_declared = std::get<bool>(field(input, "declared").data);
        for (const auto& name : std::get<Json::Array>(field(input, "masked").data)) {
            entity.audit_mask_fields.emplace_back(text(name));
        }
        SqlBindings bindings(r);
        for (const auto& item : std::get<Json::Array>(field(input, "values").data)) {
            auto v = value(r, field(item, "value"));
            auto* f = optional(item, "field");
            if (f && std::holds_alternative<String>(f->data)) {
                bindings.push_field(entity, text(*f), v);
            } else {
                bindings.push(v);
            }
        }
        if (std::get<bool>(field(input, "untrusted").data)) {
            bindings.mark_untrusted_sql();
        }
        auto log = bindings.log_context(r, DatabaseKind::Sqlite);
        Json::Array policies(r), values(r);
        for (auto p : log.parameter_policies) {
            policies.emplace_back(unsigned(p));
        }
        for (const auto& v : std::move(bindings).into_values()) {
            values.emplace_back(v.to_json_value(r));
        }
        detail::model_put(out, "policies", policies);
        detail::model_put(out, "values", values);
        detail::model_put(out, "generated", log.generated_sql);
        detail::model_put(out, "database", *log.database_kind);
        return out;
    }
    if (kind == "error") {
        SqlCompileError e(
            std::allocator_arg,
            Allocator(r),
            SqlCompileError::Code(std::get<std::uint64_t>(field(input, "code").data)));
        if (auto* v = optional(input, "name")) {
            e.name = text(*v);
        }
        if (auto* v = optional(input, "reason")) {
            e.reason = text(*v);
        }
        if (auto* v = optional(input, "identifier")) {
            e.identifier = text(*v);
        }
        if (auto* v = optional(input, "version")) {
            e.version = number<std::int64_t>(*v);
        }
        if (auto* v = optional(input, "data_type")) {
            e.data_type = DataType(std::get<std::uint64_t>(v->data));
        }
        if (auto* v = optional(input, "actual")) {
            e.actual_bytes = std::get<std::uint64_t>(v->data);
        }
        if (auto* v = optional(input, "max")) {
            e.max_bytes = std::get<std::uint64_t>(v->data);
        }
        detail::model_put(out, "text", e.to_string(r));
        return out;
    }
    throw std::runtime_error("unknown fixture kind");
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("invalid fixture JSON");
        }
        std::cout << evaluate(r, *input).dump(r) << '\n';
    }
}
