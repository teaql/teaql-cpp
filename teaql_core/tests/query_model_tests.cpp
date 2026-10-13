#include <teaql/core/query.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejects(F&& operation, const char* message) {
    bool rejected = false;
    try {
        operation();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, message);
}

struct NoDefaultAllocator {
    Memory previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    ~NoDefaultAllocator() {
        std::pmr::set_default_resource(previous);
    }
};

static_assert(ArenaObject<Expr> && ArenaObject<SelectQuery> && ArenaObject<RelationAggregate>);
static_assert(!std::is_copy_constructible_v<SelectQuery> &&
              !std::is_copy_constructible_v<RelationLoad>);
static void query_domain(const SelectQuery&, Memory);

static void expr_domain(const Expr& expr, Memory r) {
    check(expr.memory_resource() == r && expr.literal.memory_resource() == r &&
              expr.args.get_allocator().resource() == r,
          "recursive Expr allocator");
    for (const auto& arg : expr.args) {
        expr_domain(arg, r);
    }
    if (expr.subquery_data) {
        check(expr.subquery_data.get_deleter().resource == r, "subquery owner allocator");
        check(expr.subquery_data->entity.memory_resource() == r, "subquery metadata allocator");
        for (const auto& property : expr.subquery_data->entity.properties) {
            check(property.name.get_allocator().resource() == r, "nested metadata strings");
        }
        query_domain(expr.subquery_data->query, r);
    }
}

static void query_domain(const SelectQuery& query, Memory r) {
    check(query.memory_resource() == r, "query allocation domain");
    for (auto* text :
         {&query.search_text, &query.partition_field, &query.comment_text, &query.raw_sql_text}) {
        if (*text) {
            check((*text)->get_allocator().resource() == r, "optional query string allocator");
        }
    }
    for (auto* strings :
         {&query.projection, &query.group_by_fields, &query.raw_sql_search_criteria}) {
        check(strings->get_allocator().resource() == r, "query string vector allocator");
        for (const auto& text : *strings) {
            check(text.get_allocator().resource() == r, "query string element allocator");
        }
    }
    for (auto* expr : {&query.predicate, &query.having_predicate}) {
        if (*expr) {
            expr_domain(**expr, r);
        }
    }
    for (const auto& named : query.expr_projection) {
        check(named.memory_resource() == r, "named expression allocator");
        expr_domain(named.expr, r);
    }
    for (const auto& order : query.order_by) {
        check(order.memory_resource() == r, "order allocator");
        if (order.expr) {
            expr_domain(*order.expr, r);
        }
    }
    for (const auto& aggregate : query.aggregates) {
        check(aggregate.field.get_allocator().resource() == r &&
                  aggregate.alias.get_allocator().resource() == r,
              "aggregate allocator");
    }
    for (const auto& relation : query.relations) {
        check(relation.memory_resource() == r, "relation allocator");
        if (relation.query) {
            check(relation.query.get_deleter().resource == r, "relation query owner allocator");
            query_domain(*relation.query, r);
        }
    }
    for (const auto& trace : query.trace_chain) {
        check(trace.entity_type.get_allocator().resource() == r &&
                  trace.comment.get_allocator().resource() == r,
              "trace allocator");
    }
    for (auto* projections : {&query.dynamic_properties, &query.raw_projections}) {
        for (const auto& projection : *projections) {
            check(projection.property_name.get_allocator().resource() == r &&
                      projection.raw_sql_segment.get_allocator().resource() == r,
                  "raw projection allocator");
        }
    }
    for (const auto& object : query.object_group_bys) {
        check(object.property_name.get_allocator().resource() == r &&
                  object.storage_field.get_allocator().resource() == r,
              "object grouping allocator");
        query_domain(object.query, r);
    }
    for (const auto& child : query.child_enhancements) {
        query_domain(child, r);
    }
    if (query.continuous_page_fetch) {
        check(query.continuous_page_fetch->memory_resource() == r, "cursor allocator");
    }
    if (query.id_set_pagination) {
        check(query.id_set_pagination->memory_resource() == r, "ID set allocator");
    }
}

