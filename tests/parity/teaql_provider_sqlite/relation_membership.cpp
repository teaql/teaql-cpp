// Complete locked Rust 5.0.6 relation_membership.rs counterpart.
#include "fixture.hpp"
#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/transaction.hpp>
#include <teaql/runtime/log_model_debug.hpp>
#include <iostream>
using namespace original_provider;
constexpr std::string_view private_name = "MEMBERSHIP-PRIVATE-NAME";

static EntityDescriptor entity(Memory r, std::string_view name, std::string_view table) {
    EntityDescriptor value(std::allocator_arg, Allocator(r), name);
    value.table(table);
    value.property("id", DataType::I64).id();
    value.property("version", DataType::I64).version();
    value.property("name", DataType::Text);
    return value;
}

template <bool Parent>
struct MembershipModel : EntityDefaults<MembershipModel<Parent>> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = Parent ? "MembershipParent" : "MembershipChild";
    CompactRow row;
    std::optional<EntityRuntimeState> state;

    MembershipModel(std::allocator_arg_t, Allocator a, CompactRow row)
        : row(std::allocator_arg, a, std::move(row)) {}

    MembershipModel(std::allocator_arg_t, Allocator a, const MembershipModel& other)
        : row(std::allocator_arg, a, other.row),
          state(detail::clone_in(a.resource(), other.state)) {}

    MembershipModel(std::allocator_arg_t, Allocator a, MembershipModel&& other)
        : MembershipModel(detail::move_to(a.resource(), std::move(other))) {}

    MembershipModel(MembershipModel&&) noexcept = default;

    Memory memory_resource() const {
        return row.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return entity(r, ENTITY_NAME, Parent ? "membership_parent" : "membership_child");
    }

    static Result<MembershipModel, EntityError> from_compact_row(Memory r, CompactRow row) {
        return Result<MembershipModel, EntityError>::ok(
            r, MembershipModel(std::allocator_arg, Allocator(r), std::move(row)));
    }

    bool is_field_loaded(std::string_view field) const {
        return row.get(field) != nullptr;
    }

    Value id_value() const {
        return Value(memory_resource(), *row.get("id"));
    }

    MutationValues into_values() && {
        MutationValues values(memory_resource());
        values.fields = row.into_map(memory_resource());
        return values;
    }

    void on_loaded(AnyRef context) {
        auto* loaded = context.get<EntityRuntimeState>();
        check(loaded, "typed runtime state supplied");
        state.emplace(std::allocator_arg, Allocator(memory_resource()), *loaded);
    }
};

using MembershipParent = MembershipModel<true>;
using MembershipChild = MembershipModel<false>;

struct MembershipFixture : Fixture {
    explicit MembershipFixture(bool aliased) {
        auto r = resource;
        auto parent = entity(r, "MembershipParent", "membership_parent");
        parent.audit_mask_fields_declared = true;
        parent.audit_mask_fields.emplace_back("name");
        auto& children = parent.relation("children", "MembershipChild");
        children.many = true;
        children.local_key = "id";
        children.foreign_key = "parent_id";
        auto child = entity(r, "MembershipChild", "membership_child");
        child.property("parent_id", DataType::I64);
        for (auto name : {aliased ? "parent_id" : "parent", "second_parent"}) {
            auto& forward = child.relation(name, "MembershipParent");
            forward.local_key = "parent_id";
            forward.foreign_key = "id";
        }
        auto& decoders = user->make<InMemoryEntityGraphDecoderRegistry>();
        decoders.register_entity<MembershipParent>();
        decoders.register_entity<MembershipChild>();
        user->set_entity_graph_decoder_registry(decoders);
        metadata.register_entity(std::move(parent));
        metadata.register_entity(std::move(child));
        use_sqlite_provider(*user, *user, transport);
        check(bool(user->ensure_schema().wait()), "membership schema");
        user->insert_resource(*user, executor);
        const auto& seeded =
            user->execute_in_transaction<Executor, MutationResult>(
                    [](const TransactionScope<Executor>& scope) -> auto& {
                        auto r = scope.context().memory_resource();
                        AsyncOperation<Result<MutationResult, RuntimeError>>* last = nullptr;
                        for (std::int64_t id : {100, 101, 102}) {
                            InsertCommand command(std::allocator_arg,
                                                  Allocator(r),
                                                  id == 100 ? "MembershipParent"
                                                            : "MembershipChild");
                            command.value("id", id)
                                .value("version", std::int64_t(1))
                                .value("name",
                                       id == 100   ? private_name
                                       : id == 101 ? std::string_view("first")
                                                   : std::string_view("second"));
                            if (id != 100) {
                                command.value("parent_id", std::int64_t(100));
                            }
                            last =
                                &scope.mutate(MutationRequest(std::allocator_arg,
                                                              Allocator(r),
                                                              std::move(command),
                                                              "seed relation membership fixture"));
                            check(bool(last->wait()), "seed membership row");
                        }
                        return *last;
                    })
                .wait();
        check(bool(seeded), "seed committed");
        user->enable_all_sql_log();
        user->clear_sql_logs();
    }

