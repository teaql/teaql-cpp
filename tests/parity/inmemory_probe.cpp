#include "fixture_value.hpp"
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/inmemory_engine.hpp>
#include <iostream>

static Expr expression(Memory r, const Json& input) {
    auto kind = text(field(input, "kind"));
    if (kind == "column") {
        return Expr::column(r, text(field(input, "name")));
    }
    if (kind == "value") {
        return Expr::value(r, value(r, field(input, "literal")));
    }
    auto tag = Expr::Kind::Function;
    if (kind == "binary") {
        tag = Expr::Kind::Binary;
    } else if (kind == "and") {
        tag = Expr::Kind::And;
    } else if (kind == "or") {
        tag = Expr::Kind::Or;
    } else if (kind == "not") {
        tag = Expr::Kind::Not;
    } else if (kind == "isnull") {
        tag = Expr::Kind::IsNull;
    } else if (kind == "notnull") {
        tag = Expr::Kind::IsNotNull;
    } else if (kind == "between") {
        tag = Expr::Kind::Between;
    } else if (kind == "subquery") {
        tag = Expr::Kind::SubQuery;
    }
    Expr out(r, tag);
    if (auto* args = optional(input, "args")) {
        for (const auto& arg : std::get<Json::Array>(args->data)) {
            out.args.emplace_back(expression(r, arg));
        }
    }
    if (kind == "binary") {
        out.op = BinaryOp(detail::model_integer<unsigned>(field(input, "op"), "op"));
    }
    return out;
}

static CompactRow row(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    Vector<String> names(r);
    Vector<Value> values(r);
    for (const auto& pair : std::get<Json::Array>(input.data)) {
        const auto& p = std::get<Json::Array>(pair.data);
        names.emplace_back(text(p[0]));
        values.emplace_back(value(r, p[1]));
    }
    auto& layout = user.make<ColumnLayout>(std::move(names));
    return CompactRow(std::allocator_arg, Allocator(r), layout, std::move(values));
}

static Json evaluate(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    auto out = detail::model_json(r);
    if (text(field(input, "op")) == "eval") {
        auto expr = expression(r, field(input, "expr"));
        auto data = row(user, field(input, "row"));
        detail::model_put(out, "eval", ExprEvaluator::eval(expr, data));
        detail::model_put(out, "value", detail::debug_value(r, ExprEvaluator::resolve(expr, data)));
        return out;
    }
    const auto& q = field(input, "query");
    SelectQuery query(std::allocator_arg, Allocator(r), "Item");
    if (auto* p = optional(q, "filter")) {
        query.predicate = expression(r, *p);
    }
    if (auto* p = optional(q, "having")) {
        query.having_predicate = expression(r, *p);
    }
    if (auto* p = optional(q, "projection")) {
        for (const auto& name : std::get<Json::Array>(p->data)) {
            query.projection.emplace_back(text(name));
        }
    }
    if (auto* p = optional(q, "groups")) {
        for (const auto& name : std::get<Json::Array>(p->data)) {
            query.group_by_fields.emplace_back(text(name));
        }
    }
    if (auto* p = optional(q, "aggregates")) {
        for (const auto& agg : std::get<Json::Array>(p->data)) {
            const auto& a = std::get<Json::Array>(agg.data);
            query.aggregates.emplace_back(
                AggregateFunction(detail::model_integer<unsigned>(a[0], "aggregate")),
                text(a[1]),
                text(a[2]));
        }
    }
    if (auto* p = optional(q, "order")) {
        for (const auto& ob : std::get<Json::Array>(p->data)) {
            const auto& o = std::get<Json::Array>(ob.data);
            query.order_by.emplace_back(
                text(o[0]), std::get<bool>(o[1].data) ? SortDirection::Desc : SortDirection::Asc);
            if (o.size() > 2) {
                query.order_by.back().expr = expression(r, o[2]);
            }
        }
    }
    if (auto* p = optional(q, "slice")) {
        const auto& s = std::get<Json::Array>(p->data);
        query.slice = Slice{{}, detail::model_integer<std::uint64_t>(s[0], "offset")};
        if (!std::holds_alternative<std::monostate>(s[1].data)) {
            query.slice->limit = detail::model_integer<std::uint64_t>(s[1], "limit");
        }
    }
    if (auto* p = optional(q, "comment")) {
        query.comment_text = String(text(*p), r);
    }
    if (auto* p = optional(q, "hard_limit")) {
        query.hard_limit(detail::model_integer<std::uint64_t>(*p, "hard limit"));
    }
    Vector<CompactRow> data(r);
    for (const auto& item : std::get<Json::Array>(field(input, "rows").data)) {
        data.emplace_back(row(user, item));
    }
    auto result = InMemoryQueryEngine::execute(r, query, std::move(data));
    Json::Array rows(r);
    for (const auto& record : result.rows) {
        Json::Array pairs(r);
        for (std::size_t i = 0; i < record.len(); ++i) {
            Json::Array pair(r);
            pair.emplace_back(record.keys()[i]);
            pair.emplace_back(detail::debug_value(r, record.values()[i]));
            pairs.emplace_back(pair);
        }
        rows.emplace_back(pairs);
    }
    const auto& m = result.metadata;
    auto metadata = detail::model_json(r);
    detail::model_put(metadata, "backend", m.backend);
    detail::model_put(
        metadata, "operation", m.operation == DataServiceOperation::Query ? "Query" : "Mutation");
    detail::model_put(
        metadata, "result_count", m.result_count ? Json(r, *m.result_count) : Json(r));
    detail::model_put(metadata, "statements", m.statements.size());
    detail::model_put(metadata, "sql_log_default", m.sql_log == SqlLogContext(r));
    detail::model_put(metadata, "debug_query", m.debug_query ? Json(r, *m.debug_query) : Json(r));
    detail::model_put(
        metadata, "affected_rows", m.affected_rows ? Json(r, *m.affected_rows) : Json(r));
    detail::model_put(metadata, "trace", m.trace_chain.size());
    detail::model_put(metadata, "comment", m.comment ? Json(r, *m.comment) : Json(r));
    detail::model_put(metadata,
                      "backend_request_id",
                      m.backend_request_id ? Json(r, *m.backend_request_id) : Json(r));
    detail::model_put(metadata,
                      "parameterized_query",
                      m.parameterized_query ? Json(r, *m.parameterized_query) : Json(r));
    detail::model_put(metadata, "params", m.params.size());
    detail::model_put(metadata, "timestamps_ordered", m.started_at <= m.ended_at);
    detail::model_put(out, "rows", rows);
    detail::model_put(out, "metadata", metadata);
    return out;
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--pointer-bits") {
        std::cout << sizeof(std::size_t) * 8 << '\n';
        return 0;
    }
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            return 2;
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
