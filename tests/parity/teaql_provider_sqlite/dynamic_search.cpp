// Semantic port of the locked Rust 5.0.6 provider dynamic-search integration test.
#include "fixture.hpp"
#include <teaql/core/dynamic_search.hpp>

using namespace original_provider;
using namespace teaql::dynamic_search;

static void scoped_dynamic_search_retains_outer_and_related_tenant_filters() {
    Fixture fixture;
    auto r = fixture.resource;
    EntityDescriptor platform(std::allocator_arg, Allocator(r), "Platform");
    platform.table("search_platform");
    platform.property("id", DataType::I64).id();
    platform.property("tenant_id", DataType::I64);
    platform.property("name", DataType::Text);
    EntityDescriptor school(std::allocator_arg, Allocator(r), "School");
    school.table("search_school");
    school.property("id", DataType::I64).id();
    school.property("tenant_id", DataType::I64);
    school.property("platform_id", DataType::I64);
    school.property("name", DataType::Text);
    fixture.metadata.register_entity(detail::clone_in(r, platform));
    fixture.metadata.register_entity(detail::clone_in(r, school));
    use_sqlite_provider(*fixture.user, *fixture.user, fixture.transport);
    check(bool(fixture.user->ensure_schema().wait()), "physical schema uses the Context boundary");
    for (auto [id, tenant] : {std::pair{std::int64_t(1), std::int64_t(7)}, {2, 8}}) {
        InsertCommand insert(std::allocator_arg, Allocator(r), "Platform");
        insert.value("id", id).value("tenant_id", tenant).value("name", "Campus");
        fixture.seed(platform, insert);
    }

    struct Row {
        std::int64_t id, tenant, parent;
        const char* name;
    };

    for (const auto& row : {Row{1, 7, 1, "School"},
                            {2, 7, 1, "School"},
                            {3, 7, 1, "School"},
                            {4, 8, 1, "School"},
                            {5, 7, 2, "School"},
                            {6, 7, 1, "Other"}}) {
        InsertCommand insert(std::allocator_arg, Allocator(r), "School");
        insert.value("id", row.id)
            .value("tenant_id", row.tenant)
            .value("platform_id", row.parent)
            .value("name", row.name);
        fixture.seed(school, insert);
    }
    SearchModels models(r);
    auto& search_school = models.try_emplace("School").first->second;
    search_school.fields.emplace("id", "integer");
    search_school.fields.emplace("name", "string");
    search_school.relations.emplace("platform", "Platform");
    models.try_emplace("Platform").first->second.fields.emplace("name", "string");
    SelectQuery base(std::allocator_arg, Allocator(r), "School");
    base.project("id")
        .project("name")
        .filter(Expr::eq(r, "tenant_id", std::int64_t(7)))
        .order_desc("id")
        .limit(2)
        .comment("what: search tenant schools")
        .hard_limit(2);
    base.trace_chain.emplace_back(
        TraceKind::Purpose, "School", "why: verify schema drift isolation");
    auto before = base.clone(r);
    Vector<DynamicSearchWarning> warnings(r);
    auto capture = [&](const auto& warning) {
        warnings.emplace_back(warning);
    };
    WarningSink sink(capture);
    auto filter = [&](const DynamicSearchFilter& filter) -> SearchResult<Expr> {
        const auto& value = std::get<String>(filter.value.data);
        if (filter.field_path == "name") {
            return SearchResult<Expr>::ok(r, Expr::eq(r, "name", value));
        }
        if (filter.field_path == "platform.name") {
            SelectQuery related(std::allocator_arg, Allocator(r), "Platform");
            related.filter(Expr::eq(r, "tenant_id", std::int64_t(7)))
                .and_filter(Expr::eq(r, "name", value));
            return SearchResult<Expr>::ok(
                r, Expr::in_subquery(r, "platform_id", platform, related, "id"));
        }
        return SearchResult<Expr>::err(r, DynamicSearchError{"Unbound trusted field"});
    };
    auto order = [&](const DynamicSearchOrder& order) {
        return SearchResult<OrderBy>::ok(r, OrderBy::asc(r, order.field_path));
    };
    auto result = merge_dynamic_search(
        r,
        base,
        R"({"filter":{"name":"School","platform.name":"Campus","old_name":"secret","old_relation.name":"secret","platform.old_name":"secret"},"orderBy":[{"field":"removed","direction":"asc"},{"field":"name","direction":"asc"}]})",
        models,
        FilterBinding(filter),
        OrderBinding(order),
        &sink);
    check(result && base == before && result.value().query.hard_limit_value == 2 &&
              result.value().query.trace_chain == base.trace_chain,
          "schema drift does not alter original scope, limit or provenance");
    auto intent = QueryIntent::create(r, *base.comment_text, "verify dynamic search");
    check(bool(intent), "original 5.0.6 explicit query intent is valid");
    QueryRequest request(
        std::allocator_arg, Allocator(r), result.value().query, intent.value(), true, true);
    check(request.intent.comment() == "what: search tenant schools" &&
              request.intent.purpose() == "verify dynamic search",
          "owned request intent remains distinct from the original purpose trace");
    request.trace_chain = detail::clone_in(r, base.trace_chain);
    const auto& response = fixture.executor.query(r, std::move(request)).wait();
    check(response && response.value().rows.size() == 2 && response.value().rows[0].get("id") &&
              response.value().rows[1].get("id") &&
              *response.value().rows[0].get("id") == Value(r, std::int64_t(3)) &&
              *response.value().rows[1].get("id") == Value(r, std::int64_t(2)) &&
              warnings.size() == 4,
          "real SQLite preserves outer and related tenant filters and source row order");
    Json::Array warning_json(r);
    for (const auto& warning : warnings) {
        warning_json.emplace_back(warning.to_json(r));
    }
    check(Json(r, std::move(warning_json)).dump(r).find("secret") == String::npos,
          "provider integration warnings contain none of the ignored input values");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    scoped_dynamic_search_retains_outer_and_related_tenant_filters();
}