static void expression_contracts() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto context = global.make_user_context();
    auto r = context->memory_resource();
    auto& literal = context->make<Expr>(Expr::value(r, "a long literal with its own allocation"));
    using Factory = Expr (*)(const Expr&);
    const std::array<Factory, 14> functions{Expr::soundex,
                                            Expr::gbk,
                                            Expr::count_expr,
                                            Expr::sum_expr,
                                            Expr::avg_expr,
                                            Expr::min_expr,
                                            Expr::max_expr,
                                            Expr::stddev_expr,
                                            Expr::stddev_pop_expr,
                                            Expr::var_samp_expr,
                                            Expr::var_pop_expr,
                                            Expr::bit_and_expr,
                                            Expr::bit_or_expr,
                                            Expr::bit_xor_expr};
    const std::array<ExprFunction, 14> kinds{ExprFunction::Soundex,
                                             ExprFunction::Gbk,
                                             ExprFunction::Count,
                                             ExprFunction::Sum,
                                             ExprFunction::Avg,
                                             ExprFunction::Min,
                                             ExprFunction::Max,
                                             ExprFunction::Stddev,
                                             ExprFunction::StddevPop,
                                             ExprFunction::VarSamp,
                                             ExprFunction::VarPop,
                                             ExprFunction::BitAnd,
                                             ExprFunction::BitOr,
                                             ExprFunction::BitXor};
    for (std::size_t i = 0; i < functions.size(); ++i) {
        auto expr = functions[i](literal);
        check(expr.kind == Expr::Kind::Function && expr.function_kind == kinds[i] &&
                  expr.args.size() == 1 && expr.args[0] == literal,
              "function node shape");
    }
    check(Expr::count_all(r).args.empty(), "count-all has zero arguments");
    using TextFactory = Expr (*)(Memory, std::string_view, std::string_view);
    const std::array<TextFactory, 6> text_factories{Expr::contain,
                                                    Expr::not_contain,
                                                    Expr::begin_with,
                                                    Expr::not_begin_with,
                                                    Expr::end_with,
                                                    Expr::not_end_with};
    const std::array<std::string_view, 6> patterns{
        "%tea%", "%tea%", "tea%", "tea%", "%tea", "%tea"};
    for (std::size_t index = 0; index < text_factories.size(); ++index) {
        auto expression = text_factories[index](r, "name", "tea");
        auto operation = index % 2 ? BinaryOp::NotLike : BinaryOp::Like;
        auto expected = Expr::binary(
            Expr::column(r, "name"), operation, Expr::like_pattern(r, patterns[index], "tea"));
        check(expression == expected && expression.args[1].name == "tea",
              "typed text predicates preserve the 5.0.6 original-operand node");
        auto raw = operation == BinaryOp::Like ? Expr::like(r, "name", patterns[index])
                                               : Expr::not_like(r, "name", patterns[index]);
        check(raw.kind == Expr::Kind::Binary && raw.op == operation && raw.args.size() == 2 &&
                  raw.args[1].kind == Expr::Kind::Value &&
                  raw.args[1].literal.try_text() == patterns[index] && !raw.lowered_like_operand(),
              "raw LIKE preserves literal bytes without typed lowering provenance");
        check(!(expression == raw), "typed lowering provenance is distinct from a raw LIKE value");
    }
    auto sound = Expr::sound_like(r, "name", "Robert");
    check(sound.op == BinaryOp::Eq && sound.args[0].function_kind == ExprFunction::Soundex &&
              sound.args[1].function_kind == ExprFunction::Soundex,
          "sound-like compares functions");
    auto compare = Expr::compare_columns(r, "updated_at", BinaryOp::Gte, "created_at");
    check(compare.args[1].kind == Expr::Kind::Column && compare.args[1].name == "created_at",
          "column-to-column comparison");
    auto& values = context->make<Vector<Value>>();
    for (int i = 0; i < 20; ++i) {
        values.emplace_back(i);
    }
    auto in = Expr::in_list(r, "id", values);
    check(in.op == BinaryOp::In && in.args.size() == 2 &&
              in.args[1].literal.as_list()->size() == 20,
          "IN uses core List value");
    values.emplace_back(20);
    check(Expr::not_in_list(r, "id", values).op == BinaryOp::NotInLarge, "21-item promotion");
    check(Expr::in_large(r, "id", {}).op == BinaryOp::InLarge,
          "explicit large representation even for empty input");
    auto& parts = context->make<Vector<Expr>>();
    parts.emplace_back(Expr::eq(r, "id", 1));
    parts.emplace_back(parts.front());
    parts.emplace_back(Expr::gt(r, "version", 0));
    auto all = Expr::all_of(r, parts);
    auto any = Expr::any_of(r, parts);
    check(all.args.size() == 2 && any.args.size() == 2, "stable logical deduplication");
    check(all.and_expr(parts[0]) == all && any.or_expr(parts[0]) == any,
          "appending duplicate is idempotent");
    check(Expr::any_of(r, {}).kind == Expr::Kind::Or, "empty logical node remains representable");
    check(all.and_expr(any).args.back().kind == Expr::Kind::Or, "mixed logical groups stay nested");
    check(Expr::negate(all).args[0] == all, "logical negation preserves child");
}

