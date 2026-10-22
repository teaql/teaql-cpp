#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/logging.hpp>
#include <teaql/runtime/facet_runtime.hpp>
#include <teaql/core/debug.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/schema.hpp>
#include <teaql/sql/executor.hpp>
#include <sqlite3.h>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static EntityDescriptor entity(Memory r, std::string_view name, std::string_view table) {
    EntityDescriptor out(std::allocator_arg, Allocator(r), name);
    out.table(table);
    out.property("id", DataType::I64).id();
    out.property("version", DataType::I64).version();
    out.property("name", DataType::Text);
    return out;
}

template <bool Platform>
struct View : EntityDefaults<View<Platform>> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = Platform ? "Platform" : "SchoolType";
    CompactRow row;
    EntityRuntimeState state;

    View(std::allocator_arg_t, Allocator a, CompactRow row)
        : row(std::allocator_arg, a, std::move(row)), state(std::allocator_arg, a) {}

    View(std::allocator_arg_t, Allocator a, const View& other)
        : row(std::allocator_arg, a, other.row), state(std::allocator_arg, a, other.state) {}

    View(std::allocator_arg_t, Allocator a, View&& other)
        : View(detail::move_to(a.resource(), std::move(other))) {}

    View(View&&) noexcept = default;

    Memory memory_resource() const {
        return row.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return entity(r, ENTITY_NAME, Platform ? "facet_platform" : "facet_school_type");
    }

    static Result<View, EntityError> from_compact_row(Memory r, CompactRow row) {
        return Result<View, EntityError>::ok(
            r, View(std::allocator_arg, Allocator(r), std::move(row)));
    }

    Value id_value() const {
        return Value(memory_resource(), *row.get("id"));
    }

    MutationValues into_values() && {
        MutationValues out(memory_resource());
        out.fields = row.into_map(memory_resource());
        return out;
    }

    void on_loaded(AnyRef context) {
        state = detail::clone_in(memory_resource(), *context.get<EntityRuntimeState>());
    }
};

using PlatformView = View<true>;
using SchoolTypeView = View<false>;
using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;

static Executor&
setup(UserContext& user_context, SqliteConnection** native = nullptr, bool loaded = true) {
    auto r = user_context.memory_resource();
    auto school = entity(r, "School", "facet_school");
    school.mask_fields({"name"});
    school.property("type_id", DataType::I64);
    auto& school_type = school.relation("school_type", "SchoolType");
    school_type.local_key = "type_id";
    school_type.foreign_key = "id";
    auto type = entity(r, "SchoolType", "facet_school_type");
    type.mask_fields({"name"});
    type.property("platform_id", DataType::I64);
    auto& schools = type.relation("schools", "School");
    schools.many = true;
    schools.local_key = "id";
    schools.foreign_key = "type_id";
    auto& platform = type.relation("platform", "Platform");
    platform.local_key = "platform_id";
    platform.foreign_key = "id";
    auto root = entity(r, "Platform", "facet_platform");
    auto& types = root.relation("school_types", "SchoolType");
    types.many = true;
    types.local_key = "id";
    types.foreign_key = "platform_id";
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    metadata.register_entity(std::move(school));
    metadata.register_entity(std::move(type));
    metadata.register_entity(std::move(root));
    user_context.set_metadata(user_context, metadata);
    auto& entities = user_context.make<InMemoryEntityRegistry>();
    for (auto name : {"Platform", "SchoolType", "School"}) {
        entities.register_entity(name);
    }
    user_context.set_entity_registry(user_context, entities);
    auto& connection = user_context.make_resource<SqliteConnection>();
    if (native) {
        *native = &connection;
    }
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto dialect = user_context.require_resource<SqliteDialect>();
    check(bool(dialect), "SQLite dialect installed");
    auto& state = user_context.make<SqlExecutorState>();
    auto& executor = user_context.make<Executor>(dialect.value().get(), transport, metadata, state);
    user_context.register_executor(user_context, executor);
    check(bool(user_context.ensure_schema().wait()), "modeled schema initialized");
    // Fixture seeding precedes the observed read; no rows or trace frames are injected.
    const char* seed =
        "BEGIN;"
        "INSERT INTO facet_platform(id,version,name) VALUES(1,1,'used'),(2,1,'empty');"
        "INSERT INTO facet_school_type(id,version,name,platform_id) VALUES(10,1,'primary',1),(20,1,'secondary',1),(30,1,'unused',1);"
        "INSERT INTO facet_school(id,version,name,type_id) VALUES(100,1,'FACET-PRIVATE-SCHOOL',10),(101,1,'FACET-PRIVATE-SCHOOL',10),(102,1,'FACET-PRIVATE-SCHOOL',20),(103,1,'excluded',20);"
        "COMMIT;";
    check(sqlite3_exec(connection.native_handle(), seed, nullptr, nullptr, nullptr) == SQLITE_OK,
          "transactional fixture seed");
    if (loaded) {
        check(
            sqlite3_exec(
                connection.native_handle(),
                "INSERT INTO facet_platform(id,version,name) VALUES(3,1,'no types');"
                "INSERT INTO facet_school_type(id,version,name,platform_id) VALUES(40,1,'primary',2);",
                nullptr,
                nullptr,
                nullptr) == SQLITE_OK,
            "loaded fixture additions");
    }
    auto& decoders = user_context.make<InMemoryEntityGraphDecoderRegistry>();
    decoders.register_entity<PlatformView>();
    decoders.register_entity<SchoolTypeView>();
    user_context.set_entity_graph_decoder_registry(decoders);
    user_context.clear_sql_logs();
    return executor;
}