    auto& service(std::string_view root) {
        return EntityDataService<Executor>::for_executor(*user, root, *user, executor);
    }
};

static void verify(bool filtered, std::size_t threshold, bool aliased, bool sibling, bool logging) {
    MembershipFixture f(aliased);
    auto r = f.resource;
    if (!logging) {
        f.user->disable_sql_log();
    }
    auto forward = aliased ? "parent_id" : "parent";
    SelectQuery detail_query(std::allocator_arg, Allocator(r), "MembershipParent");
    detail_query.filter(Expr::eq(r, "name", filtered ? std::string_view("ABSENT") : private_name));
    SelectQuery children_query(std::allocator_arg, Allocator(r), "MembershipChild");
    children_query.limit(10).top_n_probe_parent_threshold(threshold).relation_query(forward,
                                                                                    detail_query);
    if (sibling) {
        SelectQuery second(std::allocator_arg, Allocator(r), "MembershipParent");
        children_query.relation_query("second_parent", second);
    }
    SelectQuery query(std::allocator_arg, Allocator(r), "MembershipParent");
    query.limit(10)
        .relation_query("children", children_query)
        .comment(String("load child membership and filtered parent ", r) + String(private_name, r));
    PurposedSelectQuery request(
        std::allocator_arg,
        Allocator(r),
        query,
        String("verify nested trace execution and predicate preservation ", r) +
            String(private_name, r));
    const auto& result = f.service("MembershipParent").fetch_all(request).wait();
    check(bool(result) && result.value().size() == 1, "one parent returned");
    const auto* children = result.value()[0].get("children");
    check(children && children->as_list() && children->as_list()->size() == 2,
          "forward filter does not remove children");
    for (const auto& item : *children->as_list()) {
        const auto* child = item.as_object();
        check(child, "child object");
        if (!aliased) {
            check(child->at(String("parent_id", r)).try_i64() == 100, "original scalar FK");
        }
        const auto* view = child->at(String(forward, r)).as_object();
        check(view, "forward relation object");
        if (filtered) {
            check(view->size() == 1 && view->at(String("id", r)).try_i64() == 100,
                  "filtered edge preserves only real identity");
        }
        if (sibling) {
            check(child->at(String("second_parent", r)).as_object(), "sibling uses original FK");
        }
    }
    auto logs = f.user->sql_logs();
    check(logs.size() == (logging ? 3 + unsigned(sibling) : 0), "unique branch physical log count");
    for (std::size_t i = 0; i < logs.size(); ++i) {
        if (!filtered) {
            check(debug_string(r, logs[i]).find(private_name) == String::npos,
                  "nested private provenance survives planner stripping");
        }
        Vector<String> path(r);
        for (const auto& node : logs[i].trace_path) {
            if (node.kind == TraceKind::Relation) {
                path.emplace_back(node.entity_type);
            }
        }
        Vector<String> expected(r);
        if (i) {
            expected.emplace_back("children");
        }
        if (i >= 2) {
            expected.emplace_back(i == 2 ? forward : "second_parent");
        }
        check(path == expected, "exact executed unique relation paths");
    }
    f.user->clear_sql_logs();
    SelectQuery next(std::allocator_arg, Allocator(r), "MembershipChild");
    next.limit(10).comment("verify original foreign key");
    PurposedSelectQuery independent(
        std::allocator_arg, Allocator(r), next, "check query scope isolation");
    const auto& rows = f.service("MembershipChild").fetch_all(independent).wait();
    check(bool(rows) && rows.value().size() == 2, "independent rows");
    for (const auto& row : rows.value()) {
        check(row.get("parent_id") && row.get("parent_id")->try_i64() == 100,
              "independent scalar FK unchanged");
    }
    check(f.user->sql_logs().size() == unsigned(logging), "independent logging switch");
}

