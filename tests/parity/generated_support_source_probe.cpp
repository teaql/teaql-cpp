#include <teaql/runtime.hpp>
#include <teaql/core/query_debug.hpp>
#include "../support/query_full_state_fixture.hpp"
#include "fixture_value.hpp"
#include <iostream>

template <unsigned Missing>
struct ServiceShape {
    using Error = String;
    using Rows = AsyncOperation<Result<Vector<CompactRow>, DataServiceError<Error>>>;
    using List = AsyncOperation<Result<SmartList<CompactRow>, DataServiceError<Error>>>;
    using Stream = AsyncOperation<Result<EntityQueryStream<Error>, DataServiceError<Error>>>;

    Rows& fetch_all(const PurposedSelectQuery&) const
        requires((Missing & 1) == 0);
    List& fetch_smart_list(const PurposedSelectQuery&) const
        requires((Missing & 2) == 0);
    List& fetch_smart_list_with_relation_aggregates(const PurposedSelectQuery&,
                                                    std::span<const RelationAggregate>) const
        requires((Missing & 4) == 0);
    Stream& fetch_stream(const PurposedSelectQuery&) const
        requires((Missing & 8) == 0);
};

static_assert(TeaqlQueryDataService<ServiceShape<0>>);
static_assert(!TeaqlQueryDataService<ServiceShape<1>>);
static_assert(!TeaqlQueryDataService<ServiceShape<2>>);
static_assert(!TeaqlQueryDataService<ServiceShape<4>>);
static_assert(!TeaqlQueryDataService<ServiceShape<8>>);
static_assert(std::same_as<TeaqlDataServiceError<ServiceShape<0>>, DataServiceError<String>>);
static_assert(!TeaqlEntityDataService<ServiceShape<0>, int>);

struct HiddenSave {
    using Error = RuntimeError;
    AsyncOperation<Result<GraphNode, Error>>& save(const TeaqlRuntime&) &&;
};

struct LvalueSave {
    using Error = RuntimeError;
    AsyncOperation<Result<GraphNode, Error>>& save(const TeaqlRuntime&) &;
};

static_assert(AuditedSave<HiddenSave, TeaqlRuntime>);
static_assert(!AuditedSave<LvalueSave, TeaqlRuntime>);
static_assert(!AuditedSave<HiddenSave, int>);

static SelectQuery source_leaf(UserContext& user_context) {
    auto owner = user_context.memory_resource();
    SelectQuery query(std::allocator_arg, Allocator(owner), "Leaf");
    query.filter(Expr::eq(owner, "name", "private leaf"));
    SelectQuery terminal(std::allocator_arg, Allocator(owner), "Terminal");
    query.facets.emplace_back("leaf facet", "unused", terminal, true);
    return query;
}

static SelectQuery source_traversal(UserContext& user_context, unsigned shape) {
    auto owner = user_context.memory_resource();
    SelectQuery query(std::allocator_arg, Allocator(owner), "Root");
    auto leaf = source_leaf(user_context);
    if (shape == 1 || shape == 8) {
        query.facets.emplace_back("direct", "unused", leaf, false);
    }
    if (shape == 2 || shape == 8) {
        query.relation_query("branch", leaf);
    }
    if (shape == 3 || shape == 8) {
        query.relation("unqueried");
    }
    if (shape == 4 || shape == 8) {
        query.child_enhancement(leaf);
    }
    if (shape == 5 || shape == 8) {
        query.object_group_by("object", "id", leaf);
    }
    if (shape == 6 || shape == 8) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), "Leaf");
        query.filter(Expr::in_subquery(owner, "id", descriptor, leaf, "id"));
    }
    if (shape == 7 || shape == 8) {
        query.relation_aggregates.emplace_back("unused", "core aggregate", leaf, false);
    }
    return query;
}

static request::QueryOptions source_options(UserContext& user_context) {
    auto owner = user_context.memory_resource();
    SelectQuery query(std::allocator_arg, Allocator(owner), "Selection");
    request::QuerySelection selection(std::allocator_arg, Allocator(owner), query);
    selection.query_options.comment.emplace("selection metadata", owner);
    selection.query_options.raw_sql.emplace("trusted diagnostic metadata", owner);
    selection.query_options.raw_sql_search_criteria.emplace_back("selection criterion");
    auto leaf = source_leaf(user_context);
    selection.child_enhancements.emplace_back(leaf);
    selection.query_options.relation_aggregates.emplace_back(
        "aggregate edge", "builder aggregate", leaf, false);
    selection.query_options.facets.emplace_back("nested", "nested edge", leaf, false);
    request::QueryOptions options(std::allocator_arg, Allocator(owner));
    options.facets.emplace_back("future", "future edge", selection, true);
    return options;
}