static PurposedSelectQuery loaded_query(Memory r, bool include_all, std::size_t threshold) {
    SelectQuery choice_query(std::allocator_arg, Allocator(r), "Platform");
    choice_query.limit(10);
    request::QuerySelection choices(std::allocator_arg, Allocator(r), choice_query);
    SelectQuery aggregate(std::allocator_arg, Allocator(r), "SchoolType");
    choices.query_options.relation_aggregates.emplace_back(
        "school_types", "type_count", aggregate, true);
    aggregate.filter(Expr::eq(r, "name", Value(r, "primary")));
    choices.query_options.relation_aggregates.emplace_back(
        "school_types", "matching_count", aggregate, true);
    SelectQuery type_query(std::allocator_arg, Allocator(r), "SchoolType");
    type_query.filter(Expr::ne(r, "name", Value(r, "unused")))
        .order_asc("id")
        .limit(1)
        .top_n_probe_parent_threshold(threshold);
    request::QuerySelection types(std::allocator_arg, Allocator(r), type_query);
    types.query_options.facets.emplace_back("platformChoices", "platform", choices, include_all);
    SelectQuery root_query(std::allocator_arg, Allocator(r), "Platform");
    root_query.order_asc("id").limit(10);
    request::QuerySelection root(std::allocator_arg, Allocator(r), root_query);
    root.relation_selections.emplace_back("school_types", types);
    auto query = std::move(root).into_query();
    query.comment("load primary choices by platform");
    PurposedSelectQuery out(
        std::allocator_arg, Allocator(r), query, "render primary counts independently");
    request::QueryOptions options(std::allocator_arg, Allocator(r));
    out.with_facet_diagnostics(options);
    return out;
}