static RelationAggregate count(Memory r, std::string_view relation, std::string_view alias) {
    SelectQuery query(std::allocator_arg, Allocator(r), "MembershipParent");
    query.count("value");
    return RelationAggregate(std::allocator_arg, Allocator(r), relation, alias, query, true);
}

static void aggregate(bool aliased, bool filtered, bool typed) {
    for (bool logging : {false, true}) {
        MembershipFixture f(aliased);
        auto r = f.resource;
        if (!logging) {
            f.user->disable_sql_log();
        }
        auto forward = aliased ? "parent_id" : "parent";
        SelectQuery parent(std::allocator_arg, Allocator(r), "MembershipParent");
        parent.filter(Expr::eq(r, "name", filtered ? std::string_view("ABSENT") : private_name));
        SelectQuery query(std::allocator_arg, Allocator(r), "MembershipChild");
        query.limit(10)
            .relation_query(forward, parent)
            .comment("load children and their parent count");
        PurposedSelectQuery request(std::allocator_arg,
                                    Allocator(r),
                                    query,
                                    "verify aggregation after filtered forward hydration");
        Vector<RelationAggregate> aggregates(r);
        aggregates.emplace_back(count(r, "second_parent", "parent_count"));
        auto& repo = f.service("MembershipChild");
        auto check_row = [&](const CompactRow& row) {
            check(row.get("parent_count") && row.get("parent_count")->try_i64() == 1,
                  "aggregate uses original scalar FK");
            if (filtered && !typed) {
                const auto* view = row.get(forward);
                check(view && view->as_object() && view->as_object()->size() == 1 &&
                          view->as_object()->at(String("id", r)).try_i64() == 100,
                      "filtered raw detail is identity only");
            }
        };
        if (typed) {
            const auto& rows =
                repo.fetch_enhanced_entities_with_relation_aggregates<MembershipChild>(request,
                                                                                       aggregates)
                    .wait();
            check(bool(rows) && rows.value().data.size() == 2, "two typed children");
            for (const auto& child : rows.value().data) {
                check(bool(child.state) && (child.state->resolve_entity<MembershipParent>(100) ==
                                            nullptr) == filtered,
                      "typed shared parent visibility");
                check_row(child.row);
            }
        } else {
            const auto& rows =
                repo.fetch_smart_list_with_relation_aggregates(request, aggregates).wait();
            check(bool(rows) && rows.value().data.size() == 2, "two raw children");
            for (const auto& row : rows.value().data) {
                check_row(row);
            }
        }
        auto logs = f.user->sql_logs();
        check(logs.size() == (logging ? 3 : 0), "aggregate physical log count");
        for (std::size_t i = 0; i < logs.size(); ++i) {
            check(logs[i].trace_path.front().entity_type == "MembershipChild",
                  "aggregate route root");
            Vector<String> path(r);
            for (const auto& node : logs[i].trace_path) {
                if (node.kind == TraceKind::Relation) {
                    path.emplace_back(node.entity_type);
                }
            }
            if (i == 0) {
                check(path.empty(), "root has no relation");
            } else {
                const auto expected = typed ? (i == 1 ? "second_parent" : forward)
                                            : (i == 1 ? forward : "second_parent");
                check(path.size() == 1 && path[0] == expected,
                      "exact typed/raw aggregate scheduling route");
            }
        }
    }
}

static void aggregate_alias() {
    MembershipFixture f(false);
    auto r = f.resource;
    SelectQuery query(std::allocator_arg, Allocator(r), "MembershipChild");
    query.limit(10).comment("project two independent counts");
    PurposedSelectQuery request(std::allocator_arg,
                                Allocator(r),
                                query,
                                "verify aliases cannot change subsequent aggregation keys");
    Vector<RelationAggregate> aggregates(r);
    aggregates.emplace_back(count(r, "second_parent", "parent_id"));
    aggregates.emplace_back(count(r, "second_parent", "parent_count"));
    const auto& rows = f.service("MembershipChild")
                           .fetch_smart_list_with_relation_aggregates(request, aggregates)
                           .wait();
    check(bool(rows) && rows.value().data.size() == 2, "two aggregate alias rows");
    for (const auto& row : rows.value().data) {
        check(row.get("parent_id")->try_i64() == 1 && row.get("parent_count")->try_i64() == 1,
              "earlier aggregate alias cannot overwrite later membership");
    }
    check(f.user->sql_logs().size() == 3, "two aggregate executions plus root");
}

