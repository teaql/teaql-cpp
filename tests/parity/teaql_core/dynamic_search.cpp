// Semantic port of all eight locked teaql-core/tests/dynamic_search.rs contracts.
#include <teaql/core/dynamic_search.hpp>
#include <teaql/memory/arena.hpp>
#include <iostream>

using namespace teaql;
using namespace teaql::dynamic_search;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static SearchModels models(Memory r) {
    SearchModels result(r);
    auto& school = result.try_emplace("School").first->second;
    for (auto [name, type] : {std::pair{"name", "string"},
                              {"id", "integer"},
                              {"active", "boolean"},
                              {"amount", "decimal"},
                              {"established_date", "date"},
                              {"create_time", "timestamp"},
                              {"capacity", "number"}}) {
        school.fields.emplace(name, type);
    }
    school.relations.emplace("platform", "Platform");
    result.try_emplace("Platform").first->second.fields.emplace("name", "string");
    return result;
}

static void unknown_complete_clauses_warn_without_values(Memory r) {
    auto schema = models(r);
    Vector<DynamicSearchWarning> warnings(r);
    auto capture = [&](const auto& warning) {
        warnings.emplace_back(warning);
    };
    WarningSink sink(capture);
    auto result = normalize_dynamic_search(
        r,
        R"({"filter":{"name":"School","old_name":"secret","platform.old_name":"secret","missing.name":"secret","platform.name":"Campus"},"orderBy":[{"field":"gone","direction":"asc"},{"field":"id","direction":"desc"}]})",
        "School",
        schema,
        100,
        &sink);
    check(result && result.value().filters.size() == 2 &&
              result.value().filters[0].field_path == "name" &&
              result.value().filters[1].field_path == "platform.name" &&
              result.value().orders.size() == 1 && result.value().orders[0].field_path == "id" &&
              warnings.size() == 4,
          "unknown complete clauses are warnings, not fatal input");
    Json::Array serialized(r);
    for (const auto& warning : warnings) {
        check(warning.code == "DYNAMIC_SEARCH_UNKNOWN_FIELD", "warning code is preserved");
        serialized.emplace_back(warning.to_json(r));
    }
    auto json = Json(r, std::move(serialized)).dump(r);
    check(json.find("secret") == String::npos && json.find("fieldPath") != String::npos,
          "structured warnings retain paths but omit submitted values");
}

static void invalid_input_remains_fatal(Memory r) {
    auto schema = models(r);
    for (auto source : {"[]",
                        "{} {}",
                        R"({"tenant":2})",
                        R"({"filter":{"id":true}})",
                        R"({"filter":{"id":1.2}})",
                        R"({"filter":{"capacity":1e999}})",
                        R"({"filter":{"established_date":"2026-02-30"}})",
                        R"({"filter":{"gone":{"$wat":1}}})",
                        R"({"filter":{"name":{"$in":1}}})",
                        R"({"filter":{"constructor":1}})",
                        R"({"filter":{"platform..name":1}})",
                        R"({"orderBy":[{"field":"id","direction":"bad"}]})"}) {
        Vector<DynamicSearchWarning> warnings(r);
        auto capture = [&](const auto& warning) {
            warnings.emplace_back(warning);
        };
        WarningSink sink(capture);
        check(!normalize_dynamic_search(r, source, "School", schema, 100, &sink) &&
                  warnings.empty(),
              "invalid input is fatal and emits no partially collected warnings");
    }
}

static void dates_booleans_and_exact_decimal_strings_are_retained(Memory r) {
    auto result = normalize_dynamic_search(
        r,
        R"({"filter":{"id":1.0,"active":true,"amount":"12345678901234567890.123456789","established_date":"2024-02-29","create_time":1700000000000,"name":null}})",
        "School",
        models(r),
        100);
    check(result && result.value().filters.size() == 6, "all six valid typed filters survive");
    const auto& filters = result.value().filters;
    auto amount = std::find_if(filters.begin(), filters.end(), [](const auto& filter) {
        return filter.field_path == "amount";
    });
    check(amount != filters.end() &&
              std::get<String>(amount->value.data) == "12345678901234567890.123456789",
          "exact decimal text is not narrowed through a floating conversion");
}