static void per_parent_facets() {
    for (bool include_all : {true, false}) {
        for (bool logging : {true, false}) {
            for (std::size_t threshold : {0, 32}) {
                GlobalContext global;
                auto user_context = global.make_user_context();
                auto r = user_context->memory_resource();
                auto& executor = setup(*user_context);
                if (!logging) {
                    user_context->disable_sql_log();
                }
                auto& service = EntityDataService<Executor>::for_executor(
                    *user_context, "Platform", *user_context, executor);
                auto query = loaded_query(r, include_all, threshold);
                const auto& result = service.fetch_enhanced_entities<PlatformView>(query).wait();
                check(bool(result), "enhanced SQLite facet query succeeds");
                check(result.value().data.size() == 3, "three parent rows survive");
                for (const auto& parent : result.value().data) {
                    auto id = *parent.row.get("id")->try_u64();
                    auto handle =
                        parent.state.relation_list<SchoolTypeView>("Platform", id, "school_types");
                    auto* types = handle.value();
                    check(types && types->data.size() == std::size_t(id != 3),
                          "TopN retains one row or a selected empty list");
                    auto* choices = types->facet("platformChoices");
                    check(choices != nullptr, "typed relation retains per-parent facets");
                    check(choices->data.size() == (include_all ? 3U
                                                   : id == 3   ? 0U
                                                               : 1U),
                          "include-all controls empty facet members");
                    Vector<std::uint64_t> choice_ids(r);
                    for (const auto& choice : choices->data) {
                        auto choice_id = *choice.get("id")->try_u64();
                        choice_ids.push_back(choice_id);
                        auto expected = choice_id == id ? (id == 1 ? 2 : id == 2 ? 1 : 0) : 0;
                        check(choice.get("type_count")->try_i64() == expected,
                              "facet counts use the full filtered relation, not TopN rows");
                        check(choice.get("matching_count")->try_i64() ==
                                  (id != 3 && choice_id == id ? 1 : 0),
                              "future aggregate predicate remains scoped to its parent");
                    }
                    std::sort(choice_ids.begin(), choice_ids.end());
                    for (std::size_t index = 0; index < choice_ids.size(); ++index) {
                        check(choice_ids[index] == (include_all ? index + 1 : id),
                              "facet member identities match the source, without duplicates");
                    }
                    check(!parent.row.contains_key("platformChoices"),
                          "facets are not root columns");
                    for (const auto& type : types->data) {
                        check(!type.row.contains_key("platformChoices"),
                              "facets are not child columns");
                    }
                    check(handle.state() ==
                              (id == 3 ? LoadedRelation::Empty : LoadedRelation::Loaded),
                          "selected empty relation state is explicit");
                }
                const auto& logs = user_context->sql_logs();
                check(logs.size() ==
                          (logging ? 1 + (threshold == 0 ? 1 : 3) + (include_all ? 9 : 7) : 0),
                      "bounded batch/probe and per-owner facet statement count");
                bool nested_route = false;
                for (const auto& log : logs) {
                    check(detail::debug_sql_log(r, log).find("primary") == String::npos,
                          "future facet binding is masked in every SQL log");
                    check(log.trace_path.size() >= 2 &&
                              log.trace_path[0].entity_type == "Platform" &&
                              log.trace_path[1].entity_type == "Platform",
                          "original root ancestry is preserved");
                    Vector<String> edges(r);
                    for (const auto& node : log.trace_path) {
                        if (node.kind == TraceKind::Relation) {
                            edges.emplace_back(node.comment);
                        }
                    }
                    nested_route |= edges.size() == 3 && edges[0] == "Platform.school_types" &&
                                    edges[1] == "SchoolType.platform" &&
                                    edges[2] == "Platform.school_types";
                }
                check(!logging || nested_route, "nested relation route includes all field edges");
            }
        }
    }
}

static void full_choices(const SmartList<CompactRow>* choices, std::uint64_t owner) {
    check(choices && choices->data.size() == 3, "all three facet choices survive");
    Vector<std::uint64_t> ids(choices->memory_resource());
    for (const auto& row : choices->data) {
        auto id = *row.get("id")->try_u64();
        ids.push_back(id);
        check(row.get("type_count")->try_i64() == (id == owner ? (owner == 1   ? 2
                                                                  : owner == 2 ? 1
                                                                               : 0)
                                                               : 0),
              "compact and slow typed paths retain full filtered counts");
    }
    std::sort(ids.begin(), ids.end());
    check(ids[0] == 1 && ids[1] == 2 && ids[2] == 3, "facet choice identity set");
}

static void compact_and_slow_paths() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& executor = setup(*user_context);
    auto& service = EntityDataService<Executor>::for_executor(
        *user_context, "Platform", *user_context, executor);
    auto query = loaded_query(r, true, 0);
    const auto& compact = service.fetch_all(query).wait();
    check(compact && compact.value().size() == 3, "ordinary compact query returns all parents");
    for (const auto& row : compact.value()) {
        auto id = *row.get("id")->try_u64();
        const auto* types = row.loaded_relation("school_types");
        check(types && types->data.size() == std::size_t(id != 3),
              "compact sidecar retains TopN and empty lists");
        full_choices(types->facet("platformChoices"), id);
        check(compact_row_to_json_value(r, row).dump(r).find("platformChoices") == String::npos,
              "compact JSON excludes query-only facet sidecars");
        check(debug_string(r, row.into_map(r)).find("platformChoices") == String::npos,
              "persistent maps exclude query-only facet sidecars");
    }
    user_context->clear_sql_logs();
    SelectQuery count(std::allocator_arg, Allocator(r), "SchoolType");
    Vector<RelationAggregate> aggregates(r);
    aggregates.emplace_back("school_types", "whole_count", count, true);
    const auto& typed =
        service.fetch_enhanced_entities_with_relation_aggregates<PlatformView>(query, aggregates)
            .wait();
    check(typed && typed.value().data.size() == 3,
          "slow typed aggregate query returns all parents");
    for (const auto& parent : typed.value().data) {
        auto id = *parent.row.get("id")->try_u64();
        auto types = parent.state.relation_list<SchoolTypeView>("Platform", id, "school_types");
        check(types.value() != nullptr, "slow hydration retains the selected relation");
        full_choices(types.value()->facet("platformChoices"), id);
        check(parent.row.get("whole_count")->try_i64() == (id == 1   ? 3
                                                           : id == 2 ? 1
                                                                     : 0),
              "root aggregate differs from filtered relation membership");
    }
    check(user_context->sql_logs().size() == 12, "slow typed path executes twelve statements");
    for (const auto& log : user_context->sql_logs()) {
        check(detail::debug_sql_log(r, log).find("primary") == String::npos,
              "slow typed path masks the future relation predicate");
    }
}

