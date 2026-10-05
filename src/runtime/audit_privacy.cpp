#include <teaql/runtime/detail/audit_privacy.hpp>
#include <teaql/runtime/logging.hpp>
#include <teaql/runtime/log_manager.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/sha256.hpp>
#include <teaql/sql/types.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/number_text.hpp>
#include <teaql/data_service/sql_log.hpp>
#include <algorithm>
#include <cstdlib>
#include <charconv>
#include <numeric>

namespace teaql {
namespace {
Vector<std::size_t> boundaries(Memory r, std::string_view text) {
    Vector<std::size_t> out(r);
    out.push_back(0);
    std::size_t i = 0;
    while (i < text.size()) {
        auto b = static_cast<unsigned char>(text[i++]);
        char32_t code = b;
        unsigned n = b < 0x80 ? 0 : b < 0xE0 ? 1 : b < 0xF0 ? 2 : 3;
        if (n) {
            if (b < 0xC2 || b > 0xF4 || i + n > text.size())
                throw std::invalid_argument("Invalid UTF-8 in audit value");
            code = b & ((1u << (6 - n)) - 1);
            for (unsigned k = 0; k < n; ++k) {
                auto c = static_cast<unsigned char>(text[i++]);
                if ((c & 0xC0) != 0x80)
                    throw std::invalid_argument("Invalid UTF-8 in audit value");
                code = (code << 6) | (c & 63);
            }
            if ((n == 1 && code < 0x80) || (n == 2 && code < 0x800) || (n == 3 && code < 0x10000) ||
                code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
                throw std::invalid_argument("Invalid UTF-8 in audit value");
        }
        out.push_back(i);
    }
    return out;
}

String mask(Memory r, std::string_view text) {
    auto b = boundaries(r, text);
    auto count = b.size() - 1;
    if (count < 8 || std::all_of(text.begin(), text.end(), [](char c) {
            return c >= '0' && c <= '9';
        }))
        return String(count, '*', r);
    String out(text.substr(0, b[2]), r);
    out.append(count - 4, '*');
    out += text.substr(b[count - 2]);
    return out;
}

bool credentials(const Json& value) {
    if (auto* fields = std::get_if<Json::Object>(&value.data)) {
        for (const auto& [key, v] : *fields)
            if (is_credential_log_name(key) || credentials(v))
                return true;
    } else if (auto* values = std::get_if<Json::Array>(&value.data)) {
        for (const auto& v : *values)
            if (credentials(v))
                return true;
    }
    return false;
}

bool credentials(const Value& value) {
    if (auto* fields = value.as_object()) {
        for (const auto& [key, v] : *fields)
            if (is_credential_log_name(key) || credentials(v))
                return true;
    } else if (auto* values = value.as_list()) {
        for (const auto& v : *values)
            if (credentials(v))
                return true;
    } else if (auto* json = std::get_if<Json>(&value.data))
        return credentials(*json);
    return false;
}

void collect(const Json& value, Vector<String>& out) {
    auto r = out.get_allocator().resource();
    if (auto* s = std::get_if<String>(&value.data)) {
        if (!s->empty())
            out.emplace_back(*s);
    } else if (auto* values = std::get_if<Json::Array>(&value.data)) {
        for (const auto& v : *values)
            collect(v, out);
    } else if (auto* fields = std::get_if<Json::Object>(&value.data)) {
        for (const auto& [_, v] : *fields)
            collect(v, out);
    } else if (!std::holds_alternative<std::monostate>(value.data))
        out.emplace_back(value.dump(r));
}

template <class T>
String number(Memory r, T value) {
    char buffer[32];
    auto end = std::to_chars(buffer, buffer + sizeof(buffer), value).ptr;
    return String(buffer, end, r);
}

void collect(const Value& value, Vector<String>& out) {
    auto r = out.get_allocator().resource();
    std::visit(
        [&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::same_as<T, String>) {
                if (!v.empty())
                    out.emplace_back(v);
            } else if constexpr (std::same_as<T, Value::Object>) {
                for (const auto& [_, x] : v)
                    collect(x, out);
            } else if constexpr (std::same_as<T, Value::List>) {
                for (const auto& x : v)
                    collect(x, out);
            } else if constexpr (std::same_as<T, Json>)
                collect(v, out);
            else if constexpr (std::same_as<T, bool>)
                out.emplace_back(v ? "true" : "false");
            else if constexpr (std::same_as<T, std::int64_t> || std::same_as<T, std::uint64_t>)
                out.emplace_back(number(r, v));
            else if constexpr (std::same_as<T, double>) {
                String text(r);
                detail::append_float(text, v, false);
                out.emplace_back(std::move(text));
            } else if constexpr (std::same_as<T, Decimal> || std::same_as<T, Date>)
                out.emplace_back(v.to_string(r));
            else if constexpr (std::same_as<T, Timestamp>)
                out.emplace_back(number(r, v.as_millis()));
        },
        value.data);
}

void sort_secrets(Vector<String>& secrets) {
    auto r = secrets.get_allocator().resource();
    Vector<std::size_t> order(secrets.size(), r);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](auto a, auto b) {
        return secrets[a].size() != secrets[b].size() ? secrets[a].size() > secrets[b].size()
                                                      : a < b;
    });
    Vector<String> sorted(r);
    sorted.reserve(secrets.size());
    for (auto i : order)
        sorted.emplace_back(std::move(secrets[i]));
    secrets = std::move(sorted);
}

