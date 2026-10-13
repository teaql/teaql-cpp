#include <teaql/runtime/detail/data_service/relation_execution.hpp>
#include <teaql/core/detail/debug.hpp>
#include "fixture_sql.hpp"
#include <iostream>

static bool flag(const Json& j, std::string_view key) {
    return std::get<bool>(field(j, key).data);
}

struct Mock : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory r;
    bool probes;
    mutable unsigned capabilities_calls = 0;

    Mock(std::allocator_arg_t, Allocator a, bool p) : r(a.resource()), probes(p) {}

    Memory memory_resource() const {
        return r;
    }

    DataServiceCapabilities capabilities() const override {
        ++capabilities_calls;
        DataServiceCapabilities c;
        c.small_parent_relation_probes = probes;
        return c;
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory, QueryRequest) override {
        throw std::logic_error("relation planner bypassed child query entry point");
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unused");
    }
};

struct Fetch {
    using allocator_type = Allocator;
    Memory r;
    Vector<SelectQuery> seen;
    std::size_t fail_at, pending, drops = 0;

    Fetch(std::allocator_arg_t, Allocator a, const Json& j)
        : r(a.resource()), seen(r), fail_at(integer(field(j, "fail_at"))),
          pending(integer(field(j, "pending"))) {}

    struct Work {
        using allocator_type = Allocator;
        Memory r;
        Fetch& fetch;
        std::size_t index, pending;

        Work(std::allocator_arg_t, Allocator a, Fetch& f, std::size_t i)
            : r(a.resource()), fetch(f), index(i), pending(f.pending) {}

        std::optional<detail::RelationRowsResult<Mock>> poll(WakeSignal& wake) {
            using R = detail::RelationRowsResult<Mock>;
            if (pending) {
                --pending;
                wake.notify();
                return {};
            }
            if (index == fetch.fail_at) {
                return R::err(r, DataServiceError<String>::executor(r, String("child failed", r)));
            }
            Vector<CompactRow> rows(r);
            Record record(r);
            record.emplace(String("probe", r), Value(r, std::uint64_t(index)));
            rows.emplace_back(record);
            return R::ok(r, std::move(rows));
        }

        void finish_and_wait() noexcept {
            ++fetch.drops;
        }

        void cancel_and_wait() noexcept {
            ++fetch.drops;
        }
    };

    AsyncOperation<detail::RelationRowsResult<Mock>>& operator()(SelectQuery query) {
        seen.emplace_back(std::move(query));
        return make_operation<detail::RelationRowsResult<Mock>, Work>(r, *this, seen.size());
    }
};

static Json expr_out(Memory r, const Expr& e) {
    auto out = detail::model_json(r);
    unsigned kind;
    switch (e.kind) {
    case Expr::Kind::Column:
        kind = 0;
        break;
    case Expr::Kind::Value:
        kind = 1;
        break;
    case Expr::Kind::Binary:
        kind = 3;
        break;
    case Expr::Kind::And:
        kind = 8;
        break;
    case Expr::Kind::Or:
        kind = 9;
        break;
    default:
        throw std::logic_error("Unsupported planning fixture expression");
    }
    detail::model_put(out, "kind", kind);
    detail::model_put(out, "op", unsigned(e.op));
    detail::model_put(out, "name", e.name);
    detail::model_put(out, "value", detail::debug_value(r, e.literal));
    Json::Array args(r);
    for (const auto& a : e.args) {
        args.emplace_back(expr_out(r, a));
    }
    detail::model_put(out, "args", args);
    return out;
}

static Json query_out(Memory r, const SelectQuery& q) {
    auto out = detail::model_json(r);
    detail::model_put(out, "entity", q.entity);
    detail::model_put(out, "filter", q.predicate ? expr_out(r, *q.predicate) : Json(r));
    detail::model_put(out, "partition", q.partition_field ? Json(r, *q.partition_field) : Json(r));
    Json::Array projection(r), relations(r), order(r);
    for (const auto& s : q.projection) {
        projection.emplace_back(s);
    }
    for (const auto& l : q.relations) {
        relations.emplace_back(l.name);
    }
    for (const auto& o : q.order_by) {
        auto item = detail::model_json(r);
        detail::model_put(item, "field", o.field);
        detail::model_put(item, "direction", unsigned(o.direction));
        order.emplace_back(std::move(item));
    }
    detail::model_put(out, "projection", projection);
    detail::model_put(out, "relations", relations);
    detail::model_put(out, "order", order);
    return out;
}