static PurposedSelectQuery
to_one_query(Memory r, bool filtered, bool missing_key, bool include_all) {
    SelectQuery choice_query(std::allocator_arg, Allocator(r), "SchoolType");
    choice_query.limit(10);
    request::QuerySelection choices(std::allocator_arg, Allocator(r), choice_query);
    SelectQuery aggregate(std::allocator_arg, Allocator(r), "Platform");
    choices.query_options.relation_aggregates.emplace_back(
        "platform", "platform_count", aggregate, true);
    SelectQuery target_query(std::allocator_arg, Allocator(r), "Platform");
    if (filtered) {
        target_query.filter(Expr::ne(r, "name", Value(r, "empty")));
    }
    request::QuerySelection target(std::allocator_arg, Allocator(r), target_query);
    target.query_options.facets.emplace_back("typeChoices", "school_types", choices, include_all);
    SelectQuery root_query(std::allocator_arg, Allocator(r), "SchoolType");
    root_query.projects({"id", "version", "name", "platform_id"}).limit(10);
    if (filtered) {
        root_query.order_asc("id");
    }
    if (missing_key) {
        root_query.raw_sql("SELECT id, version, name FROM facet_school_type ORDER BY id");
    }
    request::QuerySelection source(std::allocator_arg, Allocator(r), root_query);
    source.query_options.raw_sql = detail::clone_in(r, source.query.raw_sql_text);
    source.relation_selections.emplace_back("platform", target);
    auto query = std::move(source).into_query();
    query.comment(filtered ? "load selected platform facets" : "load nullable relation choices");
    return PurposedSelectQuery(std::allocator_arg,
                               Allocator(r),
                               query,
                               filtered ? "retain read-only relation metadata"
                                        : "preserve empty relation Facets");
}

static void to_one_counts(const SmartList<CompactRow>* choices,
                          std::uint64_t owner,
                          bool empty,
                          bool include_all) {
    check(choices != nullptr, "to-one relation retains its facet metadata");
    auto r = choices->memory_resource();
    using Count = std::pair<std::int64_t, std::int64_t>;
    Vector<Count> actual(r), expected(r);
    for (const auto& row : choices->data) {
        auto* id = row.get("id");
        auto* count = row.get("platform_count");
        check(id && count && id->try_i64() && count->try_i64(),
              "facet counts have integer identities and values");
        actual.emplace_back(*id->try_i64(), *count->try_i64());
    }
    std::sort(actual.begin(), actual.end());
    for (std::int64_t id : {10, 20, 30, 40, 50}) {
        if (include_all || (!empty && (owner == 40 ? id == 40 : id <= 30))) {
            expected.emplace_back(id, !empty && id != 50 && (id == 40) == (owner == 40) ? 1 : 0);
        }
    }
    check(actual == expected,
          "to-one facet identities and counts are isolated per owner without orphan counts");
}

static void shared_and_filtered_to_one() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& executor = setup(*user_context);
    auto& service = EntityDataService<Executor>::for_executor(
        *user_context, "SchoolType", *user_context, executor);
    auto query = to_one_query(r, true, false, false);
    const auto& typed = service.fetch_enhanced_entities<SchoolTypeView>(query).wait();
    check(typed && typed.value().data.size() == 4, "typed to-one query retains four owners");
    for (const auto& row : typed.value().data) {
        auto id = *row.row.get("id")->try_u64();
        to_one_counts(row.state.relation_facet("SchoolType", id, "platform", "typeChoices"),
                      id,
                      id == 40,
                      false);
        check(debug_string(r, row.row.into_map(r)).find("typeChoices") == String::npos,
              "typed to-one facets do not become persistent values");
    }
    const auto& compact = service.fetch_all(query).wait();
    check(compact && compact.value().size() == 4, "compact to-one query retains four owners");
    for (const auto& row : compact.value()) {
        auto id = *row.get("id")->try_u64();
        auto* target = row.loaded_relation("platform");
        check(target && target->data.size() == 1, "filtered to-one keeps its foreign-key identity");
        if (id == 40) {
            check(target->data[0].len() == 1 && target->data[0].contains_key("id") &&
                      !target->data[0].contains_key("name"),
                  "filtered target is identity-only, not leaked details");
        }
        to_one_counts(target->facet("typeChoices"), id, id == 40, false);
        check(compact_row_to_json_value(r, row).dump(r).find("typeChoices") == String::npos,
              "compact to-one JSON excludes facet metadata");
    }
}

