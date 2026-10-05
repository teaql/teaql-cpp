#include <teaql/core/request.hpp>
#include <teaql/core/dynamic_search.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>
#include <sstream>
using namespace teaql;
namespace rq = teaql::request;
namespace ds = teaql::dynamic_search;

static void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

struct NoDefaultAllocator {
    Memory previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    ~NoDefaultAllocator() {
        std::pmr::set_default_resource(previous);
    }
};

static_assert(!std::same_as<rq::RelationAggregate, teaql::RelationAggregate> &&
              !std::same_as<rq::RawProjection, teaql::RawSqlProjection>);
static_assert(!std::same_as<rq::RelationSelection, rq::RelationFilter> &&
              !std::same_as<rq::RawProjection, rq::RawDynamicProperty>);
static_assert(rq::EntityReference<Value> && rq::EntityReference<std::uint64_t> &&
              !rq::EntityReference<std::int64_t>);
static void options_domain(const rq::QueryOptions&, Memory);

static void selection_domain(const rq::QuerySelection& selection, Memory r) {
    check(selection.memory_resource() == r &&
              selection.relation_selections.get_allocator().resource() == r &&
              selection.relation_filters.get_allocator().resource() == r,
          "selection domain");
    options_domain(selection.query_options, r);
    for (const auto& child : selection.child_enhancements)
        selection_domain(child, r);
    for (const auto& relation : selection.relation_selections) {
        check(relation.name.get_allocator().resource() == r &&
                  relation.query.memory_resource() == r,
              "relation selection domain");
        options_domain(relation.query_options, r);
    }
    for (const auto& filter : selection.relation_filters) {
        check(filter.name.get_allocator().resource() == r && filter.query.memory_resource() == r,
              "relation filter domain");
        options_domain(filter.query_options, r);
    }
}

static void options_domain(const rq::QueryOptions& options, Memory r) {
    check(options.memory_resource() == r, "query options domain");
    for (const auto* s : {&options.comment, &options.raw_sql})
        if (*s)
            check((*s)->get_allocator().resource() == r, "optional option text domain");
    for (const auto& p : options.dynamic_properties)
        check(p.property_name.get_allocator().resource() == r &&
                  p.raw_sql_segment.get_allocator().resource() == r,
              "dynamic property domain");
    for (const auto& p : options.raw_projections)
        check(p.property_name.get_allocator().resource() == r &&
                  p.raw_sql_segment.get_allocator().resource() == r,
              "raw projection domain");
    for (const auto& a : options.relation_aggregates) {
        check(a.relation_name.get_allocator().resource() == r, "aggregate name domain");
        selection_domain(a.query, r);
    }
    for (const auto& g : options.object_group_bys)
        selection_domain(g.query, r);
    for (const auto& f : options.facets) {
        check(f.facet_name.get_allocator().resource() == r, "facet name domain");
        selection_domain(f.query, r);
    }
}