static Json evaluate(UserContext& c, const Json& j) {
    auto r = c.memory_resource();
    c.disable_sql_log();
    if (flag(j, "capture")) {
        c.enable_select_sql_log();
    }
    auto& metadata = c.make<InMemoryMetadataStore>();
    EntityDescriptor target(std::allocator_arg, Allocator(r), "Child");
    target.property("child_id", DataType::U64).is_id = true;
    metadata.register_entity(std::move(target));
    c.set_metadata(c, metadata);
    RelationDescriptor relation(std::allocator_arg, Allocator(r), "children", "Child");
    relation.local_key = "id";
    relation.foreign_key = "owner_id";
    relation.many = flag(j, "many");
    RelationLoadPlan plan(std::allocator_arg, Allocator(r), "Root", relation);
    plan.path = "nested.children";
    if (flag(j, "query")) {
        SelectQuery q(std::allocator_arg, Allocator(r), "IgnoredTarget");
        q.project("name").order_desc("name");
        q.partition_field.emplace("existing", r);
        q.relations.emplace_back("nested");
        auto slice = text(field(j, "slice"));
        if (slice == "limit") {
            q.limit(2);
        } else if (slice == "offset") {
            q.offset(3);
        } else if (slice == "zero") {
            q.limit(0);
        }
        if (auto* t = optional(j, "threshold");
            t && !std::holds_alternative<std::monostate>(t->data)) {
            q.top_n_probe_parent_threshold_value = integer(*t);
        }
        plan.query.emplace(std::move(q));
    }
    RelationDescriptor nested(std::allocator_arg, Allocator(r), "nested", "Leaf");
    nested.local_key = "ref_id";
    plan.children.emplace_back("Child", nested);
    Vector<CompactRow> parents(r);
    for (std::uint64_t i = 0; i < integer(field(j, "parents")); ++i) {
        Record record(r);
        record.emplace(String("id", r), Value(r, i));
        parents.emplace_back(record);
        parents.emplace_back(record);
    }
    parents.emplace_back();
    auto& mock = c.make<Mock>(flag(j, "probes"));
    ResourceBorrow<Mock> borrow(std::allocator_arg, Allocator(r), mock, detail::resource_arena(r));
    auto& service = c.make<detail::ContextDataService<Mock>>(c, borrow);
    auto& fetch = c.make<Fetch>(j);
    detail::RelationFetch<Mock> callback(
        std::allocator_arg, Allocator(r), [&fetch](SelectQuery q) -> auto& {
            return fetch(std::move(q));
        });
    auto& operation =
        detail::fetch_relation_rows(service, plan, parents, flag(j, "compact"), callback);
    DataServiceCapabilities capabilities;
    capabilities.small_parent_relation_probes = flag(j, "probes");
    auto selected =
        detail::relation_top_n_execution_plan(capabilities, plan, integer(field(j, "parents")));
    auto telemetry =
        detail::relation_top_n_operation(r, plan, selected, integer(field(j, "parents")));
    Json out = detail::model_json(r);
    bool ready = false;
    for (std::uint64_t i = 0; i < integer(field(j, "polls")); ++i) {
        if (auto* result = operation.poll()) {
            ready = true;
            if (!*result) {
                detail::model_put(out, "error", result->error().to_string(r));
            } else {
                Json::Array values(r);
                for (const auto& row : result->value()) {
                    values.emplace_back(*row.get("probe")->try_u64());
                }
                detail::model_put(out, "rows", values);
            }
            break;
        }
    }
    if (!ready) {
        operation.cancel();
        detail::model_put(out, "cancelled", true);
    }
    Json::Array queries(r);
    for (const auto& q : fetch.seen) {
        queries.emplace_back(query_out(r, q));
    }
    detail::model_put(out, "queries", queries);
    detail::model_put(out, "capabilities_calls", mock.capabilities_calls);
    detail::model_put(out, "drops", fetch.drops);
    auto attrs = detail::model_json(r);
    for (const auto& [key, v] : telemetry.attributes) {
        std::visit(
            [&](const auto& value) {
                detail::model_put(attrs, key, value);
            },
            v.data);
    }
    detail::model_put(out, "telemetry", attrs);
    return out;
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext g;
        auto c = g.make_user_context();
        auto j = Json::parse(c->memory_resource(), line);
        std::cout << evaluate(*c, *j).dump(c->memory_resource()) << '\n';
    }
}