static Json source_state(Memory owner, const PurposedSelectQuery& query) {
    Json::Object output(owner);
    output.emplace("query", Json(owner, debug_string(owner, query.as_query())));
    auto* diagnostic = detail::EntityDataServiceAccess::diagnostic_source(query);
    output.emplace("diagnostic",
                   diagnostic ? Json(owner, debug_string(owner, *diagnostic)) : Json(owner));
    output.emplace("wrapper", Json(owner, debug_string(owner, query)));
    return Json(owner, std::move(output));
}

static Json observe(UserContext& user_context, const Json& input) {
    auto owner = user_context.memory_resource();
    auto integer = [&](std::string_view name) {
        return unsigned(std::get<std::uint64_t>(field(input, name).data));
    };
    auto full = text(field(input, "kind")) == "full";
    auto base = full ? query_full_state_fixture::make(user_context, integer("profile"))
                     : source_traversal(user_context, integer("shape"));
    if (full && integer("field") != 31) {
        query_full_state_fixture::mutate(user_context, base, integer("field"));
    }
    auto original = base.clone(owner);
    auto detected = detail::query_has_facet_diagnostics(base);
    auto expanded = base.clone(owner);
    detail::expand_query_facet_diagnostics(expanded);
    auto options = source_options(user_context);
    auto selection_source = detail::facet_diagnostic_source(options.facets[0].query);
    auto* query = &user_context.make<PurposedSelectQuery>(base, " source purpose \t");
    Json::Array states(owner);
    states.emplace_back(source_state(owner, *query));
    for (const auto& step : std::get<Json::Array>(field(input, "steps").data)) {
        auto action = text(step);
        if (action == "count") {
            query->for_exact_count("count");
        } else if (action == "facets") {
            query->with_facet_diagnostics(options);
        } else if (action == "empty") {
            request::QueryOptions empty(std::allocator_arg, Allocator(owner));
            query->with_facet_diagnostics(empty);
        } else if (action == "clone") {
            query = &user_context.make<PurposedSelectQuery>(*query);
        } else {
            throw std::runtime_error("Unknown source step");
        }
        states.emplace_back(source_state(owner, *query));
    }
    auto copy = detail::clone_in(owner, *query);
    Json::Object output(owner);
    output.emplace("base", Json(owner, debug_string(owner, base)));
    output.emplace("has_facets", Json(owner, detected));
    output.emplace("expanded", Json(owner, debug_string(owner, expanded)));
    output.emplace("selection_source", Json(owner, debug_string(owner, selection_source)));
    output.emplace("states", Json(owner, std::move(states)));
    output.emplace("copy", source_state(owner, copy));
    output.emplace("exported", Json(owner, debug_string(owner, std::move(copy).into_query())));
    output.emplace("base_unchanged", Json(owner, base == original));
    return Json(owner, std::move(output));
}

static void retention() {
    GlobalContext global;
    Ref<PurposedSelectQuery> retained;
    Ref<String> expected;
    {
        auto user_context = global.make_user_context();
        auto owner = user_context->memory_resource();
        {
            auto base = query_full_state_fixture::make(*user_context, 2);
            auto& query = user_context->make<PurposedSelectQuery>(base, "retained source");
            auto options = source_options(*user_context);
            query.for_exact_count("count").with_facet_diagnostics(options);
            retained = user_context->make_global<PurposedSelectQuery>(query);
            auto* diagnostic = detail::EntityDataServiceAccess::diagnostic_source(query);
            expected = user_context->make_global<String>(debug_string(owner, *diagnostic));
        }
    }
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("Private diagnostic retention kept source User storage alive");
    }
    auto next_user_context = global.make_user_context();
    auto& copy = next_user_context->get(next_user_context->rebind_global(retained));
    auto* diagnostic = detail::EntityDataServiceAccess::diagnostic_source(copy);
    const auto& text = next_user_context->get(next_user_context->rebind_global(expected));
    if (!diagnostic || diagnostic->memory_resource() != global.memory_resource() ||
        debug_string(next_user_context->memory_resource(), *diagnostic) != text) {
        throw std::runtime_error("Private diagnostic destination copy lost nested state");
    }
    auto clone = detail::clone_in(next_user_context->memory_resource(), copy);
    auto* rebound = detail::EntityDataServiceAccess::diagnostic_source(clone);
    if (!rebound || rebound->memory_resource() != next_user_context->memory_resource() ||
        debug_string(next_user_context->memory_resource(), *rebound) != text) {
        throw std::runtime_error("Private diagnostic source cannot rebind into the next User");
    }
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
            retention();
            return 0;
        }
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid generated-support source fixture");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
