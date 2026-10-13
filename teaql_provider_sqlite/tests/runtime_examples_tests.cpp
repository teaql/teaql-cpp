#include "../../examples/scenarios.hpp"
#include <iostream>
#include <fstream>
#include <iterator>
using namespace teaql;

static void ownership_and_provenance(const Json& reference) {
    for (bool graph : {false, true}) {
        GlobalContext global;
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        const auto& rows = graph ? example::relations_graph(*user) : example::schema_crud(*user);
        auto before = example::observations(r, rows).dump(global.memory_resource());
        auto& retained = global.make<SmartList<example::Order>>(rows);
        bool audited = false, purposed = false;
        for (const auto& log : user->sql_logs()) {
            auto reason = graph ? "Create the example order graph" : "Submit the example order";
            audited |= log.audit_reason == reason ||
                       (log.comment && log.comment->find(reason) != String::npos);
            purposed |= log.purpose == (graph ? "Display the saved example order"
                                              : "Inspect the soft-deleted example order");
        }
        example::require(audited && purposed,
                         "runtime retains audit reason and query purpose in SQL provenance");
        {
            Json::Array observed(r);
            for (const auto& log : user->sql_logs()) {
                Json::Object fields(std::less<>{}, r);
                std::string_view operation;
                switch (log.operation) {
                case SqlLogOperation::Select:
                    operation = "Select";
                    break;
                case SqlLogOperation::Insert:
                    operation = "Insert";
                    break;
                case SqlLogOperation::Update:
                    operation = "Update";
                    break;
                case SqlLogOperation::Delete:
                    operation = "Delete";
                    break;
                case SqlLogOperation::Recover:
                    operation = "Recover";
                    break;
                }
                fields.emplace(String("operation", r), Json(r, operation));
                fields.emplace(String("purpose", r), log.purpose ? Json(r, *log.purpose) : Json(r));
                fields.emplace(String("audit", r),
                               log.audit_reason ? Json(r, *log.audit_reason) : Json(r));
                observed.push_back(Json(r, fields));
            }
            const auto& expected =
                std::get<Json::Object>(reference.data)
                    .at(String(graph ? "relations" : "schema", reference.memory_resource()));
            example::require(
                Json(r, observed) == expected,
                "operation/purpose/audit provenance matches the 5.0.6 explicit-intent example flow");
        }
        user.reset();
        auto target = global.memory_resource();
        example::require(
            example::observations(target, retained).dump(target) == before,
            "explicit Global copy retains complete nested results after User destruction");
        if (graph) {
            example::require(
                retained[0].lines[0].name.get_allocator().resource() == target &&
                    retained[0].lines[0].product->name.get_allocator().resource() == target &&
                    retained[0].lines[0].product->root.memory_resource() == target,
                "nested model strings and runtime snapshots use their destination domain");
        }
    }
}

static void graph_failure_rolls_back_all_nodes() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    (void)example::relations_graph(*user);
    auto service = user->entity_data_service<example::Executor>("Order");
    auto& repository = example::value(service).get();
    auto order = example::make_order(*user, 1, "must roll back");
    order.id = 2;
    for (auto id : {20u, 10u}) {
        example::OrderLine line(std::allocator_arg, Allocator(r));
        line.id = id;
        line.order_id = 2;
        line.name = "must roll back";
        line.product_id = 100;
        line.mark_as_new();
        order.lines.data.emplace_back(std::move(line));
    }
    order.lines.loaded();
    auto& result = AuditedSaveExt<example::Order>::save(
                       std::move(order).audit_as("Reject the conflicting child graph"), *user)
                       .wait();
    example::require(!result, "a conflicting explicitly-new child fails graph save");
    auto query = repository.select();
    query.comment("Inspect complete graph rollback");
    query.filter(Expr::eq(r, "id", std::uint64_t(2)));
    PurposedSelectQuery purpose(
        std::allocator_arg, Allocator(r), query, "Check complete graph rollback");
    example::require(
        example::value(repository.fetch_entities<example::Order>(purpose).wait()).len() == 0,
        "failed child write rolls back the previously inserted root");
    auto lines = user->entity_data_service<example::Executor>("OrderLine");
    auto line_query = example::value(lines).get().select();
    line_query.comment("Inspect child graph rollback");
    line_query.filter(Expr::eq(r, "id", std::uint64_t(20)));
    PurposedSelectQuery line_purpose(
        std::allocator_arg, Allocator(r), line_query, "Check child graph rollback");
    example::require(
        example::value(
            example::value(lines).get().fetch_entities<example::OrderLine>(line_purpose).wait())
                .len() == 0,
        "failed graph save rolls back the earlier sibling write");
    auto existing = repository.select();
    existing.comment("Inspect the existing graph after rollback");
    SelectQuery products(std::allocator_arg, Allocator(r), "OrderLine");
    products.relation("product");
    existing.filter(Expr::eq(r, "id", std::uint64_t(1))).relation_query("lines", products);
    PurposedSelectQuery existing_purpose(
        std::allocator_arg, Allocator(r), existing, "Check existing graph after rollback");
    const auto& saved =
        example::value(repository.fetch_enhanced_entities<example::Order>(existing_purpose).wait());
    example::require(saved.len() == 1 && saved[0].lines.len() == 2 &&
                         saved[0].lines[0].name == "first-line" &&
                         saved[0].lines[0].product->name == "keyboard",
                     "original graph and referenced products survive failed save");
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        example::require(argc == 2, "provide the recorded original provenance fixture");
        std::ifstream input(argv[1]);
        example::require(bool(input), "open original provenance fixture");
        std::string source{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        GlobalContext evidence;
        auto reference = Json::parse(evidence.memory_resource(), source);
        example::require(reference.has_value(), "parse original provenance fixture");
        ownership_and_provenance(*reference);
        graph_failure_rolls_back_all_nodes();
        std::pmr::set_default_resource(previous);
        std::cout << "Runtime example ownership and graph rollback contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
