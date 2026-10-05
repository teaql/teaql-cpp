#include <teaql/sql/dialect.hpp>
#include <teaql/core/detail/uppercase_tables.hpp>
#include <algorithm>

namespace teaql {
namespace {
char32_t next_codepoint(std::string_view text, std::size_t& i) {
    auto byte = static_cast<unsigned char>(text[i++]);
    char32_t code = byte;
    if (byte < 0x80)
        return code;
    unsigned count = byte >= 0xF0 && byte <= 0xF4   ? 3
                     : byte >= 0xE0 && byte <= 0xEF ? 2
                     : byte >= 0xC2 && byte <= 0xDF ? 1
                                                    : 0;
    if (!count || i + count > text.size())
        throw std::invalid_argument("Invalid UTF-8 in SQL identifier");
    code &= (1u << (6 - count)) - 1;
    for (unsigned j = 0; j < count; ++j) {
        auto next = static_cast<unsigned char>(text[i++]);
        if ((next & 0xC0) != 0x80)
            throw std::invalid_argument("Invalid UTF-8 in SQL identifier");
        code = (code << 6) | (next & 63);
    }
    if ((count == 1 && code < 0x80) || (count == 2 && code < 0x800) ||
        (count == 3 && code < 0x10000) || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
        throw std::invalid_argument("Invalid UTF-8 in SQL identifier");
    return code;
}

void append_codepoint(String& out, char32_t c) {
    if (c < 0x80)
        out += char(c);
    else if (c < 0x800) {
        out += char(0xC0 | (c >> 6));
        out += char(0x80 | (c & 63));
    } else if (c < 0x10000) {
        out += char(0xE0 | (c >> 12));
        out += char(0x80 | ((c >> 6) & 63));
        out += char(0x80 | (c & 63));
    } else {
        out += char(0xF0 | (c >> 18));
        out += char(0x80 | ((c >> 12) & 63));
        out += char(0x80 | ((c >> 6) & 63));
        out += char(0x80 | (c & 63));
    }
}

String uppercase(Memory r, std::string_view text) {
    String out(r);
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size();) {
        auto code = next_codepoint(text, i);
        auto expanded = std::lower_bound(detail::uppercase_expansions.begin(),
                                         detail::uppercase_expansions.end(),
                                         code,
                                         [](const auto& e, char32_t c) {
                                             return e.from < c;
                                         });
        if (expanded != detail::uppercase_expansions.end() && expanded->from == code) {
            for (auto c : expanded->to)
                if (c)
                    append_codepoint(out, c);
            continue;
        }
        auto range = std::lower_bound(detail::uppercase_ranges.begin(),
                                      detail::uppercase_ranges.end(),
                                      code,
                                      [](const auto& e, char32_t c) {
                                          return e.last < c;
                                      });
        if (range != detail::uppercase_ranges.end() && code >= range->first &&
            (code - range->first) % range->step == 0)
            code = char32_t(std::int64_t(code) + range->delta);
        append_codepoint(out, code);
    }
    return out;
}

std::size_t physical_length(std::string_view identifier) {
    // Only double quotes and backticks are unwrapped by the source validator.
    // A lone quote is not a wrapped string here (unlike quote_ident's test).
    if (identifier.size() < 2 || (identifier.front() != '"' && identifier.front() != '`') ||
        identifier.front() != identifier.back())
        return identifier.size();
    char quote = identifier.front();
    auto inner = identifier.substr(1, identifier.size() - 2);
    std::size_t bytes = 0;
    for (std::size_t i = 0; i < inner.size(); ++i) {
        ++bytes;
        if (inner[i] == quote && i + 1 < inner.size() && inner[i + 1] == quote)
            ++i;
    }
    return bytes;
}
}

Vector<SchemaIndexSpec>
schema_index_specs(Memory r, const EntityDescriptor& entity, std::optional<std::size_t> max) {
    Vector<SchemaIndexSpec> specs(r);
    auto table_upper = uppercase(r, entity.table_name);
    auto bound = [&](String full) {
        return max ? bounded_sql_identifier(r, full, *max) : std::move(full);
    };
    if (auto* version = entity.version_property()) {
        String name("PK_", r);
        name += table_upper;
        name += "_ID_VERSION";
        specs.emplace_back();
        auto& spec = specs.back();
        spec.name = bound(std::move(name));
        spec.table = entity.table_name;
        spec.unique = true;
        auto* id = entity.id_property();
        spec.columns.emplace_back(id ? std::string_view(id->column_name) : std::string_view("id"));
        spec.columns.emplace_back(version->column_name);
    }
    for (const auto& property : entity.properties) {
        auto name = std::string_view(property.name);
        if (!name.ends_with("Id") && !name.ends_with("Time") && !name.ends_with("_time") &&
            name != "create_time" && name != "update_time")
            continue;
        String full("IDX_", r);
        full += table_upper;
        full += '_';
        full += uppercase(r, property.column_name);
        specs.emplace_back();
        auto& spec = specs.back();
        spec.name = bound(std::move(full));
        spec.table = entity.table_name;
        spec.columns.emplace_back(property.column_name);
    }
    return specs;
}

String schema_foreign_key_name(Memory r,
                               std::string_view source_table,
                               std::string_view source_column,
                               std::string_view referenced_table,
                               std::string_view referenced_column,
                               std::size_t max) {
    String full("FK_", r);
    full += source_table;
    full += '_';
    full += source_column;
    full += '_';
    full += referenced_table;
    full += '_';
    full += referenced_column;
    return bounded_sql_identifier(r, uppercase(r, full), max);
}

void validate_schema_identifier_lengths(Memory r,
                                        std::span<const EntityDescriptor* const> entities,
                                        std::size_t max) {
    auto validate = [&](String object, std::string_view identifier) {
        auto actual = physical_length(identifier);
        if (actual > max) {
            SqlCompileError error(std::allocator_arg,
                                  Allocator(r),
                                  SqlCompileError::Code::SchemaIdentifierTooLong,
                                  object);
            error.identifier = identifier;
            error.actual_bytes = actual;
            error.max_bytes = max;
            throw error;
        }
    };
    for (auto* entity : entities) {
        if (!entity)
            throw std::invalid_argument("Null schema entity");
        String table_object("table for entity ", r);
        table_object += entity->name;
        validate(std::move(table_object), entity->table_name);
        for (const auto& property : entity->properties) {
            String object("column for entity ", r);
            object += entity->name;
            object += " property ";
            object += property.name;
            validate(std::move(object), property.column_name);
        }
    }
}

Vector<String> SqlDialect::schema_indexes_sqls(Memory r, const EntityDescriptor& entity) const {
    Vector<String> result(r);
    for (const auto& spec : schema_index_specs(r, entity, {})) {
        String columns(r);
        bool first = true;
        for (const auto& column : spec.columns) {
            if (!first)
                columns += ", ";
            first = false;
            columns += quote_ident(r, column);
        }
        String sql(
            spec.unique ? "CREATE UNIQUE INDEX IF NOT EXISTS " : "CREATE INDEX IF NOT EXISTS ", r);
        sql += quote_ident(r, spec.name);
        sql += " ON ";
        sql += quote_ident(r, spec.table);
        sql += " (";
        sql += columns;
        sql += ')';
        result.emplace_back(std::move(sql));
    }
    return result;
}
}
