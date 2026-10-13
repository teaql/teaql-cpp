#include "generated/include/cpp_examples.hpp"
#include <iostream>

using namespace cpp_examples;

static_assert(Entity<Product> && LedgerEntity<Order> && VersionedEntity<Order>);

int main() {
    using E = cpp_examples::E;
    GlobalContext global;
    auto user_context_owner = global.make_user_context();
    auto& user_context = *user_context_owner;
    try {
        setup(user_context);
        auto Q = user_context.queries<cpp_examples::query_factory>();
        auto& product = Q.products()
                            .comment("Construct the example product")
                            .purpose("Prepare an audited product")
                            .newEntity(user_context);
        product.updateName("keyboard");
        auto product_id = product.id();
        require_value(
            AuditedSaveExt<Product>::save(
                std::move(product).audit_as("Seed the generated example product"), user_context)
                .wait());
        auto& order = Q.orders()
                          .comment("Construct the order graph")
                          .purpose("Prepare an audited order")
                          .newEntity(user_context);
        order.updateName("generated-order");
        auto order_id = order.id();
        auto& line = Q.orderLines()
                         .comment("Construct the order line")
                         .purpose("Prepare the order child")
                         .newEntity(user_context);
        line.updateName("first-line").updateOrderId(order_id).updateProductId(product_id);
        order.lines().data.emplace_back(std::move(line));
        order.lines().loaded();
        require_value(
            AuditedSaveExt<Order>::save(
                std::move(order).audit_as("Create the generated order graph"), user_context)
                .wait());
        auto products = Q.products().selectName();
        auto lines = Q.orderLines().selectName().selectProductWith(products);
        auto request = Q.orders()
                           .withNameIs("generated-order")
                           .selectName()
                           .selectLinesWith(lines)
                           .comment("Load the order with its product")
                           .purpose("Display the generated order graph");
        const auto& orders = require_value(request.executeForList(user_context).wait());
        if (orders.len() != 1) {
            throw std::runtime_error("Expected one generated order");
        }
        const auto& one = require_value(request.executeForOne(user_context).wait());
        if (!one || one->id() != order_id) {
            throw std::runtime_error("Expected the generated order as the first result");
        }
        auto streaming_request = Q.orderLines()
                                     .orderByIdAsc()
                                     .stream(1)
                                     .comment("Stream the saved order lines")
                                     .purpose("Read order lines incrementally");
        const auto& handle = require_value(streaming_request.executeForStream(user_context).wait());
        auto& stream = handle.get();
        auto* item = stream.wait_next();
        if (!item || require_value(*item).name() != "first-line" || stream.wait_next()) {
            throw std::runtime_error("Expected one streamed order line");
        }
        auto name = E::order(orders[0]).getName().unwrap();
        auto product_name = E::order(orders[0]).getLines().first().getProduct().getName().unwrap();
        std::cout << name << ": " << product_name << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
