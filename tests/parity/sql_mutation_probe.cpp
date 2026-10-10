#include "fixture_sql.hpp"
#include <charconv>
#include <iostream>

static MutationValues mutation_values(Memory r, const Json& j) {
    MutationValues out(r);
    for (const auto& [key, v] : std::get<Json::Object>(j.data)) {
        out.fields.emplace(String(key, r), value(r, v));
    }
    return out;
}

static std::optional<std::int64_t> version(const Json& j, std::string_view key) {
    auto p = optional(j, key);
    return p && std::holds_alternative<String>(p->data) ? std::optional(number<std::int64_t>(*p))
                                                        : std::nullopt;
}

static CompiledQuery
compile(Memory r, const SqlDialect& d, const EntityDescriptor& e, const Json& j) {
    auto mode = text(field(j, "mode"));
    std::optional<Expr> guard;
    if (auto p = optional(j, "guard")) {
        guard.emplace(expression(r, *p));
    }
    if (mode == "insert") {
        InsertCommand c(std::allocator_arg, Allocator(r), "IgnoredCommandEntity");
        c.values = mutation_values(r, field(j, "values"));
        return d.compile_insert(r, e, c);
    }
    if (mode == "batch_insert") {
        BatchInsertCommand c(std::allocator_arg, Allocator(r), "IgnoredCommandEntity");
        for (const auto& row : items(j, "rows")) {
            c.batch_values.emplace_back(mutation_values(r, row));
        }
        return d.compile_batch_insert(r, e, c);
    }
    if (mode == "update") {
        UpdateCommand c(
            std::allocator_arg, Allocator(r), "IgnoredCommandEntity", value(r, field(j, "id")));
        c.values = mutation_values(r, field(j, "values"));
        c.expected_version_value = version(j, "version");
        return guard ? d.compile_guarded_update(r, e, c, *guard) : d.compile_update(r, e, c);
    }
    if (mode == "delete") {
        DeleteCommand c(
            std::allocator_arg, Allocator(r), "IgnoredCommandEntity", value(r, field(j, "id")));
        c.soft_delete = std::get<bool>(field(j, "soft").data);
        c.expected_version_value = version(j, "version");
        return guard ? d.compile_guarded_delete(r, e, c, *guard) : d.compile_delete(r, e, c);
    }
    if (mode == "recover") {
        RecoverCommand c(std::allocator_arg,
                         Allocator(r),
                         "IgnoredCommandEntity",
                         value(r, field(j, "id")),
                         *version(j, "version"));
        return guard ? d.compile_guarded_recover(r, e, c, *guard) : d.compile_recover(r, e, c);
    }
    if (mode == "batch_update") {
        BatchUpdateCommand c(std::allocator_arg, Allocator(r), "IgnoredCommandEntity");
        for (const auto& v : items(j, "fields")) {
            c.update_fields.emplace_back(text(v));
        }
        for (const auto& v : items(j, "ids")) {
            c.batch_ids.emplace_back(value(r, v));
        }
        for (const auto& v : items(j, "versions")) {
            c.batch_expected_versions.emplace_back(std::holds_alternative<String>(v.data)
                                                       ? std::optional(number<std::int64_t>(v))
                                                       : std::nullopt);
        }
        for (const auto& v : items(j, "rows")) {
            c.batch_values.emplace_back(mutation_values(r, v));
        }
        return d.compile_batch_update(r, e, c);
    }
    throw std::runtime_error("unknown mutation");
}

class RecordingDialect : public FixtureDialect {
public:
    Memory owner;
    bool record;
    std::optional<std::string_view> fail_quote;
    mutable Vector<String> events;

    RecordingDialect(Memory owner, bool record) : owner(owner), record(record), events(owner) {}

    DatabaseKind kind() const override {
        event("kind");
        return FixtureDialect::kind();
    }

    String quote_ident(Memory r, std::string_view name) const override {
        if (record) {
            String item("quote:", owner);
            item += name;
            event(item);
        }
        if (fail_quote == name) {
            throw std::runtime_error("fixture quote failure");
        }
        return FixtureDialect::quote_ident(r, name);
    }

    String placeholder(Memory r, std::size_t index) const override {
        if (record) {
            String item("placeholder:", owner);
            char buffer[32];
            auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), index);
            if (error != std::errc{}) {
                throw std::runtime_error("Cannot observe placeholder index");
            }
            item.append(buffer, end);
            event(item);
        }
        return FixtureDialect::placeholder(r, index);
    }

private:
    void event(std::string_view text) const {
        if (record) {
            events.emplace_back(text);
        }
    }
};

static Json evaluate(Memory r, const Json& j) {
    auto observed = optional(j, "observe_hooks");
    RecordingDialect d(r, observed && std::get<bool>(observed->data));
    if (auto p = optional(j, "numbered")) {
        d.numbered = std::get<bool>(p->data);
    }
    if (auto p = optional(j, "fail_quote")) {
        if (std::holds_alternative<String>(p->data)) {
            d.fail_quote = text(*p);
        }
    }
    auto e = entity(r, field(j, "entity"));
    auto out = detail::model_json(r);
    try {
        auto q = compile(r, d, e, j);
        detail::model_put(out, "sql", q.sql);
        detail::model_put(out, "comment", q.comment ? Json(r, *q.comment) : Json(r));
        detail::model_put(out, "generated", q.log_context.generated_sql);
        Json::Array values(r), kinds(r), null_types(r), policies(r);
        for (const auto& v : q.params) {
            values.emplace_back(v.to_json_value(r));
            kinds.emplace_back(unsigned(v.data.index()));
            if (auto n = std::get_if<TypedNull>(&v.data)) {
                null_types.emplace_back(unsigned(n->type));
            } else {
                null_types.emplace_back();
            }
        }
        for (auto p : q.log_context.parameter_policies) {
            policies.emplace_back(unsigned(p));
        }
        detail::model_put(out, "values", values);
        detail::model_put(out, "kinds", kinds);
        detail::model_put(out, "null_types", null_types);
        detail::model_put(out, "policies", policies);
    } catch (const SqlCompileError& e) {
        detail::model_put(out, "error", e.to_string(r));
    } catch (const std::overflow_error&) {
        detail::model_put(out, "panic", "overflow");
    } catch (const std::out_of_range&) {
        detail::model_put(out, "panic", "index");
    } catch (const std::runtime_error& error) {
        if (std::string_view(error.what()) != "fixture quote failure") {
            throw;
        }
        detail::model_put(out, "panic", "hook");
    }
    if (d.record) {
        Json::Array events(r);
        for (const auto& item : d.events) {
            events.emplace_back(item);
        }
        detail::model_put(out, "events", events);
    }
    return out;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("invalid fixture");
        }
        std::cout << evaluate(r, *input).dump(r) << '\n';
    }
}