static void query_clone_contracts() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto context = global.make_user_context();
    auto r = context->memory_resource();
    auto& descriptor = context->make<EntityDescriptor>("OrderLine");
    descriptor.property("order_id", DataType::U64);
    descriptor.property("name", DataType::Text);
    auto& child = context->make<SelectQuery>("OrderLine");
    child.projects({"name", "id"})
        .filter(Expr::eq(r, "name", "long query text kept across allocation domains"));
    auto& positive = context->make<Expr>(Expr::in_subquery(r, "id", descriptor, child, "order_id"));
    auto& negative =
        context->make<Expr>(Expr::not_in_subquery(r, "id", descriptor, child, "order_id"));
    check(negative.kind == Expr::Kind::SubQuery && negative.op == BinaryOp::NotIn,
          "negative subquery kind");
    check(negative.subquery_data->query.projection.size() == 1 &&
              negative.subquery_data->query.projection[0] == "order_id",
          "subquery replaces projection");
    check(negative.subquery_data->query.predicate->kind == Expr::Kind::And &&
              negative.subquery_data->query.predicate->args[1] == Expr::is_not_null(r, "order_id"),
          "negative subquery excludes NULL projected keys");
    check(positive.subquery_data->query.predicate == child.predicate,
          "positive subquery does not add NULL filter");
    check(child.projection.size() == 2 && child.predicate->kind == Expr::Kind::Binary,
          "building subquery preserves input");
    auto& query = context->make<SelectQuery>("Order");
    check(!query.comment_text && !query.slice && !query.stream_config && !query.aggregation_cache &&
              !query.continuous_page_fetch && !query.id_set_pagination,
          "query optional defaults");
    query.projects({"id", "name"})
        .project_expr("countExpr", Expr::count_all(r))
        .search_with_text("full text search phrase")
        .filter(negative)
        .or_filter(Expr::is_null(r, "name"))
        .having(Expr::gt(r, "total", 1))
        .and_having(Expr::lt(r, "total", 100))
        .order_desc("id")
        .order_gbk_asc("name")
        .group_by("name")
        .count("total")
        .sum("version", "versionSum")
        .stddev("version", "versionStddev")
        .enable_aggregation_cache_for(1000)
        .propagate_aggregation_cache(2000)
        .comment("initial diagnostic comment")
        .comment("replacement diagnostic comment")
        .relation("customer")
        .relation_query("lines", child)
        .object_group_by("owner", "owner_id", child)
        .child_enhancement(child)
        .raw_sql("SELECT id FROM orders")
        .add_raw_sql_search_criteria("version > 0")
        .dynamic_property_raw("dynamic", "id + 1")
        .project_raw("raw", "id + 2")
        .partition_by("owner_id")
        .top_n_probe_parent_threshold(0)
        .page(20, 10)
        .stream(0)
        .optimize_for_continuous_page_fetch_with("recent-orders-with-long-name", 30)
        .optimize_pagination_with_id_set_config("retained-orders-with-long-name", 40, 5000);
    check(query.trace_chain.size() == 2 && query.trace_chain.front().kind == TraceKind::Comment &&
              query.trace_chain.front().comment == "initial diagnostic comment",
          "comment appends provenance instead of overwriting trace");
    check(*query.comment_text == "replacement diagnostic comment", "comment retains last text");
    check(query.order_by[1].field.empty() &&
              query.order_by[1].expr->function_kind == ExprFunction::Gbk,
          "expression order has no field name");
    check(query.stream_config->chunk_size == 0 && query.top_n_probe_parent_threshold_value == 0,
          "builder preserves explicit zero hints");
    check(query.aggregation_cache == AggregationCacheOptions{true, 1000, true, 2000},
          "cache option composition");
    auto& relation_aggregate = context->make<RelationAggregate>("lines", "lineCount", child, true);
    auto& retained_aggregate = global.make<RelationAggregate>(relation_aggregate);
    auto& retained = global.make<SelectQuery>(query);
    auto& assigned = global.make<SelectQuery>("destination");
    assigned = std::move(query);
    auto& retained_expr = global.make<Expr>(negative);
    check(retained == query && assigned == query, "complete query clone includes every field");
    query_domain(retained, global.memory_resource());
    query_domain(assigned, global.memory_resource());
    expr_domain(retained_expr, global.memory_resource());
    query.comment("only source changes");
    check(!(retained == query), "query clone independent of later source edits");
    context.reset();
    check(*retained.comment_text == "replacement diagnostic comment",
          "global query survives source Context destruction");
    check(retained.relations[1].query->entity == "OrderLine" &&
              retained.object_group_bys[0].query.entity == "OrderLine",
          "nested queries survive");
    check(retained_aggregate.query.entity == "OrderLine" && retained_aggregate.single_result,
          "relation aggregate is a separate retained type");
    check(retained_expr.subquery_data->entity.property_by_name("order_id") != nullptr,
          "subquery metadata survives");
    // A vector insertion propagates the destination allocator even when given an rvalue.
    auto second = global.make_user_context();
    auto& list = second->make<Vector<SelectQuery>>();
    list.emplace_back(std::move(assigned));
    query_domain(list.front(), second->memory_resource());
    check(assigned.entity == "Order", "cross-domain move keeps the source ownership intact");
}