static void null_and_missing_to_one() {
    for (bool missing_key : {false, true}) {
        for (bool include_all : {false, true}) {
            for (bool logging : {false, true}) {
                GlobalContext global;
                auto user_context = global.make_user_context();
                auto r = user_context->memory_resource();
                SqliteConnection* native = nullptr;
                auto& executor = setup(*user_context, &native);
                check(
                    sqlite3_exec(
                        native->native_handle(),
                        "INSERT INTO facet_school_type(id,version,name,platform_id) VALUES(50,1,'unassigned',NULL)",
                        nullptr,
                        nullptr,
                        nullptr) == SQLITE_OK,
                    "seed nullable to-one owner");
                user_context->clear_sql_logs();
                if (!logging) {
                    user_context->disable_sql_log();
                }
                auto& service = EntityDataService<Executor>::for_executor(
                    *user_context, "SchoolType", *user_context, executor);
                auto query = to_one_query(r, false, missing_key, include_all);
                const auto& rows = service.fetch_enhanced_entities<SchoolTypeView>(query).wait();
                check(rows && rows.value().data.size() == 5,
                      "nullable or missing-key query retains five root rows");
                for (const auto& row : rows.value().data) {
                    auto id = *row.row.get("id")->try_u64();
                    to_one_counts(
                        row.state.relation_facet("SchoolType", id, "platform", "typeChoices"),
                        id,
                        missing_key || id == 50,
                        include_all);
                }
                check(user_context->sql_logs().size() <= 12,
                      "to-one facet work stays bounded per selected owner");
                check(logging || user_context->sql_logs().empty(),
                      "disabled logging retains no diagnostics");
            }
        }
    }
}

struct SchoolRow : EntityDefaults<SchoolRow> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "School";
    CompactRow row;

    SchoolRow(std::allocator_arg_t, Allocator a, CompactRow value)
        : row(std::allocator_arg, a, std::move(value)) {}

    SchoolRow(std::allocator_arg_t, Allocator a, const SchoolRow& other)
        : row(std::allocator_arg, a, other.row) {}

    SchoolRow(std::allocator_arg_t, Allocator a, SchoolRow&& other)
        : row(std::allocator_arg, a, std::move(other.row)) {}

    SchoolRow(SchoolRow&&) noexcept = default;

    Memory memory_resource() const {
        return row.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return entity(r, ENTITY_NAME, "facet_school");
    }

    static Result<SchoolRow, EntityError> from_compact_row(Memory r, CompactRow row) {
        return Result<SchoolRow, EntityError>::ok(
            r, SchoolRow(std::allocator_arg, Allocator(r), std::move(row)));
    }

    MutationValues into_values() && {
        MutationValues values(memory_resource());
        values.fields = row.into_map(memory_resource());
        return values;
    }
};

static request::QueryOptions future_private_options(Memory r, std::string_view secret) {
    SelectQuery types(std::allocator_arg, Allocator(r), "SchoolType");
    types.limit(10);
    request::QuerySelection selection(std::allocator_arg, Allocator(r), types);
    SelectQuery schools(std::allocator_arg, Allocator(r), "School");
    schools.filter(Expr::eq(r, "name", Value(r, secret)));
    selection.query_options.relation_aggregates.emplace_back(
        "schools", "school_count", schools, true);
    request::QueryOptions options(std::allocator_arg, Allocator(r));
    options.facets.emplace_back("typeChoices", "school_type", selection, true);
    return options;
}