static void typed_edges() {
    for (bool aliased : {false, true}) {
        for (bool optimized : {false, true}) {
            MembershipFixture f(aliased);
            auto r = f.resource;
            auto forward = aliased ? "parent_id" : "parent";
            SelectQuery parent(std::allocator_arg, Allocator(r), "MembershipParent");
            parent.filter(Expr::eq(r, "name", std::string_view("ABSENT")));
            SelectQuery sibling(std::allocator_arg, Allocator(r), "MembershipParent");
            SelectQuery query(std::allocator_arg, Allocator(r), "MembershipChild");
            query.relation_query(forward, parent)
                .relation_query("second_parent", sibling)
                .limit(10)
                .comment("load filtered and visible references to the same parent");
            PurposedSelectQuery request(std::allocator_arg,
                                        Allocator(r),
                                        query,
                                        "preserve identity without leaking sibling detail");
            Vector<RelationAggregate> aggregates(r);
            aggregates.emplace_back(count(r, "second_parent", "parent_count"));
            auto& repo = f.service("MembershipChild");
            const auto& rows =
                optimized ? repo.fetch_enhanced_entities<MembershipChild>(request).wait()
                          : repo.fetch_enhanced_entities_with_relation_aggregates<MembershipChild>(
                                    request, aggregates)
                                .wait();
            check(bool(rows) && rows.value().data.size() == 2, "typed edge children");
            for (const auto& child : rows.value().data) {
                auto id = child.row.get("id")->try_u64();
                check(bool(id) && bool(child.state), "child identity and state");
                auto* owned = child.state->resolve_relation_option<MembershipParent>(
                    "MembershipChild", *id, forward);
                check(owned && bool(*owned), "filtered FK owns non-null identity view");
                const auto& view = **owned;
                check(view.row.get("id")->try_i64() == 100 && !view.is_field_loaded("name") &&
                          !view.row.get("version"),
                      "filtered edge excludes sibling detail");
                check(child.row.get("parent_id")->try_i64() == 100,
                      "typed edge scalar FK preserved");
                auto* visible = child.state->resolve_entity<MembershipParent>(100);
                check(visible && *visible->row.get("name") == Value(r, private_name),
                      "sibling shared detail remains visible");
            }
        }
    }
}

static void explicit_projection() {
    for (bool aliased : {false, true}) {
        for (bool filtered : {false, true}) {
            for (bool aggregated : {false, true}) {
                for (bool logging : {false, true}) {
                    MembershipFixture f(aliased);
                    auto r = f.resource;
                    if (!logging) {
                        f.user->disable_sql_log();
                    }
                    auto forward = aliased ? "parent_id" : "parent";
                    SelectQuery detail(std::allocator_arg, Allocator(r), "MembershipParent");
                    detail.filter(Expr::eq(r, "id", std::int64_t(filtered ? 0 : 100))).limit(10);
                    SelectQuery child(std::allocator_arg, Allocator(r), "MembershipChild");
                    child.relation_query(forward, detail).limit(10);
                    SelectQuery query(std::allocator_arg, Allocator(r), "MembershipParent");
                    query.project("id")
                        .project("version")
                        .limit(10)
                        .relation_query("children", child)
                        .comment("inspect explicitly selected parent detail");
                    PurposedSelectQuery request(
                        std::allocator_arg,
                        Allocator(r),
                        query,
                        "inverse convenience must not replace an explicit projection");
                    SelectQuery metric(std::allocator_arg, Allocator(r), "MembershipChild");
                    metric.filter(Expr::eq(r, "name", std::string_view("first"))).count("value");
                    Vector<RelationAggregate> aggregates(r);
                    aggregates.emplace_back("children", "selected_count", metric, true);
                    auto& repo = f.service("MembershipParent");
                    const auto& rows =
                        aggregated ? repo.fetch_enhanced_entities_with_relation_aggregates<
                                             MembershipParent>(request, aggregates)
                                         .wait()
                                   : repo.fetch_enhanced_entities<MembershipParent>(request).wait();
                    check(bool(rows) && rows.value().data.size() == 1, "projected root");
                    const auto& parent = rows.value().data[0];
                    check(!parent.is_field_loaded("name"), "root name remains not loaded");
                    if (aggregated) {
                        check(parent.row.get("selected_count")->try_i64() == 1, "selected count");
                    }
                    auto* children = parent.state->resolve_relation_list<MembershipChild>(
                        "MembershipParent", 100, "children");
                    check(children && children->data.size() == 2, "projection children");
                    for (const auto& member : children->data) {
                        auto id = member.row.get("id")->try_u64();
                        check(bool(id) && member.row.get("parent_id")->try_i64() == 100,
                              "projection scalar FK");
                        auto* owned = member.state->resolve_relation_option<MembershipParent>(
                            "MembershipChild", *id, forward);
                        const auto* view =
                            owned && *owned ? &**owned
                                            : member.state->resolve_entity<MembershipParent>(100);
                        check(view && view->row.get("id")->try_i64() == 100 &&
                                  view->is_field_loaded("name") == !filtered,
                              "explicit forward projection survives inverse wiring");
                        if (!filtered) {
                            check(*view->row.get("name") == Value(r, private_name),
                                  "explicit detail value");
                        }
                    }
                }
            }
        }
    }
}

