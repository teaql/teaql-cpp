#include "generated/include/cpp_examples.hpp"
#include <iostream>

using namespace cpp_examples;

static_assert(Entity<Product> && LedgerEntity<Order> && VersionedEntity<Order>);

int main() {
    GlobalContext global;
    auto user = global.make_user_context();
    try {
        setup(*user);
        auto& product = Q::products(*user)
                            .comment("Construct the example product")
                            .purpose("Prepare an audited product")
                            .new_entity(*user);
        product.update_name("keyboard");
        auto product_id = product.id();
        require_value(AuditedSaveExt<Product>::save(
                          std::move(product).audit_as("Seed the generated example product"), *user)
                          .wait());
        auto& order = Q::orders(*user)
                          .comment("Construct the order graph")
                          .purpose("Prepare an audited order")
                          .new_entity(*user);
        order.update_name("generated-order");
        auto order_id = order.id();
        auto& line = Q::order_lines(*user)
                         .comment("Construct the order line")
                         .purpose("Prepare the order child")
                         .new_entity(*user);
        line.update_name("first-line").update_order_id(order_id).update_product_id(product_id);
        order.lines().data.emplace_back(std::move(line));
        order.lines().loaded();
        require_value(AuditedSaveExt<Order>::save(
                          std::move(order).audit_as("Create the generated order graph"), *user)
                          .wait());
        auto& products = Q::products(*user).select_name();
        auto& lines = Q::order_lines(*user).select_name().select_product_with(products);
        auto& request = Q::orders(*user)
                            .with_name_is("generated-order")
                            .select_name()
                            .select_lines_with(lines)
                            .comment("Load the order with its product")
                            .purpose("Display the generated order graph");
        const auto& orders = require_value(request.execute_for_list(*user).wait());
        if (orders.len() != 1) {
            throw std::runtime_error("Expected one generated order");
        }
        auto name = E::order(orders[0]).get_name().unwrap();
        auto product_name =
            E::order(orders[0]).get_lines().first().get_product().get_name().unwrap();
        std::cout << name << ": " << product_name << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