void scrub(String& value, const Vector<String>& secrets) {
    for (const auto& secret : secrets) {
        if (secret.empty())
            continue;
        std::size_t at = 0;
        while ((at = value.find(secret, at)) != value.npos) {
            value.replace(at, secret.size(), "[REDACTED]");
            at += 10;
        }
    }
}

void scrub_trace(TraceChain& trace, const Vector<String>& secrets) {
    for (auto& node : trace) {
        scrub(node.comment, secrets);
        scrub(node.entity_type, secrets);
    }
}

bool marked(std::span<const String> fields, std::string_view field) {
    return std::any_of(fields.begin(), fields.end(), [&](const auto& f) {
        return f == field;
    });
}

bool nested_credentials(const EntityPropertyChange& change) {
    return (change.old_value && credentials(*change.old_value)) ||
           (change.new_value && credentials(*change.new_value));
}
}

namespace detail {
bool audit_plaintext_enabled(Memory r) {
    return application_context(r).log_manager().plaintext_enabled();
}

RawAuditEvent audit_event_projection(Memory r, const RawAuditEvent& source, bool allow) {
    auto result = clone_in(r, source);
    Vector<String> secrets(r);
    auto redact = [&](std::string_view field, Value& value) {
        if (is_credential_log_name(field) || credentials(value) ||
            (!allow && field != "id" && field != "version")) {
            collect(value, secrets);
            if (!value.is_null())
                value = Value(r, "[REDACTED]");
        }
    };
    auto record = [&](Record& values) {
        for (auto& [field, value] : values)
            redact(field, value);
    };
    record(result.values);
    if (result.old_values)
        record(*result.old_values);
    if (result.new_values)
        record(*result.new_values);
    for (auto& change : result.changes) {
        if (change.old_value)
            redact(change.field, *change.old_value);
        if (change.new_value)
            redact(change.field, *change.new_value);
    }
    if (auto id = source.values.find("id"); id != source.values.end())
        collect(id->second, secrets);
    sort_secrets(secrets);
    scrub_trace(result.trace_chain, secrets);
    if (result.bootstrap_audit) {
        scrub(result.bootstrap_audit->reason, secrets);
        scrub(result.bootstrap_audit->actor, secrets);
    }
    return result;
}
}

std::pair<String, bool> limit_audit_value(Memory r, std::string_view value, std::size_t maximum) {
    auto b = boundaries(r, value);
    auto count = b.size() - 1;
    if (count <= maximum)
        return {String(value, r), false};
    if (maximum <= 3)
        return {String(maximum, '*', r), true};
    auto keep = maximum - 3, head = keep / 2, tail = keep - head;
    String out(value.substr(0, b[head]), r);
    out += "...";
    out += value.substr(b[count - tail]);
    return {std::move(out), true};
}