static void future_privacy() {
    for (bool logging : {false, true}) {
        for (unsigned shape : {0u, 1u, 2u}) {
            MembershipFixture f(false);
            auto r = f.resource;
            if (!logging) {
                f.user->disable_sql_log();
            }
            SelectQuery query(std::allocator_arg, Allocator(r), "MembershipChild");
            query.limit(10).comment(String("inspect ", r) + String(private_name, r));
            PurposedSelectQuery request(std::allocator_arg,
                                        Allocator(r),
                                        query,
                                        String("verify aggregate ", r) + String(private_name, r));
            SelectQuery metric(std::allocator_arg, Allocator(r), "MembershipParent");
            metric.filter(Expr::eq(r, "name", private_name)).count("value");
            Vector<RelationAggregate> aggregates(r);
            aggregates.emplace_back("parent", "private_count", metric, true);
            auto& repo = f.service("MembershipChild");
            auto verify_row = [&](const CompactRow& row) {
                check(row.get("private_count") && row.get("private_count")->try_i64() == 1,
                      "future private count");
            };
            if (shape == 0) {
                const auto& rows =
                    repo.fetch_smart_list_with_relation_aggregates(request, aggregates).wait();
                check(bool(rows) && rows.value().data.size() == 2, "future raw rows");
                for (const auto& row : rows.value().data) {
                    verify_row(row);
                }
            } else {
                const auto& rows =
                    shape == 1
                        ? repo.fetch_enhanced_entities_with_relation_aggregates<MembershipChild>(
                                  request, aggregates)
                              .wait()
                        : repo.fetch_enhanced_entities_with_relation_aggregates_owned<
                                  MembershipChild>(std::move(request), aggregates)
                              .wait();
                check(bool(rows) && rows.value().data.size() == 2, "future typed rows");
                for (const auto& row : rows.value().data) {
                    verify_row(row.row);
                }
            }
            auto logs = f.user->sql_logs();
            check(logs.size() == (logging ? 2 : 0), "future private log count");
            for (const auto& entry : logs) {
                check(entry.comment == "inspect [REDACTED]" &&
                          entry.purpose == "verify aggregate [REDACTED]",
                      "future aggregate masks parent intent");
                check(debug_string(r, entry).find(private_name) == String::npos,
                      "future aggregate complete debug privacy");
            }
            f.user->clear_sql_logs();
            SelectQuery next(std::allocator_arg, Allocator(r), "MembershipChild");
            next.limit(10).comment(String("independent ", r) + String(private_name, r));
            PurposedSelectQuery independent(
                std::allocator_arg, Allocator(r), next, "no aggregate in this request");
            const auto& rows = repo.fetch_enhanced_entities<MembershipChild>(independent).wait();
            check(bool(rows) && rows.value().data.size() == 2, "independent typed rows");
            if (logging) {
                check(f.user->sql_logs()[0].comment ==
                          String("independent ", r) + String(private_name, r),
                      "privacy scope does not leak to next request");
            }
        }
    }
}