static void metadata_conversion() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto user = global.make_user_context(64);
    auto r = user->memory_resource();
    auto& query = user->make<SelectQuery>("School");
    query.filter(Expr::eq(r, "tenant_id", 7))
        .comment("original comment")
        .raw_sql("old sql")
        .project_raw("old", "old raw")
        .dynamic_property_raw("old", "old dynamic");
    auto& child_query = user->make<SelectQuery>("Platform");
    child_query.project("name");
    auto& child = user->make<rq::QuerySelection>(child_query);
    child.query_options.comment.emplace("nested intent", r);
    auto& selection = user->make<rq::QuerySelection>(query);
    selection.relation_selections.emplace_back("platform", child);
    selection.relation_filters.emplace_back(
        "filter_only", child); // Kept for runtime; not turned into eager relation loading.
    selection.child_enhancements.emplace_back(child);
    auto& options = selection.query_options;
    options.comment.emplace("new comment", r);
    options.raw_sql.emplace("select * from school", r);
    options.raw_sql_search_criteria.emplace_back("name");
    options.dynamic_properties.emplace_back("dynamic",
                                            rq::UnsafeRawSqlSegment::trusted(r, "upper(name)"));
    options.raw_projections.emplace_back("projected",
                                         rq::UnsafeRawSqlSegment::trusted(r, "length(name)"));
    options.object_group_bys.emplace_back("platform", "platform_id", child);
    options.relation_aggregates.emplace_back("platform", "platform_count", child, true);
    options.facets.emplace_back("platforms", "platform", child, true);
    auto& snapshot = global.make<rq::QuerySelection>(selection);
    auto& retained = global.make<rq::QuerySelection>(selection);
    check(snapshot == selection && retained == selection, "all builder metadata copied");
    selection_domain(retained, global.memory_resource());
    auto& compiled = global.make<SelectQuery>(std::move(retained).into_query());
    check(compiled.comment_text == "new comment" && compiled.trace_chain.size() == 2,
          "metadata comment uses core trace builder");
    check(compiled.raw_sql_text == "select * from school" && compiled.raw_projections.size() == 1 &&
              compiled.raw_projections[0].property_name == "projected",
          "metadata replaces raw query fields");
    check(compiled.dynamic_properties.size() == 1 &&
              compiled.dynamic_properties[0].property_name == "dynamic",
          "dynamic projection remains distinct");
    check(compiled.relations.size() == 1 && compiled.relations[0].name == "platform" &&
              compiled.relations[0].query->comment_text == "nested intent",
          "relation selections recurse but relation filters remain separate");
    check(compiled.object_group_bys.size() == 1 && compiled.child_enhancements.size() == 1,
          "object grouping and child enhancements converted");
    auto& aggregates = global.make<Vector<teaql::RelationAggregate>>(
        rq::runtime_relation_aggregates(global.memory_resource(), snapshot.query_options));
    check(aggregates.size() == 1 && aggregates[0].single_result &&
              aggregates[0].query.comment_text == "nested intent",
          "separate aggregate conversion");
    {
        auto empty_options = rq::QueryOptions(std::allocator_arg, Allocator(r));
        auto reset = rq::apply_runtime_metadata(query.clone(r), empty_options, {});
        check(reset.comment_text == query.comment_text && !reset.raw_sql_text &&
                  reset.raw_projections.empty() && reset.dynamic_properties.empty(),
              "None comment preserves existing intent while empty options replace raw fields");
    }
    user->close();
    check(snapshot.query_options.facets[0].include_all_facets &&
              snapshot.relation_filters[0].query_options.comment == "nested intent",
          "facet and filter metadata survive source close");
    check(compiled.predicate == snapshot.query.predicate, "query scope preserved");
    selection_domain(snapshot, global.memory_resource());
}

static void operators_and_filters() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto r = global.memory_resource();
    auto& values = global.make<Vector<Value>>();
    values.emplace_back("hello");
    values.emplace_back("world");
    for (unsigned i = 0; i < 18; ++i) {
        auto op = static_cast<rq::FieldOperator>(i);
        auto e = rq::field_operator_expr(r, "name", op, values);
        check(e.memory_resource() == r, "all operator constructors explicit domain");
    }
    check(rq::field_operator_expr(r, "name", rq::FieldOperator::Contain, values) ==
              Expr::like(r, "name", "%hello%"),
          "contains pattern");
    check(rq::field_operator_column_expr(r, "name", rq::FieldOperator::Contain, "other") ==
              Expr::compare_columns(r, "name", BinaryOp::Like, "other"),
          "column comparison does not add wildcards");
    for (auto op : {rq::FieldOperator::Between,
                    rq::FieldOperator::In,
                    rq::FieldOperator::NotIn,
                    rq::FieldOperator::SoundsLike,
                    rq::FieldOperator::IsNull,
                    rq::FieldOperator::IsNotNull}) {
        bool rejected = false;
        try {
            rq::field_operator_column_expr(r, "a", op, "b");
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "unsupported property-to-property operator rejects");
    }
    bool rejected = false;
    try {
        rq::field_operator_expr(r, "a", rq::FieldOperator::Between, {});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "missing operand rejects");
    auto live = Expr::gt(r, "version", std::int64_t(0));
    auto tenant = Expr::eq(r, "tenant_id", 7);
    auto filter = live.and_expr(tenant).and_expr(live);
    check(rq::remove_filter_expr(r, filter, live) == std::optional<Expr>(tenant.clone(r)),
          "remove target from conjunction and collapse singleton");
    check(rq::remove_filter_expr(r, live.or_expr(tenant), live) ==
              std::optional<Expr>(live.or_expr(tenant)),
          "do not remove target inside OR");
    check(!rq::remove_default_live_filter(r, std::optional<Expr>(live.clone(r))),
          "default live filter removed");
    check(rq::remove_default_live_filter(r, std::optional<Expr>(Expr::gt(r, "version", 0u)))
              .has_value(),
          "typed value equality prevents deleting a different filter");
    const auto converted = rq::dynamic_json_value_to_teaql_value(
        r, *Json::parse(r, R"([{"id":7},"　 text ",{"other":1},9223372036854775808,1.0])"));
    auto list = converted.as_list();
    check(list && std::holds_alternative<std::int64_t>((*list)[0].data) &&
              (*list)[1].try_text() == "text" && (*list)[2].is_null(),
          "dynamic JSON ID extraction, Unicode trim, missing ID null");
    check(std::holds_alternative<std::uint64_t>((*list)[3].data) &&
              std::holds_alternative<double>((*list)[4].data),
          "dynamic JSON retains numeric kind boundaries");
    check(rq::dynamic_json_operator(*Json::parse(r, "[1,2]")) == rq::FieldOperator::Between &&
              rq::dynamic_json_operator(*Json::parse(r, "[\"a\",\"b\"]")) == rq::FieldOperator::In,
          "array operator precedence");
    check(rq::dynamic_json_operator(Json(r, "__IS_NULL__")) == rq::FieldOperator::IsNull &&
              rq::dynamic_json_operator(Json(r, " __IS_NULL__ ")) == rq::FieldOperator::Contain,
          "sentinel detection uses original untrimmed input");
    auto object = Json::parse(r, R"({"positive":7,"negative":-1,"float":7.0})");
    auto& map = std::get<Json::Object>(object->data);
    check(rq::dynamic_json_u64_field(map, "positive") == 7 &&
              !rq::dynamic_json_u64_field(map, "negative") &&
              !rq::dynamic_json_u64_field(map, "float"),
          "unsigned helper rejects negative and floating values");
}