SafeAuditField build_safe_audit_field(Memory r,
                                      std::string_view field,
                                      std::optional<std::string_view> raw,
                                      std::span<const String> masks,
                                      std::optional<std::size_t> maximum) {
    SafeAuditField out(std::allocator_arg, Allocator(r), field);
    if (!raw)
        return out;
    out.raw_length = boundaries(r, *raw).size() - 1;
    bool credential = is_credential_log_name(field);
    out.masked = credential || (marked(masks, field) && !detail::audit_plaintext_enabled(r));
    out.value.emplace(credential   ? String("[REDACTED]", r)
                      : out.masked ? mask(r, *raw)
                                   : String(*raw, r));
    if (maximum) {
        auto limited = limit_audit_value(r, *out.value, *maximum);
        out.value = std::move(limited.first);
        out.truncated = limited.second;
    }
    out.output_length = boundaries(r, *out.value).size() - 1;
    if (out.masked)
        out.mask_reason.emplace("_audit_mask_fields", r);
    if (out.truncated)
        out.truncate_reason.emplace("_audit_value_max_len", r);
    return out;
}

SafeAuditEvent RawAuditEvent::build_safe_event(Memory r,
                                               std::span<const String> masks,
                                               std::optional<std::size_t> maximum) const {
    SafeAuditEvent out(std::allocator_arg, Allocator(r), kind, entity);
    auto allow = detail::audit_plaintext_enabled(r);
    Vector<String> secrets(r);
    for (const auto& change : changes)
        if (is_credential_log_name(change.field) || (!allow && marked(masks, change.field)) ||
            nested_credentials(change)) {
            if (change.old_value)
                collect(*change.old_value, secrets);
            if (change.new_value)
                collect(*change.new_value, secrets);
        }
    sort_secrets(secrets);
    for (const auto& change : changes) {
        if (change.field.starts_with('_'))
            continue;
        std::optional<String> raw;
        if (change.new_value) {
            auto& value = *change.new_value;
            if (auto* text = std::get_if<String>(&value.data))
                raw.emplace(*text, r);
            else if (auto* n = std::get_if<std::int64_t>(&value.data))
                raw.emplace(number(r, *n));
            else if (auto* n = std::get_if<std::uint64_t>(&value.data))
                raw.emplace(number(r, *n));
            else
                raw.emplace(detail::debug_value(r, value));
        }
        auto field =
            build_safe_audit_field(r,
                                   change.field,
                                   raw ? std::optional<std::string_view>(*raw) : std::nullopt,
                                   masks,
                                   maximum);
        if (nested_credentials(change)) {
            field.value.emplace("[REDACTED]", r);
            field.masked = true;
            field.output_length = 10;
            field.truncated = false;
        } else if (field.value) {
            scrub(*field.value, secrets);
            field.output_length = boundaries(r, *field.value).size() - 1;
        }
        out.fields.emplace_back(std::move(field));
    }
    for (const auto& node : trace_chain)
        out.trace_chain.emplace_back(node);
    if (auto id = values.find("id"); id != values.end())
        collect(id->second, secrets);
    sort_secrets(secrets);
    scrub_trace(out.trace_chain, secrets);
    out.mutation_governance = detail::clone_in(r, mutation_governance);
    return out;
}
}

