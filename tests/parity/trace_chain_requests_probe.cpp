#include "../generator/parity/hydration_allocation_contracts/cpp/include/cpp_hydration_allocation.hpp"
#include <teaql/runtime/data_service.hpp>
#include <iostream>
#include <tuple>
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct NoIo final : QueryExecutor<String>, MutationExecutor<String> {
    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory, QueryRequest) override {
        throw std::runtime_error("unexpected provider query");
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::runtime_error("unexpected provider mutation");
    }
};

struct NoPolicy final : RequestPolicy {
    RuntimeHookResult enforce_select(Memory, const UserContext&, SelectQuery&) const override {
        throw std::runtime_error("missing intent reached Policy");
    }
};

static void metadata(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto& store = user_context.make<InMemoryMetadataStore>();
    for (auto name : {"Order", "OrderItem", "Payment", "PaymentAttempt", "Shipment"}) {
        EntityDescriptor entity(std::allocator_arg, Allocator(r), name);
        entity.property("id", DataType::U64).id();
        entity.property("version", DataType::I64).version();
        if (entity.name == "Order") {
            entity.relation("items", "OrderItem").many = true;
            entity.relation("payment", "Payment");
            entity.relation("shipment", "Shipment");
        }
        if (entity.name == "Payment") {
            entity.relation("attempts", "PaymentAttempt").many = true;
        }
        store.register_entity(std::move(entity));
    }
    user_context.set_metadata(user_context, store);
}

static GraphNode create(Memory r, std::string_view name, std::uint64_t id) {
    GraphNode result(std::allocator_arg, Allocator(r), name);
    result.with_operation(GraphOperation::Create).value("id", id);
    return result;
}

static void intent(const DataServiceError<String>& error, RequestKind kind) {
    check(error.kind() == DataServiceError<String>::Kind::Runtime, "structured runtime error");
    const auto& runtime = error.runtime_error();
    check(runtime.kind() == RuntimeError::Kind::RequestIntent, "structured intent error");
    const auto& value = runtime.request_intent_error();
    check(value.request_kind == kind && value.field == "comment" &&
              value.code() == "REQUEST_COMMENT_REQUIRED",
          "exact kind/field/code");
}

using Expected = std::tuple<std::string_view, std::uint64_t, std::string_view>;

static void lineage(const TraceChain& trace, std::initializer_list<Expected> expected) {
    check(trace.size() == expected.size(), "exact lineage length");
    std::size_t index = 0;
    for (const auto& [entity, id, reason] : expected) {
        const auto& frame = trace[index++];
        check(frame.kind == TraceKind::AuditReason && frame.entity_type == entity &&
                  frame.entity_id == id && frame.comment == reason,
              "exact lineage tuple");
    }
}