static void limits_and_missing_relation_metadata_fail(Memory r) {
    auto schema = models(r);
    check(!normalize_dynamic_search(r, R"({"filter":{"name":"x","gone":1}})", "School", schema, 1),
          "unknown clauses also count toward the limit");
    String source("{\"filter\":{\"id\":{\"$in\":[", r);
    for (unsigned i = 0; i < 1001; ++i) {
        if (i) {
            source += ',';
        }
        source += '1';
    }
    source += "]}}}";
    check(!normalize_dynamic_search(r, source, "School", schema, 100),
          "the original 1001-element IN list fails");
    schema.erase("Platform");
    check(
        !normalize_dynamic_search(r, R"({"filter":{"platform.name":"x"}})", "School", schema, 100),
        "a known relation with absent target metadata fails");
}

static void composition_preserves_the_original_scope_and_limits(Memory r) {
    auto schema = models(r);
    SelectQuery base(std::allocator_arg, Allocator(r), "School");
    base.filter(Expr::eq(r, "tenant_id", std::int64_t(7)))
        .order_desc("id")
        .limit(2)
        .comment("what: tenant search")
        .hard_limit(3);
    auto before = base.clone(r);
    Vector<DynamicSearchWarning> warnings(r);
    auto capture = [&](const auto& warning) {
        warnings.emplace_back(warning);
    };
    WarningSink sink(capture);
    auto filter = [&](const DynamicSearchFilter& f) {
        return SearchResult<Expr>::ok(r, Expr::eq(r, f.field_path, std::get<String>(f.value.data)));
    };
    auto order = [&](const DynamicSearchOrder& o) {
        return SearchResult<OrderBy>::ok(r, OrderBy::asc(r, o.field_path));
    };
    auto result = merge_dynamic_search(
        r,
        base,
        R"({"filter":{"name":"School","gone":1},"orderBy":[{"field":"name","direction":"asc"}]})",
        schema,
        FilterBinding(filter),
        OrderBinding(order),
        &sink);
    check(result && base == before && result.value().query.predicate != base.predicate &&
              result.value().query.order_by.size() == 2 &&
              result.value().query.order_by[0] == OrderBy::desc(r, "id") &&
              result.value().query.order_by[1] == OrderBy::asc(r, "name") &&
              result.value().query.slice == base.slice &&
              result.value().query.hard_limit_value == 3 &&
              result.value().query.comment_text == base.comment_text && warnings.size() == 1,
          "composition preserves the immutable base scope, order, limits and comment");
}

static void late_failures_do_not_emit_warnings(Memory r) {
    auto schema = models(r);
    Vector<DynamicSearchWarning> warnings(r);
    auto capture = [&](const auto& warning) {
        warnings.emplace_back(warning);
    };
    WarningSink sink(capture);
    check(!normalize_dynamic_search(
              r, R"({"filter":{"gone":1,"id":"bad"}})", "School", schema, 100, &sink),
          "a later invalid value fails normalization");
    SelectQuery base(std::allocator_arg, Allocator(r), "School");
    auto filter = [&](const DynamicSearchFilter&) {
        return SearchResult<Expr>::err(r, DynamicSearchError{"binding failure"});
    };
    auto order = [&](const DynamicSearchOrder& o) {
        return SearchResult<OrderBy>::ok(r, OrderBy::asc(r, o.field_path));
    };
    check(!merge_dynamic_search(r,
                                base,
                                R"({"filter":{"gone":1,"name":"School"}})",
                                schema,
                                FilterBinding(filter),
                                OrderBinding(order),
                                &sink) &&
              warnings.empty(),
          "binding failures emit none of the earlier unknown-field warnings");
}

static void default_warning_log_canary_child(Memory r) {
    auto result =
        normalize_dynamic_search(r,
                                 R"({"filter":{"CLIENT_SECRET_FIELD_PATH_91":"SECRET_VALUE_99"}})",
                                 "School",
                                 models(r),
                                 100);
    check(result && result.value().warnings[0].field_path == "CLIENT_SECRET_FIELD_PATH_91",
          "returned structured warning retains the original path");
    std::cout << "running 1 test\n";
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        Arena memory(4096, std::pmr::new_delete_resource());
        if (argc == 2 && std::string_view(argv[1]) == "--canary-child") {
            default_warning_log_canary_child(&memory);
        } else {
            unknown_complete_clauses_warn_without_values(&memory);
            invalid_input_remains_fatal(&memory);
            dates_booleans_and_exact_decimal_strings_are_retained(&memory);
            limits_and_missing_relation_metadata_fail(&memory);
            composition_preserves_the_original_scope_and_limits(&memory);
            late_failures_do_not_emit_warnings(&memory);
            std::cout << "Original core dynamic-search contracts passed\n";
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
