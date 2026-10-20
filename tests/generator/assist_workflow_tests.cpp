#include "assist_workflow_examples.hpp"
#include <iostream>

using namespace teaql;
using namespace cpp_examples;
using namespace cpp_examples::assist_workflows;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

int main() {
    try {
        GlobalContext global;
        auto user_context_owner = global.make_user_context();
        auto& user_context = *user_context_owner;
        setup(user_context);
        auto Q = user_context.queries<query_factory>();
        const auto& first = product::create(user_context, [](Product& entity) {
            entity.updateName("first");
        });
        auto id = first.id();
        check(first.version() == 1 && product::readName(first).value() == "first",
              "Create and E examples use the actual generated APIs");
        const auto& sparse = product::queryById(user_context, id);
        check(sparse.len() == 1 && product::readId(sparse[0]).value() == id,
              "Minimal identity query and E identity read agree");
        auto logs_before = user_context.sql_logs().size();
        bool not_loaded = false;
        try {
            (void)product::readName(sparse[0]);
        } catch (const ExpressionNotLoadedError&) {
            not_loaded = true;
        }
        check(not_loaded && user_context.sql_logs().size() == logs_before,
              "E preserves NotLoaded without an implicit query");
        const auto& full = require_value(Q.products()
                                             .withIdIs(id)
                                             .comment("Read the independent stale snapshot")
                                             .purpose("Check optimistic concurrency")
                                             .executeForOne(user_context)
                                             .wait());
        auto& stale = user_context.make<Product>(*full);
        auto updated = product::update(user_context, id, [](Product& entity) {
            entity.updateName("updated");
        });
        check(updated && updated->get().version() == 2 &&
                  product::readName(updated->get()).value() == "updated",
              "Update reloads full scalar state and preserves the optimistic version");
        check(!product::update(user_context, 0, [](Product&) {}), "Missing update is empty");
        stale.updateName("stale");
        const auto& conflict =
            AuditedSaveExt<Product>::save(std::move(stale).audit_as("Reject the stale operation"),
                                          user_context)
                .wait();
        check(!conflict, "The ordinary runtime rejects a stale save");
        const auto& parent = order::create(user_context, [](Order& entity) {
            entity.updateName("parent");
        });
        const auto& line = order_line::create(user_context, [&](OrderLine& entity) {
            entity.updateName("line").updateOrderId(parent.id()).updateProductId(id);
        });
        const auto& minimal_line = require_value(Q.orderLinesMinimal()
                                                     .withIdIs(line.id())
                                                     .comment("Read an incomplete save target")
                                                     .purpose("Retain the scalar loading rejection")
                                                     .executeForOne(user_context)
                                                     .wait());
        auto& incomplete = user_context.make<OrderLine>(*minimal_line);
        incomplete.updateName("must fail");
        check(!AuditedSaveExt<OrderLine>::save(
                   std::move(incomplete).audit_as("Reject missing scalar loading"), user_context)
                   .wait(),
              "Sparse mutation targets retain runtime Checker rejection");
        check(order_line::update(user_context,
                                 line.id(),
                                 [](OrderLine& entity) {
                                     entity.updateName("complete");
                                 })
                  .has_value(),
              "The action example loads complete foreign keys before modifying a relation model");
        for (std::uint64_t index = 0; index != 102; ++index) {
            product::create(user_context, [](Product& entity) {
                entity.updateName("batch");
            });
        }
        const auto& page = product::listPage(user_context, 0, 1);
        const auto& next = product::listPage(user_context, 1, 1);
        check(page.len() == 1 && next.len() == 1 && page[0].id() != next[0].id() &&
                  page.total_count == 103,
              "Stable pages preserve the complete filtered count");
        logs_before = user_context.sql_logs().size();
        for (auto size : {std::uint64_t{0}, std::uint64_t{101}}) {
            bool rejected = false;
            try {
                (void)product::listPage(user_context, 0, size);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            check(rejected, "Application page bounds reject invalid sizes");
        }
        check(user_context.sql_logs().size() == logs_before, "Invalid pages do no database work");
        const auto& ids = product::streamIds(user_context);
        check(ids.size() == 100 && std::is_sorted(ids.begin(), ids.end()),
              "The extracted stream crosses chunks, stays ordered and stops at its declared limit");
        check(product::erase(user_context, id) && product::queryById(user_context, id).len() == 0 &&
                  !product::erase(user_context, id),
              "Audited deletion hides the row and a repeated deletion returns false");
        const auto& deleted = require_value(Q.products()
                                                .withDeletedRows()
                                                .withIdIs(id)
                                                .comment("Read the retained deleted row")
                                                .purpose("Verify physical soft-delete state")
                                                .executeForOne(user_context)
                                                .wait());
        check(deleted && deleted->version() == -3 && deleted->name() == "updated",
              "The row remains persisted with the source soft-delete version");
        std::cout << "Extracted generated action workflows passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