static void future_bindings_at_all_entries() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& executor = setup(*user_context);
    auto& service =
        EntityDataService<Executor>::for_executor(*user_context, "School", *user_context, executor);
    SelectQuery root(std::allocator_arg, Allocator(r), "School");
    root.limit(10).comment("future facets FACET-PRIVATE-SCHOOL");
    PurposedSelectQuery query(
        std::allocator_arg, Allocator(r), root, "render future choices FACET-PRIVATE-SCHOOL");
    query.with_facet_diagnostics(future_private_options(r, "FACET-PRIVATE-SCHOOL"));
    check(query.as_query().child_enhancements.empty(),
          "future facet diagnostics are not executable enhancements");
    const auto& raw = service.fetch_all(query).wait();
    check(raw && raw.value().size() == 4, "raw entry returns four rows");
    const auto& owned = service.fetch_all_owned(detail::clone_in(r, query)).wait();
    check(owned && owned.value().size() == 4, "owned entry returns four rows");
    const auto& smart = service.fetch_smart_list(query).wait();
    check(smart && smart.value().data.size() == 4, "smart-list entry returns four rows");
    const auto& aggregate = service.fetch_smart_list_with_relation_aggregates(query, {}).wait();
    check(aggregate && aggregate.value().data.size() == 4, "aggregate entry returns four rows");
    const auto& typed = service.fetch_entities<SchoolRow>(query).wait();
    check(typed && typed.value().data.size() == 4, "typed entry returns four rows");
    const auto& enhanced = service.fetch_enhanced_entities<SchoolRow>(query).wait();
    check(enhanced && enhanced.value().data.size() == 4, "enhanced entry returns four rows");
    const auto& typed_aggregate =
        service.fetch_enhanced_entities_with_relation_aggregates<SchoolRow>(query, {}).wait();
    check(typed_aggregate && typed_aggregate.value().data.size() == 4,
          "enhanced aggregate entry returns four rows");
    const auto& typed_owned =
        service
            .fetch_enhanced_entities_with_relation_aggregates_owned<SchoolRow>(
                detail::clone_in(r, query), {})
            .wait();
    check(typed_owned && typed_owned.value().data.size() == 4,
          "owned enhanced aggregate entry returns four rows");
    const auto& stream = service.fetch_stream(query).wait();
    check(bool(stream), "stream opens");
    std::size_t count = 0;
    while (auto* chunk = stream.value().get().wait_next()) {
        check(bool(*chunk), "stream chunk succeeds");
        count += chunk->value().rows.size();
    }
    check(count == 4, "stream returns four rows");
    check(user_context->sql_logs().size() == 9,
          "each entry executes once; future facets never execute");
    for (const auto& log : user_context->sql_logs()) {
        check(log.trace_path.size() == 4 && log.params.empty(),
              "future facet bindings and routes do not enter root execution");
        check(detail::debug_sql_log(r, log).find("FACET-PRIVATE-SCHOOL") == String::npos,
              "future secret is masked at every root entry");
    }
    check(query.as_query().comment_text &&
              query.as_query().comment_text->find("FACET-PRIVATE-SCHOOL") != String::npos,
          "source intent is preserved unchanged");
}

static void repeated_count_diagnostics() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& executor = setup(*user_context);
    auto& service =
        EntityDataService<Executor>::for_executor(*user_context, "School", *user_context, executor);
    SelectQuery child(std::allocator_arg, Allocator(r), "School");
    child.filter(Expr::eq(r, "name", Value(r, "PRIVATE-REMOVED-COUNT-RELATION")));
    SelectQuery root(std::allocator_arg, Allocator(r), "School");
    root.relation_query("unused", child);
    root.comment("count with PRIVATE-FUTURE-A PRIVATE-FUTURE-B PRIVATE-REMOVED-COUNT-RELATION");
    PurposedSelectQuery query(
        std::allocator_arg,
        Allocator(r),
        root,
        "page with PRIVATE-FUTURE-A PRIVATE-FUTURE-B PRIVATE-REMOVED-COUNT-RELATION");
    query.for_exact_count("count");
    query.with_facet_diagnostics(future_private_options(r, "PRIVATE-FUTURE-A"));
    query.with_facet_diagnostics(future_private_options(r, "PRIVATE-FUTURE-B"));
    query.with_facet_diagnostics(request::QueryOptions(std::allocator_arg, Allocator(r)));
    check(query.as_query().relations.empty() && query.as_query().child_enhancements.empty(),
          "exact count removes executable relations and enhancements");
    const auto& result = service.fetch_all(query).wait();
    check(result && result.value().size() == 1 && result.value()[0].get("count")->try_i64() == 4,
          "exact count retains all four School rows");
    check(user_context->sql_logs().size() == 1, "diagnostic-only sources never execute");
    auto rendered = detail::debug_sql_log(r, user_context->sql_logs()[0]);
    for (auto secret : {"PRIVATE-FUTURE-A", "PRIVATE-FUTURE-B", "PRIVATE-REMOVED-COUNT-RELATION"}) {
        check(rendered.find(secret) == String::npos,
              "repeated facet and removed relation secrets remain masked");
    }
}