static void nested_counts() {
    for (bool logging : {false, true}) {
        for (bool cyclic : {false, true}) {
            MembershipFixture f(false);
            auto r = f.resource;
            if (!logging) {
                f.user->disable_sql_log();
            }
            SelectQuery selected(std::allocator_arg, Allocator(r), "MembershipParent");
            selected.project("id").project("version").limit(10);
            if (cyclic) {
                SelectQuery parent(std::allocator_arg, Allocator(r), "MembershipParent");
                parent.limit(10);
                SelectQuery children(std::allocator_arg, Allocator(r), "MembershipChild");
                children.limit(10).relation_query("parent", parent);
                selected.relation_query("children", children);
            }
            request::QuerySelection nested(std::allocator_arg, Allocator(r), selected);
            SelectQuery metric(std::allocator_arg, Allocator(r), "MembershipChild");
            metric.filter(Expr::eq(r, "name", std::string_view("first"))).count("value");
            nested.query_options.relation_aggregates.emplace_back(
                "children", "selected_count", metric, true);
            SelectQuery query(std::allocator_arg, Allocator(r), "MembershipChild");
            query.limit(10)
                .relation_query("parent", std::move(nested).into_query())
                .comment("load nested related count");
            PurposedSelectQuery purposed(std::allocator_arg,
                                         Allocator(r),
                                         query,
                                         "typed selections must preserve related metrics");
            const auto& rows = f.service("MembershipChild")
                                   .fetch_enhanced_entities<MembershipChild>(purposed)
                                   .wait();
            check(bool(rows) && rows.value().data.size() == 2, "nested typed children");
            for (const auto& child : rows.value().data) {
                auto id = child.row.get("id")->try_u64();
                check(bool(id), "nested owner identity");
                auto* owned = child.state->resolve_relation_option<MembershipParent>(
                    "MembershipChild", *id, "parent");
                const auto* parent =
                    owned && *owned ? &**owned : child.state->resolve_entity<MembershipParent>(100);
                check(parent && parent->row.get("selected_count") &&
                          parent->row.get("selected_count")->try_i64() == 1 &&
                          !parent->is_field_loaded("name"),
                      "nested projected count and load state");
                if (cyclic) {
                    auto* members = parent->state->resolve_relation_list<MembershipChild>(
                        "MembershipParent", 100, "children");
                    check(members && members->data.size() == 2, "nested cyclic members");
                    for (const auto& member : members->data) {
                        auto* detail = member.state->resolve_entity<MembershipParent>(100);
                        check(detail && detail->row.get("name") &&
                                  *detail->row.get("name") == Value(r, private_name),
                              "nested cyclic shared detail");
                    }
                }
            }
            auto logs = f.user->sql_logs();
            check(logs.size() == (logging ? (cyclic ? 5 : 3) : 0), "nested physical SQL count");
            unsigned counts = 0;
            for (const auto& entry : logs) {
                String sql(entry.sql, r);
                for (auto& c : sql) {
                    if (c >= 'a' && c <= 'z') {
                        c -= 'a' - 'A';
                    }
                }
                if (sql.find("COUNT(") == String::npos) {
                    continue;
                }
                ++counts;
                Vector<String> edges(r);
                for (const auto& node : entry.trace_path) {
                    if (node.kind == TraceKind::Relation) {
                        edges.emplace_back(node.comment);
                    }
                }
                check(edges.size() == 2 && edges[0] == "MembershipChild.parent" &&
                          edges[1] == "MembershipParent.children",
                      "nested aggregate trace route");
            }
            check(counts == (logging ? 1 : 0), "nested aggregate executed once");
        }
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    unsigned cases = 0;
    for (bool filtered : {false, true}) {
        for (std::size_t threshold : {0, 32}) {
            for (bool aliased : {false, true}) {
                for (bool sibling : {false, true}) {
                    for (bool logging : {false, true}) {
                        verify(filtered, threshold, aliased, sibling, logging);
                        ++cases;
                    }
                }
            }
        }
    }
    for (bool aliased : {false, true}) {
        for (bool filtered : {false, true}) {
            aggregate(aliased, filtered, false);
            aggregate(aliased, filtered, true);
        }
    }
    aggregate_alias();
    typed_edges();
    explicit_projection();
    future_privacy();
    nested_counts();
    std::cout << "RELATION_MEMBERSHIP_SOURCE_TESTS=13;forward_cases=" << cases << ";passed\n";
}
