// Semantic port of the locked runtime masking contracts, retaining SQLite scope.
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/log_formatter.hpp>
#include <teaql/data_service/sql_log_debug.hpp>
#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <limits>
#include <thread>

using namespace teaql;
using Policy = SqlParameterLogPolicy;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static bool contains(std::string_view text, std::string_view fragment) {
    return text.find(fragment) != std::string_view::npos;
}

static SqlLogEntry entry(Memory r, std::string_view value) {
    SqlLogEntry result(std::allocator_arg, Allocator(r), SqlLogOperation::Update);
    result.log_context.database_kind.emplace("sqlite", r);
    result.comment.emplace("what: edit customer", r);
    result.purpose.emplace("why: verify mask contract", r);
    result.audit_reason.emplace("verify mask contract", r);
    result.sql = "UPDATE customer SET name = ?";
    result.params.emplace_back(value);
    result.debug_sql = "UPDATE customer SET name = '";
    for (auto c : value) {
        result.debug_sql += c;
        if (c == '\'') {
            result.debug_sql += c;
        }
    }
    result.debug_sql += '\'';
    result.pretty_sql = result.debug_sql;
    result.elapsed = {0, 1000};
    result.affected_rows = 1;
    result.result_summary = "1 row affected";
    return result;
}

static SqlLogEntry project(Memory r, const SqlLogEntry& source, bool allow = false) {
    return detail::sql_entry_projection(r, source, allow);
}

static void omitted(Memory r, const SqlLogEntry& value, std::string_view secret) {
    check(!contains(detail::debug_sql_log(r, value), secret), "private value leaked in Debug");
    check(!contains(HumanReaderFormatter{}.format_sql_log(r, {}, value), secret),
          "private value leaked in human output");
}

static void check_audit(Memory r, std::string_view raw, std::string_view expected) {
    Vector<String> fields(r);
    fields.emplace_back("name");
    auto safe = build_safe_audit_field(r, "name", raw, fields, {});
    check(safe.masked && safe.value == expected, "original audit mask golden");
}

static void mask_golden(Memory r, const char* path) {
    std::ifstream input(path);
    check(input.is_open(), "open unchanged masking golden");
    std::string line;
    std::getline(input, line);
    std::size_t count = 0;
    while (std::getline(input, line)) {
        auto first = line.find('\t');
        auto second = line.find('\t', first + 1);
        check(first != std::string::npos && second != std::string::npos, "golden columns");
        std::string_view raw(line.data() + first + 1, second - first - 1);
        std::string_view expected(line.data() + second + 1, line.size() - second - 1);
        check_audit(r, raw, expected);
        auto source = entry(r, raw);
        source.log_context.parameter_policies = {Policy::Masked};
        auto safe = project(r, source);
        check(safe.params.size() == 1 && safe.params[0].try_text() == expected,
              "original SQL mask golden");
        String rendered("name = '", r);
        for (auto c : expected) {
            rendered += c;
            if (c == '\'') {
                rendered += c;
            }
        }
        rendered += "' /* masked */";
        check(contains(safe.debug_sql, rendered), "quoted mask annotation");
        Record values(r);
        values.emplace("name", Value(r, raw));
        auto event = RawAuditEvent::created(r, "Customer", values);
        Vector<String> fields(r);
        fields.emplace_back("name");
        auto audit = event.build_safe_event(r, fields, {});
        check(audit.fields.size() == 1 && audit.fields[0].value == expected &&
                  event.values.at(String("name", r)).try_text() == raw,
              "actual audit event preserves input and projects golden");
        ++count;
    }
    check(count == 10, "all original golden rows retained");
}

static void expanded_sql_and_audit_macros(Memory r) {
    const std::pair<std::string_view, std::string_view> cases[] = {{"", ""},
                                                                   {"Ada", "***"},
                                                                   {"12345678", "********"},
                                                                   {"ABCDEFGH", "AB****GH"},
                                                                   {"Riverside", "Ri*****de"},
                                                                   {"O'Reilly", "O'****ly"}};
    for (auto [raw, masked] : cases) {
        check_audit(r, raw, masked);
        auto source = entry(r, raw);
        auto safe = project(r, source);
        auto formatted = HumanReaderFormatter{}.format_sql_log(r, {}, safe);
        check(source.params.size() == 1 && source.params[0] == Value(r, raw) &&
                  contains(safe.debug_sql, "name = '") && contains(formatted, "name = '") &&
                  contains(formatted, "masked") && !contains(safe.debug_sql, "name = ?") &&
                  !contains(formatted, "[REDACTED SQL"),
              "original expanded SQL retains statement and unknown provenance masking");
        if (!raw.empty()) {
            check(safe.params[0].try_text() != raw, "unclassified raw value is hidden");
            String quoted("'", r);
            for (auto c : raw) {
                quoted += c;
                if (c == '\'') {
                    quoted += c;
                }
            }
            quoted += '\'';
            check(!contains(formatted, quoted), "formatted SQL cannot expose original literal");
        }
        if (raw.size() >= 8 && !masked.starts_with('*')) {
            String escaped(r);
            for (auto character : masked) {
                escaped += character;
                if (character == '\'') {
                    escaped += character;
                }
            }
            check(!contains(formatted, escaped), "unknown provenance cannot use partial mask");
        }
    }
}