class FacetRuntime final : public TeaqlRuntime {
public:
    using allocator_type = Allocator;
    using Output = Result<SmartList<CompactRow>, RuntimeError>;
    using Child = Result<SmartList<CompactRow>, DataServiceError<typename Executor::Error>>;

    FacetRuntime(std::allocator_arg_t, Allocator, UserContext& context) : context_(context) {}

    const UserContext& user_context() const override {
        return context_;
    }

    class Work {
    public:
        using allocator_type = Allocator;

        Work(std::allocator_arg_t, Allocator a, AsyncOperation<Child>& child)
            : resource_(a.resource()), child_(child) {}

        std::optional<Output> poll(WakeSignal& wake) {
            forwarding_.attach(child_.wake_signal(), wake);
            auto* result = child_.poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return Output::err(
                    resource_,
                    RuntimeError::graph(resource_, result->error().to_string(resource_)));
            }
            return Output::ok(resource_, detail::clone_in(resource_, result->value()));
        }

        void finish_and_wait() noexcept {
            forwarding_.reset();
        }

        void cancel_and_wait() noexcept {
            forwarding_.reset();
            child_.cancel();
        }

    private:
        Memory resource_;
        AsyncOperation<Child>& child_;
        WakeSignal::Forward forwarding_;
    };

    AsyncOperation<Output>&
    fetch_facet_smart_list(std::string_view entity_name,
                           const PurposedSelectQuery& query,
                           std::span<const RelationAggregate> aggregates,
                           std::span<const TraceNode> trace) const override {
        auto service = context_.entity_data_service<Executor>(entity_name);
        check(bool(service), "facet service resolves from Context");
        auto& scoped = context_.make<EntityDataService<Executor>>(service.value().get());
        scoped.with_trace_context(trace);
        auto& child = scoped.fetch_smart_list_with_relation_aggregates(query, aggregates);
        return make_operation<Output, Work>(context_.memory_resource(), child);
    }

private:
    UserContext& context_;
};

static request::QueryOptions root_options(Memory r, bool nested, bool include_all) {
    SelectQuery type_query(std::allocator_arg, Allocator(r), "SchoolType");
    type_query.limit(10);
    request::QuerySelection types(std::allocator_arg, Allocator(r), type_query);
    SelectQuery schools(std::allocator_arg, Allocator(r), "School");
    types.query_options.relation_aggregates.emplace_back("schools", "school_count", schools, true);
    if (nested) {
        SelectQuery platform_query(std::allocator_arg, Allocator(r), "Platform");
        platform_query.limit(10);
        request::QuerySelection platforms(std::allocator_arg, Allocator(r), platform_query);
        SelectQuery aggregate(std::allocator_arg, Allocator(r), "SchoolType");
        platforms.query_options.relation_aggregates.emplace_back(
            "school_types", "type_count", aggregate, true);
        types.query_options.facets.emplace_back(
            "platformChoices", "platform", platforms, include_all);
    }
    request::QueryOptions options(std::allocator_arg, Allocator(r));
    options.facets.emplace_back("typeChoices", "school_type", types, include_all);
    return options;
}