static ds::SearchModels models(Memory r) {
    ds::SearchModels models(r);
    auto [school, inserted] = models.try_emplace("School");
    (void)inserted;
    school->second.fields.emplace("name", "string");
    school->second.fields.emplace("id", "integer");
    return models;
}

static void search_bindings() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    ds::DynamicSearchResult* retained = nullptr;
    {
        auto schema = models(r);
        auto& base = user->make<SelectQuery>("School");
        base.filter(Expr::eq(r, "tenant_id", 7))
            .order_desc("id")
            .limit(2)
            .hard_limit(3)
            .comment("scoped search");
        auto before = base.clone(r);
        Vector<String> calls(r);
        Vector<ds::DynamicSearchWarning> warnings(r);
        auto warn = [&](const auto& w) {
            warnings.emplace_back(w);
        };
        ds::WarningSink sink(warn);
        auto filter = [&](const ds::DynamicSearchFilter& f) {
            calls.emplace_back("filter");
            return ds::SearchResult<Expr>::ok(
                r, Expr::eq(r, f.field_path, std::get<String>(f.value.data)));
        };
        auto order = [&](const ds::DynamicSearchOrder& o) {
            calls.emplace_back("order");
            return ds::SearchResult<OrderBy>::ok(r, OrderBy::asc(r, o.field_path));
        };
        auto source =
            R"({"filter":{"name":"School","gone":1},"orderBy":[{"field":"name","direction":"asc"}]})";
        auto result = ds::merge_dynamic_search(
            r, base, source, schema, ds::FilterBinding(filter), ds::OrderBinding(order), &sink);
        check(result && base == before && result.value().query.predicate != base.predicate,
              "merge adds bound filters without changing scoped base");
        check(result.value().query.order_by.size() == 2 &&
                  result.value().query.slice == base.slice &&
                  result.value().query.hard_limit_value == 3 &&
                  result.value().query.comment_text == base.comment_text,
              "existing order, limits and comment preserved");
        check(calls.size() == 2 && calls[0] == "filter" && calls[1] == "order" &&
                  warnings.size() == 1,
              "bindings run in order and warnings emit after success");
        warnings.clear();
        calls.clear();
        auto fail = [&](const ds::DynamicSearchOrder&) {
            calls.emplace_back("order failed");
            return ds::SearchResult<OrderBy>::err(r, ds::DynamicSearchError{"binding failure"});
        };
        auto failed = ds::merge_dynamic_search(
            r, base, source, schema, ds::FilterBinding(filter), ds::OrderBinding(fail), &sink);
        check(!failed && warnings.empty() && calls.size() == 2 && base == before,
              "late binding error emits no warnings and preserves base");
        calls.clear();
        auto filter_fail = [&](const ds::DynamicSearchFilter&) {
            calls.emplace_back("filter failed");
            return ds::SearchResult<Expr>::err(r, ds::DynamicSearchError{"filter failure"});
        };
        auto early = ds::merge_dynamic_search(r,
                                              base,
                                              source,
                                              schema,
                                              ds::FilterBinding(filter_fail),
                                              ds::OrderBinding(order),
                                              &sink);
        check(!early && warnings.empty() && calls.size() == 1 && calls[0] == "filter failed",
              "filter binding error short circuits ordering and warning callbacks");
        auto invalid = ds::normalize_dynamic_search(
            r, R"({"filter":{"gone":1,"id":"bad"}})", "School", schema, 100, &sink);
        check(!invalid && warnings.empty(), "late validation error emits no earlier warnings");
        auto throwing = [](const ds::DynamicSearchWarning&) {
            throw ds::DynamicSearchError{"callback failure"};
        };
        ds::WarningSink throwing_sink(throwing);
        bool propagated = false;
        try {
            ds::normalize_dynamic_search(
                r, R"({"filter":{"gone":1}})", "School", schema, 100, &throwing_sink);
        } catch (const ds::DynamicSearchError& error) {
            propagated = error.message == "callback failure";
        }
        check(propagated,
              "warning callback exception is not reclassified as an input validation error");
        std::ostringstream captured;
        auto previous = std::cerr.rdbuf(captured.rdbuf());
        auto unknown = ds::normalize_dynamic_search(
            r,
            R"({"filter":{"CLIENT_SECRET_FIELD_PATH_91":"SECRET_VALUE_99"}})",
            "School",
            schema,
            100);
        std::cerr.rdbuf(previous);
        check(unknown && captured.str().find("<omitted>") != std::string::npos &&
                  captured.str().find("CLIENT_SECRET") == std::string::npos &&
                  captured.str().find("SECRET_VALUE") == std::string::npos,
              "default warning sink omits path and value");
        retained = &global.make<ds::DynamicSearchResult>(result.value());
    }
    user->close();
    check(retained->query.memory_resource() == global.memory_resource() &&
              retained->warnings[0].field_path.get_allocator().resource() ==
                  global.memory_resource() &&
              retained->query.order_by.size() == 2,
          "search result and warnings survive source close through deep copy");
}