static void target_id_scrubs_sql_free_text(Memory r) {
    auto source = entry(r, "unused");
    source.log_context.generated_sql = true;
    source.log_context.parameter_policies = {Policy::Plain};
    source.log_context.intent_redactions.capture_target_id(Value(r, std::uint64_t(1)));
    source.sql = "UPDATE customer SET version = version + 1 WHERE id = ?";
    source.params.clear();
    source.params.emplace_back(std::uint64_t(1));
    source.comment.emplace("what: update customer 1", r);
    source.audit_reason.emplace("what: update customer 1", r);
    source.trace_path.emplace_back(
        TraceKind::AuditReason, "Customer", "what: update customer 1", 1);
    source.result_summary = "1 rows affected";
    for (auto allow : {false, true}) {
        auto safe = project(r, source, allow);
        check(safe.audit_reason == "what: update customer [REDACTED]" &&
                  safe.comment == "what: update customer [REDACTED]" &&
                  safe.trace_path[0].comment == "what: update customer [REDACTED]" &&
                  safe.result_summary == "1 rows affected" && safe.params == source.params &&
                  safe.log_context.intent_redactions.is_empty(),
              "target ID versus factual count");
    }
    check(source.audit_reason == "what: update customer 1", "source annotation immutable");
}

static void retained_debug_readback_downgrades_after_clone(Memory r) {
    SqlLogContext write(r);
    write.generated_sql = true;
    write.parameter_policies = {Policy::Masked};
    Vector<Value> values(r);
    values.emplace_back("Riverside");
    auto source = entry(r, "unused");
    source.sql = "SELECT name FROM customer WHERE id = ? LIMIT 10000";
    source.params.clear();
    source.params.emplace_back(std::uint64_t(1));
    source.log_context.generated_sql = true;
    source.log_context.parameter_policies = {Policy::Plain};
    source.log_context.intent_redactions =
        SqlIntentRedactions::from_bindings(r, write, values, "UPDATE customer");
    source.comment.emplace("what: read Riverside authoritative snapshot", r);
    auto debug = detail::clone_in(r, project(r, source, true));
    check(debug.log_context.intent_redactions.is_empty(), "inherited provenance consumed");
    auto safe = project(r, debug);
    check(safe.comment == "what: read [REDACTED] authoritative snapshot" &&
              contains(safe.debug_sql, "WHERE id = 1 LIMIT 10000"),
          "retained readback downgrade");
    omitted(r, safe, "Riverside");
    check(project(r, project(r, debug, true)).comment == safe.comment, "repeated debug downgrade");
}

static void modified_or_reconstructed_debug_hides_unknown_intent(Memory r) {
    for (auto reconstructed : {false, true}) {
        auto source = entry(r, "Riverside");
        source.log_context.parameter_policies = {Policy::Masked};
        auto debug = project(r, source, true);
        if (reconstructed) {
            debug.log_context = SqlLogContext(r);
            debug.log_context.database_kind.emplace("sqlite", r);
            debug.log_context.parameter_policies = {Policy::Masked};
        }
        debug.comment.emplace("INHERITED-CANARY", r);
        debug.trace_path.emplace_back(TraceKind::Entity, "INHERITED-CANARY", "INHERITED-CANARY", 1);
        debug.result_summary = "INHERITED-CANARY";
        debug.result_type.emplace("INHERITED-CANARY", r);
        auto safe = project(r, debug);
        omitted(r, safe, "CANARY");
        check(contains(safe.debug_sql, "'Ri*****de' /* masked */"), "tampered binding downgrade");
    }
}

static void safe_projection_never_upgrades_its_label(Memory r) {
    auto safe = project(r, entry(r, "Riverside"));
    auto again = project(r, safe, true);
    check(again.log_context.mode == "safe" && again.debug_sql == safe.debug_sql,
          "safe permission cannot upgrade");
}