static void root_and_nested_facets(bool nested, bool include_all, bool logging) {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& executor = setup(*user_context, nullptr, false);
    if (!logging) {
        user_context->disable_sql_log();
    }
    auto& service =
        EntityDataService<Executor>::for_executor(*user_context, "School", *user_context, executor);
    SelectQuery root(std::allocator_arg, Allocator(r), "School");
    root.limit(10)
        .filter(Expr::eq(r, "name", Value(r, "FACET-PRIVATE-SCHOOL")))
        .comment("load school facets FACET-PRIVATE-SCHOOL");
    PurposedSelectQuery query(
        std::allocator_arg, Allocator(r), root, "render choices FACET-PRIVATE-SCHOOL");
    const auto& rows = service.fetch_all(query).wait();
    check(rows && rows.value().size() == 3, "private root binding reaches SQLite unchanged");
    auto& runtime = user_context->make<FacetRuntime>(*user_context);
    auto options = root_options(r, nested, include_all);
    const auto& result = execute_facets(runtime, query.as_query(), options).wait();
    check(bool(result), "root facets execute");
    const auto& types = result.value().at(String("typeChoices", r));
    auto counts = [r](const SmartList<CompactRow>& list, std::string_view alias) {
        Vector<std::pair<std::int64_t, std::int64_t>> values(r);
        for (const auto& row : list.data) {
            values.emplace_back(*row.get("id")->try_i64(), *row.get(alias)->try_i64());
        }
        std::sort(values.begin(), values.end());
        return values;
    };
    Vector<std::pair<std::int64_t, std::int64_t>> expected(r);
    expected.emplace_back(10, 2);
    expected.emplace_back(20, 1);
    if (include_all) {
        expected.emplace_back(30, 0);
    }
    check(counts(types, "school_count") == expected, "root facet identity and count set");
    if (nested) {
        const auto& platforms = types.facets.at(String("platformChoices", r));
        expected.clear();
        expected.emplace_back(1, include_all ? 3 : 2);
        if (include_all) {
            expected.emplace_back(2, 0);
        }
        check(counts(platforms, "type_count") == expected, "nested facet identity and count set");
    }
    const auto& logs = user_context->sql_logs();
    check(logs.size() == (logging ? (nested ? 5 : 3) : 0), "root and nested SQL statement counts");
    if (logging) {
        const std::vector<std::vector<std::string_view>> edges = {
            {},
            {"School.school_type"},
            {"School.school_type", "SchoolType.schools"},
            {"School.school_type", "SchoolType.platform"},
            {"School.school_type", "SchoolType.platform", "Platform.school_types"}};
        for (std::size_t index = 0; index < logs.size(); ++index) {
            const auto& log = logs[index];
            const auto& path = log.trace_path;
            check(path.size() == edges[index].size() + 4, "no duplicate trace ancestors");
            check(path[0].kind == TraceKind::Operation && path[0].entity_type == "School" &&
                      path[1].kind == TraceKind::Request && path[1].entity_type == "School",
                  "original root ancestry");
            for (std::size_t edge = 0; edge < edges[index].size(); ++edge) {
                const auto& node = path[edge + 2];
                check(node.kind == TraceKind::Relation && node.comment == edges[index][edge] &&
                          node.entity_type ==
                              edges[index][edge].substr(edges[index][edge].find('.') + 1),
                      "exact field relation frame");
            }
            check(path[path.size() - 2].kind == TraceKind::Provider &&
                      path[path.size() - 2].entity_type == "sqlite" &&
                      path.back().kind == TraceKind::Sql && path.back().entity_type == "select",
                  "provider and SQL trace terminal");
            check(log.comment && log.comment->find("load school facets") != String::npos &&
                      log.purpose && log.purpose->find("render choices") != String::npos,
                  "inherited business intent");
            check(detail::debug_sql_log(r, log).find("FACET-PRIVATE-SCHOOL") == String::npos,
                  "inherited intent stays masked");
        }
    }
    check(query.as_query().comment_text->find("FACET-PRIVATE-SCHOOL") != String::npos,
          "original query intent unchanged");
    user_context->clear_sql_logs();
    auto& independent = EntityDataService<Executor>::for_executor(
        *user_context, "Platform", *user_context, executor);
    SelectQuery followup(std::allocator_arg, Allocator(r), "Platform");
    followup.limit(10).comment("independent public literal FACET-PRIVATE-SCHOOL");
    PurposedSelectQuery followup_query(
        std::allocator_arg, Allocator(r), followup, "verify isolation");
    check(bool(independent.fetch_all(followup_query).wait()), "independent follow-up succeeds");
    check(user_context->sql_logs().size() == (logging ? 1 : 0), "follow-up logging switch");
    if (logging) {
        const auto independent_logs = user_context->sql_logs();
        const auto& log = independent_logs[0];
        check(log.trace_path.size() == 4 && log.trace_path[0].entity_type == "Platform" &&
                  log.comment && log.comment->find("FACET-PRIVATE-SCHOOL") != String::npos,
              "follow-up inherits neither route nor redactions");
    }
}

int main() {
    root_and_nested_facets(false, true, true);
    for (bool include_all : {false, true}) {
        for (bool logging : {false, true}) {
            root_and_nested_facets(true, include_all, logging);
        }
    }
    future_bindings_at_all_entries();
    repeated_count_diagnostics();
    per_parent_facets();
    compact_and_slow_paths();
    shared_and_filtered_to_one();
    null_and_missing_to_one();
    std::cout
        << "SQLite per-parent facet source contracts passed: 16 relation combinations plus compact, slow typed and shared/filtered to-one paths\n";
}