static void service_contracts() {
    GlobalContext global;
    for (bool disable_logs : {false, true}) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        metadata(*user_context);
        auto& policy = user_context->make_resource<NoPolicy>();
        user_context->set_request_policy(*user_context, policy);
        if (disable_logs) {
            user_context->disable_sql_log();
        }
        auto& executor = user_context->make_resource<NoIo>();
        auto& service =
            EntityDataService<NoIo>::for_executor(*user_context, "Order", *user_context, executor);
        TraceChain ambient(r);
        ambient.emplace_back(TraceKind::Comment, "Order", "fabricated root comment");
        service.with_trace_context(ambient);
        for (auto comment : {std::optional<std::string_view>{},
                             std::optional<std::string_view>{""},
                             std::optional<std::string_view>{" \t\n"},
                             std::optional<std::string_view>{"\u2003"}}) {
            SelectQuery query(std::allocator_arg, Allocator(r), "Order");
            query.limit(10);
            if (comment) {
                query.comment(*comment);
            }
            PurposedSelectQuery purposed(std::allocator_arg, Allocator(r), query, "render orders");
            const auto& rejected = service.fetch_all(purposed).wait();
            check(!rejected, "missing query comment rejected");
            intent(rejected.error(), RequestKind::Query);
            auto graph = create(r, "Order", 100);
            if (comment) {
                graph.with_comment(*comment);
            }
            auto child = create(r, "OrderItem", 201);
            child.with_comment("add item");
            graph.relation("items", child);
            const auto& invalid_graph = service.plan_graph(std::move(graph)).wait();
            check(!invalid_graph, "missing graph comment rejected");
            intent(invalid_graph.error(), RequestKind::Mutation);
        }
    }
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    metadata(*user_context);
    auto& executor = user_context->make_resource<NoIo>();
    auto& service =
        EntityDataService<NoIo>::for_executor(*user_context, "Order", *user_context, executor);
    auto graph = create(r, "Order", 100);
    graph.with_comment("submit order");
    graph.relation("items", create(r, "OrderItem", 201));
    GraphNode removed(std::allocator_arg, Allocator(r), "OrderItem");
    removed.value("id", std::uint64_t{202})
        .value("version", std::int64_t{1})
        .remove()
        .with_comment("remove unavailable item");
    graph.relation("items", removed);
    auto payment = create(r, "Payment", 301);
    payment.with_comment("authorize payment")
        .relation("attempts", create(r, "PaymentAttempt", 401));
    graph.relation("payment", payment);
    auto shipment = create(r, "Shipment", 501);
    shipment.with_comment("dispatch shipment");
    graph.relation("shipment", shipment);
    const auto& result = service.plan_graph(std::move(graph)).wait();
    check(bool(result) && result.value().items.size() == 6, "six actual planned items");
    for (const auto& item : result.value().items) {
        check(item.scope_token != nullptr, "planner supplies scope token");
        auto trace = item.scope_token->recover_trace_chain(r);
        const auto id = item.values.fields.at("id").try_u64();
        if (item.entity == "OrderItem" && id == 202) {
            check(item.kind == GraphMutationKind::Delete, "deleted child stays Delete");
            lineage(
                trace,
                {{"Order", 100, "submit order"}, {"OrderItem", 202, "remove unavailable item"}});
        } else if (item.entity == "Payment" || item.entity == "PaymentAttempt") {
            lineage(trace, {{"Order", 100, "submit order"}, {"Payment", 301, "authorize payment"}});
        } else if (item.entity == "Shipment") {
            lineage(trace,
                    {{"Order", 100, "submit order"}, {"Shipment", 501, "dispatch shipment"}});
        } else {
            lineage(trace, {{"Order", 100, "submit order"}});
        }
    }
    auto blank = create(r, "Order", 100);
    auto child = create(r, "OrderItem", 201);
    child.with_comment(" \n");
    blank.with_comment("submit order").relation("items", child);
    const auto& inherited = service.plan_graph(std::move(blank)).wait();
    check(bool(inherited) && inherited.value().items.size() == 2, "blank descendant plan");
    lineage(inherited.value().items[1].scope_token->recover_trace_chain(r),
            {{"Order", 100, "submit order"}});
    auto allocated = create(r, "Order", 100);
    payment = create(r, "Payment", 100);
    payment.with_comment("authorize payment");
    GraphNode fresh(std::allocator_arg, Allocator(r), "Shipment");
    fresh.with_operation(GraphOperation::Create).with_comment("dispatch shipment");
    allocated.with_comment("submit order").relation("payment", payment).relation("shipment", fresh);
    const auto& assigned = service.plan_graph(std::move(allocated)).wait();
    check(bool(assigned) && assigned.value().items.size() == 3, "three assigned items");
    for (const auto& item : assigned.value().items) {
        auto trace = item.scope_token->recover_trace_chain(r);
        check(!trace.empty(), "assigned item trace");
        auto id = item.values.fields.at("id").try_u64();
        check(id && *id > 0 && trace.back().entity_type == item.entity &&
                  trace.back().entity_id == id,
              "allocated identity includes entity type");
    }
}

static void generated_contracts() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto decode = [&](std::uint64_t id) {
        Record values(r);
        values.emplace(String("id", r), Value(r, id));
        values.emplace(String("version", r), Value(r, std::int64_t{1}));
        values.emplace(String("name", r), Value(r, "probe"));
        auto result = cpp_hydration_allocation::HydrationProbe::from_compact_row(
            r, CompactRow(std::allocator_arg, Allocator(r), std::move(values)));
        check(bool(result), "real generated decoder");
        return std::move(result).value();
    };
    auto root = decode(100);
    auto child = decode(201);
    EntityRuntimeState shared(std::allocator_arg, Allocator(r));
    root.root = detail::clone_in(r, shared);
    child.root = detail::clone_in(r, shared);
    root.set_comment("submit order");
    child.set_comment("add item");
    check(root.get_comment() == "submit order" && child.get_comment() == "add item",
          "comments keyed in shared ledger");
    auto unannotated = decode(100);
    auto annotated = decode(201);
    annotated.set_comment("add item");
    check(bool(unannotated.includePendingMutationsFrom(annotated)),
          "generated composition succeeds");
    check(!unannotated.get_comment() && annotated.get_comment() == "add item",
          "composition cannot repair root comment");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        service_contracts();
        generated_contracts();
        std::cout << "TRACE_CHAIN_REQUEST_SOURCE_CONTRACTS=7;passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