static void cloned_debug_alternatives_are_independent_and_thread_safe(Memory r) {
    auto source = entry(r, "Riverside");
    source.log_context.parameter_policies = {Policy::Masked};
    source.comment.emplace("find Riverside", r);
    const auto debug = project(r, source, true);
    auto poison = project(r, debug);
    poison.comment.emplace("POISON", r);
    poison.params[0] = Value(r, "POISON");
    std::atomic<unsigned> passed = 0;
    {
        std::vector<std::jthread> workers;
        for (unsigned i = 0; i < 16; ++i) {
            workers.emplace_back([&] {
                try {
                    // Only immutable host data is borrowed; every allocation is worker-local.
                    // All threads join before the source owner can close.
                    Arena arena(4096, std::pmr::new_delete_resource());
                    auto cloned = detail::clone_in(&arena, debug);
                    auto safe = project(&arena, cloned);
                    check(safe.params.size() == 1 && safe.params[0].try_text() == "Ri*****de" &&
                              safe.comment == "find [REDACTED]",
                          "independent cloned safe view");
                    omitted(&arena, safe, "Riverside");
                    ++passed;
                } catch (...) {
                }
            });
        }
    }
    check(passed == 16, "all original sixteen clone workers passed");
    check(!contains(debug_string(r, debug.log_context), "Riverside"),
          "complete log context Debug preserves private diagnostic state");
}

static void malformed_flags_and_annotation_channels(Memory r) {
    for (auto allow : {false, true}) {
        auto source = entry(r, "Riverside");
        source.log_context.parameter_policies = {Policy::Plain};
        source.log_context.masked_parameters.clear();
        source.log_context.masked_parameters.push_back(false);
        source.log_context.masked_parameters.push_back(false);
        auto safe = project(r, source, allow);
        check(safe.log_context.omission_reason == "parameter mask count mismatch",
              "mask flag count");
        omitted(r, safe, "Riverside");
    }
    auto source = entry(r, "Riverside");
    source.trace_path.emplace_back(TraceKind::Entity, "Riverside", "Riverside", 1);
    source.result_summary = "rows for Riverside";
    source.result_type.emplace("Riverside", r);
    omitted(r, project(r, source), "Riverside");
    source = entry(r, "Riverside");
    source.log_context.omission_reason.emplace("INHERITED-CANARY", r);
    omitted(r, project(r, source), "CANARY");
}

static void inherited_intent_has_safe_and_debug_views(Memory r) {
    SqlLogContext write(r);
    write.generated_sql = true;
    write.parameter_policies = {Policy::Masked, Policy::Plain, Policy::Credential, Policy::Unknown};
    Vector<Value> params(r);
    for (auto text : {"Riverside", "PublicAddress", "PASSWORD-CANARY", "UNKNOWN-CANARY"}) {
        params.emplace_back(text);
    }
    auto source = entry(r, "unused");
    source.sql = "SELECT name FROM customer WHERE id = ?";
    source.params.clear();
    source.params.emplace_back(std::uint64_t(1));
    source.log_context.generated_sql = true;
    source.log_context.parameter_policies = {Policy::Plain};
    source.log_context.intent_redactions =
        SqlIntentRedactions::from_bindings(r, write, params, "UPDATE customer");
    constexpr auto intent = "Riverside PublicAddress PASSWORD-CANARY UNKNOWN-CANARY";
    source.comment.emplace(intent, r);
    source.purpose.emplace(intent, r);
    source.audit_reason.emplace(intent, r);
    source.trace_path.emplace_back(TraceKind::AuditReason, "Customer", intent, 1);
    check(!contains(debug_string(r, source.log_context), "CANARY"),
          "complete log context Debug preserves intent diagnostic privacy");
    for (auto allow : {false, true}) {
        auto safe = project(r, source, allow);
        check(safe.log_context.intent_redactions.is_empty() && safe.params == source.params &&
                  contains(*safe.comment, "Riverside") == allow &&
                  contains(safe.trace_path[0].comment, "Riverside") == allow &&
                  contains(*safe.audit_reason, "PublicAddress"),
              "classified inherited intent");
        omitted(r, safe, "PASSWORD-CANARY");
        omitted(r, safe, "UNKNOWN-CANARY");
        if (allow) {
            check(contains(safe.debug_sql, "DEBUG PLAINTEXT; EXPLICIT OPT-IN"), "debug label");
        }
    }
    auto safe = project(r, source);
    omitted(r, project(r, safe, true), "Riverside");
    check(source.comment == intent && params[0].try_text() == "Riverside",
          "immutable inherited source");
}

