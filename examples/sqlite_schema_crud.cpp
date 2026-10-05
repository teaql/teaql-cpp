// Semantic port of Rust v5.0.5 examples/src/bin/sqlite_schema_crud.rs.
#include "models.hpp"

int main() {
    try {
        using namespace teaql;
        GlobalContext global;
        const auto& models = example::models(global);
        auto context = global.make_user_context();
        auto& db = context->make_resource<SqliteSession>(*context, std::span(models.all));
        db.ensure_schema();

        auto& create = context->make<InsertCommand>("Order");
        create.value("id", 1u)
            .value("version", 1)
            .value("name", "draft")
            .audit_as("Create the example order");
        example::require(db.execute(create) == 1, "create order");

        auto& submit = context->make<UpdateCommand>("Order", 1u);
        submit.expected_version(1).value("name", "submitted").audit_as("Submit the example order");
        example::require(db.execute(submit) == 1, "submit order");

        auto& remove = context->make<DeleteCommand>("Order", 1u);
        remove.expected_version(2).audit_as("Delete the example order");
        example::require(db.execute(remove) == 1, "soft-delete order");

        auto& query = context->make<SelectQuery>("Order");
        query.project("id")
            .project("version")
            .project("name")
            .filter(Expr::eq(context->memory_resource(), "id", 1u))
            .purpose("Inspect the soft-deleted example order")
            .comment("schema+crud example");
        const auto& orders = db.query(query);
        example::require(orders.size() == 1, "soft-deleted row is retained");
        const auto& row = orders.front();
        example::require(example::field(row, "version").try_i64() == -3,
                         "1 -> 2 -> -3 version transitions");
        example::require(example::text(row, "name") == "submitted", "saved order name");
        std::cout << "schema+crud example rows: [{id: 1, version: -3, name: submitted}]\n";
        example::print_logs(db);
        context.reset(); // SQLite connection and all context-owned objects/blocks released.
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
