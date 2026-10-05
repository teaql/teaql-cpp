#include <teaql/sql/types.hpp>
#include <teaql/core/detail/number_text.hpp>
#include <teaql/core/detail/debug.hpp>
#include <algorithm>
#include <charconv>
#include <cmath>

namespace teaql {
namespace {
template <class T>
void number(String& out, T n) {
    char buf[32];
    auto [end, error] = std::to_chars(buf, buf + sizeof(buf), n);
    (void)error;
    out.append(buf, end);
}

String quote(Memory r, std::string_view text) {
    String out("'", r);
    for (char c : text) {
        out += c;
        if (c == '\'') {
            out += '\'';
        }
    }
    out += '\'';
    return out;
}

bool finite(const Value& value) {
    if (auto* n = std::get_if<double>(&value.data)) {
        return std::isfinite(*n);
    }
    if (auto* values = value.as_list()) {
        return std::all_of(values->begin(), values->end(), finite);
    }
    return true;
}

String literal(Memory r, const Value& value) {
    return std::visit(
        [&](const auto& v) -> String {
            using T = std::decay_t<decltype(v)>;
            String out(r);
            if constexpr (std::same_as<T, std::monostate> || std::same_as<T, TypedNull>) {
                out = "NULL";
            } else if constexpr (std::same_as<T, bool>) {
                out = v ? "TRUE" : "FALSE";
            } else if constexpr (std::same_as<T, std::int64_t> || std::same_as<T, std::uint64_t>) {
                number(out, v);
            } else if constexpr (std::same_as<T, double>) {
                detail::append_float(out, v, false);
            } else if constexpr (std::same_as<T, Decimal>) {
                out = v.to_string(r);
            } else if constexpr (std::same_as<T, String>) {
                out = quote(r, v);
            } else if constexpr (std::same_as<T, Json>) {
                out = quote(r, v.dump(r));
            } else if constexpr (std::same_as<T, Date>) {
                out = quote(r, v.to_string(r));
            } else if constexpr (std::same_as<T, Timestamp>) {
                number(out, v.as_millis());
            } else if constexpr (std::same_as<T, Value::Object>) {
                out = quote(r, value.to_json_value(r).dump(r));
            } else if constexpr (std::same_as<T, Value::List>) {
                out += '(';
                bool first = true;
                for (const auto& item : v) {
                    if (!first) {
                        out += ", ";
                    }
                    first = false;
                    out += literal(r, item);
                }
                out += ')';
            }
            return out;
        },
        value.data);
}

}

SqlRenderResult render_sql_value(Memory r, const Value& value, DatabaseKind kind) {
    if (kind != DatabaseKind::Sqlite) {
        return SqlRenderResult::err(r, "unsupported database kind");
    }
    if (!finite(value)) {
        return SqlRenderResult::err(r, "non-finite SQL number");
    }
    return SqlRenderResult::ok(r, literal(r, value));
}

SqlRenderResult render_sql_with(Memory r,
                                std::string_view sql,
                                DatabaseKind kind,
                                std::size_t count,
                                FunctionRef<SqlRenderResult(std::size_t)> render) {
    auto error = [&](std::string_view reason) {
        return SqlRenderResult::err(r, reason);
    };
    if (kind != DatabaseKind::Sqlite) {
        return error("unsupported database kind");
    }
    String out(r);
    out.reserve(sql.size());
    std::size_t i = 0, positional = 0;
    while (i < sql.size()) {
        auto start = i;
        char c = sql[i];
        if (c == '\'' || c == '"' || c == '`' || c == '[') {
            char close = c == '[' ? ']' : c;
            ++i;
            for (;;) {
                if (i >= sql.size()) {
                    return error("unterminated quoted SQL");
                }
                if (sql[i++] == close) {
                    if (i < sql.size() && sql[i] == close && c != '[') {
                        ++i;
                    } else {
                        break;
                    }
                }
            }
            out.append(sql.substr(start, i - start));
        } else if (sql.substr(i).starts_with("--")) {
            i += 2;
            while (i < sql.size() && sql[i] != '\n' && sql[i] != '\r') {
                ++i;
            }
            out.append(sql.substr(start, i - start));
        } else if (sql.substr(i).starts_with("/*")) {
            i += 2;
            std::size_t depth = 1;
            while (i < sql.size() && depth) {
                if (sql.substr(i).starts_with("/*")) {
                    ++depth;
                    i += 2;
                } else if (sql.substr(i).starts_with("*/")) {
                    --depth;
                    i += 2;
                } else {
                    ++i;
                }
            }
            if (depth) {
                return error("unterminated SQL comment");
            }
            out.append(sql.substr(start, i - start));
        } else if (c == '?') {
            if (i + 1 < sql.size() && sql[i + 1] >= '0' && sql[i + 1] <= '9') {
                return error("unsupported numbered positional binding");
            }
            if (positional >= count) {
                return error("missing binding");
            }
            auto rendered = render(positional++);
            if (!rendered) {
                return error(rendered.error());
            }
            out += rendered.value();
            ++i;
        } else if (c == ':' || c == '@' || c == '$') {
            return error("unsupported named binding");
        } else {
            out += c;
            ++i;
        }
    }
    if (positional != count) {
        return error("unused binding");
    }
    return SqlRenderResult::ok(r, std::move(out));
}

String CompiledQuery::sql_with_comment(Memory r) const {
    if (!comment || comment->empty()) {
        return String(sql, r);
    }
    String out("/* ", r);
    for (std::size_t i = 0; i < comment->size(); ++i) {
        if ((*comment)[i] == '*' && i + 1 < comment->size() && (*comment)[i + 1] == '/') {
            out += "* /";
            ++i;
        } else {
            out += (*comment)[i];
        }
    }
    out += " */ ";
    out += sql;
    return out;
}

String CompiledQuery::debug_sql(Memory r, DatabaseKind kind) const {
    auto render = [&](std::size_t i) {
        return render_sql_value(r, params[i], kind);
    };
    auto rendered = render_sql_with(r, sql_with_comment(r), kind, params.size(), render);
    if (rendered) {
        return std::move(rendered).value();
    }
    String out("[SQL omitted: ", r);
    out += rendered.error();
    out += "; NOT REPLAYABLE]";
    return out;
}

String SqlCompileError::to_string(Memory r) const {
    String out(r);
    switch (code) {
    case Code::UnknownEntity:
        out = "unknown entity: ";
        out += name;
        break;
    case Code::UnknownField:
        out = "unknown field: ";
        out += name;
        break;
    case Code::EmptyInList:
        out = "IN requires at least one value";
        break;
    case Code::MissingIdProperty:
        out = "entity ";
        out += name;
        out += " has no id property";
        break;
    case Code::MissingVersionProperty:
        out = "entity ";
        out += name;
        out += " has no version property";
        break;
    case Code::EmptyMutation:
        out = name;
        out += " requires at least one writable field";
        break;
    case Code::InvalidRecoverVersion:
        out = "recover requires a negative version, got ";
        number(out, version);
        break;
    case Code::UnsupportedSchemaType: {
        constexpr std::string_view names[] = {"Bool",
                                              "I64",
                                              "U64",
                                              "F64",
                                              "Decimal",
                                              "Text",
                                              "LargeText",
                                              "Json",
                                              "Date",
                                              "Timestamp"};
        out = "unsupported schema type: ";
        out += names[unsigned(data_type)];
        break;
    }
    case Code::InvalidSchemaShape:
        out = "invalid schema shape for property ";
        out += name;
        out += ": ";
        out += reason;
        break;
    case Code::SchemaIdentifierTooLong:
        out = name;
        out += " SQL identifier ";
        out += detail::debug_text(r, identifier);
        out += " is ";
        number(out, actual_bytes);
        out += " bytes; this database allows at most ";
        number(out, max_bytes);
        out += " bytes. Shorten the model-derived table or column name before ensure_schema";
        break;
    case Code::InvalidFunctionArguments:
        out = name;
        break;
    case Code::InvalidSubQueryOperator:
        out = "subquery does not support operator: ";
        out += name;
        break;
    }
    return out;
}
}