static void pagination_contracts() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto context = global.make_user_context();
    auto& child = context->make<SelectQuery>("Line");
    child.hard_limit(20'000).offset(7);
    auto& query = context->make<SelectQuery>("Order");
    query.hard_limit(20'000)
        .relation("implicit")
        .relation_query("lines", child)
        .child_enhancement(child)
        .object_group_by("owner", "owner_id", child);
    auto prepared = query.prepare_for_list();
    check(prepared.slice->limit == 20'000 && prepared.relations[1].query->slice->limit == 10'000,
          "relation ceiling stays 10000 independently of overrides");
    check(prepared.relations[1].query->slice->offset == 7 &&
              prepared.child_enhancements[0].slice->limit == 10'000,
          "nested offset and enhancement limit");
    check(!prepared.relations[0].query && !prepared.object_group_bys[0].query.slice->limit,
          "preparation touches only Rust-defined nested query branches");
    check(!query.slice && !query.relations[1].query->slice->limit,
          "list preparation never mutates source");
    query.relations[1].query->limit(10'001);
    rejects(
        [&] {
            query.prepare_for_list();
        },
        "nested hard-limit violation");
    query.relations[1].query->limit(0);
    check(query.prepare_for_list().relations[1].query->slice->limit == 0,
          "nested zero limit retained");
    query.optimize_for_continuous_page_fetch().optimize_pagination_with_id_set();
    check(query.continuous_page_fetch->namespace_name == "default" &&
              query.continuous_page_fetch->ttl_seconds == 600,
          "cursor defaults");
    check(query.id_set_pagination->max_ids == 3'000'000 &&
              query.id_set_pagination->ttl_seconds == 600,
          "ID-set defaults");
    for (std::string_view blank : {"", " \t\r\n", "\xc2\xa0", "\xe3\x80\x80"}) {
        rejects(
            [&] {
                query.optimize_for_continuous_page_fetch_with(blank, 30);
            },
            "Unicode whitespace namespace rejected");
    }
    rejects(
        [&] {
            query.optimize_for_continuous_page_fetch_with("valid", 0);
        },
        "zero cursor ttl");
    rejects(
        [&] {
            query.optimize_pagination_with_id_set_config("valid", 1, 0);
        },
        "zero ID max");
    rejects(
        [&] {
            query.optimize_pagination_with_id_set_config("valid", 0, 1);
        },
        "zero ID ttl");
    check(query.continuous_page_fetch->namespace_name == "default" &&
              query.id_set_pagination->max_ids == 3'000'000,
          "invalid option replacement preserves previous value");
    query.stream_default();
    check(query.stream_config->chunk_size == 1000, "default streaming chunk");
    auto& empty_comment = context->make<SelectQuery>("Order");
    empty_comment.comment("");
    check(empty_comment.comment_text && empty_comment.comment_text->empty(),
          "Some empty comment differs from None");
}

struct FailingResource : std::pmr::memory_resource {
    bool fail = false;
    std::size_t live_blocks = 0;

    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (fail) {
            throw std::bad_alloc();
        }
        auto* block = std::pmr::new_delete_resource()->allocate(bytes, alignment);
        ++live_blocks;
        return block;
    }

    void do_deallocate(void* block, std::size_t bytes, std::size_t alignment) override {
        --live_blocks;
        std::pmr::new_delete_resource()->deallocate(block, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

static void failed_query_clone() {
    NoDefaultAllocator guard;
    FailingResource upstream;
    {
        GlobalContext global(&upstream, 256);
        auto context = global.make_user_context(256);
        auto& text = context->make<String>(256 * 1024, 'x');
        auto& child = context->make<SelectQuery>("Inner");
        child.comment(text);
        auto& source = context->make<SelectQuery>("Outer");
        source.relation_query("nested", child);
        auto& target = global.make<SelectQuery>("Previous");
        target.comment("keep the destination alive");
        auto& expected = global.make<SelectQuery>(target);
        upstream.fail = true;
        bool failed = false;
        try {
            target = std::move(source);
        } catch (const std::bad_alloc&) {
            failed = true;
        }
        upstream.fail = false;
        check(failed && target == expected,
              "failed nested clone leaves destination alive and unchanged");
        check(source.relations[0].query->comment_text->size() == text.size(),
              "failed cross-domain move preserves source");
        target = std::move(source);
        context.reset();
        check(target.relations[0].query->comment_text->size() == 256 * 1024,
              "retry yields independently owned nested query");
    }
    check(upstream.live_blocks == 0,
          "all arena blocks reclaimed after failed and successful query clones");
}

int main() {
    try {
        expression_contracts();
        query_clone_contracts();
        pagination_contracts();
        failed_query_clone();
        std::cout << "Query model contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