static void facet_helpers() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto r = global.memory_resource();
    auto& base = global.make<SelectQuery>("School");
    base.filter(Expr::eq(r, "tenant_id", 7));
    auto& same = global.make<rq::QuerySelection>(base);
    same.query.predicate.reset();
    auto& other_q = global.make<SelectQuery>("Platform");
    auto& other = global.make<rq::QuerySelection>(other_q);
    auto& selection = global.make<rq::QuerySelection>(base);
    selection.query_options.relation_aggregates.emplace_back("self", "same", same, false);
    selection.query_options.relation_aggregates.emplace_back("platform", "other", other, false);
    rq::merge_outer_filter_into_facet_aggregates(selection, base);
    check(selection.query_options.relation_aggregates[0].query.query.predicate == base.predicate &&
              !selection.query_options.relation_aggregates[1].query.query.predicate,
          "outer filter merges only matching aggregate entity");
    auto& rows = global.make<SmartList<Value>>();
    auto& facet = global.make<SmartList<CompactRow>>();
    facet.with_total_count(9).loaded();
    std::pmr::map<String, SmartList<CompactRow>, std::less<>> facets(r);
    facets.emplace("counts", facet);
    rq::attach_facets(rows, facets);
    check(rows.facet("counts")->total_count == 9 && rows.facet("counts")->is_loaded,
          "facets retain count and loaded metadata");
}

struct FailingResource : std::pmr::memory_resource {
    bool fail = false;
    std::size_t live = 0;

    void* do_allocate(std::size_t n, std::size_t a) override {
        if (fail)
            throw std::bad_alloc();
        auto p = std::pmr::new_delete_resource()->allocate(n, a);
        ++live;
        return p;
    }

    void do_deallocate(void* p, std::size_t n, std::size_t a) override {
        --live;
        std::pmr::new_delete_resource()->deallocate(p, n, a);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return &other == this;
    }
};

static void request_copy_failure() {
    NoDefaultAllocator guard;
    FailingResource upstream;
    {
        GlobalContext source;
        auto r = source.memory_resource();
        GlobalContext destination(&upstream, 64);
        auto& large_query = source.make<SelectQuery>("School");
        auto& selection = source.make<rq::QuerySelection>(large_query);
        selection.query_options.raw_sql.emplace(100000, 'x', r);
        auto& nested = source.make<rq::QuerySelection>(large_query);
        nested.query_options.comment.emplace(10000, 'n', r);
        selection.query_options.facets.emplace_back("nested", "self", nested, true);
        auto& stable = source.make<SelectQuery>("Stable");
        auto& target = destination.make<rq::QuerySelection>(stable);
        upstream.fail = true;
        bool failed = false;
        try {
            target = std::move(selection);
        } catch (const std::bad_alloc&) {
            failed = true;
        }
        check(failed && target.query.entity == "Stable" && !target.query_options.raw_sql &&
                  selection.query_options.raw_sql->size() == 100000,
              "failed cross-domain request move preserves destination and source");
    }
    check(upstream.live == 0, "failed recursive request copy returns all arena blocks");
}

int main() {
    try {
        metadata_conversion();
        operators_and_filters();
        search_bindings();
        facet_helpers();
        request_copy_failure();
        std::cout << "Request and dynamic search contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
