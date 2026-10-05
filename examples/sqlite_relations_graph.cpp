// Semantic port of Rust v5.0.5 examples/src/bin/sqlite_relations_graph.rs.
// The small graph mapping is handwritten; no generator or generic graph-save claim.
#include "models.hpp"
#include <tuple>

namespace {
struct LoadedLine {
    const teaql::Record* line;
    const teaql::Record* product;
};

struct LoadedOrder {
    const teaql::Record* order;
    teaql::Vector<LoadedLine>* lines;
};
}

int main() {
    try {
        using namespace teaql;
        GlobalContext global;
        const auto& models = example::models(global);
        auto context = global.make_user_context();
        auto& db = context->make_resource<SqliteSession>(*context, std::span(models.all));
        db.ensure_schema();
        for (const auto& [id, name] : {std::pair{100u, "keyboard"}, std::pair{101u, "mouse"}}) {
            auto& command = context->make<InsertCommand>("Product");
            command.value("id", id).value("name", name).audit_as("Seed the referenced product");
            db.execute(command);
        }
        // One atomic order graph, matching the saved root + children in the Rust example.
        db.transaction([&] {
            auto& command = context->make<InsertCommand>("Order");
            command.value("id", 1u)
                .value("version", 1)
                .value("name", "graph-order")
                .audit_as("Create the example order graph");
            db.execute(command);
            for (const auto& [id, name, product_id] :
                 {std::tuple{10u, "first-line", 100u}, std::tuple{11u, "second-line", 101u}}) {
                auto& line = context->make<InsertCommand>("OrderLine");
                line.value("id", id)
                    .value("order_id", 1u)
                    .value("name", name)
                    .value("product_id", product_id)
                    .audit_as("Create the example order graph");
                db.execute(line);
            }
        });
        auto& query = context->make<SelectQuery>("Order");
        query.project("id")
            .project("name")
            .order("id")
            .purpose("Display the saved example order")
            .comment("relation+graph root");
        const auto& roots = db.query(query);
        auto& graph = context->make<Vector<LoadedOrder>>();
        for (const auto& root : roots) {
            auto& line_query = context->make<SelectQuery>("OrderLine");
            line_query
                .filter(
                    Expr::eq(context->memory_resource(), "order_id", example::field(root, "id")))
                .order("id")
                .purpose("Load order lines")
                .comment("relation: Order.lines");
            const auto& rows = db.query(line_query);
            auto& lines = context->make<Vector<LoadedLine>>();
            for (const auto& row : rows) {
                auto& product_query = context->make<SelectQuery>("Product");
                product_query
                    .filter(Expr::eq(
                        context->memory_resource(), "id", example::field(row, "product_id")))
                    .purpose("Load the referenced product")
                    .comment("relation: OrderLine.product");
                const auto& products = db.query(product_query);
                example::require(products.size() == 1, "referenced product exists");
                lines.push_back({&row, &products.front()});
            }
            graph.push_back({&root, &lines});
        }
        example::require(graph.size() == 1, "one order");
        example::require(example::text(*graph[0].order, "name") == "graph-order", "order name");
        example::require(graph[0].lines->size() == 2, "two order lines");
        example::require(example::text(*graph[0].lines->at(0).product, "name") == "keyboard",
                         "first product");
        example::require(example::text(*graph[0].lines->at(1).product, "name") == "mouse",
                         "second product");
        std::cout
            << "relation+graph example rows: [{name: graph-order, lines: [first-line -> keyboard, second-line -> mouse]}]\n";
        example::print_logs(db);
        context.reset(); // Graph, loaded records and SQL log values all become invalid here.
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