namespace teaql {
namespace {
Value mask_business_value(Memory r, const Value& value) {
    if (value.is_null())
        return Value(r, value);
    if (auto* list = value.as_list()) {
        Value::List result(r);
        for (const auto& v : *list)
            result.emplace_back(mask_business_value(r, v));
        return Value(r, result);
    }
    if (value.as_object() || std::holds_alternative<Json>(value.data))
        return Value(r, "[REDACTED]");
    Vector<String> texts(r);
    collect(value, texts);
    String text(r);
    for (const auto& t : texts)
        text += t;
    return Value(r, mask(r, text));
}

std::array<std::uint8_t, 32> sql_fingerprint(Memory r, const SqlLogEntry& entry) {
    auto text = detail::debug_sql_log(r, entry);
    detail::Sha256 hash;
    hash.update({reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
    return hash.finish();
}

struct SafeSqlProjection {
    using allocator_type = Allocator;
    std::array<std::uint8_t, 32> fingerprint;
    SqlLogEntry safe;

    SafeSqlProjection(std::allocator_arg_t,
                      Allocator a,
                      std::array<std::uint8_t, 32> fingerprint,
                      const SqlLogEntry& safe)
        : fingerprint(fingerprint), safe(std::allocator_arg, a, safe) {}

    SafeSqlProjection(std::allocator_arg_t, Allocator a, const SafeSqlProjection& other)
        : SafeSqlProjection(std::allocator_arg, a, other.fingerprint, other.safe) {}
};

void scrub_intent(SqlLogEntry& entry, const Vector<String>& secrets) {
    for (auto* text : {&entry.comment, &entry.purpose, &entry.audit_reason, &entry.result_type})
        if (*text)
            scrub(**text, secrets);
    scrub(entry.result_summary, secrets);
    scrub_trace(entry.trace_path, secrets);
}

SqlLogEntry sql_entry_inner(Memory r, const SqlLogEntry& entry, bool allow, bool hide_intent) {
    using Policy = SqlParameterLogPolicy;
    auto result = detail::clone_in(r, entry);
    result.log_context.projection_state = SqlProjectionState(r);
    // The edge runtime supports SQLite only; other dialects are deliberately
    // unknown to this projection, even though their core discriminators exist.
    bool sqlite = entry.log_context.database_kind == "sqlite";
    const auto& policies = entry.log_context.parameter_policies;
    const auto& masks = entry.log_context.masked_parameters;
    bool invalid_policies = !policies.empty() && policies.size() != entry.params.size();
    bool invalid_masks = !masks.empty() && masks.size() != entry.params.size();
    bool credential = is_credential_log_name(entry.sql) &&
                      (!entry.log_context.generated_sql || policies.empty() || invalid_policies);
    Vector<String> secrets(r);
    Vector<bool> masked(r);
    masked.reserve(entry.params.size());
    for (std::size_t i = 0; i < entry.params.size(); ++i) {
        const auto& value = entry.params[i];
        auto policy = invalid_policies || invalid_masks ? Policy::Unknown
                      : i < policies.size()             ? policies[i]
                                                        : Policy::Unknown;
        bool already_masked =
            !invalid_masks && entry.log_context.mode.has_value() && i < masks.size() && masks[i];
        bool force = credential || credentials(value) || policy == Policy::Credential;
        bool protect = force || policy == Policy::Unknown || (policy == Policy::Masked && !allow);
        masked.push_back(protect || already_masked);
        if (protect && !already_masked) {
            collect(value, secrets);
            result.params[i] = value.is_null()                      ? Value(r, value)
                               : policy == Policy::Masked && !force ? mask_business_value(r, value)
                                                                    : Value(r, "[REDACTED]");
        }
    }
    entry.log_context.intent_redactions.extend_secrets(allow, secrets);
    sort_secrets(secrets);
    scrub_intent(result, secrets);
    result.log_context.intent_redactions.clear();
    result.log_context.masked_parameters = std::move(masked);
    result.log_context.mode.emplace(allow ? "debug_plaintext" : "safe", r);
    auto render = [&]() -> SqlRenderResult {
        auto fail = [&](std::string_view reason) {
            return SqlRenderResult::err(r, reason);
        };
        if (entry.log_context.omission_reason)
            return fail("previously omitted SQL");
        if (!sqlite)
            return fail("unknown database dialect");
        if (invalid_policies)
            return fail("parameter policy count mismatch");
        if (invalid_masks)
            return fail("parameter mask count mismatch");
        if (!entry.log_context.generated_sql) {
            auto placeholder = [&](std::size_t) {
                return SqlRenderResult::ok(r, String(r));
            };
            auto text = render_sql_with(
                r, entry.sql, DatabaseKind::Sqlite, entry.params.size(), placeholder);
            if (!text)
                return text;
            if (text.value().find_first_of("'\"`") != String::npos ||
                text.value().find("--") != String::npos ||
                text.value().find("/*") != String::npos ||
                std::any_of(text.value().begin(), text.value().end(), [](char c) {
                    return c >= '0' && c <= '9';
                }))
                return fail("untrusted SQL literals or comments");
        }
        auto literal = [&](std::size_t i) {
            auto text = render_sql_value(r, result.params[i], DatabaseKind::Sqlite);
            if (text && result.log_context.masked_parameters[i])
                text.value() += " /* masked */";
            return text;
        };
        return render_sql_with(r, entry.sql, DatabaseKind::Sqlite, result.params.size(), literal);
    };
    auto rendered = render();
    bool has_masked = std::any_of(result.log_context.masked_parameters.begin(),
                                  result.log_context.masked_parameters.end(),
                                  [](bool b) {
                                      return b;
                                  });
    std::string_view mode = allow ? "DEBUG PLAINTEXT; EXPLICIT OPT-IN" : "SAFE";
    result.debug_sql = "-- TeaQL ";
    result.debug_sql += mode;
    if (rendered) {
        result.log_context.omission_reason.reset();
        if (has_masked)
            result.debug_sql += "; MASKED; NOT REPLAYABLE";
        result.debug_sql += '\n';
        result.debug_sql += rendered.value();
    } else {
        for (const auto& value : entry.params)
            collect(value, secrets);
        sort_secrets(secrets);
        scrub_intent(result, secrets);
        auto reason = rendered.error();
        if (entry.log_context.omission_reason) {
            constexpr std::string_view known[]{"parameter policy count mismatch",
                                               "parameter mask count mismatch",
                                               "unknown database dialect",
                                               "untrusted SQL literals or comments",
                                               "unsupported escape string",
                                               "unterminated quoted SQL",
                                               "ambiguous MySQL backslash quoting",
                                               "unsupported executable SQL comment",
                                               "unterminated SQL comment",
                                               "unsupported MySQL hash comment",
                                               "missing binding",
                                               "invalid binding index",
                                               "unsupported dollar expression",
                                               "unterminated dollar quote",
                                               "unsupported numbered positional binding",
                                               "unsupported named binding",
                                               "unused binding",
                                               "non-finite SQL number",
                                               "ambiguous MySQL backslash literal"};
            auto previous = std::string_view(*entry.log_context.omission_reason);
            reason = std::find(std::begin(known), std::end(known), previous) != std::end(known)
                         ? previous
                         : "previously omitted SQL";
        }
        result.log_context.omission_reason.emplace(reason, r);
        result.sql = "[REDACTED SQL; NOT REPLAYABLE]";
        for (auto& value : result.params)
            value = Value(r);
        result.debug_sql += "; MASKED; NOT REPLAYABLE\n[SQL omitted: ";
        result.debug_sql += reason;
        result.debug_sql += ']';
    }
    result.pretty_sql = result.debug_sql;
    if (hide_intent) {
        for (auto* text :
             {&result.comment, &result.purpose, &result.audit_reason, &result.result_type})
            if (*text)
                **text = "[REDACTED]";
        result.result_summary = "[REDACTED]";
        for (auto& node : result.trace_path) {
            node.comment = "[REDACTED]";
            node.entity_type = "[REDACTED]";
        }
    }
    std::optional<std::uint64_t> count;
    std::string_view verb;
    if (entry.result_count) {
        count = std::uint64_t(*entry.result_count);
        verb = "returned";
    } else if (entry.affected_rows) {
        count = entry.affected_rows;
        verb = "affected";
    }
    if (count) {
        auto singular = number(r, *count);
        singular += " row ";
        singular += verb;
        auto plural = number(r, *count);
        plural += " rows ";
        plural += verb;
        if (entry.result_summary == singular || entry.result_summary == plural)
            result.result_summary = entry.result_summary;
        else if (hide_intent)
            result.result_summary = plural;
    }
    return result;
}
}

namespace detail {
SqlLogEntry sql_entry_projection(Memory r, const SqlLogEntry& entry, bool allow) {
    allow = allow && entry.log_context.mode != "safe";
    bool was_debug = entry.log_context.mode == "debug_plaintext" ||
                     entry.debug_sql.starts_with("-- TeaQL DEBUG PLAINTEXT; EXPLICIT OPT-IN");
    const SqlLogEntry* alternative = nullptr;
    if (was_debug)
        if (auto* state = entry.log_context.projection_state.get<SafeSqlProjection>();
            state && state->fingerprint == sql_fingerprint(r, entry))
            alternative = &state->safe;
    if (!allow && alternative)
        return clone_in(r, *alternative);
    auto result = sql_entry_inner(r, entry, allow, was_debug && !allow);
    if (allow) {
        auto safe =
            alternative ? clone_in(r, *alternative) : sql_entry_inner(r, entry, false, was_debug);
        auto fingerprint = sql_fingerprint(r, result);
        result.log_context.projection_state =
            SqlProjectionState::make<SafeSqlProjection>(r, fingerprint, safe);
    }
    return result;
}
}
}