static void inherited_nested_credentials_invalid_policies_and_longest_first(Memory r) {
    SqlLogContext plain(r);
    plain.generated_sql = true;
    plain.parameter_policies = {Policy::Plain};
    Vector<Value> nested(r);
    nested.emplace_back(*Json::parse(
        r, R"({"payload":[{"access_token":"NESTED-CANARY"}],"note":"SIBLING-CANARY"})"));
    auto protection = SqlIntentRedactions::from_bindings(r, plain, nested, "UPDATE customer");
    Vector<String> secrets(r);
    protection.extend_secrets(true, secrets);
    auto has = [&](std::string_view value) {
        return std::find(secrets.begin(), secrets.end(), value) != secrets.end();
    };
    check(has("NESTED-CANARY") && has("SIBLING-CANARY"),
          "nested credential protects sibling intent");
    Vector<Value> invalid_params(r);
    invalid_params.emplace_back("FIRST-CANARY");
    invalid_params.emplace_back("SECOND-CANARY");
    auto invalid = SqlIntentRedactions::from_bindings(r, plain, invalid_params, "UPDATE customer");
    secrets.clear();
    invalid.extend_secrets(true, secrets);
    check(secrets.size() == 2 && has("FIRST-CANARY") && has("SECOND-CANARY"),
          "invalid policies fail closed");
    SqlLogContext write(r);
    write.generated_sql = true;
    write.parameter_policies = {Policy::Masked};
    Vector<Value> values(r);
    values.emplace_back("Riverside");
    auto source = entry(r, "River");
    source.log_context.parameter_policies = {Policy::Masked};
    source.log_context.intent_redactions =
        SqlIntentRedactions::from_bindings(r, write, values, "UPDATE customer");
    source.comment.emplace("Riverside River", r);
    check(project(r, source).comment == "[REDACTED] [REDACTED]", "longest-first replacement");
    check(SqlIntentRedactions::from_bindings(r, plain, values, "UPDATE customer").is_empty(),
          "plain intent is public");
}

static void inherited_numeric_intent_preserves_typed_text(Memory r) {
    SqlLogContext write(r);
    write.generated_sql = true;
    write.parameter_policies = {Policy::Masked};
    Vector<Value> scalar(r);
    scalar.emplace_back(12345.0);
    Vector<Value> list(r);
    list.emplace_back(scalar);
    Vector<Value> nan(r);
    nan.emplace_back(std::numeric_limits<double>::quiet_NaN());
    for (auto* values : {&scalar, &list, &nan}) {
        Vector<String> actual(r);
        auto protection = SqlIntentRedactions::from_bindings(r, write, *values, "UPDATE customer");
        protection.extend_secrets(false, actual);
        check(actual.size() == 1 && actual[0] == (values == &nan ? "NaN" : "12345"),
              "typed numeric intent text");
    }
}

static void debug_provenance_and_mixed_policies(Memory r) {
    for (int i = 0; i < 2; ++i) {
        auto source = entry(r, "Riverside");
        source.log_context.parameter_policies = {Policy::Masked};
        auto safe = project(r, source, true);
        auto log = HumanReaderFormatter{}.format_sql_log(r, {}, safe);
        check(contains(log, "'Riverside'") && contains(log, "DEBUG") && contains(log, "PLAINTEXT"),
              "plaintext provenance label");
    }
    auto source = entry(r, "Riverside");
    source.sql = "UPDATE customer SET name = ?, status = ?, password = ?, note = ? WHERE id = ?";
    source.params.clear();
    for (auto text : {"Riverside", "ACTIVE", "PASSWORD-CANARY", "UNKNOWN-CANARY"}) {
        source.params.emplace_back(text);
    }
    source.params.emplace_back(std::int64_t(17));
    source.debug_sql = "UNTRUSTED-LEGACY-DEBUG-CANARY";
    source.log_context.generated_sql = true;
    source.log_context.parameter_policies = {
        Policy::Masked, Policy::Plain, Policy::Credential, Policy::Unknown, Policy::Plain};
    for (auto allow : {false, true}) {
        auto safe = project(r, source, allow);
        check(contains(safe.debug_sql,
                       allow ? "name = 'Riverside'" : "name = 'Ri*****de' /* masked */") &&
                  contains(safe.debug_sql, "status = 'ACTIVE'") &&
                  contains(safe.debug_sql, "WHERE id = 17") &&
                  contains(safe.debug_sql, "password = '[REDACTED]' /* masked */") &&
                  contains(safe.debug_sql, "note = '[REDACTED]' /* masked */") &&
                  contains(safe.debug_sql, "NOT REPLAYABLE"),
              "original mixed policies");
        for (auto canary : {"PASSWORD-CANARY", "UNKNOWN-CANARY", "UNTRUSTED-LEGACY-DEBUG-CANARY"}) {
            omitted(r, safe, canary);
        }
    }
    check(source.params[2].try_text() == "PASSWORD-CANARY", "execution values never modified");
}

