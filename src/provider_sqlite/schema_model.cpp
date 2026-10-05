#include <teaql/provider_sqlite/detail/schema.hpp>
#include <teaql/core/detail/text.hpp>
#include <charconv>
#include <set>

namespace teaql::detail {
namespace {
void number(String& out, std::uint64_t n) {
    char b[32];
    auto end = std::to_chars(b, b + sizeof(b), n).ptr;
    out.append(b, end);
}

String location(Memory r,
                std::string_view message,
                const EntityDescriptor& e,
                const PropertyDescriptor& p) {
    String out(message, r);
    out += "entity=";
    out += e.name;
    out += ", table=";
    out += e.table_name;
    out += ", column=";
    out += p.column_name;
    return out;
}

void option(String& out, std::optional<std::uint32_t> n) {
    if (n) {
        out += "Some(";
        number(out, *n);
        out += ')';
    } else {
        out += "None";
    }
}
}

std::string_view strip_sqlite_identifier_quotes(std::string_view value) {
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                              (value.front() == '`' && value.back() == '`') ||
                              (value.front() == '[' && value.back() == ']'))) {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

String sqlite_type_family(Memory r, std::string_view type) {
    trim(type);
    String normalized(type, r);
    for (auto& c : normalized) {
        if (c >= 'a' && c <= 'z') {
            c -= 32;
        }
    }
    auto has = [&](std::string_view s) {
        return normalized.find(s) != normalized.npos;
    };
    if (has("INT")) {
        return String("integer", r);
    }
    if (has("CHAR") || has("CLOB") || has("TEXT")) {
        return String("text", r);
    }
    if (has("REAL") || has("FLOA") || has("DOUB")) {
        return String("real", r);
    }
    if (has("NUM") || has("DEC")) {
        return String("numeric", r);
    }
    if (has("BOOL")) {
        return String("boolean", r);
    }
    if (has("JSON")) {
        return String("json", r);
    }
    if (has("TIMESTAMP") || has("DATETIME")) {
        return String("timestamp", r);
    }
    if (has("DATE")) {
        return String("date", r);
    }
    for (auto& c : normalized) {
        if (c >= 'A' && c <= 'Z') {
            c += 32;
        }
    }
    return normalized;
}

Vector<std::uint32_t> sqlite_declared_type_args(Memory r, std::string_view type) {
    Vector<std::uint32_t> out(r);
    auto start = type.find('(');
    if (start == type.npos) {
        return out;
    }
    type.remove_prefix(start + 1);
    auto end = type.find(')');
    if (end == type.npos) {
        return out;
    }
    type = type.substr(0, end);
    for (;;) {
        auto split = type.find(',');
        auto part = type.substr(0, split);
        trim(part);
        if (part.starts_with('+')) {
            part.remove_prefix(1);
        }
        std::uint32_t n = 0;
        auto [stop, ec] = std::from_chars(part.data(), part.data() + part.size(), n);
        if (ec == std::errc{} && stop == part.data() + part.size()) {
            out.push_back(n);
        }
        if (split == type.npos) {
            break;
        }
        type.remove_prefix(split + 1);
    }
    return out;
}

void ensure_sqlite_column_compatibility(Memory r,
                                        const EntityDescriptor& e,
                                        const PropertyDescriptor& p,
                                        std::string_view actual,
                                        bool nullable) {
    auto family = sqlite_type_family(r, actual);
    bool compatible = false;
    switch (p.data_type) {
    case DataType::Bool:
        compatible = family == "boolean" || family == "integer";
        break;
    case DataType::I64:
    case DataType::U64:
        compatible = family == "integer";
        break;
    case DataType::F64:
        compatible = family == "real";
        break;
    case DataType::Decimal:
        compatible = family == "numeric";
        break;
    case DataType::Text:
    case DataType::LargeText:
        compatible = family == "text";
        break;
    case DataType::Json:
        compatible = family == "json" || family == "text";
        break;
    case DataType::Date:
        compatible = family == "date" || family == "text";
        break;
    case DataType::Timestamp:
        compatible = family == "timestamp" || family == "integer";
        break;
    }
    if (!compatible) {
        constexpr std::string_view types[]{"Bool",
                                           "I64",
                                           "U64",
                                           "F64",
                                           "Decimal",
                                           "Text",
                                           "LargeText",
                                           "Json",
                                           "Date",
                                           "Timestamp"};
        auto message = location(r, "ensure schema incompatible column type: ", e, p);
        message += ", expected=";
        message += types[unsigned(p.data_type)];
        message += ", actual=";
        message += actual;
        throw MutationExecutorError::bind(r, message);
    }
    if (p.nullable != nullable) {
        auto message = location(r, "ensure schema incompatible column nullability: ", e, p);
        message += ", expected_nullable=";
        message += p.nullable ? "true" : "false";
        message += ", actual_nullable=";
        message += nullable ? "true" : "false";
        throw MutationExecutorError::bind(r, message);
    }
    auto args = sqlite_declared_type_args(r, actual);
    auto precision = args.empty() ? std::nullopt : std::optional<std::uint32_t>(args[0]);
    auto scale = args.size() < 2 ? std::nullopt : std::optional<std::uint32_t>(args[1]);
    if (p.max_length && !storage_length_covers(*p.max_length, precision)) {
        auto message = location(r, "ensure schema existing column is too narrow: ", e, p);
        message += ", required_max_length=";
        number(message, *p.max_length);
        message += ", actual_max_length=";
        option(message, precision);
        throw MutationExecutorError::bind(r, message);
    }
    if (p.numeric_precision && p.numeric_scale &&
        !storage_numeric_covers(*p.numeric_precision, *p.numeric_scale, precision, scale)) {
        auto message = location(
            r,
            "ensure schema existing numeric column does not cover the model value domain: ",
            e,
            p);
        message += ", required_precision=";
        number(message, *p.numeric_precision);
        message += ", required_scale=";
        number(message, *p.numeric_scale);
        message += ", actual_precision=";
        option(message, precision);
        message += ", actual_scale=";
        option(message, scale);
        throw MutationExecutorError::bind(r, message);
    }
}

std::array<char, 4> sqlite_compatible_soundex(std::optional<std::string_view> input) {
    std::array<char, 4> out{'?', '0', '0', '0'};
    if (!input) {
        return out;
    }
    auto upper = [](unsigned char c) {
        return c >= 'a' && c <= 'z' ? static_cast<unsigned char>(c - 32) : c;
    };
    auto code = [&](unsigned char c) {
        switch (upper(c)) {
        case 'B':
        case 'F':
        case 'P':
        case 'V':
            return 1;
        case 'C':
        case 'G':
        case 'J':
        case 'K':
        case 'Q':
        case 'S':
        case 'X':
        case 'Z':
            return 2;
        case 'D':
        case 'T':
            return 3;
        case 'L':
            return 4;
        case 'M':
        case 'N':
            return 5;
        case 'R':
            return 6;
        default:
            return 0;
        }
    };
    std::size_t n = 0;
    unsigned previous = 0;
    for (unsigned char raw : *input) {
        auto c = upper(raw);
        if (c < 'A' || c > 'Z') {
            continue;
        }
        if (!n) {
            out[n++] = char(c);
            previous = code(c);
            continue;
        }
        auto current = code(c);
        if (current && unsigned(current) != previous) {
            out[n++] = char('0' + current);
            if (n == out.size()) {
                break;
            }
        }
        previous = unsigned(current);
    }
    return out;
}

Vector<SqliteForeignKey>
sqlite_foreign_keys_for(Memory r,
                        const EntityDescriptor& source_entity,
                        std::span<const EntityDescriptor* const> entities) {
    std::pmr::set<SqliteForeignKey> keys(r);
    for (auto* entity : entities) {
        for (const auto& relation : entity->relations) {
            const EntityDescriptor* target = nullptr;
            for (auto* candidate : entities) {
                if (candidate->name == relation.target_entity) {
                    target = candidate;
                    break;
                }
            }
            if (!target || entity->data_service != target->data_service) {
                continue;
            }
            auto* source = relation.many ? target : entity;
            auto* referenced = relation.many ? entity : target;
            const auto& source_key = relation.many ? relation.foreign_key : relation.local_key;
            const auto& referenced_key = relation.many ? relation.local_key : relation.foreign_key;
            if (source->name != source_entity.name) {
                continue;
            }
            auto* source_property = source->property_by_name(source_key);
            auto* referenced_property = referenced->property_by_name(referenced_key);
            if (!source_property || !referenced_property) {
                String message("cannot ensure relation ", r);
                message += entity->name;
                message += '.';
                message += relation.name;
                message += source_property ? ": referenced key " : ": source key ";
                message += source_property ? referenced->name : source->name;
                message += '.';
                message += source_property ? referenced_key : source_key;
                message += " does not exist";
                throw MutationExecutorError::bind(r, message);
            }
            keys.emplace(source_property->column_name,
                         referenced->table_name,
                         referenced_property->column_name);
        }
    }
    Vector<SqliteForeignKey> result(r);
    for (const auto& key : keys) {
        result.emplace_back(key);
    }
    return result;
}

String compile_sqlite_create_table(Memory r,
                                   const SqliteDialect& dialect,
                                   const EntityDescriptor& entity,
                                   std::span<const EntityDescriptor* const> entities) {
    String sql("CREATE TABLE IF NOT EXISTS ", r);
    sql += dialect.quote_ident(r, entity.table_name);
    sql += " (";
    bool first = true;
    auto separator = [&] {
        if (!first) {
            sql += ", ";
        }
        first = false;
    };
    try {
        for (const auto& property : entity.properties) {
            separator();
            sql += dialect.column_definition_sql(r, property);
        }
    } catch (SqlCompileError& error) {
        throw MutationExecutorError::sql_compile(r, std::move(error));
    }
    for (const auto& key : sqlite_foreign_keys_for(r, entity, entities)) {
        separator();
        sql += "FOREIGN KEY (";
        sql += dialect.quote_ident(r, key[0]);
        sql += ") REFERENCES ";
        sql += dialect.quote_ident(r, key[1]);
        sql += " (";
        sql += dialect.quote_ident(r, key[2]);
        sql += ')';
    }
    sql += ')';
    return sql;
}
}