static void idempotence_unknown_provenance_and_render_errors(Memory r) {
    auto source = entry(r, "Riverside");
    source.log_context.parameter_policies = {Policy::Masked};
    auto safe = project(r, source);
    check(project(r, safe) == safe, "safe projection idempotence");
    auto debug_after_safe = project(r, safe, true);
    check(!contains(debug_after_safe.debug_sql, "Riverside") &&
              contains(debug_after_safe.debug_sql, "Ri*****de") &&
              contains(debug_after_safe.debug_sql, "NOT REPLAYABLE"),
          "safe cannot unmask later");
    source = entry(r, "UNKNOWN-CANARY");
    for (auto allow : {false, true}) {
        auto hidden = project(r, source, allow);
        omitted(r, hidden, "UNKNOWN-CANARY");
        check(contains(hidden.debug_sql, "name = '[REDACTED]' /* masked */"),
              "unknown provenance hidden");
    }
    for (auto mismatch : {false, true}) {
        source = entry(r, "PRIVATE-CANARY");
        source.sql =
            mismatch ? "UPDATE customer SET name = ?" : "UPDATE customer SET name = ?, code = ?";
        source.log_context.parameter_policies =
            mismatch ? Vector<Policy>({Policy::Plain, Policy::Plain}, r)
                     : Vector<Policy>({Policy::Plain}, r);
        auto hidden = project(r, source);
        check(hidden.log_context.omission_reason.has_value() &&
                  contains(hidden.debug_sql, "NOT REPLAYABLE") && project(r, hidden) == hidden,
              "binding/policy errors omit and remain idempotent");
        omitted(r, hidden, "PRIVATE-CANARY");
    }
}

static void literals_annotations_nested_credentials_and_unknown_dialect(Memory r) {
    for (auto sql : {"SELECT 'LITERAL-CANARY'",
                     "SELECT name /* LITERAL-CANARY */ FROM customer",
                     "SELECT 12345678"}) {
        auto source = entry(r, "unused");
        source.sql = sql;
        source.params.clear();
        for (auto allow : {false, true}) {
            auto safe = project(r, source, allow);
            check(safe.log_context.omission_reason == "untrusted SQL literals or comments" &&
                      !contains(safe.debug_sql, "12345678"),
                  "literal SQL fails closed");
            omitted(r, safe, "LITERAL-CANARY");
        }
    }
    auto source = entry(r, "1");
    source.comment.emplace("edit customer 1", r);
    auto safe = project(r, source);
    check(safe.comment == "edit customer [REDACTED]" && safe.affected_rows == 1 &&
              safe.result_summary == "1 row affected",
          "annotations do not change count facts");
    source = entry(r, "unused");
    source.params.clear();
    source.params.emplace_back(*Json::parse(r, R"({"nested":{"api_key":"NESTED-CANARY"}})"));
    source.log_context.generated_sql = true;
    source.log_context.parameter_policies = {Policy::Plain};
    for (auto allow : {false, true}) {
        omitted(r, project(r, source, allow), "NESTED-CANARY");
    }
    source = entry(r, "PRIVATE-CANARY");
    source.log_context.database_kind.reset();
    safe = project(r, source);
    check(safe.log_context.omission_reason == "unknown database dialect", "no guessed dialect");
    omitted(r, safe, "PRIVATE-CANARY");
}

int main(int argc, char** argv) {
    try {
        check(argc == 2, "masking golden path required");
        GlobalContext global;
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        mask_golden(r, argv[1]);
        expanded_sql_and_audit_macros(r);
        target_id_scrubs_sql_free_text(r);
        retained_debug_readback_downgrades_after_clone(r);
        modified_or_reconstructed_debug_hides_unknown_intent(r);
        safe_projection_never_upgrades_its_label(r);
        cloned_debug_alternatives_are_independent_and_thread_safe(r);
        malformed_flags_and_annotation_channels(r);
        inherited_intent_has_safe_and_debug_views(r);
        inherited_nested_credentials_invalid_policies_and_longest_first(r);
        inherited_numeric_intent_preserves_typed_text(r);
        debug_provenance_and_mixed_policies(r);
        idempotence_unknown_provenance_and_render_errors(r);
        literals_annotations_nested_credentials_and_unknown_dialect(r);
        std::cout << "Original SQLite masking contracts passed; PostgreSQL array case excluded\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
